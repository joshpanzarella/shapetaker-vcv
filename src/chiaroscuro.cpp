#include "plugin.hpp"
#include <cmath>
#include <array>

struct Chiaroscuro : Module {
    enum ParamIds {
        VCA_PARAM,
        TYPE_PARAM,
        DIST_PARAM,
        DIST_ATT_PARAM,
        DRIVE_PARAM,
        DRIVE_ATT_PARAM,
        MIX_PARAM,
        MIX_ATT_PARAM,
        LINK_PARAM,
        RESPONSE_PARAM,    // Linear/Exponential response switch
        NUM_PARAMS
    };

    enum InputIds {
        AUDIO_L_INPUT,
        AUDIO_R_INPUT,
        VCA_CV_INPUT,
        SIDECHAIN_INPUT,
        TYPE_CV_INPUT,
        DIST_CV_INPUT,
        DRIVE_CV_INPUT,
        MIX_CV_INPUT,
        NUM_INPUTS
    };

    enum OutputIds {
        AUDIO_L_OUTPUT,
        AUDIO_R_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds {
        DIST_LED_R,
        DIST_LED_G,
        DIST_LED_B,
        GAIN_LED_R,
        GAIN_LED_G,
        GAIN_LED_B,
        NUM_LIGHTS
    };

    // DSP constants (Clairaudient-style: keep process() free of unnamed literals)
    static constexpr float UNIT_MIN = 0.f;
    static constexpr float UNIT_MAX = 1.f;
    static constexpr float BIPOLAR_MIN = -1.f;
    static constexpr float BIPOLAR_MAX = 1.f;
    static constexpr float SWITCH_ON_THRESHOLD = 0.5f;
    static constexpr float DEFAULT_SAMPLE_RATE = 44100.f;
    static constexpr float NOMINAL_LEVEL = 5.f;          // Reference voltage used for distortion normalization
    static constexpr float CV_TO_UNIT_SCALE = 0.1f;      // 10V -> 1.0
    static constexpr float SIDECHAIN_MIN_WET_MIX = 0.8f;
    static constexpr float SIDECHAIN_DUCK_DRIVE = 0.7f;
    static constexpr float RATE_THRESHOLD_HZ10 = 6.28f;  // 2*pi*10Hz
    static constexpr float LED_INTENSITY_PEAK_HINT = 0.1f;
    static constexpr float LED_BRIGHTNESS_GAMMA = 0.55f;
    static constexpr float LED_BRIGHTNESS_BOOST = 1.8f;
    static constexpr float VCA_GAIN_MAX = 2.f;
    static constexpr float VCA_OPEN_START = 0.9f;
    static constexpr float VCA_OPEN_RANGE = 0.4f;
    static constexpr float HOT_SIGNAL_START_V = 6.f;
    static constexpr float HOT_SIGNAL_RANGE_V = 4.f;
    static constexpr float AGGRESSION_GAIN_SCALE = 0.18f;
    static constexpr float PRE_DRIVE_BOOST_SCALE = 0.35f;
    static constexpr float DRIVE_PRE_GAIN_SCALE = 1.8f;
    // Floor must be low enough for the makeup to fully duck the wet path back to
    // the clean level: at low VCA gain the input sits below the distortion's
    // clipping knee, so its input drive-gain (e.g. hard clip ~63x at full dist)
    // acts as pure gain. A higher floor cannot cancel that, leaving a large
    // volume jump when mix is raised.
    static constexpr float MAKEUP_GAIN_MIN = 0.005f;
    static constexpr float MAKEUP_GAIN_MAX = 4.f;
    static constexpr float MIX_MAKEUP_SCALE = 0.12f;
    static constexpr float SOFTCLIP_KNEE_V = 8.f;
    static constexpr float SOFTCLIP_CEIL_V = 11.7f;
    static constexpr float GAIN_LED_RESPONSE_HZ = 45.f;
    // How far the gain jewel drifts toward white across the above-unity boost
    // region; short of 1.0 so the theme colour stays readable at full CV.
    static constexpr float GAIN_BOOST_WHITE_MIX = 0.8f;
    static constexpr float DIST_SLEW_HZ = 400.f;
    static constexpr float SIDECHAIN_ATTACK_MS = 10.f;
    static constexpr float SIDECHAIN_RELEASE_MS = 200.f;
    static constexpr float AGGRESSION_ATTACK_MS = 5.f;
    static constexpr float AGGRESSION_RELEASE_MS = 80.f;
    static constexpr float DISPLAY_SMOOTH_FAST_HZ = 15.f;
    static constexpr float DISPLAY_SMOOTH_SLOW_HZ = 2.f;
    static constexpr float CONTROL_SMOOTH_HZ = 60.f;
    static constexpr float VCA_SMOOTH_HZ = 2000.f; // Use 200.f for a softer, slower VCA response.
    static constexpr float TYPE_SMOOTH_HZ = 50.f;
    static constexpr float TYPE_TRANSITION_MS = 12.f;
    static constexpr float LEVEL_TRACK_HZ = 5.f;
    // Fast-attack path for the power envelopes, engaged only when the
    // instantaneous power jumps past LEVEL_JUMP_RATIO x the tracked envelope.
    // Ordinary program material never trips it (steady-state tracking is
    // identical to plain LEVEL_TRACK_HZ smoothing); knob/CV-driven loudness
    // jumps and note onsets do. Both envelopes share the rule, so onsets keep
    // the clean/wet ratio — and thus the makeup gain — stable, while
    // wet-only jumps (the distortion knob) are caught in milliseconds.
    static constexpr float LEVEL_ATTACK_HZ = 60.f;
    static constexpr float LEVEL_JUMP_RATIO = 4.f;
    static constexpr float MAKEUP_SMOOTH_HZ = 10.f;
    // Limiter-style fast gain reduction, engaged only for large drops
    // (desired below MAKEUP_DROP_RATIO x current) so ordinary envelope
    // ripple keeps the original lazy smoothing.
    static constexpr float MAKEUP_ATTACK_HZ = 100.f;
    static constexpr float MAKEUP_DROP_RATIO = 0.7f;
    // Both fast-attack rules above are triggered by the *signal*, and neither
    // fires for an ordinary distortion-control move: the wet level typically
    // shifts 2-3 dB, which is under LEVEL_JUMP_RATIO and over
    // MAKEUP_DROP_RATIO, so a control move landed in the dead zone between
    // them and got the slowest possible correction — a 3-5 dB surge lasting
    // 70-100 ms every time DIST was turned or its CV swept. Falling levels
    // were worse still, having no fast path at all.
    //
    // The fix is to force the fast path from control motion instead. A signal
    // test cannot tell a knob move from a change in program level; the control
    // itself can. Gating on it leaves every program-driven response on exactly
    // the tuning above (measured bit-identical on a tremolo), so this does not
    // make the makeup gain any more compressor-like.
    //
    // A crossfade traversal quicker than this is faster than the loop can
    // follow reactively, so it engages the fast coefficients.
    static constexpr float CONTROL_MOTION_TIME_S = 0.02f;
    // How long they stay engaged after the control settles — long enough for
    // the power envelopes and the gain smoother to converge.
    static constexpr float CONTROL_MOTION_HOLD_S = 0.06f;
    static constexpr float MAKEUP_MIN_LEVEL = 1e-4f;
    static constexpr float MAKEUP_ACTIVE_THRESHOLD = 1e-3f;
    static constexpr float DISTORTION_BYPASS_THRESHOLD = 1e-4f;
    static constexpr float DISTORTION_ONSET_RANGE = 0.08f;
    static constexpr int MIN_OVERSAMPLE_FACTOR = 1;
    static constexpr int MAX_OVERSAMPLE_FACTOR = 8;
    static constexpr int DEFAULT_OVERSAMPLE_FACTOR = 4;
    static constexpr int OVERSAMPLE_QUALITY = 8;
    static constexpr int   DISTORTION_TYPE_COUNT    = 5; // hard clip, tube sat, wave fold, bit crush, ring mod
    static constexpr float TYPE_CV_SPAN = static_cast<float>(DISTORTION_TYPE_COUNT);
    static constexpr int MAX_DISTORTION_TYPE_INDEX = DISTORTION_TYPE_COUNT - 1;

    enum SidechainMode {
        SIDECHAIN_ENHANCEMENT = 0,
        SIDECHAIN_DUCKING = 1,
        SIDECHAIN_DIRECT = 2
    };

    struct OversamplerSet {
        rack::dsp::Upsampler<2, OVERSAMPLE_QUALITY> up2;
        rack::dsp::Decimator<2, OVERSAMPLE_QUALITY> down2;
        rack::dsp::Upsampler<4, OVERSAMPLE_QUALITY> up4;
        rack::dsp::Decimator<4, OVERSAMPLE_QUALITY> down4;
        rack::dsp::Upsampler<8, OVERSAMPLE_QUALITY> up8;
        rack::dsp::Decimator<8, OVERSAMPLE_QUALITY> down8;
        float buf[8] = {};

        OversamplerSet()
        : up2(0.9f), down2(0.9f), up4(0.9f), down4(0.9f), up8(0.9f), down8(0.9f) {
        }

        void reset() {
            up2.reset();
            down2.reset();
            up4.reset();
            down4.reset();
            up8.reset();
            down8.reset();
            for (float& sample : buf) {
                sample = 0.f;
            }
        }
    };

    // Per-channel (L/R) distortion and level state within one voice
    struct ChannelState {
        shapetaker::DistortionEngine engine;     // active distortion type
        shapetaker::DistortionEngine engineOld;  // outgoing type during a transition
        OversamplerSet oversampler;
        OversamplerSet oversamplerOld;
        float cleanPow = 0.f;
        float wetPow = 0.f;
        float makeupGain = 1.f;
    };

    // All per-voice DSP state
    struct VoiceState {
        ChannelState left;
        ChannelState right;
        float smoothedVcaGain = 0.f;
        float aggressionEnv = 0.f;
        bool wasDistortionBypassed = true;

        // Fresh start for both channels' engines and oversamplers (used when
        // distortion re-engages from bypass)
        void resetDistortion() {
            for (ChannelState* cs : {&left, &right}) {
                cs->engine.reset();
                cs->engineOld.reset();
                cs->oversampler.reset();
                cs->oversamplerOld.reset();
            }
        }
    };

    VoiceState voiceState[shapetaker::PolyphonicProcessor::MAX_VOICES];

    shapetaker::PolyphonicProcessor polyProcessor;
    shapetaker::SidechainDetector detector;
    dsp::ExponentialSlewLimiter distortion_slew;

    // Smoothed values for LED display (prevents audio-rate flickering)
    float smoothed_distortion_display = 0.0f;
    float smoothed_drive_display = 0.0f;
    float smoothed_mix_display = 0.0f;
    float smoothed_gain_led = 0.0f;
    float smoothed_gain_boost = 0.0f;   // Above-unity VCA gain, drives the jewel's white shift
    int currentDistortionType = 0;

    // Audio-rate smoothing for control CVs
    float smoothed_drive = 0.0f;
    float smoothed_mix = 0.0f;
    float smoothed_type = 0.0f;
    bool smoothersInitialized = false;
    bool typeTransitionInitialized = false;
    float typeTransition = 1.0f;
    int oldDistortionType = 0;

    // Rate-of-change tracking for adaptive smoothing
    float prev_distortion = 0.0f;
    float prev_drive = 0.0f;
    float prev_mix = 0.0f;

    // Cached smoothing coefficients (updated on sample-rate changes)
    float displaySmoothFast = 0.0f;
    float displaySmoothSlow = 0.0f;
    float controlSmoothCoeff = 0.0f;
    float vcaSmoothCoeff = 0.0f;
    float typeSmoothCoeff = 0.0f;
    float levelSmoothCoeff = 0.0f;
    float levelAttackCoeff = 0.0f;
    float makeupSmoothCoeff = 0.0f;
    float makeupAttackCoeff = 0.0f;
    float aggressionAttackCoeff = 0.0f;
    float aggressionReleaseCoeff = 0.0f;
    float typeTransitionStep = 0.0f;
    float gainLedSmoothCoeff = 0.0f;
    float controlMotionScale = 0.0f;
    float controlMotionDecay = 0.0f;

    // Control-motion detector state (see CONTROL_MOTION_TIME_S)
    float prevDistortionOnset = 0.0f;
    float controlMotion = 0.0f;

    // Sidechain mode (context menu): enhancement, ducking, direct
    int sidechainMode = SIDECHAIN_ENHANCEMENT;
    int oversampleFactor = DEFAULT_OVERSAMPLE_FACTOR;
    float currentSampleRate = DEFAULT_SAMPLE_RATE;
    int gainJewelTheme = shapetaker::ui::ThemeManager::DisplayTheme::PHOSPHOR;
    mutable bool useSharedGainJewelTheme = true;
    mutable int sharedGainJewelThemeRevisionSeen = 0;

    bool oversampleBypass = true;

    Chiaroscuro() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        shapetaker::ParameterHelper::configGain(this, VCA_PARAM, "vca gain");
        shapetaker::ParameterHelper::configSwitch(this, TYPE_PARAM, "dist type",
            {"hard clip", "tube sat", "wave fold", "bit crush", "ring mod"}, 0);
        shapetaker::ParameterHelper::configGain(this, DIST_PARAM, "dist %");
        shapetaker::ParameterHelper::configAttenuverter(this, DIST_ATT_PARAM, "dist cv");
        shapetaker::ParameterHelper::configDrive(this, DRIVE_PARAM);
        shapetaker::ParameterHelper::configAttenuverter(this, DRIVE_ATT_PARAM, "drive cv");
        shapetaker::ParameterHelper::configMix(this, MIX_PARAM);
        shapetaker::ParameterHelper::configAttenuverter(this, MIX_ATT_PARAM, "mix cv");
        shapetaker::ParameterHelper::configToggle(this, LINK_PARAM, "link L/R channels");
        shapetaker::ParameterHelper::configToggle(this, RESPONSE_PARAM, "vca resp: lin/exp", true);
        shapetaker::ParameterHelper::configAudioInput(this, AUDIO_L_INPUT, "L");
        shapetaker::ParameterHelper::configAudioInput(this, AUDIO_R_INPUT, "R");
        shapetaker::ParameterHelper::configCVInput(this, VCA_CV_INPUT, "vca cv");
        shapetaker::ParameterHelper::configAudioInput(this, SIDECHAIN_INPUT, "sidechain detect");
        shapetaker::ParameterHelper::configCVInput(this, TYPE_CV_INPUT, "dist type");
        shapetaker::ParameterHelper::configCVInput(this, DIST_CV_INPUT, "dist amt");
        shapetaker::ParameterHelper::configCVInput(this, DRIVE_CV_INPUT, "drive amt");
        shapetaker::ParameterHelper::configCVInput(this, MIX_CV_INPUT, "mix control");
        
        shapetaker::ParameterHelper::configAudioOutput(this, AUDIO_L_OUTPUT, "L");
        shapetaker::ParameterHelper::configAudioOutput(this, AUDIO_R_OUTPUT, "R");
        // Bypass passes dry audio through instead of going silent
        configBypass(AUDIO_L_INPUT, AUDIO_L_OUTPUT);
        configBypass(AUDIO_R_INPUT, AUDIO_R_OUTPUT);
        // Initialize distortion smoothing - fast enough to be responsive but slow enough to avoid clicks
        distortion_slew.setRiseFall(DIST_SLEW_HZ, DIST_SLEW_HZ);

        currentSampleRate = APP->engine->getSampleRate();
        detector.setTiming(SIDECHAIN_ATTACK_MS, SIDECHAIN_RELEASE_MS, currentSampleRate);
        updateSmoothingCoeffs();
        seedDistortionEngines();
        configureOversampling();
        resetLevelTracking();
        resetSmoothers();

        shapetaker::ui::LabelFormatter::normalizeModuleControls(this);
    }
    
    // Module::onReset() only restores parameters. Without this the engines,
    // level trackers and smoothers survive an Initialize, so the module keeps
    // the previous patch's makeup gain and distortion state under freshly
    // defaulted knobs. Oversampling and the jewel theme are deliberately left
    // alone: they are context-menu preferences, not patch state.
    void onReset() override {
        Module::onReset();
        seedDistortionEngines();
        resetLevelTracking();
        resetSmoothers();
    }

    void onSampleRateChange() override {
        currentSampleRate = APP->engine->getSampleRate();
        detector.setTiming(SIDECHAIN_ATTACK_MS, SIDECHAIN_RELEASE_MS, currentSampleRate);
        updateSmoothingCoeffs();
        configureOversampling();
        resetLevelTracking();
        resetSmoothers();
    }

    void resetLevelTracking() {
        for (VoiceState& vs : voiceState) {
            for (ChannelState* cs : {&vs.left, &vs.right}) {
                cs->cleanPow = 0.0f;
                cs->wetPow = 0.0f;
                cs->makeupGain = 1.0f;
                cs->oversampler.reset();
                cs->oversamplerOld.reset();
            }
            vs.aggressionEnv = 0.0f;
            vs.wasDistortionBypassed = true;
        }
    }

    void resetSmoothers() {
        smoothersInitialized = false;
        smoothed_drive = 0.0f;
        smoothed_mix = 0.0f;
        smoothed_type = 0.0f;
        smoothed_gain_led = 0.0f;
        smoothed_gain_boost = 0.0f;
        for (VoiceState& vs : voiceState) {
            vs.smoothedVcaGain = 0.0f;
        }
        typeTransitionInitialized = false;
        typeTransition = 1.0f;
        distortion_slew.reset();
        smoothed_distortion_display = 0.0f;
        smoothed_drive_display = 0.0f;
        smoothed_mix_display = 0.0f;
        prev_distortion = 0.0f;
        prev_drive = 0.0f;
        prev_mix = 0.0f;
        prevDistortionOnset = 0.0f;
        controlMotion = 0.0f;
    }

    void updateSmoothingCoeffs() {
        float sampleTime = 1.0f / std::max(currentSampleRate, 1.0f);
        auto coeffForHz = [sampleTime](float hz) {
            return 1.0f - expf(-2.0f * M_PI * hz * sampleTime);
        };
        displaySmoothFast = coeffForHz(DISPLAY_SMOOTH_FAST_HZ);
        displaySmoothSlow = coeffForHz(DISPLAY_SMOOTH_SLOW_HZ);
        controlSmoothCoeff = coeffForHz(CONTROL_SMOOTH_HZ);
        vcaSmoothCoeff = coeffForHz(VCA_SMOOTH_HZ);
        typeSmoothCoeff = coeffForHz(TYPE_SMOOTH_HZ);
        levelSmoothCoeff = coeffForHz(LEVEL_TRACK_HZ);
        levelAttackCoeff = coeffForHz(LEVEL_ATTACK_HZ);
        makeupSmoothCoeff = coeffForHz(MAKEUP_SMOOTH_HZ);
        makeupAttackCoeff = coeffForHz(MAKEUP_ATTACK_HZ);
        aggressionAttackCoeff = 1.0f - expf(-1.0f / (AGGRESSION_ATTACK_MS * 0.001f * currentSampleRate));
        aggressionReleaseCoeff = 1.0f - expf(-1.0f / (AGGRESSION_RELEASE_MS * 0.001f * currentSampleRate));
        typeTransitionStep = sampleTime / (TYPE_TRANSITION_MS * 0.001f);
        gainLedSmoothCoeff = coeffForHz(GAIN_LED_RESPONSE_HZ);
        controlMotionScale = CONTROL_MOTION_TIME_S * currentSampleRate;
        controlMotionDecay = 1.0f / (CONTROL_MOTION_HOLD_S * currentSampleRate);
    }

    void configureOversampling() {
        float oversampleRate = currentSampleRate * oversampleFactor;
        for (VoiceState& vs : voiceState) {
            for (ChannelState* cs : {&vs.left, &vs.right}) {
                for (auto* engine : {&cs->engine, &cs->engineOld}) {
                    engine->setSampleRate(oversampleRate);
                    engine->setRateScale(oversampleFactor);
                }
            }
        }

        oversampleBypass = (oversampleFactor <= MIN_OVERSAMPLE_FACTOR);
    }

    void setOversampleFactor(int factor) {
        factor = rack::math::clamp(factor, MIN_OVERSAMPLE_FACTOR, MAX_OVERSAMPLE_FACTOR);
        // processDistortionPath() only implements 1x/2x/4x/8x; any other value
        // falls through to its default branch and runs the engines once at the
        // host rate while configureOversampling() has told them they are
        // running at factor x that rate — a silently detuned ring-mod carrier
        // and a stretched bit-crush hold. The menu only ever offers powers of
        // two, but dataFromJson() takes whatever the patch contains, so snap
        // down to the nearest supported ratio here.
        int snapped = MIN_OVERSAMPLE_FACTOR;
        while (snapped * 2 <= factor)
            snapped *= 2;
        factor = snapped;
        if (factor == oversampleFactor)
            return;
        oversampleFactor = factor;
        configureOversampling();
        resetLevelTracking();
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "sidechainMode", json_integer(sidechainMode));
        json_object_set_new(rootJ, "oversampleFactor", json_integer(oversampleFactor));
        json_object_set_new(rootJ, "gainJewelTheme", json_integer(getEffectiveGainJewelTheme()));
        json_object_set_new(rootJ, "localGainJewelTheme", json_integer(gainJewelTheme));
        json_object_set_new(rootJ, "useSharedGainJewelTheme", json_boolean(useSharedGainJewelTheme));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        if (json_t* j = json_object_get(rootJ, "sidechainMode"))
            sidechainMode = rack::math::clamp((int)json_integer_value(j), SIDECHAIN_ENHANCEMENT, SIDECHAIN_DIRECT);
        if (json_t* j = json_object_get(rootJ, "oversampleFactor"))
            setOversampleFactor((int)json_integer_value(j));
        if (json_t* j = json_object_get(rootJ, "gainJewelTheme"))
            gainJewelTheme = rack::math::clamp(
                (int)json_integer_value(j), 0,
                shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1);
        if (json_t* j = json_object_get(rootJ, "localGainJewelTheme"))
            gainJewelTheme = rack::math::clamp(
                (int)json_integer_value(j), 0,
                shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1);
        if (json_t* j = json_object_get(rootJ, "useSharedGainJewelTheme"))
            useSharedGainJewelTheme = json_is_true(j);
        if (useSharedGainJewelTheme) {
            if (json_t* j = json_object_get(rootJ, "gainJewelTheme"))
                shapetaker::ui::ThemeManager::DisplayTheme::restoreSharedTheme(
                    rack::math::clamp((int)json_integer_value(j), 0,
                        shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT - 1));
        }
        sharedGainJewelThemeRevisionSeen =
            shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeRevision();
    }

    int getEffectiveGainJewelTheme() const {
        using DisplayTheme = shapetaker::ui::ThemeManager::DisplayTheme;
        int revision = DisplayTheme::getSharedThemeRevision();
        if (sharedGainJewelThemeRevisionSeen != revision) {
            useSharedGainJewelTheme = true;
            sharedGainJewelThemeRevisionSeen = revision;
        }
        return useSharedGainJewelTheme
            ? DisplayTheme::getSharedThemeIndex()
            : rack::math::clamp(gainJewelTheme, 0, DisplayTheme::THEME_COUNT - 1);
    }

    static inline float clampUnit(float v) {
        return clamp(v, UNIT_MIN, UNIT_MAX);
    }

    static inline float normalizeCV10V(float cv) {
        return cv * CV_TO_UNIT_SCALE;
    }

    static inline float smoothstepUnit(float v) {
        v = clampUnit(v);
        return v * v * (3.0f - 2.0f * v);
    }

    static inline float softLimit(float x) {
        float ax = fabsf(x);
        if (ax <= SOFTCLIP_KNEE_V)
            return x;
        float over = ax - SOFTCLIP_KNEE_V;
        float range = SOFTCLIP_CEIL_V - SOFTCLIP_KNEE_V;
        float y = SOFTCLIP_KNEE_V + range * shapetaker::dsp::OscillatorHelper::fastTanh(over / range);
        return std::copysign(y, x);
    }

    void seedDistortionEngines() {
        for (int ch = 0; ch < shapetaker::PolyphonicProcessor::MAX_VOICES; ++ch) {
            const unsigned int base = 0x9E3779B9u;
            VoiceState& vs = voiceState[ch];
            vs.left.engine.seedDither(base * static_cast<unsigned int>(ch * 4 + 1));
            vs.right.engine.seedDither(base * static_cast<unsigned int>(ch * 4 + 2));
            vs.left.engineOld.seedDither(base * static_cast<unsigned int>(ch * 4 + 3));
            vs.right.engineOld.seedDither(base * static_cast<unsigned int>(ch * 4 + 4));
        }
    }

    // Fresh start for the incoming type's engines after a type switch (the
    // old engines keep running the outgoing type during the crossfade).
    void resetActiveDistortionEngines() {
        for (VoiceState& vs : voiceState) {
            for (ChannelState* cs : {&vs.left, &vs.right}) {
                cs->engine.reset();
                cs->oversampler.reset();
            }
        }
    }

    void copyActiveEnginesToOld() {
        for (VoiceState& vs : voiceState) {
            for (ChannelState* cs : {&vs.left, &vs.right}) {
                cs->engineOld = cs->engine;
                cs->oversamplerOld.reset();
            }
        }
    }

    float processDistortionPath(shapetaker::DistortionEngine& engine,
                                OversamplerSet& oversampler,
                                float normalized,
                                float amount,
                                shapetaker::DistortionEngine::Type type) {
        if (oversampleBypass) {
            return engine.process(normalized, amount, type);
        }

        switch (oversampleFactor) {
            case 2:
                oversampler.up2.process(normalized, oversampler.buf);
                for (int i = 0; i < 2; ++i) {
                    oversampler.buf[i] = engine.process(oversampler.buf[i], amount, type);
                }
                return oversampler.down2.process(oversampler.buf);
            case 4:
                oversampler.up4.process(normalized, oversampler.buf);
                for (int i = 0; i < 4; ++i) {
                    oversampler.buf[i] = engine.process(oversampler.buf[i], amount, type);
                }
                return oversampler.down4.process(oversampler.buf);
            case 8:
                oversampler.up8.process(normalized, oversampler.buf);
                for (int i = 0; i < 8; ++i) {
                    oversampler.buf[i] = engine.process(oversampler.buf[i], amount, type);
                }
                return oversampler.down8.process(oversampler.buf);
            default:
                return engine.process(normalized, amount, type);
        }
    }

    float getModulatedUnitParam(int paramId, int cvInputId, int attenuverterParamId) {
        float value = params[paramId].getValue();
        if (inputs[cvInputId].isConnected()) {
            value += normalizeCV10V(inputs[cvInputId].getVoltage()) * params[attenuverterParamId].getValue();
        }
        return clampUnit(value);
    }

    void process(const ProcessArgs& args) override {
        // Update polyphonic channel count and set outputs (use max of L/R for full stereo poly)
        int channels = polyProcessor.updateChannels(
            {inputs[AUDIO_L_INPUT], inputs[AUDIO_R_INPUT]},
            {outputs[AUDIO_L_OUTPUT], outputs[AUDIO_R_OUTPUT]});
        
        // Link switch state
        bool linked = params[LINK_PARAM].getValue() > SWITCH_ON_THRESHOLD;
        
        // Sidechain processing (shared across all voices)
        float sidechain = inputs[SIDECHAIN_INPUT].isConnected() ?
            inputs[SIDECHAIN_INPUT].getVoltage() : 0.0f;
        sidechain = clampUnit(fabsf(sidechain) * CV_TO_UNIT_SCALE);
        
        float sc_env = detector.process(sidechain);

        // Global parameters (shared across all voices)
        // Drive parameter with CV (smoothed to avoid zippering)
        float driveTarget = getModulatedUnitParam(DRIVE_PARAM, DRIVE_CV_INPUT, DRIVE_ATT_PARAM);

        // Mix parameter with CV (smoothed to avoid zippering)
        float mixTarget = getModulatedUnitParam(MIX_PARAM, MIX_CV_INPUT, MIX_ATT_PARAM);
        
        // Distortion type with CV (light smoothing to prevent chatter)
        float typeTarget = params[TYPE_PARAM].getValue();
        if (inputs[TYPE_CV_INPUT].isConnected()) {
            float cv = normalizeCV10V(inputs[TYPE_CV_INPUT].getVoltage());
            typeTarget = params[TYPE_PARAM].getValue() + cv * TYPE_CV_SPAN;
        }
        typeTarget = clamp(typeTarget, UNIT_MIN, static_cast<float>(MAX_DISTORTION_TYPE_INDEX));

        bool initializeVoiceSmoothers = !smoothersInitialized;
        if (!smoothersInitialized) {
            smoothed_drive = driveTarget;
            smoothed_mix = mixTarget;
            smoothed_type = typeTarget;
            smoothersInitialized = true;
        } else {
            smoothed_drive += controlSmoothCoeff * (driveTarget - smoothed_drive);
            smoothed_mix += controlSmoothCoeff * (mixTarget - smoothed_mix);
            smoothed_type += typeSmoothCoeff * (typeTarget - smoothed_type);
        }

        float drive = clampUnit(smoothed_drive);
        float mix = clampUnit(smoothed_mix);
        int distortion_type = clamp((int)std::round(smoothed_type), 0, MAX_DISTORTION_TYPE_INDEX);
        if (!typeTransitionInitialized) {
            currentDistortionType = distortion_type;
            oldDistortionType = distortion_type;
            typeTransition = 1.0f;
            typeTransitionInitialized = true;
        } else if (distortion_type != currentDistortionType) {
            oldDistortionType = currentDistortionType;
            copyActiveEnginesToOld();
            currentDistortionType = distortion_type;
            resetActiveDistortionEngines();
            typeTransition = 0.0f;
        }

        float typeTransitionShape = smoothstepUnit(typeTransition);
        
        // Distortion amount parameter with CV
        float dist_amount = getModulatedUnitParam(DIST_PARAM, DIST_CV_INPUT, DIST_ATT_PARAM);

        // Enhanced sidechain behavior with three modes
        float combined_distortion, effective_drive, effective_mix;

        if (inputs[SIDECHAIN_INPUT].isConnected()) {
            float sidechain_intensity = sc_env; // 0.0 to 1.0 based on sidechain input

            switch (sidechainMode) {
                case SIDECHAIN_ENHANCEMENT:
                    // Both distortion and drive scale together with sidechain
                    // Knob positions set the maximum values that sidechain can reach
                    combined_distortion = dist_amount * sidechain_intensity;
                    effective_drive = drive * sidechain_intensity;
                    effective_mix = fmaxf(mix, SIDECHAIN_MIN_WET_MIX);
                    break;

                case SIDECHAIN_DUCKING:
                    // Reduce distortion when sidechain is hot
                    combined_distortion = dist_amount * (1.0f - sidechain_intensity);
                    effective_drive = drive * (1.0f - sidechain_intensity * SIDECHAIN_DUCK_DRIVE);
                    effective_mix = mix;
                    break;

                case SIDECHAIN_DIRECT:
                    // Sidechain directly controls distortion amount
                    combined_distortion = sidechain_intensity;
                    effective_drive = drive;
                    effective_mix = mix;
                    break;

                default:
                    combined_distortion = dist_amount;
                    effective_drive = drive;
                    effective_mix = mix;
                    break;
            }
        } else {
            // Normal mode: knobs control directly
            combined_distortion = dist_amount;
            effective_drive = drive;
            effective_mix = mix;
        }

        combined_distortion = clampUnit(combined_distortion);

        // Apply smoothing to the combined distortion to prevent clicks
        float smoothed_distortion = distortion_slew.process(args.sampleTime, combined_distortion);
        float processed_distortion = smoothed_distortion;
        float processed_drive = drive;
        float processed_mix = mix;

        // Apply adaptive smoothing to display values to prevent audio-rate flickering
        // Calculate rate of change for each parameter
        float dist_rate = fabsf(processed_distortion - prev_distortion) / args.sampleTime;
        float drive_rate = fabsf(processed_drive - prev_drive) / args.sampleTime;
        float mix_rate = fabsf(processed_mix - prev_mix) / args.sampleTime;

        // Update previous values
        prev_distortion = processed_distortion;
        prev_drive = processed_drive;
        prev_mix = processed_mix;

        // Adaptive cutoff frequency: good response up to 10Hz, then averaging above
        // Rate threshold of ~6.28 corresponds to 10Hz sine wave at full amplitude
        float dist_smooth_factor = (dist_rate > RATE_THRESHOLD_HZ10) ? displaySmoothSlow : displaySmoothFast;
        float drive_smooth_factor = (drive_rate > RATE_THRESHOLD_HZ10) ? displaySmoothSlow : displaySmoothFast;
        float mix_smooth_factor = (mix_rate > RATE_THRESHOLD_HZ10) ? displaySmoothSlow : displaySmoothFast;

        smoothed_distortion_display += (processed_distortion - smoothed_distortion_display) * dist_smooth_factor;
        smoothed_drive_display += (processed_drive - smoothed_drive_display) * drive_smooth_factor;
        smoothed_mix_display += (processed_mix - smoothed_mix_display) * mix_smooth_factor;

        // Distortion controls algorithm depth; drive controls how hard the wet path hits it.
        float distortion_amount = smoothed_distortion;
        float drive_onset = smoothstepUnit(effective_drive);
        float distortion_onset = smoothstepUnit(distortion_amount / DISTORTION_ONSET_RANGE);
        // Makeup engages with distortion presence, NOT the mix knob, so the wet
        // path is normalized to the clean loudness *before* the wet/dry blend and
        // sweeping mix becomes an equal-loudness crossfade. When this scaled with
        // mix, the un-normalized wet - which carries the distortion's large
        // small-signal gain at low VCA settings - dominated intermediate mix
        // positions and the level jumped up dramatically as mix was raised.
        float makeup_amount = distortion_onset;
        float drive_pre_gain = 1.0f + drive_onset * distortion_amount * DRIVE_PRE_GAIN_SCALE;

        // Control-motion detector. distortion_onset is the dry/wet crossfade
        // coefficient, and it is what slams the wet level; tracking its rate of
        // change tells the makeup loop below that the jump it is about to see is
        // control-driven rather than program material, so it can correct fast
        // without becoming level-sensitive. Held briefly after the move so the
        // envelopes have time to converge.
        float onsetDelta = fabsf(distortion_onset - prevDistortionOnset);
        prevDistortionOnset = distortion_onset;
        controlMotion = fmaxf(clampUnit(onsetDelta * controlMotionScale),
                              controlMotion - controlMotionDecay);

        // Distortion LED: product-based (reflects actual distortion heard) plus
        // a small peak hint so any single knob at ~half shows a subtle glow
        float product = smoothed_distortion_display * smoothed_drive_display * smoothed_mix_display;
        float peak = std::max(smoothed_distortion_display, std::max(smoothed_drive_display, smoothed_mix_display));
        float dist_intensity = clampUnit(product + peak * peak * LED_INTENSITY_PEAK_HINT);
        // Boost brightness while keeping darker base hues; allow >1.0 pre-clamp for stronger glow.
        float dist_led_brightness = std::pow(dist_intensity, LED_BRIGHTNESS_GAMMA) * LED_BRIGHTNESS_BOOST;

        // Color palette for each distortion type (normalized RGB)
        float dist_r = 0.26f;
        float dist_g = 0.34f;
        float dist_b = 0.46f;
        getDistortionTypeColor(distortion_type, dist_r, dist_g, dist_b);

        // Apply brightness to the type color, clamping per channel for LED intensity
        shapetaker::RGBColor currentLEDColor(
            clampUnit(dist_r * dist_led_brightness),
            clampUnit(dist_g * dist_led_brightness),
            clampUnit(dist_b * dist_led_brightness));

        lights[DIST_LED_R].setBrightness(currentLEDColor.r);
        lights[DIST_LED_G].setBrightness(currentLEDColor.g);
        lights[DIST_LED_B].setBrightness(currentLEDColor.b);
        
        // VCA gain calculation (polyphonic CV support)
        float base_vca_gain = params[VCA_PARAM].getValue();
        bool exponential_response = params[RESPONSE_PARAM].getValue() > SWITCH_ON_THRESHOLD;
        
        const float normalizationVoltage = NOMINAL_LEVEL;
        const float invNormalization = 1.0f / normalizationVoltage;
        const float makeupMinPower = MAKEUP_MIN_LEVEL * MAKEUP_MIN_LEVEL;
        const float mix_makeup = 1.0f + effective_mix * MIX_MAKEUP_SCALE * distortion_onset;
        const bool distortionActive = distortion_amount > DISTORTION_BYPASS_THRESHOLD;
        const auto currentType = static_cast<shapetaker::DistortionEngine::Type>(currentDistortionType);
        const auto oldType = static_cast<shapetaker::DistortionEngine::Type>(oldDistortionType);

        // Process one channel (L or R) of one voice: distortion, power
        // tracking, adaptive makeup, wet/dry mix, and output conditioning.
        auto processChannel = [&](ChannelState& cs, float vca, float preDriveBoost) -> float {
            float wet = vca;
            if (distortionActive) {
                float normalized = (vca * preDriveBoost * drive_pre_gain) * invNormalization;
                float wetNorm = processDistortionPath(cs.engine, cs.oversampler,
                                                      normalized, distortion_amount, currentType);
                if (typeTransitionShape < 1.0f) {
                    float oldWetNorm = processDistortionPath(cs.engineOld, cs.oversamplerOld,
                                                             normalized, distortion_amount, oldType);
                    wetNorm = rack::math::crossfade(oldWetNorm, wetNorm, typeTransitionShape);
                }
                wet = rack::math::crossfade(vca, wetNorm * normalizationVoltage, distortion_onset);
            }

            // Track power envelopes for wet/dry auto-compensation. Both
            // envelopes take the fast-attack path only on large power jumps
            // (see LEVEL_JUMP_RATIO): steady program tracks exactly like the
            // plain smoothing, note onsets move clean and wet together (ratio
            // stays put), and a distortion-knob loudness jump — which moves
            // only the wet side — is caught in milliseconds.
            float cleanPower = vca * vca;
            float wetPower = wet * wet;
            // controlMotion blends both envelopes toward the fast coefficient
            // while the distortion control is moving, in either direction — the
            // signal tests below are one-sided and only catch rising level.
            float cleanCoeff = (cleanPower > cs.cleanPow * LEVEL_JUMP_RATIO) ? levelAttackCoeff : levelSmoothCoeff;
            cleanCoeff += (levelAttackCoeff - cleanCoeff) * controlMotion;
            cs.cleanPow += (cleanPower - cs.cleanPow) * cleanCoeff + 1e-18f;
            float wetCoeff = (wetPower > cs.wetPow * LEVEL_JUMP_RATIO) ? levelAttackCoeff : levelSmoothCoeff;
            wetCoeff += (levelAttackCoeff - wetCoeff) * controlMotion;
            cs.wetPow += (wetPower - cs.wetPow) * wetCoeff + 1e-18f;

            float desiredGain = 1.0f;
            if (makeup_amount > MAKEUP_ACTIVE_THRESHOLD && cs.wetPow > makeupMinPower) {
                desiredGain = (cs.cleanPow > makeupMinPower) ? std::sqrt(cs.cleanPow / cs.wetPow) : 1.0f;
            }
            desiredGain = clamp(desiredGain, MAKEUP_GAIN_MIN, MAKEUP_GAIN_MAX);
            // Limiter-style: large reductions apply fast; ordinary ripple and
            // recovery keep the original lazy smoothing.
            float gainCoeff = (desiredGain < cs.makeupGain * MAKEUP_DROP_RATIO) ? makeupAttackCoeff : makeupSmoothCoeff;
            gainCoeff += (makeupAttackCoeff - gainCoeff) * controlMotion;
            cs.makeupGain += (desiredGain - cs.makeupGain) * gainCoeff;

            // The adaptive makeup gain level-matches all distortion types to
            // the clean signal; no per-type boost is needed.
            float activeMakeup = rack::math::crossfade(1.0f, cs.makeupGain, makeup_amount);
            float compensated = wet * activeMakeup;

            // Wet/dry crossfade (effective_mix covers sidechain modes), with
            // a slight boost to compensate the wet/dry balance.
            float output = vca * (1.0f - effective_mix) + compensated * effective_mix;
            output *= mix_makeup;

            // Safety check for NaN/infinity
            if (!std::isfinite(output)) output = 0.0f;
            return softLimit(output);
        };

        // Process each voice
        for (int ch = 0; ch < channels; ch++) {
            VoiceState& vs = voiceState[ch];

            // Per-voice VCA gain calculation
            float vca_gain = base_vca_gain;

            if (inputs[VCA_CV_INPUT].isConnected()) {
                float cv = normalizeCV10V(inputs[VCA_CV_INPUT].getPolyVoltage(ch));
                cv = clamp(cv, BIPOLAR_MIN, BIPOLAR_MAX);
                vca_gain += cv; // Direct CV control without attenuverter
            }

            vca_gain = clamp(vca_gain, UNIT_MIN, VCA_GAIN_MAX);

            // Apply response curve
            if (exponential_response) {
                vca_gain = vca_gain * vca_gain; // Square for exponential
            }

            if (initializeVoiceSmoothers) {
                vs.smoothedVcaGain = vca_gain;
            } else {
                vs.smoothedVcaGain += (vca_gain - vs.smoothedVcaGain) * vcaSmoothCoeff;
            }
            vca_gain = vs.smoothedVcaGain;

            // Get audio inputs for this voice
            float input_l = inputs[AUDIO_L_INPUT].getPolyVoltage(ch);
            float input_r = linked ? input_l :
                           (inputs[AUDIO_R_INPUT].isConnected() ? inputs[AUDIO_R_INPUT].getPolyVoltage(ch) : input_l);

            float base_vca_l = input_l * vca_gain;
            float base_vca_r = input_r * vca_gain;

            // Aggression: a hot signal through a wide-open VCA pushes a
            // little extra gain and drive into the distortion.
            float openFactor = clamp((vca_gain - VCA_OPEN_START) / VCA_OPEN_RANGE, UNIT_MIN, UNIT_MAX);
            float signalPeak = fmaxf(fabsf(base_vca_l), fabsf(base_vca_r));
            float aggressionCoeff = (signalPeak > vs.aggressionEnv) ? aggressionAttackCoeff : aggressionReleaseCoeff;
            vs.aggressionEnv += (signalPeak - vs.aggressionEnv) * aggressionCoeff;
            float hotSignal = clamp((vs.aggressionEnv - HOT_SIGNAL_START_V) / HOT_SIGNAL_RANGE_V, UNIT_MIN, UNIT_MAX);
            float aggression = openFactor * hotSignal;
            float aggression_gain = 1.0f + aggression * AGGRESSION_GAIN_SCALE;
            float pre_drive_boost = 1.0f + aggression * PRE_DRIVE_BOOST_SCALE;

            float vca_l = base_vca_l * aggression_gain;
            float vca_r = base_vca_r * aggression_gain;

            // Give the engines a fresh start when distortion re-engages.
            if (!distortionActive) {
                vs.wasDistortionBypassed = true;
            } else if (vs.wasDistortionBypassed) {
                vs.resetDistortion();
                vs.wasDistortionBypassed = false;
            }

            outputs[AUDIO_L_OUTPUT].setVoltage(processChannel(vs.left, vca_l, pre_drive_boost), ch);
            outputs[AUDIO_R_OUTPUT].setVoltage(processChannel(vs.right, vca_r, pre_drive_boost), ch);
        }

        typeTransition = clampUnit(typeTransition + typeTransitionStep);

        // Gain jewel: show effective VCA gain (knob + CV). Mirrors the audio
        // path exactly, including its VCA_GAIN_MAX clamp and the square that
        // the exponential curve applies afterwards, so CV above unity is
        // reported rather than clipped away before it is ever displayed.
        const float maxDisplayGain = exponential_response
            ? VCA_GAIN_MAX * VCA_GAIN_MAX
            : VCA_GAIN_MAX;
        float gain_cv_level = 0.0f;
        if (inputs[VCA_CV_INPUT].isConnected()) {
            // Use maximum effective gain across all channels
            for (int ch = 0; ch < channels; ch++) {
                float cv = normalizeCV10V(inputs[VCA_CV_INPUT].getPolyVoltage(ch));
                cv = clamp(cv, BIPOLAR_MIN, BIPOLAR_MAX);
                float effective_gain = clamp(base_vca_gain + cv, UNIT_MIN, VCA_GAIN_MAX);
                if (exponential_response)
                    effective_gain *= effective_gain;
                gain_cv_level = fmaxf(gain_cv_level, effective_gain);
            }
        } else {
            // No CV connected - show the effective knob gain.
            gain_cv_level = clamp(base_vca_gain, UNIT_MIN, VCA_GAIN_MAX);
            if (exponential_response)
                gain_cv_level *= gain_cv_level;
        }

        // Brightness covers 0..unity only, on the same sqrt curve as before —
        // low-end visibility, with a fast symmetric slew so decreases follow
        // knob/CV motion as readily as increases do. Knob-only patches never
        // exceed unity, so they look exactly as they always have.
        float gainTarget = clamp(std::sqrt(fminf(gain_cv_level, UNIT_MAX)), UNIT_MIN, UNIT_MAX);
        // Past unity the jewel cannot get any brighter, so the CV-driven boost
        // region (up to 2x, or 4x through the exponential curve) reads as the
        // lens running hot toward white instead of simply pinning.
        float boostTarget = (maxDisplayGain > UNIT_MAX)
            ? clampUnit((gain_cv_level - UNIT_MAX) / (maxDisplayGain - UNIT_MAX))
            : UNIT_MIN;
        smoothed_gain_led += (gainTarget - smoothed_gain_led) * gainLedSmoothCoeff;
        smoothed_gain_boost += (boostTarget - smoothed_gain_boost) * gainLedSmoothCoeff;

        using DisplayTheme = shapetaker::ui::ThemeManager::DisplayTheme;
        auto theme = static_cast<DisplayTheme::Theme>(getEffectiveGainJewelTheme());
        NVGcolor jewelColor = DisplayTheme::getLEDColor(theme);
        // Give Chiaroscuro's amber jewel a hotter, red-orange character while
        // leaving the shared amber palette unchanged for other modules.
        if (theme == DisplayTheme::AMBER)
            jewelColor = nvgRGB(255, 72, 16);
        // Run the lens toward white in the above-unity boost region. Stops
        // short of pure white so the theme colour stays recognisable even with
        // the VCA CV driven fully hot.
        float whiteMix = smoothed_gain_boost * GAIN_BOOST_WHITE_MIX;
        float jewelR = rack::math::crossfade(jewelColor.r, UNIT_MAX, whiteMix);
        float jewelG = rack::math::crossfade(jewelColor.g, UNIT_MAX, whiteMix);
        float jewelB = rack::math::crossfade(jewelColor.b, UNIT_MAX, whiteMix);
        lights[GAIN_LED_R].setBrightness(jewelR * smoothed_gain_led);
        lights[GAIN_LED_G].setBrightness(jewelG * smoothed_gain_led);
        lights[GAIN_LED_B].setBrightness(jewelB * smoothed_gain_led);
    }

private:
    static void getDistortionTypeColor(int type, float& r, float& g, float& b) {
        switch (type) {
            case 0: r = 0.09f; g = 0.45f; b = 0.38f; break; // Hard Clip  — teal
            case 1: r = 0.20f; g = 0.50f; b = 0.50f; break; // Tube Sat   — aqua
            case 2: r = 0.18f; g = 0.38f; b = 0.58f; break; // Wave Fold  — cyan blue
            case 3: r = 0.13f; g = 0.25f; b = 0.58f; break; // Bit Crush  — deep blue
            case 4: r = 0.38f; g = 0.17f; b = 0.58f; break; // Ring Mod   — magenta purple
            default: r = 0.26f; g = 0.34f; b = 0.46f; break;
        }
    }
};

struct ChiaroscuroJewelLED : JewelLEDLarge {
    ChiaroscuroJewelLED() {
        box.size = mm2px(Vec(10.f, 10.f));
    }
};

// Module widget
struct ChiaroscuroWidget : ShapetakerModuleWidget {

    void appendContextMenu(Menu* menu) override {
        Chiaroscuro* module = dynamic_cast<Chiaroscuro*>(this->module);
        if (!module)
            return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Oversampling"));
        menu->addChild(createCheckMenuItem("1x", "", [=]{ return module->oversampleFactor == 1; }, [=]{ module->setOversampleFactor(1); }));
        menu->addChild(createCheckMenuItem("2x", "", [=]{ return module->oversampleFactor == 2; }, [=]{ module->setOversampleFactor(2); }));
        menu->addChild(createCheckMenuItem("4x", "", [=]{ return module->oversampleFactor == 4; }, [=]{ module->setOversampleFactor(4); }));
        menu->addChild(createCheckMenuItem("8x (High Quality)", "", [=]{ return module->oversampleFactor == 8; }, [=]{ module->setOversampleFactor(8); }));

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Sidechain Mode"));
        menu->addChild(createCheckMenuItem("Enhancement (Trigger)", "", [=]{ return module->sidechainMode == Chiaroscuro::SIDECHAIN_ENHANCEMENT; }, [=]{ module->sidechainMode = Chiaroscuro::SIDECHAIN_ENHANCEMENT; }));
        menu->addChild(createCheckMenuItem("Ducking (Inverse)",     "", [=]{ return module->sidechainMode == Chiaroscuro::SIDECHAIN_DUCKING;     }, [=]{ module->sidechainMode = Chiaroscuro::SIDECHAIN_DUCKING;     }));
        menu->addChild(createCheckMenuItem("Direct Control",        "", [=]{ return module->sidechainMode == Chiaroscuro::SIDECHAIN_DIRECT;      }, [=]{ module->sidechainMode = Chiaroscuro::SIDECHAIN_DIRECT;      }));

        using DisplayTheme = shapetaker::ui::ThemeManager::DisplayTheme;

        menu->addChild(new MenuSeparator);
        DisplayTheme::addThemeMenu(
            menu,
            [=] { return module->useSharedGainJewelTheme; },
            [=](bool follow) {
                module->useSharedGainJewelTheme = follow;
                module->sharedGainJewelThemeRevisionSeen = DisplayTheme::getSharedThemeRevision();
            },
            [=] { return module->gainJewelTheme; },
            [=](int theme) { module->gainJewelTheme = theme; });
    }

    ChiaroscuroWidget(Chiaroscuro* module) {
        setModule(module);
        auto svgPath = asset::plugin(pluginInstance, "res/panels/Chiaroscuro.svg");
        setPanel(APP->window->loadSvg(svgPath));
        auto wearOverlay = new shapetaker::ui::PanelWearOverlay();
        wearOverlay->box.size = box.size;
        addChild(wearOverlay);

        using LayoutHelper = shapetaker::ui::LayoutHelper;

        LayoutHelper::ScrewPositions::addStandardScrews<ScrewJetBlack>(this, box.size.x);
        addAlchemicalBadge(this, 39); // Leviathan Cross

        LayoutHelper::PanelSVGParser parser(svgPath);
        auto centerPx = LayoutHelper::createCenterPxHelper(parser);

        auto addKnobWithShadow = [this](app::ParamWidget* knob) {
            ::addKnobWithShadow(this, knob);
        };

        // Jewel LEDs
        addChild(createLightCentered<ChiaroscuroJewelLED>(
            centerPx("gain_led", 9.480f, 19.000f), module, Chiaroscuro::GAIN_LED_R));
        addChild(createLightCentered<ChiaroscuroJewelLED>(
            centerPx("dist_amt_led", 51.480f, 19.000f), module, Chiaroscuro::DIST_LED_R));

        // Main VCA knob — Vintage XLarge (27mm)
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageXLarge>(centerPx("vca-knob", 30.480f, 20.500f), module, Chiaroscuro::VCA_PARAM));

        // Distortion type selector: analog blade switch with 6 positions
        auto* selector = createParamCentered<ShapetakerBladeDistortionSelector>(centerPx("dist-type-select", 30.480f, 40.000f), module, Chiaroscuro::TYPE_PARAM);
        selector->drawDetents = true;
        addParam(selector);

        // Toggle switches
        addParam(createParamCentered<ShapetakerDarkToggleOffPos4>(centerPx("lin-lr-switch",  9.480f, 36.000f), module, Chiaroscuro::LINK_PARAM));
        addParam(createParamCentered<ShapetakerDarkToggleOffPos4>(centerPx("lin-exp-switch", 51.480f, 36.000f), module, Chiaroscuro::RESPONSE_PARAM));

        // Section knobs — Vintage SmallMedium (15mm)
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("dist-knob",  10.480f, 53.500f), module, Chiaroscuro::DIST_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("drive-knob", 30.480f, 53.500f), module, Chiaroscuro::DRIVE_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(centerPx("mix-knob",   50.480f, 53.500f), module, Chiaroscuro::MIX_PARAM));

        // Attenuverters
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("dist-atten",  10.480f, 70.500f), module, Chiaroscuro::DIST_ATT_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("drive-atten", 30.480f, 70.500f), module, Chiaroscuro::DRIVE_ATT_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(centerPx("mix-atten",   50.480f, 70.500f), module, Chiaroscuro::MIX_ATT_PARAM));

        // CV inputs
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("vca-cv",             10.480f, 101.500f), module, Chiaroscuro::VCA_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("dist-cv",            10.480f, 88.000f),  module, Chiaroscuro::DIST_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("drive-cv",           30.480f, 88.000f),  module, Chiaroscuro::DRIVE_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("dist-type-cv",       30.480f, 101.500f), module, Chiaroscuro::TYPE_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("mix-cv",             50.480f, 88.000f),  module, Chiaroscuro::MIX_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("sidechain-detect-cv",50.480f, 101.500f), module, Chiaroscuro::SIDECHAIN_INPUT));

        // Audio I/O
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("audio-in-l",  10.480f, 114.500f), module, Chiaroscuro::AUDIO_L_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("audio-in-r",  23.813f, 114.500f), module, Chiaroscuro::AUDIO_R_INPUT));
        addOutputWithHalo<ShapetakerBNCPort>(this, centerPx("audio-out-l", 37.147f, 114.500f), module, Chiaroscuro::AUDIO_L_OUTPUT);
        addOutputWithHalo<ShapetakerBNCPort>(this, centerPx("audio-out-r", 50.480f, 114.500f), module, Chiaroscuro::AUDIO_R_OUTPUT);
    }
};

Model* modelChiaroscuro = createModel<Chiaroscuro, ChiaroscuroWidget>("Chiaroscuro");
