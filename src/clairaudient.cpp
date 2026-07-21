#include "plugin.hpp"
#include "ui/menu_helpers.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

using OscillatorHelper = shapetaker::dsp::OscillatorHelper;

struct ClairaudientModule : Module, IOscilloscopeSource {

    // DSP Constants
    static constexpr float MIDDLE_C_HZ           = 261.626f;
    static constexpr float CV_FINE_SCALE          = 1.f / 50.f;
    static constexpr float CV_SHAPE_SCALE         = 1.f / 5.f;
    static constexpr float CV_XFADE_SCALE         = 1.f / 10.f;
    static constexpr float CV_REV_CHANCE_SCALE    = 1.f / 10.f;
    static constexpr float OUTPUT_GAIN            = 5.f;
    static constexpr float NOISE_V_PEAK           = 0.45f;
    static constexpr float UINT32_NORM            = 1.f / 4294967296.f; // 1 / 2^32
    static constexpr float VOICE_RATIOS[8]        = {0.5f, 1.f, 1.5f, 2.f, 3.f, 4.f, 5.f, 7.f};
    // Frequency parameter ranges
    static constexpr float FREQ1_OCT_MIN          = -2.f;
    static constexpr float FREQ1_OCT_MAX          =  2.f;
    static constexpr float FREQ2_ST_RANGE         = 24.f;    // ±24 semitones
    static constexpr float FINE_TUNE_RANGE        = 0.2f;    // ±20 cents
    static constexpr float SEMITONES_PER_OCTAVE   = 12.f;
    static constexpr float DETUNE_HALF            = 0.5f;    // symmetric detune split
    // Waveform / mixing
    static constexpr float NOISE_SHAPE_EXP        = 0.65f;   // perceptual noise shaping curve
    static constexpr float PHASE_NOISE_SCALE      = 0.00005f; // per-sample phase jitter scale
    static constexpr float STEREO_SWAP_WIDTH_GAIN = 0.35f;   // crossfeed gain in stereo swap mode
    static constexpr float VOICE_CHARACTER_PITCH_OCT = 0.0025f; // ±3 cents at full voice character
    static constexpr float VOICE_CHARACTER_SHAPE  = 0.025f;
    static constexpr float VOICE_CHARACTER_LEVEL  = 0.035f;
    static constexpr float VOICE_CHARACTER_PAN    = 0.025f;
    static constexpr float BUS_COLOR_DRIVE        = 0.25f;
    static constexpr float BUS_COLOR_CROSSTALK    = 0.025f;
    // Drift
    static constexpr float BASE_DRIFT_SPEED       = 0.00002f; // random walk speed (octaves/s²)
    static constexpr float DRIFT_LIMIT            = 0.001f;   // ±octave limit at full drift amount
    // Oversampling
    static constexpr int   MIN_OVERSAMPLE         = 1;
    static constexpr int   MAX_OVERSAMPLE         = 8;
    static constexpr int   DECIMATOR_QUALITY      = 8;
    // Oscilloscope
    static constexpr float OSCOPE_TARGET_CYCLES   = 1.5f;  // Lissajous mode
    static constexpr float OSCOPE_TARGET_CYCLES_WAVE = 2.5f; // waveform mode: room for trigger + full period
    static constexpr int   OSCOPE_MAX_DOWNSAMPLE  = 128;

    enum ParamId {
        FREQ1_PARAM,
        FREQ2_PARAM,
        FINE1_PARAM,
        FINE2_PARAM,
        FINE1_ATTEN_PARAM,
        FINE2_ATTEN_PARAM,
        SHAPE1_PARAM,
        SHAPE2_PARAM,
        SHAPE1_ATTEN_PARAM,
        SHAPE2_ATTEN_PARAM,
        XFADE_PARAM,
        XFADE_ATTEN_PARAM,
        SYNC1_PARAM,
        SYNC2_PARAM,
        VOICE_DEPTH_PARAM,
        VOICE_RATIO_PARAM,
        VOICE_ASYM_PARAM,
        VOICE_WIDTH_PARAM,
        REV_CHANCE_ATTEN_PARAM,
        REV_CHANCE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        VOCT1_INPUT,
        VOCT2_INPUT,
        FINE1_CV_INPUT,
        FINE2_CV_INPUT,
        SHAPE1_CV_INPUT,
        SHAPE2_CV_INPUT,
        XFADE_CV_INPUT,
        REV_CHANCE_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    enum CrossfadeMode {
        CROSSFADE_EQUAL_POWER = 0,
        CROSSFADE_STEREO_SWAP = 1
    };

    enum WaveformMode {
        WAVEFORM_SIGMOID_SAW = 0,
        WAVEFORM_PWM = 1
    };

    // Polyphonic oscillator state, one set per voice
    static constexpr int MAX_POLY_VOICES = shapetaker::PolyphonicProcessor::MAX_VOICES;

    // One detuned copy of an oscillator. Each oscillator (V/Z) runs two
    // copies: A feeds the left output, B the right.
    struct OscCopy {
        float phase = 0.f;
        float dir = 1.f;    // phase direction: +1 forward, -1 reverse (reverse sync)
        float drift = 0.f;  // slow random-walk pitch drift, in octaves
        float noise = 0.f;  // current phase-noise sample
    };

    // One oscillator pair (V or Z) within a voice: symmetric-detuned A/B
    // copies, plus the phase travel since the last reverse-sync flip this
    // oscillator issued. The travel guards deterministic (100% chance)
    // mutual flipping against locking both phases into a wrap-boundary
    // ping-pong where every re-crossing retriggers the other's flip.
    struct OscPairState {
        OscCopy a;
        OscCopy b;
        float revTravel = 1.f;
    };

    struct DCBlockerState {
        float lastIn = 0.f;
        float lastOut = 0.f;
    };

    // All per-voice DSP state: V/Z oscillator pairs and L/R DC blockers
    struct VoiceState {
        OscPairState v;
        OscPairState z;
        DCBlockerState dcL;
        DCBlockerState dcR;
    };

    VoiceState voiceState[MAX_POLY_VOICES];

    // User-adjustable oscillator noise amount (0..1), exposed via context menu slider.
    // Defaults to 0.0 (off). Controls both subtle phase jitter and added noise floor.
    std::atomic<float> oscNoiseAmount = {0.0f};
    
    // --- Oscilloscope Buffering ---
    static constexpr int OSCILLOSCOPE_BUFFER_SIZE = 1024;
    std::array<std::atomic<uint64_t>, OSCILLOSCOPE_BUFFER_SIZE> oscilloscopeBufferPacked = {};
    mutable Vec oscilloscopeBuffer[OSCILLOSCOPE_BUFFER_SIZE] = {};
    std::atomic<int> oscilloscopeBufferIndex = {0};
    int oscilloscopeFrameCounter = 0;

    struct DecimatorSet {
        rack::dsp::Decimator<2, DECIMATOR_QUALITY> d2;
        rack::dsp::Decimator<4, DECIMATOR_QUALITY> d4;
        rack::dsp::Decimator<8, DECIMATOR_QUALITY> d8;
        float buf[8] = {};

        DecimatorSet() : d2(0.9f), d4(0.9f), d8(0.9f) {}

        void reset() {
            d2.reset();
            d4.reset();
            d8.reset();
            for (float& sample : buf) {
                sample = 0.f;
            }
        }
    };

    // Sinc decimators per voice/channel for oscillator oversampling.
    std::array<DecimatorSet, MAX_POLY_VOICES> decimatorLeft{};
    std::array<DecimatorSet, MAX_POLY_VOICES> decimatorRight{};

    shapetaker::PolyphonicProcessor polyProcessor;

    // Quantization mode settings
    std::atomic<bool> quantizeOscV = {true};  // V oscillator quantized to octaves by default
    std::atomic<bool> quantizeOscZ = {true};  // Z oscillator quantized to semitones by default
    std::atomic<int> crossfadeMode = {CROSSFADE_EQUAL_POWER};
    std::atomic<int> waveformMode = {WAVEFORM_SIGMOID_SAW};
    std::atomic<int> oversampleFactor = {4};
    // Single macro for the analog imperfection package (drift, voice
    // character, output color). 0 = clean, 0.5 = calibrated "seasoning"
    // (the old individual sliders at max), 1.0 = 4x past calibration.
    std::atomic<float> vintageAmount = {0.5f};

    std::atomic<int> oscilloscopeTheme = {shapetaker::ui::ThemeManager::DisplayTheme::PHOSPHOR};
    std::atomic<int> oscilloscopeDisplayMode = {0}; // 0 = X-Y Lissajous, 1 = triggered waveform
    mutable std::atomic<bool> useSharedOscilloscopeTheme = {true};
    mutable std::atomic<int> sharedOscilloscopeThemeRevisionSeen = {0};
    std::atomic<bool> pendingFilterReset = {false};

    // Parameter decimation for performance (update every N samples instead of every sample)
    static constexpr int kParamDecimation = 32;  // ~0.7ms at 44.1kHz - imperceptible latency
    int paramDecimationCounter = 0;
    static constexpr int kDriftDecimation = 64;  // Drift is extremely slow; update less often
    int driftDecimationCounter = 0;

    // Cached parameter values (updated every kParamDecimation samples)
    float cachedBasePitch1 = 0.f;
    float cachedBaseSemitoneZ = 0.f;
    float cachedFineTune1 = 0.f;
    float cachedFineTune2 = 0.f;
    float cachedShape1 = 0.5f;
    float cachedShape2 = 0.5f;
    float cachedXfade = 0.5f;
    float cachedFine1Atten = 0.f;
    float cachedFine2Atten = 0.f;
    float cachedShape1Atten = 0.f;
    float cachedShape2Atten = 0.f;
    float cachedXfadeAtten = 0.f;
    // Crossfade trig derived from cachedXfade; per-voice CV paths recompute
    // these locally when the crossfade CV input is connected.
    float cachedXfadeClamped = 0.5f;
    float cachedXfadeCos = 0.70710678f;
    float cachedXfadeSin = 0.70710678f;
    float cachedWidthBlend = 1.f;
    float cachedVoiceDepth = 0.5f;
    float cachedVoiceRatio = 2.f;
    float cachedVoiceAsym = 0.35f;
    float cachedVoiceWidth = 0.5f;
    float cachedRevChanceAtten = 0.f;
    bool cachedRevChanceCVConnected = false;
    bool cachedSync1 = false;
    bool cachedSync2 = false;
    bool cachedMutualRev = false;
    float cachedRevChance = 1.f;

    // Cached input connection states (updated every kParamDecimation samples)
    bool cachedVoct2Connected = false;
    bool cachedFine1CVConnected = false;
    bool cachedFine2CVConnected = false;
    bool cachedShape1CVConnected = false;
    bool cachedShape2CVConnected = false;
    bool cachedXfadeCVConnected = false;

    // Cached noise shaping
    float cachedOscNoiseAmount = -1.f;
    float cachedShapedNoise = 0.f;

    mutable int oscilloscopeReadIndex = 0;

    // Cached filter coefficients to avoid recompute every sample
    float cachedDCCoeff = 0.995f;
    float cachedSampleRate = 0.f;
    int cachedOversample = 0;

    // Per-voice PRNG state for fast drift/noise updates
    shapetaker::dsp::VoiceArray<uint32_t, MAX_POLY_VOICES> rngState;
    uint32_t sharedDriftRngState = 0x6d2b79f5u;
    float sharedDrift1A = 0.f;
    float sharedDrift1B = 0.f;
    float sharedDrift2A = 0.f;
    float sharedDrift2B = 0.f;

    // Precalculated stable voice offsets for Voice Character
    float voiceOffsets[MAX_POLY_VOICES][6];

    // Update parameter snapping based on quantization modes
    void updateParameterSnapping() {
        // V Oscillator snapping
        bool quantizeV = quantizeOscV.load(std::memory_order_relaxed);
        getParamQuantity(FREQ1_PARAM)->snapEnabled = quantizeV;
        getParamQuantity(FREQ1_PARAM)->smoothEnabled = !quantizeV;

        // Z Oscillator snapping
        bool quantizeZ = quantizeOscZ.load(std::memory_order_relaxed);
        getParamQuantity(FREQ2_PARAM)->snapEnabled = quantizeZ;
        getParamQuantity(FREQ2_PARAM)->smoothEnabled = !quantizeZ;
    }

    void resetFilters() {
        for (auto& decimator : decimatorLeft) {
            decimator.reset();
        }
        for (auto& decimator : decimatorRight) {
            decimator.reset();
        }
    }

    // Called only when the sample rate or oversample factor changed.
    void updateFilterCoefficients(float sampleRate, int oversample) {
        cachedSampleRate = sampleRate;
        cachedOversample = oversample;
        resetFilters();

        // DC blocker coefficient: R = 1 - (2π × 10Hz / sampleRate)
        // Keeps the high-pass cutoff at ~10 Hz regardless of sample rate.
        // Without this, the hardcoded 0.995 pushes the cutoff up to ~150 Hz at 192 kHz.
        cachedDCCoeff = 1.f - (2.f * float(M_PI) * 10.f / sampleRate);
    }

    static float applyAttenuvertedCv(float base, bool connected, Input& input, int channel, float precalcScale, float minValue, float maxValue) {
        if (connected) {
            base += input.getPolyVoltage(channel) * precalcScale;
        }
        return clamp(base, minValue, maxValue);
    }

    struct OscillatorPairFrame {
        float freqA = 0.f;
        float freqB = 0.f;
        float deltaA = 0.f;
        float deltaB = 0.f;
        float shape = 0.5f;
        float blend = 0.f;      // saw -> sigmoid crossfade (full sigmoid by ~55% of knob travel)
        float rangeBase = 2.2f; // sigmoid slope before voice-engine modulation
        float season = 0.f;     // scales the subtle organic harmonic garnish
        float lowShape = 0.f;
        float pwmShape = 0.5f;
        float airGainA = 0.f;
        float airGainB = 0.f;
        float wrapJump = 0.f;
    };

    // Compute the sigmoid slope and transition center for one oscillator copy.
    // wob: in-phase modulator sample (overall slope wobble -> formant sweep).
    // asymWob: quadrature modulator sample (duty-cycle skew -> even harmonics).
    // stereoScale: static slope offset separating the L/R copies.
    static inline void voiceMod(float rangeBase, float wob, float asymWob, float baseAsym,
                                float depth, float stereoScale, float& range, float& center) {
        float scale = 1.f + 3.f * depth * wob;
        scale = std::max(scale, 0.12f) * stereoScale;
        range = rangeBase * scale;
        float a = rack::math::clamp(baseAsym + 0.7f * depth * asymWob, -0.9f, 0.9f);
        center = 0.5f + 0.22f * a;
    }

    // Sine with a skewed phase trajectory: k > 0.5 stretches the rise and
    // compresses the fall (saw-like travel) without changing the endpoints.
    // k = 0.5 is an exact identity. Used for ASYM in PWM mode, where the
    // duty cycle itself already belongs to the SHAPE knob.
    static inline float skewedSin2Pi(float phase, float k) {
        float p = phase - std::floor(phase);
        float w = (p < k) ? 0.5f * p / k : 0.5f + 0.5f * (p - k) / (1.f - k);
        return OscillatorHelper::fastSin2Pi(w);
    }

    static inline float phaseWrapBlep(float phase, float dt) {
        if (dt <= 0.f) {
            return 0.f;
        }
        if (phase < dt) {
            return OscillatorHelper::polyBLEP(phase / dt);
        }
        if (phase > 1.f - dt) {
            return OscillatorHelper::polyBLEP((phase - 1.f) / dt);
        }
        return 0.f;
    }

    static inline float processDecimator(DecimatorSet& decimator, int oversample) {
        switch (oversample) {
            case 2:
                return decimator.d2.process(decimator.buf);
            case 4:
                return decimator.d4.process(decimator.buf);
            case 8:
                return decimator.d8.process(decimator.buf);
            default:
                return decimator.buf[0];
        }
    }

    static OscillatorPairFrame makeOscillatorPair(float pitch, float fineOctaves, float driftA, float driftB, float shape, float slopeAsym, float invSampleRate, float sampleRate) {
        float halfFine = fineOctaves * DETUNE_HALF;
        OscillatorPairFrame frame;
        frame.freqA = MIDDLE_C_HZ * rack::dsp::exp2_taylor5(pitch - halfFine + driftA);
        frame.freqB = MIDDLE_C_HZ * rack::dsp::exp2_taylor5(pitch + halfFine + driftB);
        frame.deltaA = frame.freqA * invSampleRate;
        frame.deltaB = frame.freqB * invSampleRate;
        frame.shape = shape;

        float softenedShape = OscillatorHelper::softenShapeEdges(shape);
        frame.season = softenedShape;
        // Saw -> sigmoid morph completes by ~55% of knob travel so the rest of
        // the sweep steepens the slope instead of parking at "square".
        float bt = rack::math::clamp(softenedShape * 1.8f, 0.f, 1.f);
        frame.blend = bt * bt * (3.f - 2.f * bt);
        // Slope: gentle at the bottom, razor-edged by the top (well past the
        // old ceiling of 13 so the extreme is character, not just "square").
        frame.rangeBase = 2.2f + softenedShape * softenedShape * 28.f;
        float ls = rack::math::clamp(softenedShape * 500.f, 0.f, 1.f);
        frame.lowShape = ls * ls * (3.f - 2.f * ls);
        frame.pwmShape = rack::math::clamp(shape, 0.05f, 0.95f);

        float nyquistThreshold = sampleRate * 0.175f;
        float airFadeWidth = std::max(sampleRate * 0.02f, 1.f);
        frame.airGainA = rack::math::clamp((nyquistThreshold - frame.freqA) / airFadeWidth, 0.f, 1.f);
        frame.airGainB = rack::math::clamp((nyquistThreshold - frame.freqB) / airFadeWidth, 0.f, 1.f);
        // Wrap-jump BLEP amplitude from the static (unmodulated) shape; the
        // wave edges saturate toward +/-1 regardless of slope modulation, so a
        // per-frame estimate stays accurate enough inside the oversample loop.
        float aStat = rack::math::clamp(slopeAsym, -0.9f, 0.9f);
        float center0 = 0.5f + 0.22f * aStat;
        float top = OscillatorHelper::voicedSigmoidSaw(0.9999f, frame.blend, frame.rangeBase, center0, frame.lowShape, 0.f, frame.season);
        float bot = OscillatorHelper::voicedSigmoidSaw(0.0001f, frame.blend, frame.rangeBase, center0, frame.lowShape, 0.f, frame.season);
        frame.wrapJump = (top - bot) * 0.5f;

        return frame;
    }

    static uint64_t packVec(float x, float y) {
        uint32_t xi = 0;
        uint32_t yi = 0;
        std::memcpy(&xi, &x, sizeof(float));
        std::memcpy(&yi, &y, sizeof(float));
        return (static_cast<uint64_t>(yi) << 32) | xi;
    }

    static Vec unpackVec(uint64_t packed) {
        uint32_t xi = static_cast<uint32_t>(packed & 0xFFFFFFFFu);
        uint32_t yi = static_cast<uint32_t>(packed >> 32);
        float x = 0.f;
        float y = 0.f;
        std::memcpy(&x, &xi, sizeof(float));
        std::memcpy(&y, &yi, sizeof(float));
        return Vec(x, y);
    }

    // Bidirectional phase wrap for reverse sync (handles negative phase values)
    static inline void wrapPhaseBidirectional(float& phase) {
        if (phase >= 1.f) phase -= 1.f;
        else if (phase < 0.f) phase += 1.f;
    }

    static inline uint32_t xorshift32(uint32_t& state) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    static inline float fastUniform(uint32_t& state) {
        return xorshift32(state) * UINT32_NORM;
    }

    static inline float fastUniformSigned(uint32_t& state) {
        return fastUniform(state) * 2.f - 1.f;
    }

    // Very slow random walk over the four drift values (like analog
    // oscillator aging), clamped to the amount-scaled limit
    // (about ±1.2 cents at amount 1).
    static void randomWalkDrift(uint32_t& rng, float amount, float sampleTime,
                                float& d1A, float& d1B, float& d2A, float& d2B) {
        float driftSpeed = BASE_DRIFT_SPEED * amount;
        d1A += fastUniformSigned(rng) * driftSpeed * sampleTime;
        d1B += fastUniformSigned(rng) * driftSpeed * sampleTime;
        d2A += fastUniformSigned(rng) * driftSpeed * sampleTime;
        d2B += fastUniformSigned(rng) * driftSpeed * sampleTime;

        float driftLimit = DRIFT_LIMIT * amount;
        d1A = clamp(d1A, -driftLimit, driftLimit);
        d1B = clamp(d1B, -driftLimit, driftLimit);
        d2A = clamp(d2A, -driftLimit, driftLimit);
        d2B = clamp(d2B, -driftLimit, driftLimit);
    }

    static float stableVoiceOffset(int voice, int salt) {
        uint32_t state = 0x9e3779b9u ^ (uint32_t)((voice + 1) * 0x85ebca6bu) ^ (uint32_t)((salt + 1) * 0xc2b2ae35u);
        if (state == 0u)
            state = 0x6d2b79f5u;
        return fastUniformSigned(state);
    }

    ClairaudientModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        using ParameterHelper = shapetaker::ParameterHelper;

        // Frequency controls
        // V oscillator snaps to whole octaves (5 total values: -2, -1, 0, +1, +2)
        configParam(FREQ1_PARAM, FREQ1_OCT_MIN, FREQ1_OCT_MAX, 0.f, "v osc octave", " oct");

        // Z oscillator snaps to semitones (49 total values: -24 to +24 semitones)
        configParam(FREQ2_PARAM, -FREQ2_ST_RANGE, FREQ2_ST_RANGE, 0.f, "z osc semitone", " st");

        // Initialize parameter snapping based on default quantization modes
        updateParameterSnapping();

        // Fine tune controls (±20 cents, centered at 0 for no detune)
        configParam(FINE1_PARAM, -FINE_TUNE_RANGE, FINE_TUNE_RANGE, 0.f, "v fine", " cents", 0.f, 100.f);
        configParam(FINE2_PARAM, -FINE_TUNE_RANGE, FINE_TUNE_RANGE, 0.f, "z fine", " cents", 0.f, 100.f);
        
        // Fine tune CV attenuverters
        ParameterHelper::configAttenuverter(this, FINE1_ATTEN_PARAM, "v fine tune cv");
        ParameterHelper::configAttenuverter(this, FINE2_ATTEN_PARAM, "z fine tune cv");
        
        // Shape morphing controls (default to 50% for proper sigmoid)
        ParameterHelper::configGain(this, SHAPE1_PARAM, "v shape", 0.5f);
        ParameterHelper::configGain(this, SHAPE2_PARAM, "z shape", 0.5f);
        
        // Shape CV attenuverters
        ParameterHelper::configAttenuverter(this, SHAPE1_ATTEN_PARAM, "v shape cv");
        ParameterHelper::configAttenuverter(this, SHAPE2_ATTEN_PARAM, "z shape cv");
        
        // Crossfade control (centered at 0.5)
        ParameterHelper::configMix(this, XFADE_PARAM, "crossfade", 0.5f);
        
        // Crossfade CV attenuverter
        ParameterHelper::configAttenuverter(this, XFADE_ATTEN_PARAM, "crossfade cv");
        
        // Sync switches: cross-sync (V resets Z) and reverse sync (V reverses Z direction)
        configSwitch(SYNC1_PARAM, 0.f, 1.f, 0.f, "cross sync", {"off", "on"});
        configSwitch(SYNC2_PARAM, 0.f, 2.f, 0.f, "reverse sync", {"off", "on", "mutual"});

        // Voice engine: internal audio-rate sigmoid slope/skew modulation,
        // ratio-locked to each oscillator's own pitch.
        ParameterHelper::configGain(this, VOICE_DEPTH_PARAM, "voice depth", 0.5f);
        configSwitch(VOICE_RATIO_PARAM, 0.f, 7.f, 3.f, "voice ratio",
                     {"\u00d70.5", "\u00d71", "\u00d71.5", "\u00d72", "\u00d73", "\u00d74", "\u00d75", "\u00d77"});
        ParameterHelper::configGain(this, VOICE_ASYM_PARAM, "slope asymmetry", 0.35f);
        configParam(VOICE_WIDTH_PARAM, 0.f, 1.f, 0.5f, "stereo width", "%", 0.f, 200.f);
        ParameterHelper::configAttenuverter(this, REV_CHANCE_ATTEN_PARAM, "rev sync flip chance cv");
        ParameterHelper::configGain(this, REV_CHANCE_PARAM, "rev sync flip chance", 1.f);
        
        // Inputs
        ParameterHelper::configCVInput(this, VOCT1_INPUT, "v osc v/oct");
        ParameterHelper::configCVInput(this, VOCT2_INPUT, "z osc v/oct");
        ParameterHelper::configCVInput(this, FINE1_CV_INPUT, "v fine tune cv");
        ParameterHelper::configCVInput(this, FINE2_CV_INPUT, "z fine tune cv");
        ParameterHelper::configCVInput(this, SHAPE1_CV_INPUT, "v shape cv");
        ParameterHelper::configCVInput(this, SHAPE2_CV_INPUT, "z shape cv");
        ParameterHelper::configCVInput(this, XFADE_CV_INPUT, "crossfade cv");
        ParameterHelper::configCVInput(this, REV_CHANCE_CV_INPUT, "rev sync flip chance cv");
        
        // Outputs
        ParameterHelper::configAudioOutput(this, LEFT_OUTPUT, "L");
        ParameterHelper::configAudioOutput(this, RIGHT_OUTPUT, "R");

        // Per-voice init: RNG seeded (avoid zero), stable Voice Character
        // offsets. VoiceState fields default-initialize themselves.
        for (int i = 0; i < MAX_POLY_VOICES; ++i) {
            uint32_t seed = rack::random::u32();
            rngState[i] = (seed == 0u) ? 0x6d2b79f5u : seed;
            for (int j = 0; j < 6; ++j) {
                voiceOffsets[i][j] = stableVoiceOffset(i, j + 1);
            }
        }
        sharedDriftRngState = rack::random::u32();
        if (sharedDriftRngState == 0u)
            sharedDriftRngState = 0x6d2b79f5u;

        shapetaker::ui::LabelFormatter::normalizeModuleControls(this);
    }

    json_t* dataToJson() override {
        syncSharedOscilloscopeThemeOverride();
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "quantizeOscV", json_boolean(quantizeOscV.load(std::memory_order_relaxed)));
        json_object_set_new(rootJ, "quantizeOscZ", json_boolean(quantizeOscZ.load(std::memory_order_relaxed)));
        json_object_set_new(rootJ, "oscNoiseAmount", json_real(oscNoiseAmount.load(std::memory_order_relaxed)));
        json_object_set_new(rootJ, "crossfadeMode", json_integer(crossfadeMode.load(std::memory_order_relaxed)));
        json_object_set_new(rootJ, "waveformMode", json_integer(waveformMode.load(std::memory_order_relaxed)));
        json_object_set_new(rootJ, "oversampleFactor", json_integer(oversampleFactor.load(std::memory_order_relaxed)));
        json_object_set_new(rootJ, "vintageAmount", json_real(vintageAmount.load(std::memory_order_relaxed)));
        int localTheme = oscilloscopeTheme.load(std::memory_order_relaxed);
        bool useSharedTheme = useSharedOscilloscopeTheme.load(std::memory_order_relaxed);
        int effectiveTheme = useSharedTheme
            ? shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeIndex()
            : clamp(localTheme, 0, shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1);
        json_object_set_new(rootJ, "oscopeTheme", json_integer(effectiveTheme));
        json_object_set_new(rootJ, "localOscopeTheme", json_integer(clamp(localTheme, 0, shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1)));
        json_object_set_new(rootJ, "useSharedOscopeTheme", json_boolean(useSharedTheme));
        json_object_set_new(rootJ, "oscopeDisplayMode", json_integer(oscilloscopeDisplayMode.load(std::memory_order_relaxed)));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        const int prevOversample = oversampleFactor.load(std::memory_order_relaxed);

        if (json_t* j = json_object_get(rootJ, "quantizeOscV"))
            quantizeOscV.store(json_boolean_value(j), std::memory_order_relaxed);

        if (json_t* j = json_object_get(rootJ, "quantizeOscZ"))
            quantizeOscZ.store(json_boolean_value(j), std::memory_order_relaxed);

        if (json_t* j = json_object_get(rootJ, "oscNoiseAmount"))
            oscNoiseAmount.store(clamp((float)json_number_value(j), 0.f, 1.f), std::memory_order_relaxed);

        if (json_t* j = json_object_get(rootJ, "crossfadeMode"))
            crossfadeMode.store(clamp((int)json_integer_value(j), CROSSFADE_EQUAL_POWER, CROSSFADE_STEREO_SWAP), std::memory_order_relaxed);

        if (json_t* j = json_object_get(rootJ, "waveformMode"))
            waveformMode.store(clamp((int)json_integer_value(j), WAVEFORM_SIGMOID_SAW, WAVEFORM_PWM), std::memory_order_relaxed);

        if (json_t* j = json_object_get(rootJ, "oversampleFactor")) {
            int newOversample = clamp((int)json_integer_value(j), MIN_OVERSAMPLE, MAX_OVERSAMPLE);
            oversampleFactor.store(newOversample, std::memory_order_relaxed);
            if (newOversample != prevOversample)
                pendingFilterReset.store(true, std::memory_order_relaxed);
        }

        if (json_t* j = json_object_get(rootJ, "vintageAmount"))
            vintageAmount.store(clamp((float)json_number_value(j), 0.f, 1.f), std::memory_order_relaxed);

        // "oscopeTheme" is the legacy key (holds the effective theme); newer
        // patches also save "localOscopeTheme", which overrides it below.
        if (json_t* j = json_object_get(rootJ, "oscopeTheme")) {
            int theme = clamp((int)json_integer_value(j), 0, shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1);
            oscilloscopeTheme.store(theme, std::memory_order_relaxed);
        }
        if (json_t* j = json_object_get(rootJ, "localOscopeTheme")) {
            int theme = clamp((int)json_integer_value(j), 0, shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1);
            oscilloscopeTheme.store(theme, std::memory_order_relaxed);
        }
        if (json_t* j = json_object_get(rootJ, "useSharedOscopeTheme")) {
            useSharedOscilloscopeTheme.store(json_is_true(j), std::memory_order_relaxed);
        } else {
            useSharedOscilloscopeTheme.store(true, std::memory_order_relaxed);
        }
        sharedOscilloscopeThemeRevisionSeen.store(
            shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeRevision(),
            std::memory_order_relaxed);

        if (json_t* j = json_object_get(rootJ, "oscopeDisplayMode"))
            oscilloscopeDisplayMode.store(clamp((int)json_integer_value(j), 0, 1), std::memory_order_relaxed);

        // Update parameter snapping after loading settings
        updateParameterSnapping();
    }

    void process(const ProcessArgs& args) override {
        if (pendingFilterReset.exchange(false, std::memory_order_acq_rel)) {
            resetFilters();
        }

        // Determine number of polyphonic voices (capped at MAX_POLY_VOICES
        // by PolyphonicProcessor)
        int channels = polyProcessor.updateChannels(
            {
                inputs[VOCT1_INPUT],
                inputs[VOCT2_INPUT],
                inputs[FINE1_CV_INPUT],
                inputs[FINE2_CV_INPUT],
                inputs[SHAPE1_CV_INPUT],
                inputs[SHAPE2_CV_INPUT],
                inputs[XFADE_CV_INPUT],
                inputs[REV_CHANCE_CV_INPUT],
            },
            {outputs[LEFT_OUTPUT], outputs[RIGHT_OUTPUT]});

        // Apply the configured oversampling factor (1×, 2×, 4×, or 8×, default 4×)
        const int oversample = std::max(1, oversampleFactor.load(std::memory_order_relaxed));
        const int crossfadeModeLocal = crossfadeMode.load(std::memory_order_relaxed);
        const int waveformModeLocal = waveformMode.load(std::memory_order_relaxed);
        // Vintage macro: bottom half sweeps 0..1x of the calibrated analog
        // package, top half extrapolates to 4x ("seasoning" -> "sauce").
        const float vintageLocal = vintageAmount.load(std::memory_order_relaxed);
        const float vintageEff = (vintageLocal <= 0.5f)
            ? vintageLocal * 2.f
            : 1.f + (vintageLocal - 0.5f) * 6.f;
        const float driftAmountLocal = vintageEff;
        const float driftCohesionLocal = 0.4f; // mostly independent wander, a little shared breathing
        const float voiceCharacterLocal = vintageEff;
        const float outputColorLocal = vintageEff;
        const float voiceRatioLocal = cachedVoiceRatio;
        // sqrt curve: even-harmonic gain is compressed at the top, so give the
        // low end of the knob more authority
        const float baseAsym = 0.9f * std::sqrt(clamp(cachedVoiceAsym, 0.f, 1.f));
        const float stereoWidthLocal = clamp(cachedVoiceWidth, 0.f, 1.f);
        // Width morphs the right copy's modulator phase from 0 (mono-coherent)
        // through quadrature to anti-phase (180 degrees at full) so the L/R
        // formant motion visibly opposes, plus a static slope offset.
        const float widthPhase = 0.5f * stereoWidthLocal;
        const float stereoScaleA = 1.f + 0.28f * stereoWidthLocal;
        const float stereoScaleB = 1.f - 0.28f * stereoWidthLocal;
        // Mid/side stage: knob 0 collapses to mono, center = natural stereo,
        // full doubles the side content. The detuned copies carry a lot of
        // natural side energy, so the lower half is squared to spread the
        // mono->natural transition across the whole half instead of the
        // first few degrees. Applied before the output soft clip.
        const float widthT = 2.f * stereoWidthLocal;
        const float widthSideGain = (widthT <= 1.f) ? widthT * widthT : widthT;
        const bool mutualRevLocal = cachedMutualRev;
        float oversampleRate = args.sampleRate * oversample;

        // Pre-calculate constants that are the same for all voices and oversample iterations
        const float oscNoise = oscNoiseAmount.load(std::memory_order_relaxed);
        if (oscNoise != cachedOscNoiseAmount) {
            cachedOscNoiseAmount = oscNoise;
            cachedShapedNoise = std::pow(clamp(oscNoise, 0.f, 1.f), NOISE_SHAPE_EXP);
        }
        float shapedNoise = cachedShapedNoise;
        float invOversampleRate = 1.f / oversampleRate; // Pre-compute reciprocal for faster multiplication

        if (args.sampleRate != cachedSampleRate || oversample != cachedOversample) {
            updateFilterCoefficients(args.sampleRate, oversample);
        }

        // Parameter decimation: only read parameters every N samples for performance
        // ~0.7ms latency at 44.1kHz is imperceptible but saves ~15-20% CPU
        if (paramDecimationCounter == 0) {
            // Cache base parameter values (before CV modulation)
            cachedBasePitch1 = params[FREQ1_PARAM].getValue();
            if (quantizeOscV.load(std::memory_order_relaxed))
                cachedBasePitch1 = shapetaker::dsp::PitchHelper::quantizeToOctave(cachedBasePitch1);

            cachedBaseSemitoneZ = params[FREQ2_PARAM].getValue();
            if (quantizeOscZ.load(std::memory_order_relaxed))
                cachedBaseSemitoneZ = shapetaker::dsp::PitchHelper::quantizeToSemitone(cachedBaseSemitoneZ, FREQ2_ST_RANGE);

            cachedFineTune1 = params[FINE1_PARAM].getValue();
            cachedFineTune2 = params[FINE2_PARAM].getValue();
            cachedShape1 = params[SHAPE1_PARAM].getValue();
            cachedShape2 = params[SHAPE2_PARAM].getValue();
            cachedXfade = params[XFADE_PARAM].getValue();
            cachedFine1Atten = params[FINE1_ATTEN_PARAM].getValue() * CV_FINE_SCALE;
            cachedFine2Atten = params[FINE2_ATTEN_PARAM].getValue() * CV_FINE_SCALE;
            cachedShape1Atten = params[SHAPE1_ATTEN_PARAM].getValue() * CV_SHAPE_SCALE;
            cachedShape2Atten = params[SHAPE2_ATTEN_PARAM].getValue() * CV_SHAPE_SCALE;
            cachedXfadeAtten = params[XFADE_ATTEN_PARAM].getValue() * CV_XFADE_SCALE;
            cachedSync1 = params[SYNC1_PARAM].getValue() > 0.5f;
            float sync2Value = params[SYNC2_PARAM].getValue();
            cachedSync2 = sync2Value >= 0.5f;
            cachedMutualRev = sync2Value >= 1.5f;
            cachedRevChance = params[REV_CHANCE_PARAM].getValue();
            cachedVoiceDepth = params[VOICE_DEPTH_PARAM].getValue();
            cachedVoiceRatio = VOICE_RATIOS[clamp((int)std::lround(params[VOICE_RATIO_PARAM].getValue()), 0, 7)];
            cachedVoiceAsym = params[VOICE_ASYM_PARAM].getValue();
            cachedVoiceWidth = params[VOICE_WIDTH_PARAM].getValue();
            cachedRevChanceAtten = params[REV_CHANCE_ATTEN_PARAM].getValue() * CV_REV_CHANCE_SCALE;

            // Cache input connection states
            cachedVoct2Connected = inputs[VOCT2_INPUT].isConnected();
            cachedFine1CVConnected = inputs[FINE1_CV_INPUT].isConnected();
            cachedFine2CVConnected = inputs[FINE2_CV_INPUT].isConnected();
            cachedShape1CVConnected = inputs[SHAPE1_CV_INPUT].isConnected();
            cachedShape2CVConnected = inputs[SHAPE2_CV_INPUT].isConnected();
            cachedXfadeCVConnected = inputs[XFADE_CV_INPUT].isConnected();
            cachedRevChanceCVConnected = inputs[REV_CHANCE_CV_INPUT].isConnected();

            // Crossfade trig for the common (no CV) case
            cachedXfadeClamped = clamp(cachedXfade, 0.f, 1.f);
            float xfadeAngle = cachedXfadeClamped * (float)M_PI_2;
            cachedXfadeCos = std::cos(xfadeAngle);
            cachedXfadeSin = std::sin(xfadeAngle);
            cachedWidthBlend = std::sin(cachedXfadeClamped * (float)M_PI);
        }
        paramDecimationCounter = (paramDecimationCounter + 1) % kParamDecimation;

        // Drift updates can be decimated without audible impact (extremely slow movement).
        bool updateDrift = (driftDecimationCounter == 0);
        float driftSampleTime = updateDrift ? args.sampleTime * kDriftDecimation : args.sampleTime;
        driftDecimationCounter = (driftDecimationCounter + 1) % kDriftDecimation;
        updateSharedDrift(driftSampleTime, driftAmountLocal, updateDrift);

        // Pre-calculate Voice Character offsets per channel. Every read below
        // is guarded by the same voiceCharacterLocal > 0 condition that fills
        // the active-channel entries here, so no default fill is needed.
        float vcPitchOffset[MAX_POLY_VOICES][2];
        float vcShapeOffset[MAX_POLY_VOICES][2];
        float vcPanLeft[MAX_POLY_VOICES];
        float vcPanRight[MAX_POLY_VOICES];
        if (voiceCharacterLocal > 0.f) {
            float pitchScale = VOICE_CHARACTER_PITCH_OCT * voiceCharacterLocal;
            float shapeScale = VOICE_CHARACTER_SHAPE * voiceCharacterLocal;
            float levelScale = VOICE_CHARACTER_LEVEL * voiceCharacterLocal;
            float panScale = VOICE_CHARACTER_PAN * voiceCharacterLocal;
            for (int ch = 0; ch < channels; ++ch) {
                vcPitchOffset[ch][0] = voiceOffsets[ch][0] * pitchScale;
                vcPitchOffset[ch][1] = voiceOffsets[ch][1] * pitchScale;
                vcShapeOffset[ch][0] = voiceOffsets[ch][2] * shapeScale;
                vcShapeOffset[ch][1] = voiceOffsets[ch][3] * shapeScale;
                float level = 1.f + voiceOffsets[ch][4] * levelScale;
                float pan = voiceOffsets[ch][5] * panScale;
                vcPanLeft[ch] = level * (1.f - pan);
                vcPanRight[ch] = level * (1.f + pan);
            }
        }

        // Pre-calculate Output Color parameters
        float colorDrive = 1.f;
        float colorGainInvTanhDrive = 1.f;
        if (outputColorLocal > 0.f) {
            colorDrive = 1.f + BUS_COLOR_DRIVE * outputColorLocal;
            colorGainInvTanhDrive = OUTPUT_GAIN / std::tanh(colorDrive);
        }

        bool isPwm = (waveformModeLocal == WAVEFORM_PWM);

        const int voct2Channels = cachedVoct2Connected ? inputs[VOCT2_INPUT].getChannels() : 0;

        // Process each voice
        for (int ch = 0; ch < channels; ch++) {
            VoiceState& vs = voiceState[ch];
            float finalLeft = 0.f;
            float finalRight = 0.f;

            // --- Pre-calculate parameters for this voice ---
            // Get V/Oct inputs with fallback logic (use cached connection state).
            // Z follows V per-lane: a mono Z cable broadcasts to all voices, a
            // poly Z cable covers its own channels, and lanes beyond its width
            // fall back to their V partner instead of reading 0 V (which would
            // drone at the Z knob pitch).
            float voct1 = inputs[VOCT1_INPUT].getPolyVoltage(ch);
            float voct2 = (cachedVoct2Connected && (voct2Channels == 1 || ch < voct2Channels)) ?
                            inputs[VOCT2_INPUT].getPolyVoltage(ch) : voct1;

            // Get parameters for this voice (use cached base values)
            // V Oscillator: use pre-quantized cached value, then add CV
            float pitch1 = cachedBasePitch1 + voct1;

            // Z Oscillator: use pre-quantized cached value, then add CV
            float pitch2 = cachedBaseSemitoneZ / SEMITONES_PER_OCTAVE + voct2;

            if (voiceCharacterLocal > 0.f) {
                pitch1 += vcPitchOffset[ch][0];
                pitch2 += vcPitchOffset[ch][1];
            }

            float fineTune1 = applyAttenuvertedCv(cachedFineTune1, cachedFine1CVConnected, inputs[FINE1_CV_INPUT], ch, cachedFine1Atten, -FINE_TUNE_RANGE, FINE_TUNE_RANGE);
            float fineTune2 = applyAttenuvertedCv(cachedFineTune2, cachedFine2CVConnected, inputs[FINE2_CV_INPUT], ch, cachedFine2Atten, -FINE_TUNE_RANGE, FINE_TUNE_RANGE);

            // Convert semitone offsets to octaves
            fineTune1 /= SEMITONES_PER_OCTAVE;
            fineTune2 /= SEMITONES_PER_OCTAVE;

            float shape1 = applyAttenuvertedCv(cachedShape1, cachedShape1CVConnected, inputs[SHAPE1_CV_INPUT], ch, cachedShape1Atten, 0.f, 1.f);
            float shape2 = applyAttenuvertedCv(cachedShape2, cachedShape2CVConnected, inputs[SHAPE2_CV_INPUT], ch, cachedShape2Atten, 0.f, 1.f);
            if (voiceCharacterLocal > 0.f) {
                shape1 = clamp(shape1 + vcShapeOffset[ch][0], 0.f, 1.f);
                shape2 = clamp(shape2 + vcShapeOffset[ch][1], 0.f, 1.f);
            }

            // Get crossfade parameter with attenuverter (use cached base value)
            float xfade = applyAttenuvertedCv(cachedXfade, cachedXfadeCVConnected, inputs[XFADE_CV_INPUT], ch, cachedXfadeAtten, 0.f, 1.f);
            float xfadeClamped = cachedXfadeCVConnected ? xfade : cachedXfadeClamped;

            // REV. CHANCE CV is additive to the panel knob and uses the
            // repurposed bipolar attenuverter. At its maximum magnitude,
            // 0..10 V spans the complete 0..100% chance range; reversing the
            // attenuverter subtracts that unipolar CV from the panel value.
            const float revChanceLocal = applyAttenuvertedCv(
                cachedRevChance,
                cachedRevChanceCVConnected,
                inputs[REV_CHANCE_CV_INPUT],
                ch,
                cachedRevChanceAtten,
                0.f,
                1.f);

            // Add organic frequency drift and per-oscillator phase-noise sources.
            updateOrganicVariation(vs, rngState[ch], driftSampleTime, driftAmountLocal, shapedNoise, updateDrift);

            // Symmetric detune: A goes flat by half, B goes sharp by half.
            float driftBlend = clamp(driftCohesionLocal, 0.f, 1.f);
            float vDriftA = vs.v.a.drift * (1.f - driftBlend) + sharedDrift1A * driftBlend;
            float vDriftB = vs.v.b.drift * (1.f - driftBlend) + sharedDrift1B * driftBlend;
            float zDriftA = vs.z.a.drift * (1.f - driftBlend) + sharedDrift2A * driftBlend;
            float zDriftB = vs.z.b.drift * (1.f - driftBlend) + sharedDrift2B * driftBlend;
            OscillatorPairFrame vOsc = makeOscillatorPair(pitch1, fineTune1, vDriftA, vDriftB, shape1, baseAsym, invOversampleRate, oversampleRate);
            OscillatorPairFrame zOsc = makeOscillatorPair(pitch2, fineTune2, zDriftA, zDriftB, shape2, baseAsym, invOversampleRate, oversampleRate);

            // Use cached sync switch states (doesn't change during oversampling)
            bool sync1 = cachedSync1;
            bool sync2 = cachedSync2;

            // When neither sync mode is active, phase directions are always forward.
            // Set once here rather than redundantly on every oversample iteration.
            if (!sync1 && !sync2) {
                vs.z.a.dir = 1.f;
                vs.z.b.dir = 1.f;
                vs.v.revTravel = 1.f;
                vs.z.revTravel = 1.f;
            }
            // V only runs backward in mutual reverse-sync mode
            if (!sync2 || !mutualRevLocal) {
                vs.v.a.dir = 1.f;
                vs.v.b.dir = 1.f;
            }

            // Pre-calculate crossfade coefficients outside loop to avoid repeated sin/cos
            float xfadeAngle = xfadeClamped * (float)M_PI_2;
            float xfadeCos = cachedXfadeCVConnected ? std::cos(xfadeAngle) : cachedXfadeCos;
            float xfadeSin = cachedXfadeCVConnected ? std::sin(xfadeAngle) : cachedXfadeSin;
            bool stereoSwap = (crossfadeModeLocal == CROSSFADE_STEREO_SWAP);
            // Width accent for swap: crossfeed with opposite polarity peaks at mid fade
            float widthBlend = cachedXfadeCVConnected ? std::sin(xfadeClamped * (float)M_PI) : cachedWidthBlend;
            float widthGain = STEREO_SWAP_WIDTH_GAIN * widthBlend;

            // Phase steps with subtle phase noise folded in (scaled by shaped
            // user amount) for organic character
            const float noiseScale = PHASE_NOISE_SCALE * shapedNoise;
            const float step1A = vOsc.deltaA + vs.v.a.noise * noiseScale;
            const float step1B = vOsc.deltaB + vs.v.b.noise * noiseScale;
            const float step2A_base = zOsc.deltaA + vs.z.a.noise * noiseScale;
            const float step2B_base = zOsc.deltaB + vs.z.b.noise * noiseScale;

            // Voice engine depth is set directly by its panel knob. The tanh
            // sigmoid eats small slope swings, so a ^1.5 curve keeps the
            // knob's low end from feeling dead while mid-knob lands on the
            // sweet zone.
            const float voiceDepthKnob = clamp(cachedVoiceDepth, 0.f, 1.f);
            const float voiceDepthLocal = voiceDepthKnob * std::sqrt(voiceDepthKnob);
            const bool voiceEngineActive = voiceDepthLocal > 0.001f || baseAsym > 0.001f || stereoWidthLocal > 0.001f;

            for (int os = 0; os < oversample; os++) {

                // Capture pre-advance phases for direction-agnostic edge detection
                const float pre1A = vs.v.a.phase;
                const float pre2A = vs.z.a.phase;

                vs.v.a.phase += step1A * vs.v.a.dir;
                vs.v.b.phase += step1B * vs.v.b.dir;
                vs.z.a.phase += step2A_base * vs.z.a.dir;
                vs.z.b.phase += step2B_base * vs.z.b.dir;

                wrapPhaseBidirectional(vs.v.a.phase);
                wrapPhaseBidirectional(vs.v.b.phase);
                wrapPhaseBidirectional(vs.z.a.phase);
                wrapPhaseBidirectional(vs.z.b.phase);

                // A cycle wrap shows up as a large phase jump (works in both
                // directions); a half-cycle crossing flips the >=0.5 test
                // without a wrap. Both serve as reverse-sync trigger edges.
                bool vCycleComplete = std::fabs(vs.v.a.phase - pre1A) > 0.5f;
                bool vHalfCross = !vCycleComplete && ((pre1A < 0.5f) != (vs.v.a.phase < 0.5f));

                // Travel since each oscillator's last issued flip. Normal edges
                // are 0.5 cycles apart so the 0.25 guard never blocks them; it
                // only suppresses wrap-boundary re-crossings, which otherwise
                // lock deterministic (100% chance) mutual flipping into a
                // phase ping-pong that pins both oscillators at the wrap.
                vs.v.revTravel = std::min(vs.v.revTravel + step1A, 1.f);
                vs.z.revTravel = std::min(vs.z.revTravel + step2A_base, 1.f);

                if (sync1) {
                    // Cross-sync: V master resets Z slave phases
                    if (vCycleComplete) {
                        float phaseOffset = vs.z.b.phase - vs.z.a.phase;
                        vs.z.a.phase = vs.v.a.phase;
                        vs.z.b.phase = vs.v.a.phase + phaseOffset;
                        wrapPhaseBidirectional(vs.z.b.phase);
                        vs.z.a.dir = 1.f;
                        vs.z.b.dir = 1.f;
                    }
                } else if (sync2) {
                    // Reverse sync: V flips Z's direction. The left copy flips
                    // on V's cycle edge and the right copy on V's half-cycle
                    // edge, so the gnarl decorrelates across the stereo field.
                    // revChance < 1 makes flips probabilistic (loose, lurching).
                    if (vCycleComplete && vs.v.revTravel >= 0.25f && (revChanceLocal >= 1.f || fastUniform(rngState[ch]) < revChanceLocal)) {
                        vs.z.a.dir = -vs.z.a.dir;
                        vs.v.revTravel = 0.f;
                    }
                    if (vHalfCross && vs.v.revTravel >= 0.25f && (revChanceLocal >= 1.f || fastUniform(rngState[ch]) < revChanceLocal)) {
                        vs.z.b.dir = -vs.z.b.dir;
                        vs.v.revTravel = 0.f;
                    }
                    if (mutualRevLocal) {
                        // Mutual: Z's cycle edge flips V right back — two
                        // oscillators shoving each other around chaotically.
                        bool zCycleComplete = std::fabs(vs.z.a.phase - pre2A) > 0.5f;
                        if (zCycleComplete && vs.z.revTravel >= 0.25f && (revChanceLocal >= 1.f || fastUniform(rngState[ch]) < revChanceLocal)) {
                            vs.v.a.dir = -vs.v.a.dir;
                            vs.v.b.dir = -vs.v.b.dir;
                            vs.z.revTravel = 0.f;
                        }
                    }
                }

                float osc1A, osc1B, osc2A, osc2B;
                if (isPwm) {
                    float pw1A = vOsc.pwmShape, pw1B = vOsc.pwmShape;
                    float pw2A = zOsc.pwmShape, pw2B = zOsc.pwmShape;
                    if (voiceDepthLocal > 0.001f) {
                        // Ratio-locked audio-rate PW modulation; width morphs
                        // the right copy's modulator phase toward anti-phase.
                        // ASYM skews the modulation trajectory (SHAPE already
                        // owns the static duty cycle in this mode).
                        float pwDepth = 0.45f * voiceDepthLocal;
                        float pwSkewK = 0.5f + 0.44f * baseAsym;
                        pw1A = clamp(vOsc.pwmShape + pwDepth * skewedSin2Pi(vs.v.a.phase * voiceRatioLocal, pwSkewK), 0.05f, 0.95f);
                        pw1B = clamp(vOsc.pwmShape + pwDepth * skewedSin2Pi(vs.v.a.phase * voiceRatioLocal + widthPhase, pwSkewK), 0.05f, 0.95f);
                        pw2A = clamp(zOsc.pwmShape + pwDepth * skewedSin2Pi(vs.z.a.phase * voiceRatioLocal, pwSkewK), 0.05f, 0.95f);
                        pw2B = clamp(zOsc.pwmShape + pwDepth * skewedSin2Pi(vs.z.a.phase * voiceRatioLocal + widthPhase, pwSkewK), 0.05f, 0.95f);
                    }
                    osc1A = OscillatorHelper::pwmWithPolyBLEP(vs.v.a.phase, pw1A, vOsc.deltaA);
                    osc1B = OscillatorHelper::pwmWithPolyBLEP(vs.v.b.phase, pw1B, vOsc.deltaB);
                    osc2A = OscillatorHelper::pwmWithPolyBLEP(vs.z.a.phase, pw2A, zOsc.deltaA);
                    osc2B = OscillatorHelper::pwmWithPolyBLEP(vs.z.b.phase, pw2B, zOsc.deltaB);
                } else {
                    // Voice engine: sigmoid slope and transition center
                    // modulated at audio rate, ratio-locked to each
                    // oscillator's own phase. The in-phase component wobbles
                    // slope (formant sweep), the quadrature component skews
                    // the duty cycle (even harmonics), and the A/B copies
                    // swap roles so the movement swirls between the left and
                    // right outputs.
                    float r1A, c1A, r1B, c1B, r2A, c2A, r2B, c2B;
                    if (voiceEngineActive) {
                        float base1 = vs.v.a.phase * voiceRatioLocal;
                        float base2 = vs.z.a.phase * voiceRatioLocal;
                        float mv = OscillatorHelper::fastSin2Pi(base1);
                        float qv = OscillatorHelper::fastSin2Pi(base1 + 0.25f);
                        float mvB = OscillatorHelper::fastSin2Pi(base1 + widthPhase);
                        float qvB = OscillatorHelper::fastSin2Pi(base1 + 0.25f + widthPhase);
                        float mz = OscillatorHelper::fastSin2Pi(base2);
                        float qz = OscillatorHelper::fastSin2Pi(base2 + 0.25f);
                        float mzB = OscillatorHelper::fastSin2Pi(base2 + widthPhase);
                        float qzB = OscillatorHelper::fastSin2Pi(base2 + 0.25f + widthPhase);
                        voiceMod(vOsc.rangeBase, mv, qv, baseAsym, voiceDepthLocal, stereoScaleA, r1A, c1A);
                        voiceMod(vOsc.rangeBase, mvB, qvB, baseAsym, voiceDepthLocal, stereoScaleB, r1B, c1B);
                        voiceMod(zOsc.rangeBase, mz, qz, baseAsym, voiceDepthLocal, stereoScaleA, r2A, c2A);
                        voiceMod(zOsc.rangeBase, mzB, qzB, baseAsym, voiceDepthLocal, stereoScaleB, r2B, c2B);
                    } else {
                        r1A = r1B = vOsc.rangeBase;
                        r2A = r2B = zOsc.rangeBase;
                        c1A = c1B = c2A = c2B = 0.5f;
                    }

                    osc1A = OscillatorHelper::voicedSigmoidSaw(vs.v.a.phase, vOsc.blend, r1A, c1A, vOsc.lowShape, vOsc.airGainA, vOsc.season);
                    osc1B = OscillatorHelper::voicedSigmoidSaw(vs.v.b.phase, vOsc.blend, r1B, c1B, vOsc.lowShape, vOsc.airGainB, vOsc.season);
                    osc2A = OscillatorHelper::voicedSigmoidSaw(vs.z.a.phase, zOsc.blend, r2A, c2A, zOsc.lowShape, zOsc.airGainA, zOsc.season);
                    osc2B = OscillatorHelper::voicedSigmoidSaw(vs.z.b.phase, zOsc.blend, r2B, c2B, zOsc.lowShape, zOsc.airGainB, zOsc.season);

                    if (vs.v.a.dir > 0.f) {
                        osc1A -= vOsc.wrapJump * phaseWrapBlep(vs.v.a.phase, vOsc.deltaA);
                    }
                    if (vs.v.b.dir > 0.f) {
                        osc1B -= vOsc.wrapJump * phaseWrapBlep(vs.v.b.phase, vOsc.deltaB);
                    }
                    if (vs.z.a.dir > 0.f) {
                        osc2A -= zOsc.wrapJump * phaseWrapBlep(vs.z.a.phase, zOsc.deltaA);
                    }
                    if (vs.z.b.dir > 0.f) {
                        osc2B -= zOsc.wrapJump * phaseWrapBlep(vs.z.b.phase, zOsc.deltaB);
                    }
                }

                float leftOutput;
                float rightOutput;

                // Use pre-calculated trig values to avoid sin/cos in hot loop
                if (!stereoSwap) {
                    leftOutput  = osc1A * xfadeCos + osc2A * xfadeSin;
                    rightOutput = osc1B * xfadeCos + osc2B * xfadeSin;
                } else {
                    float baseLeft  = osc1A * xfadeCos + osc2B * xfadeSin;
                    float baseRight = osc1B * xfadeCos + osc2A * xfadeSin;
                    // Out-of-phase crossfeed widens and makes swap distinct from equal-power
                    float leftCross  = -(osc1B * (1.f - xfadeClamped) + osc2A * xfadeClamped);
                    float rightCross = -(osc1A * (1.f - xfadeClamped) + osc2B * xfadeClamped);
                    leftOutput  = baseLeft  + widthGain * leftCross;
                    rightOutput = baseRight + widthGain * rightCross;
                }

                if (widthSideGain != 1.f) {
                    float mid = 0.5f * (leftOutput + rightOutput);
                    float side = 0.5f * (leftOutput - rightOutput);
                    leftOutput = mid + side * widthSideGain;
                    rightOutput = mid - side * widthSideGain;
                }

                leftOutput = OscillatorHelper::fastTanh(leftOutput);
                rightOutput = OscillatorHelper::fastTanh(rightOutput);

                if (oversample > 1) {
                    decimatorLeft[ch].buf[os] = leftOutput;
                    decimatorRight[ch].buf[os] = rightOutput;
                } else {
                    finalLeft = leftOutput;
                    finalRight = rightOutput;
                }
            }

            if (oversample > 1) {
                finalLeft = processDecimator(decimatorLeft[ch], oversample);
                finalRight = processDecimator(decimatorRight[ch], oversample);
            }

            float outL = finalLeft * OUTPUT_GAIN;
            float outR = finalRight * OUTPUT_GAIN;

            // DC blocking (~10 Hz high-pass) removes offset from asymmetric waveshaping
            outL = shapetaker::dsp::AudioProcessor::processDCBlock(outL, vs.dcL.lastIn, vs.dcL.lastOut, cachedDCCoeff);
            outR = shapetaker::dsp::AudioProcessor::processDCBlock(outR, vs.dcR.lastIn, vs.dcR.lastOut, cachedDCCoeff);

            // Add audible white noise floor scaled by user amount (post waveshaping, in volts)
            if (shapedNoise > 0.f) {
                outL += fastUniformSigned(rngState[ch]) * NOISE_V_PEAK * shapedNoise;
                outR += fastUniformSigned(rngState[ch]) * NOISE_V_PEAK * shapedNoise;
            }

            if (voiceCharacterLocal > 0.f) {
                outL *= vcPanLeft[ch];
                outR *= vcPanRight[ch];
            }

            if (outputColorLocal > 0.f) {
                // Drive and crosstalk keep scaling past calibration ("sauce"),
                // but the wet/dry blend saturates at fully wet.
                float crosstalk = std::min(BUS_COLOR_CROSSTALK * outputColorLocal, 0.15f);
                float colorMix = std::min(outputColorLocal, 1.f);
                float mixedLeft = outL * (1.f - crosstalk) + outR * crosstalk;
                float mixedRight = outR * (1.f - crosstalk) + outL * crosstalk;

                float normL = mixedLeft / OUTPUT_GAIN;
                float normR = mixedRight / OUTPUT_GAIN;

                // Output Color is intentionally a light base-rate bus saturation.
                float coloredL = colorGainInvTanhDrive * OscillatorHelper::fastTanh(normL * colorDrive);
                float coloredR = colorGainInvTanhDrive * OscillatorHelper::fastTanh(normR * colorDrive);

                outL = mixedLeft * (1.f - colorMix) + coloredL * colorMix;
                outR = mixedRight * (1.f - colorMix) + coloredR * colorMix;
            }

            if (!std::isfinite(outL)) outL = 0.f;
            if (!std::isfinite(outR)) outR = 0.f;

            outputs[LEFT_OUTPUT].setVoltage(outL, ch);
            outputs[RIGHT_OUTPUT].setVoltage(outR, ch);
            
            // Use first voice for oscilloscope display
            if (ch == 0) {
                // --- Adaptive Oscilloscope Timescale ---
                // Blend the two pitches in log space by crossfade position so
                // the timescale glides instead of jumping at center.
                float xfBlend = clamp(xfade, 0.f, 1.f);
                float dominantFreq = MIDDLE_C_HZ * rack::dsp::exp2_taylor5(pitch1 + (pitch2 - pitch1) * xfBlend);
                dominantFreq = std::max(dominantFreq, 1.f); // Prevent division by zero or very small numbers

                float targetCycles = (oscilloscopeDisplayMode.load(std::memory_order_relaxed) == 1)
                    ? OSCOPE_TARGET_CYCLES_WAVE : OSCOPE_TARGET_CYCLES;
                int downsampleFactor = (int)roundf((targetCycles * args.sampleRate) / (OSCILLOSCOPE_BUFFER_SIZE * dominantFreq));
                downsampleFactor = clamp(downsampleFactor, 1, OSCOPE_MAX_DOWNSAMPLE);
                
                // --- Oscilloscope Buffering Logic ---
                // Downsample the audio rate to fill the buffer at a reasonable speed for the UI
                oscilloscopeFrameCounter++;
                if (oscilloscopeFrameCounter >= downsampleFactor) {
                    oscilloscopeFrameCounter = 0;
                    
                    int currentIndex = oscilloscopeBufferIndex.load(std::memory_order_relaxed);
                    // Store the current output voltages in the circular buffer
                    oscilloscopeBufferPacked[currentIndex].store(packVec(outL, outR), std::memory_order_relaxed);
                    oscilloscopeBufferIndex.store((currentIndex + 1) % OSCILLOSCOPE_BUFFER_SIZE, std::memory_order_release);
                }
            }
        }

    }

    // --- IOscilloscopeSource Implementation ---
    const Vec* getOscilloscopeBuffer() const override {
        int snapshot = oscilloscopeBufferIndex.load(std::memory_order_acquire);
        oscilloscopeReadIndex = snapshot;
        for (int i = 0; i < OSCILLOSCOPE_BUFFER_SIZE; ++i) {
            uint64_t packed = oscilloscopeBufferPacked[i].load(std::memory_order_relaxed);
            oscilloscopeBuffer[i] = unpackVec(packed);
        }
        return oscilloscopeBuffer;
    }
    int getOscilloscopeBufferIndex() const override { return oscilloscopeReadIndex; }
    int getOscilloscopeBufferSize() const override { return OSCILLOSCOPE_BUFFER_SIZE; }
    int getOscilloscopeDisplayMode() const override {
        return clamp(oscilloscopeDisplayMode.load(std::memory_order_relaxed), 0, 1);
    }
    int getOscilloscopeTheme() const override {
        syncSharedOscilloscopeThemeOverride();
        return useSharedOscilloscopeTheme.load(std::memory_order_relaxed)
            ? shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeIndex()
            : clamp(oscilloscopeTheme.load(std::memory_order_relaxed), 0, shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1);
    }

private:
    void syncSharedOscilloscopeThemeOverride() const {
        int revision = shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeRevision();
        if (sharedOscilloscopeThemeRevisionSeen.load(std::memory_order_relaxed) != revision) {
            useSharedOscilloscopeTheme.store(true, std::memory_order_relaxed);
            sharedOscilloscopeThemeRevisionSeen.store(revision, std::memory_order_relaxed);
        }
    }

    void updateSharedDrift(float sampleTime, float amount, bool updateDrift) {
        amount = clamp(amount, 0.f, 4.f); // vintage macro extrapolates to 4x calibration
        if (amount <= 0.f) {
            sharedDrift1A = sharedDrift1B = sharedDrift2A = sharedDrift2B = 0.f;
            return;
        }
        if (!updateDrift)
            return;

        randomWalkDrift(sharedDriftRngState, amount, sampleTime,
                        sharedDrift1A, sharedDrift1B, sharedDrift2A, sharedDrift2B);
    }

    // Update organic drift and phase-noise sources for more natural sound (per voice)
    static void updateOrganicVariation(VoiceState& vs, uint32_t& rng, float sampleTime, float driftAmountLocal, float noiseAmountLocal, bool updateDrift) {
        driftAmountLocal = clamp(driftAmountLocal, 0.f, 4.f); // vintage macro extrapolates to 4x calibration
        noiseAmountLocal = clamp(noiseAmountLocal, 0.f, 1.f);

        if (driftAmountLocal <= 0.f) {
            vs.v.a.drift = vs.v.b.drift = vs.z.a.drift = vs.z.b.drift = 0.f;
        } else if (updateDrift) {
            randomWalkDrift(rng, driftAmountLocal, sampleTime,
                            vs.v.a.drift, vs.v.b.drift, vs.z.a.drift, vs.z.b.drift);
        }

        // Generate subtle phase noise from the noise control, independent of drift.
        if (noiseAmountLocal > 0.f) {
            vs.v.a.noise = fastUniformSigned(rng) * noiseAmountLocal;
            vs.v.b.noise = fastUniformSigned(rng) * noiseAmountLocal;
            vs.z.a.noise = fastUniformSigned(rng) * noiseAmountLocal;
            vs.z.b.noise = fastUniformSigned(rng) * noiseAmountLocal;
        } else {
            vs.v.a.noise = 0.f;
            vs.v.b.noise = 0.f;
            vs.z.a.noise = 0.f;
            vs.z.b.noise = 0.f;
        }
    }
};

struct ClairaudientWidget : ShapetakerModuleWidget {

    ClairaudientWidget(ClairaudientModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Clairaudient.svg")));

        using LayoutHelper = shapetaker::ui::LayoutHelper;

        // Keep wear/scratches beneath controls by adding the overlay early.
        auto overlay = new shapetaker::ui::PanelWearOverlay();
        overlay->box = Rect(Vec(0, 0), box.size);
        addChild(overlay);

        LayoutHelper::ScrewPositions::addStandardScrews<ScrewJetBlack>(this, box.size.x);
        addAlchemicalBadge(this, 61); // Heptagram inside circle

        // Use shared panel parser utilities for control placement
        auto svgPath = asset::plugin(pluginInstance, "res/panels/Clairaudient.svg");
        LayoutHelper::PanelSVGParser parser(svgPath);
        auto panelCenterPx = LayoutHelper::createCenterPxHelper(parser);
        // Clairaudient's artwork retains its original 20 HP viewBox and is
        // rendered at 18 HP. Apply the same 90% horizontal scale to control
        // centers so their normalized positions remain unchanged.
        auto centerPx = [&panelCenterPx](const std::string& id, float fallbackX, float fallbackY) {
            Vec center = panelCenterPx(id, fallbackX, fallbackY);
            center.x *= 0.9f;
            return center;
        };

        // Use global shadow helper from plugin.hpp
        auto addKnobWithShadow = [this](app::ParamWidget* knob) {
            ::addKnobWithShadow(this, knob);
        };
        
        // V/Z oscillator frequency knobs — vintage knob with background + shadow
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageLarge>(centerPx("freq_v", 17.f, 19.981817f), module, ClairaudientModule::FREQ1_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageLarge>(centerPx("freq_z", 84.6f, 19.981817f), module, ClairaudientModule::FREQ2_PARAM));

        // V/Z sync switches — ShapetakerDarkToggle (9.5 x 10.7mm, black body, grey lever)
        addParam(createParamCentered<ShapetakerDarkToggleOffPos4>(centerPx("sync_v", 29.5f, 67.278954f), module, ClairaudientModule::SYNC1_PARAM));
        addParam(createParamCentered<ShapetakerDarkToggleThreePos>(centerPx("sync_z", 72.1f, 67.278954f), module, ClairaudientModule::SYNC2_PARAM));

        // V/Z fine tune controls — Vintage small-medium (15mm) + shadow
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("fine_v", 29.5f, 47.834789f), module, ClairaudientModule::FINE1_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("fine_z", 72.1f, 47.834789f), module, ClairaudientModule::FINE2_PARAM));

        // V/Z fine tune attenuverters — ShapetakerAttenuverterOscilloscope (10mm) + shadow
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("fine_atten_v", 11.5f, 42.361408f), module, ClairaudientModule::FINE1_ATTEN_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("fine_atten_z", 90.1f, 42.361408f), module, ClairaudientModule::FINE2_ATTEN_PARAM));

        // Crossfade control — Vintage medium (18mm) + shadow
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageMedium>(centerPx("x_fade_knob", 51.286224f, 56.527908f), module, ClairaudientModule::XFADE_PARAM));

        // Crossfade attenuverter — ShapetakerAttenuverterOscilloscope (10mm) + shadow
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("x_fade_atten", 51.628757f, 75.480347f), module, ClairaudientModule::XFADE_ATTEN_PARAM));

        // V/Z shape controls — Vintage small-medium (15mm) + shadow
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("sh_knob_v", 11.5f, 61.978146f), module, ClairaudientModule::SHAPE1_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("sh_knob_z", 90.1f, 61.978146f), module, ClairaudientModule::SHAPE2_PARAM));

        // V/Z shape attenuverters — ShapetakerAttenuverterOscilloscope (10mm) + shadow
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("sh_cv_v", 11.5f, 79.612099f), module, ClairaudientModule::SHAPE1_ATTEN_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("sh_cv_z", 90.1f, 79.612099f), module, ClairaudientModule::SHAPE2_ATTEN_PARAM));

        // Voice engine rows
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("voice_depth", 12.95f, 98.f), module, ClairaudientModule::VOICE_DEPTH_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("voice_ratio", 35.3f, 98.f), module, ClairaudientModule::VOICE_RATIO_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("voice_asym", 88.65f, 98.f), module, ClairaudientModule::VOICE_ASYM_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("voice_width", 66.3f, 98.f), module, ClairaudientModule::VOICE_WIDTH_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageAttenuverter>(centerPx("rev_chance_atten", 30.6f, 78.862625f), module, ClairaudientModule::REV_CHANCE_ATTEN_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageAttenuverter>(centerPx("rev_chance", 71.f, 78.862625f), module, ClairaudientModule::REV_CHANCE_PARAM));

        // Vintage oscilloscope display (draw even in module browser previews)
        {
            VintageOscilloscopeWidget* oscope = new VintageOscilloscopeWidget(module);
            Vec scrPx = centerPx("oscope_screen", 50.8f, 25.167439f);
            constexpr float OSCOPE_SIZE_MM = 37.3f;
            Vec sizePx = LayoutHelper::mm2px(Vec(OSCOPE_SIZE_MM, OSCOPE_SIZE_MM));
            Vec topLeft = scrPx.minus(sizePx.div(2.f));
            oscope->box.pos = topLeft;
            oscope->box.size = sizePx;
            addChild(oscope);
        }

        // Input row 1: V oscillator — ShapetakerBNCPort (8mm)
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("v_oct_v", 11.f, 115.09749f), module, ClairaudientModule::VOCT1_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("fine_cv_v", 22.371429f, 115.09749f), module, ClairaudientModule::FINE1_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("shape_cv_v", 33.742857f, 115.09749f), module, ClairaudientModule::SHAPE1_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("x_fade_cv", 51.628765f, 89.733727f), module, ClairaudientModule::XFADE_CV_INPUT));

        // Input row 2: Z oscillator
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("v_out_z", 67.857143f, 115.09749f), module, ClairaudientModule::VOCT2_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("fine_cv_z", 79.228571f, 115.09749f), module, ClairaudientModule::FINE2_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("shape_cv_z", 90.6f, 115.09749f), module, ClairaudientModule::SHAPE2_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("rev_chance_cv", 50.8f, 102.24656f), module, ClairaudientModule::REV_CHANCE_CV_INPUT));

        // Stereo outputs — ShapetakerBNCPort (8mm)
        addOutputWithHalo<ShapetakerBNCPort>(this, centerPx("output_l", 45.114286f, 115.09749f), module, ClairaudientModule::LEFT_OUTPUT);
        addOutputWithHalo<ShapetakerBNCPort>(this, centerPx("output_r", 56.485714f, 115.09749f), module, ClairaudientModule::RIGHT_OUTPUT);

    }

    void appendContextMenu(Menu* menu) override {
        ClairaudientModule* module = dynamic_cast<ClairaudientModule*>(this->module);
        if (!module) return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Settings"));

        using DisplayTheme = shapetaker::ui::ThemeManager::DisplayTheme;

        menu->addChild(createSubmenuItem("Quantize", "", [=](Menu* subMenu) {
            subMenu->addChild(createCheckMenuItem("V Oscillator (octaves)", "", [=] {
                return module->quantizeOscV.load(std::memory_order_relaxed);
            }, [=] {
                module->quantizeOscV.store(!module->quantizeOscV.load(std::memory_order_relaxed), std::memory_order_relaxed);
                module->updateParameterSnapping();
            }));
            subMenu->addChild(createCheckMenuItem("Z Oscillator (semitones)", "", [=] {
                return module->quantizeOscZ.load(std::memory_order_relaxed);
            }, [=] {
                module->quantizeOscZ.store(!module->quantizeOscZ.load(std::memory_order_relaxed), std::memory_order_relaxed);
                module->updateParameterSnapping();
            }));
        }));

        menu->addChild(createSubmenuItem("Oscilloscope Display",
            module->oscilloscopeDisplayMode.load(std::memory_order_relaxed) == 1 ? "Waveform" : "Lissajous (X-Y)",
            [=](Menu* subMenu) {
                subMenu->addChild(createCheckMenuItem("Waveform (triggered)", "", [=] {
                    return module->oscilloscopeDisplayMode.load(std::memory_order_relaxed) == 1;
                }, [=] {
                    module->oscilloscopeDisplayMode.store(1, std::memory_order_relaxed);
                }));
                subMenu->addChild(createCheckMenuItem("Lissajous (X-Y)", "", [=] {
                    return module->oscilloscopeDisplayMode.load(std::memory_order_relaxed) == 0;
                }, [=] {
                    module->oscilloscopeDisplayMode.store(0, std::memory_order_relaxed);
                }));
            }));

        DisplayTheme::addThemeMenu(
            menu,
            [=] {
                return module->useSharedOscilloscopeTheme.load(std::memory_order_relaxed);
            },
            [=](bool follow) {
                module->useSharedOscilloscopeTheme.store(follow, std::memory_order_relaxed);
                module->sharedOscilloscopeThemeRevisionSeen.store(
                    DisplayTheme::getSharedThemeRevision(),
                    std::memory_order_relaxed);
            },
            [=] {
                return module->oscilloscopeTheme.load(std::memory_order_relaxed);
            },
            [=](int theme) {
                module->oscilloscopeTheme.store(theme, std::memory_order_relaxed);
            });

        menu->addChild(createSubmenuItem("Waveform",
            module->waveformMode.load(std::memory_order_relaxed) == ClairaudientModule::WAVEFORM_PWM ? "PWM" : "Sigmoid Saw",
            [=](Menu* subMenu) {
                auto addWaveformModeItem = [&](const std::string& label, int mode) {
                    subMenu->addChild(createCheckMenuItem(label, "", [=] {
                        return module->waveformMode.load(std::memory_order_relaxed) == mode;
                    }, [=] {
                        module->waveformMode.store(mode, std::memory_order_relaxed);
                    }));
                };
                addWaveformModeItem("Sigmoid Saw", ClairaudientModule::WAVEFORM_SIGMOID_SAW);
                addWaveformModeItem("PWM", ClairaudientModule::WAVEFORM_PWM);
            }));

        menu->addChild(createSubmenuItem("Crossfade Curve",
            module->crossfadeMode.load(std::memory_order_relaxed) == ClairaudientModule::CROSSFADE_STEREO_SWAP ? "Stereo Swap" : "Equal-Power",
            [=](Menu* subMenu) {
                auto addCrossfadeModeItem = [&](const std::string& label, int mode) {
                    subMenu->addChild(createCheckMenuItem(label, "", [=] {
                        return module->crossfadeMode.load(std::memory_order_relaxed) == mode;
                    }, [=] {
                        module->crossfadeMode.store(mode, std::memory_order_relaxed);
                    }));
                };
                addCrossfadeModeItem("Equal-Power", ClairaudientModule::CROSSFADE_EQUAL_POWER);
                addCrossfadeModeItem("Stereo Swap", ClairaudientModule::CROSSFADE_STEREO_SWAP);
            }));

        menu->addChild(createSubmenuItem("Oversampling",
            string::f("%d\u00d7", module->oversampleFactor.load(std::memory_order_relaxed)),
            [=](Menu* subMenu) {
                auto addOversampleItem = [&](const std::string& label, int factor) {
                    subMenu->addChild(createCheckMenuItem(label, "", [=] {
                        return module->oversampleFactor.load(std::memory_order_relaxed) == factor;
                    }, [=] {
                        module->oversampleFactor.store(factor, std::memory_order_relaxed);
                        module->pendingFilterReset.store(true, std::memory_order_relaxed);
                    }));
                };
                addOversampleItem("1\u00d7 (Off)", 1);
                addOversampleItem("2\u00d7", 2);
                addOversampleItem("4\u00d7", 4);
                addOversampleItem("8\u00d7", 8);
            }));

        // Character: vintage macro (drift + voice character + output color)
        // and audible noise floor. 50% vintage = calibrated behavior.
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Character (vintage 50% = calibrated)"));
        menu->addChild(shapetaker::ui::createPercentageSlider(
            module,
            [](ClairaudientModule* m, float v) { m->vintageAmount.store(v, std::memory_order_relaxed); },
            [](ClairaudientModule* m) { return m->vintageAmount.load(std::memory_order_relaxed); },
            "Vintage"
        ));
        menu->addChild(shapetaker::ui::createPercentageSlider(
            module,
            [](ClairaudientModule* m, float v) { m->oscNoiseAmount.store(v, std::memory_order_relaxed); },
            [](ClairaudientModule* m) { return m->oscNoiseAmount.load(std::memory_order_relaxed); },
            "Oscillator Noise"
        ));
    }
};

constexpr float ClairaudientModule::VOICE_RATIOS[];

Model* modelClairaudient = createModel<ClairaudientModule, ClairaudientWidget>("Clairaudient");
