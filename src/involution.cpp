#include "plugin.hpp"
#include <cmath>
#include <random>
#include <array>
#include <algorithm>
#include "involution/liquid_filter.hpp"
#include "involution/dsp.hpp"

struct Involution : Module {
    enum ParamId {
        CUTOFF_A_PARAM,
        RESONANCE_A_PARAM,
        CUTOFF_B_PARAM,
        RESONANCE_B_PARAM,
        CROSS_PARAM,           // Cross-coupled filter feedback amount
        MOD_PARAM,             // Chaos LFO depth on cutoff
        SHIMMER_PARAM,         // Continuous LP -> BP -> HP form
        MOD_RATE_PARAM,        // Lorenz modulation rate
        LINK_CUTOFF_PARAM,
        LINK_RESONANCE_PARAM,
        CUTOFF_A_ATTEN_PARAM,
        RESONANCE_A_ATTEN_PARAM,
        CUTOFF_B_ATTEN_PARAM,
        RESONANCE_B_ATTEN_PARAM,
        SHIFT_DEPTH_PARAM,      // Symmetric cutoff spread
        PARAMS_LEN
    };
    enum InputId {
        AUDIO_A_INPUT,
        AUDIO_B_INPUT,
        CUTOFF_A_CV_INPUT,
        RESONANCE_A_CV_INPUT,
        CUTOFF_B_CV_INPUT,
        RESONANCE_B_CV_INPUT,
        CROSS_CV_INPUT,
        MOD_CV_INPUT,
        SHIMMER_CV_INPUT,
        MOD_RATE_CV_INPUT,
        SPREAD_CV_INPUT,
        CHAOS_FREEZE_GATE_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        AUDIO_A_OUTPUT,
        AUDIO_B_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    enum RoutingMode {
        ROUTING_PARALLEL,
        ROUTING_SERIAL_A_B,
        ROUTING_SERIAL_B_A,
        ROUTING_MID_SIDE
    };

    enum ChaosDestination {
        CHAOS_CUTOFF,
        CHAOS_CUTOFF_RESONANCE,
        CHAOS_FULL_FIELD
    };

    enum CharacterMode {
        CHARACTER_STILL,
        CHARACTER_LIQUID,
        CHARACTER_VOLATILE
    };

    // One filter lane (A or B) within a voice: the morphable liquid filter,
    // its CV/zipper smoothers, and output DC-blocker state.
    struct LaneState {
        LiquidFilter filter;
        shapetaker::FastSmoother cutoffSmoother;
        shapetaker::FastSmoother resonanceSmoother;
        shapetaker::FastSmoother pitchOctSmoother;
        float dcLastIn = 0.f;
        float dcLastOut = 0.f;
    };

    // All per-voice DSP state: A/B lanes, control smoothers, and the previous
    // outputs used by the stable one-sample cross-coupling loop.
    struct VoiceState {
        LaneState a;
        LaneState b;
        shapetaker::FastSmoother coupleSmoother;
        shapetaker::FastSmoother modSmoother;
        shapetaker::FastSmoother formSmoother;
        float coupledOutputA = 0.f;
        float coupledOutputB = 0.f;
    };

    std::array<VoiceState, shapetaker::PolyphonicProcessor::MAX_VOICES> voiceState{};

    // Lorenz strange attractor — chaotic modulation source (replaces harmonic LFO)
    shapetaker::involution::LorenzAttractor lorenzGen;
    shapetaker::involution::LorenzAttractor driftGen;

    // Rate bounds (also used by ChaosVisualizer)
    static constexpr float DEFAULT_SAMPLE_RATE = 44100.f;
    static constexpr float MOD_RATE_MIN_HZ = 0.01f;
    static constexpr float MOD_RATE_MAX_HZ = 10.f;
    static constexpr float MOD_RATE_DEFAULT_HZ = 0.5f;

    // Global parameter smoothers
    shapetaker::FastSmoother cutoffASmooth, cutoffBSmooth;
    shapetaker::FastSmoother resonanceASmooth, resonanceBSmooth;
    shapetaker::FastSmoother coupleSmooth, modSmooth, formSmooth, spreadSmooth;
    shapetaker::FastSmoother modRateSmooth;

    static constexpr float PARAM_SMOOTH_TC     = 0.015f;
    static constexpr float CUTOFF_CV_SMOOTH_TC = 0.002f;
    static constexpr float RESONANCE_CV_SMOOTH_TC = 0.002f;
    static constexpr float CV_NORMALIZED_SCALE = 0.1f;    // 10V -> 1.0
    static constexpr float LINK_SWITCH_THRESHOLD = 0.5f;
    static constexpr float LINK_PARAM_EPSILON = 1e-6f;
    static constexpr float CUTOFF_CURVE_EXP    = 1.5f;    // x^1.5 knob shaping — tighter near cutoff
    static constexpr float CUTOFF_HZ_BASE      = 20.f;    // lowest cutoff frequency in Hz
    static constexpr float CUTOFF_HZ_OCTAVES   = 10.f;    // number of octaves across knob travel
    static constexpr float CHAOS_CUTOFF_RANGE  = 0.15f;   // ±normalized range for chaos LFO on cutoff
    static constexpr float MOD_RATE_CV_SCALE   = 0.5f;    // V/oct-ish scale for mod rate CV
    static constexpr float NYQUIST_HEADROOM_RATIO = 0.45f;
    static constexpr float COUPLE_MAX_GAIN     = 0.78f;
    static constexpr float COUPLE_SAT_SCALE    = 0.20f;
    static constexpr float COUPLE_SAT_SWING    = 5.f;
    static constexpr float SPREAD_MAX_OCTAVES  = 3.f;
    static constexpr float CHAOS_RESONANCE_RANGE = 0.11f;
    static constexpr float CHAOS_COUPLE_RANGE  = 0.16f;
    static constexpr float CHAOS_FORM_RANGE    = 0.12f;
    static constexpr float OUTPUT_SOFT_KNEE    = 10.f;    // normal Rack-level audio passes untouched
    static constexpr float OUTPUT_SOFT_LIMIT   = 12.f;    // catches resonant/effect peaks smoothly
    static constexpr float EXPERIMENT_INPUT_DRIVE = 2.0f; // branch experiment: Clean=1, Warm=2, Hot=3.5, Dirty=5
    static constexpr float DRIFT_RATE_HZ       = 0.17f;
    static constexpr float DRIFT_AMOUNT        = 0.0015f; // ±0.15% cutoff wander

    // Bidirectional linking state
    float lastCutoffA = -1.f, lastCutoffB = -1.f;
    float lastResonanceA = -1.f, lastResonanceB = -1.f;
    bool lastLinkCutoff = false, lastLinkResonance = false;
    int activeVoiceCount = 0;
    bool pitchTrackA = false;
    bool pitchTrackB = false;
    float cachedDCCoeff = 0.995f;
    float voiceCutoffSpreadMult[shapetaker::PolyphonicProcessor::MAX_VOICES] = {};

    // Visualizer-readable data
    std::atomic<float> inputActivity{0.f};
    float smoothedChaosRate = MOD_RATE_DEFAULT_HZ;  // alias for visualizer compat
    float effectiveResonanceA = 0.707f;
    float effectiveResonanceB = 0.707f;
    float effectiveCutoffA = 1.0f;
    float effectiveCutoffB = 1.0f;
    float coupleAmount = 0.f;
    float modAmount    = 0.f;
    float formAmount = 0.f;
    float spreadAmount = 0.f;
    int routingMode = ROUTING_PARALLEL;
    int chaosDestination = CHAOS_FULL_FIELD;
    int characterMode = CHARACTER_LIQUID;
    bool chaosFrozen = false;
    std::atomic<bool> chaosFreezeGateActive{false};
    bool opposedForm = false;

    // Screen color theme (0=Phosphor, 1=Lunar, 2=Solar, 3=Amber)
    int chaosTheme = 0;
    bool useSharedChaosTheme = true;
    int sharedChaosThemeRevisionSeen = 0;

    Involution() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(CUTOFF_A_PARAM,     0.f,  1.f,  1.f,   "Filter A Cutoff",    " Hz", std::pow(2.f, CUTOFF_HZ_OCTAVES), CUTOFF_HZ_BASE);
        configParam(RESONANCE_A_PARAM,  LiquidFilter::RESONANCE_MIN, LiquidFilter::RESONANCE_MAX, LiquidFilter::RESONANCE_MIN, "Filter A Resonance");
        configParam(CUTOFF_B_PARAM,     0.f,  1.f,  1.f,   "Filter B Cutoff",    " Hz", std::pow(2.f, CUTOFF_HZ_OCTAVES), CUTOFF_HZ_BASE);
        configParam(RESONANCE_B_PARAM,  LiquidFilter::RESONANCE_MIN, LiquidFilter::RESONANCE_MAX, LiquidFilter::RESONANCE_MIN, "Filter B Resonance");
        configParam(CROSS_PARAM,        0.f,  1.f,  0.f,   "Filter Coupling",     "%", 0.f, 100.f);
        configParam(MOD_PARAM,          0.f,  1.f,  0.f,   "Chaos Depth",        "%", 0.f, 100.f);
        configParam(SHIMMER_PARAM,      0.f,  1.f,  0.f,   "Filter Form",         "%", 0.f, 100.f);
        configParam(MOD_RATE_PARAM, MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ, MOD_RATE_DEFAULT_HZ, "Chaos Rate", " Hz");

        configSwitch(LINK_CUTOFF_PARAM,    0.f, 1.f, 0.f, "Link Cutoff Frequencies", {"Independent", "Linked"});
        configSwitch(LINK_RESONANCE_PARAM, 0.f, 1.f, 0.f, "Link Resonance Amounts",  {"Independent", "Linked"});

        configParam(CUTOFF_A_ATTEN_PARAM,    -1.f, 1.f, 0.f, "Cutoff A CV Attenuverter",    "%", 0.f, 100.f);
        configParam(RESONANCE_A_ATTEN_PARAM, -1.f, 1.f, 0.f, "Resonance A CV Attenuverter", "%", 0.f, 100.f);
        configParam(CUTOFF_B_ATTEN_PARAM,    -1.f, 1.f, 0.f, "Cutoff B CV Attenuverter",    "%", 0.f, 100.f);
        configParam(RESONANCE_B_ATTEN_PARAM, -1.f, 1.f, 0.f, "Resonance B CV Attenuverter", "%", 0.f, 100.f);
        configParam(SHIFT_DEPTH_PARAM, 0.f, 1.f, 0.f, "Cutoff Spread", " oct", 0.f, SPREAD_MAX_OCTAVES);

        configInput(AUDIO_A_INPUT,       "Audio A");
        configInput(AUDIO_B_INPUT,       "Audio B");
        configInput(CUTOFF_A_CV_INPUT,   "Filter A Cutoff CV");
        configInput(RESONANCE_A_CV_INPUT,"Filter A Resonance CV");
        configInput(CUTOFF_B_CV_INPUT,   "Filter B Cutoff CV");
        configInput(RESONANCE_B_CV_INPUT,"Filter B Resonance CV");
        configInput(CROSS_CV_INPUT,      "Filter Coupling CV");
        configInput(MOD_CV_INPUT,        "Chaos Depth CV");
        configInput(SHIMMER_CV_INPUT,    "Filter Form CV");
        configInput(MOD_RATE_CV_INPUT,   "Chaos Rate CV");
        configInput(SPREAD_CV_INPUT,     "Cutoff Spread CV");
        configInput(CHAOS_FREEZE_GATE_INPUT, "Chaos Freeze Gate");

        configOutput(AUDIO_A_OUTPUT, "Audio A");
        configOutput(AUDIO_B_OUTPUT, "Audio B");
        // Bypass passes dry audio through instead of going silent
        configBypass(AUDIO_A_INPUT, AUDIO_A_OUTPUT);
        configBypass(AUDIO_B_INPUT, AUDIO_B_OUTPUT);

        onSampleRateChange();

        // Fixed per-voice cutoff spread — voice 0 is at exact user position,
        // voices 1+ get random ±1.5 semitone offsets so resonant peaks diverge
        // across polyphonic voices, producing natural chorus-like thickness.
        voiceCutoffSpreadMult[0] = 1.f;
        for (int v = 1; v < shapetaker::PolyphonicProcessor::MAX_VOICES; v++) {
            float semitones = rack::random::uniform() * 3.f - 1.5f;
            voiceCutoffSpreadMult[v] = std::pow(2.f, semitones / 12.f);
        }

        shapetaker::ui::LabelFormatter::normalizeModuleControls(this);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        int effectiveTheme = getEffectiveChaosTheme();
        json_object_set_new(rootJ, "chaosTheme", json_integer(effectiveTheme));
        json_object_set_new(rootJ, "localChaosTheme", json_integer(clamp(chaosTheme, 0, 3)));
        json_object_set_new(rootJ, "useSharedChaosTheme", json_boolean(useSharedChaosTheme));
        json_object_set_new(rootJ, "pitchTrackA", json_boolean(pitchTrackA));
        json_object_set_new(rootJ, "pitchTrackB", json_boolean(pitchTrackB));
        json_object_set_new(rootJ, "routingMode", json_integer(routingMode));
        json_object_set_new(rootJ, "chaosDestination", json_integer(chaosDestination));
        json_object_set_new(rootJ, "characterMode", json_integer(characterMode));
        json_object_set_new(rootJ, "chaosFrozen", json_boolean(chaosFrozen));
        json_object_set_new(rootJ, "opposedForm", json_boolean(opposedForm));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* j = json_object_get(rootJ, "chaosTheme");
        if (j) {
            chaosTheme = clamp((int)json_integer_value(j), 0, 3);
        }
        if (json_t* localThemeJ = json_object_get(rootJ, "localChaosTheme")) {
            chaosTheme = clamp((int)json_integer_value(localThemeJ), 0, 3);
        }
        if (json_t* useSharedJ = json_object_get(rootJ, "useSharedChaosTheme")) {
            useSharedChaosTheme = json_is_true(useSharedJ);
        } else {
            useSharedChaosTheme = true;
        }
        if (useSharedChaosTheme && j) {
            shapetaker::ui::ThemeManager::DisplayTheme::restoreSharedTheme(
                clamp((int)json_integer_value(j), 0, 3));
        }
        sharedChaosThemeRevisionSeen = shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeRevision();
        if (json_t* trackAJ = json_object_get(rootJ, "pitchTrackA")) {
            pitchTrackA = json_is_true(trackAJ);
        }
        if (json_t* trackBJ = json_object_get(rootJ, "pitchTrackB")) {
            pitchTrackB = json_is_true(trackBJ);
        }
        if (json_t* routingJ = json_object_get(rootJ, "routingMode")) {
            routingMode = clamp((int)json_integer_value(routingJ), (int)ROUTING_PARALLEL, (int)ROUTING_MID_SIDE);
        }
        if (json_t* destinationJ = json_object_get(rootJ, "chaosDestination")) {
            chaosDestination = clamp((int)json_integer_value(destinationJ), (int)CHAOS_CUTOFF, (int)CHAOS_FULL_FIELD);
        }
        if (json_t* characterJ = json_object_get(rootJ, "characterMode")) {
            characterMode = clamp((int)json_integer_value(characterJ), (int)CHARACTER_STILL, (int)CHARACTER_VOLATILE);
        }
        if (json_t* frozenJ = json_object_get(rootJ, "chaosFrozen")) {
            chaosFrozen = json_is_true(frozenJ);
        }
        if (json_t* opposedJ = json_object_get(rootJ, "opposedForm")) {
            opposedForm = json_is_true(opposedJ);
        }
    }

    void syncSharedChaosThemeOverride() {
        int revision = shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeRevision();
        if (sharedChaosThemeRevisionSeen != revision) {
            useSharedChaosTheme = true;
            sharedChaosThemeRevisionSeen = revision;
        }
    }

    int getEffectiveChaosTheme() {
        syncSharedChaosThemeOverride();
        return useSharedChaosTheme
            ? shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeIndex()
            : clamp(chaosTheme, 0, 3);
    }

    bool isChaosFrozen() const {
        return chaosFrozen || chaosFreezeGateActive.load(std::memory_order_relaxed);
    }

    void onSampleRateChange() override {
        float sr = DEFAULT_SAMPLE_RATE;
        if (APP && APP->engine) {
            sr = APP->engine->getSampleRate();
        }
        cachedDCCoeff = 1.f - (2.f * float(M_PI) * 10.f / sr);
        for (VoiceState& vs : voiceState) {
            for (LaneState* lane : {&vs.a, &vs.b}) {
                lane->filter.setSampleRate(sr);
            }
        }
    }

    static inline float cvToNormalized(float cv) {
        return cv * CV_NORMALIZED_SCALE;
    }

    static inline float applyUnitCv(float base, float cv) {
        return base + cvToNormalized(cv);
    }

    static inline float applyAttenuvertedCv(float base, float cv, float attenuverter) {
        return base + cvToNormalized(cv) * attenuverter;
    }

    static inline float smoothstep01(float x) {
        x = clamp(x, 0.f, 1.f);
        return x * x * (3.f - 2.f * x);
    }

    static inline float softOutputGuard(float x) {
        if (!std::isfinite(x)) {
            return 0.f;
        }
        float ax = std::abs(x);
        if (ax <= OUTPUT_SOFT_KNEE) {
            return x;
        }
        float range = std::max(OUTPUT_SOFT_LIMIT - OUTPUT_SOFT_KNEE, 0.1f);
        float limited = OUTPUT_SOFT_KNEE + range * shapetaker::dsp::OscillatorHelper::fastTanh((ax - OUTPUT_SOFT_KNEE) / range);
        return std::copysign(limited, x);
    }

    static inline float getPolyOrMonoVoltage(Input& input, int channel) {
        if (!input.isConnected()) {
            return 0.f;
        }
        int channels = input.getChannels();
        if (channels <= 1) {
            return input.getVoltage(0);
        }
        return input.getVoltage(std::min(channel, channels - 1));
    }

    static inline float cutoffNormalizedToHz(float cutoff) {
        float c = clamp(cutoff, 0.f, 1.f);
        // x^1.5 is x * sqrt(x), which is significantly faster than std::pow
        float shaped = c * std::sqrt(c);
        return CUTOFF_HZ_BASE * rack::dsp::exp2_taylor5(shaped * CUTOFF_HZ_OCTAVES);
    }

    float getCharacterScale() const {
        switch (characterMode) {
            case CHARACTER_STILL: return 0.f;
            case CHARACTER_VOLATILE: return 1.65f;
            default: return 1.f;
        }
    }

    const char* getRoutingShortLabel() const {
        switch (routingMode) {
            case ROUTING_SERIAL_A_B: return "A>B";
            case ROUTING_SERIAL_B_A: return "B>A";
            case ROUTING_MID_SIDE: return "M/S";
            default: return "PAR";
        }
    }

    const char* getCharacterShortLabel() const {
        switch (characterMode) {
            case CHARACTER_STILL: return "STILL";
            case CHARACTER_VOLATILE: return "VOLT";
            default: return "LIQ";
        }
    }

    static inline void syncLinkedParamPair(
        bool linked, bool& wasLinked,
        float& currentA, float& currentB,
        float& lastA, float& lastB,
        Param& paramA, Param& paramB) {

        if (!linked) {
            return;
        }
        if (!wasLinked) {
            paramB.setValue(currentA);
            currentB = currentA;
            return;
        }

        bool aChanged = std::abs(currentA - lastA) > LINK_PARAM_EPSILON;
        bool bChanged = std::abs(currentB - lastB) > LINK_PARAM_EPSILON;
        if (aChanged && !bChanged) {
            paramB.setValue(currentA);
            currentB = currentA;
        } else if (bChanged && !aChanged) {
            paramA.setValue(currentB);
            currentA = currentB;
        } else if (aChanged && bChanged) {
            paramB.setValue(currentA);
            currentB = currentA;
        }
    }

    void resetGlobalSmoothers() {
        cutoffASmooth.reset(params[CUTOFF_A_PARAM].getValue());
        cutoffBSmooth.reset(params[CUTOFF_B_PARAM].getValue());
        resonanceASmooth.reset(params[RESONANCE_A_PARAM].getValue());
        resonanceBSmooth.reset(params[RESONANCE_B_PARAM].getValue());
        coupleSmooth.reset(params[CROSS_PARAM].getValue());
        modSmooth.reset(params[MOD_PARAM].getValue());
        formSmooth.reset(params[SHIMMER_PARAM].getValue());
        spreadSmooth.reset(params[SHIFT_DEPTH_PARAM].getValue());
        modRateSmooth.reset(params[MOD_RATE_PARAM].getValue());
    }

    void resetVoiceState(int v) {
        VoiceState& vs = voiceState[v];
        for (LaneState* lane : {&vs.a, &vs.b}) {
            lane->filter.reset();
            lane->dcLastIn = lane->dcLastOut = 0.f;
            lane->pitchOctSmoother.reset(0.f);
        }
        vs.coupledOutputA = vs.coupledOutputB = 0.f;
        vs.a.cutoffSmoother.reset(params[CUTOFF_A_PARAM].getValue());
        vs.b.cutoffSmoother.reset(params[CUTOFF_B_PARAM].getValue());
        vs.a.resonanceSmoother.reset(params[RESONANCE_A_PARAM].getValue());
        vs.b.resonanceSmoother.reset(params[RESONANCE_B_PARAM].getValue());
        vs.coupleSmoother.reset(params[CROSS_PARAM].getValue());
        vs.modSmoother.reset(params[MOD_PARAM].getValue());
        vs.formSmoother.reset(params[SHIMMER_PARAM].getValue());
    }

    void resetInactiveVoiceState(int activeChannels) {
        activeChannels = clamp(activeChannels, 0, shapetaker::PolyphonicProcessor::MAX_VOICES);
        for (int v = activeChannels; v < activeVoiceCount; v++) {
            resetVoiceState(v);
        }
        activeVoiceCount = activeChannels;
    }

    void onReset() override {
        lorenzGen.reset();
        driftGen.reset();
        for (int v = 0; v < shapetaker::PolyphonicProcessor::MAX_VOICES; v++) {
            resetVoiceState(v);
        }
        resetGlobalSmoothers();
        lastCutoffA = lastCutoffB = -1.f;
        lastResonanceA = lastResonanceB = -1.f;
        lastLinkCutoff = lastLinkResonance = false;
        activeVoiceCount = 0;
        routingMode = ROUTING_PARALLEL;
        chaosDestination = CHAOS_FULL_FIELD;
        characterMode = CHARACTER_LIQUID;
        chaosFrozen = false;
        chaosFreezeGateActive.store(false, std::memory_order_relaxed);
        opposedForm = false;
        onSampleRateChange();
    }

    void process(const ProcessArgs& args) override {
        bool linkCutoff = params[LINK_CUTOFF_PARAM].getValue() > LINK_SWITCH_THRESHOLD;
        bool linkResonance = params[LINK_RESONANCE_PARAM].getValue() > LINK_SWITCH_THRESHOLD;

        float currentCutoffA = params[CUTOFF_A_PARAM].getValue();
        float currentCutoffB = params[CUTOFF_B_PARAM].getValue();
        float currentResonanceA = params[RESONANCE_A_PARAM].getValue();
        float currentResonanceB = params[RESONANCE_B_PARAM].getValue();
        syncLinkedParamPair(linkCutoff, lastLinkCutoff, currentCutoffA, currentCutoffB,
            lastCutoffA, lastCutoffB, params[CUTOFF_A_PARAM], params[CUTOFF_B_PARAM]);
        syncLinkedParamPair(linkResonance, lastLinkResonance, currentResonanceA, currentResonanceB,
            lastResonanceA, lastResonanceB, params[RESONANCE_A_PARAM], params[RESONANCE_B_PARAM]);
        lastCutoffA = currentCutoffA;
        lastCutoffB = currentCutoffB;
        lastResonanceA = currentResonanceA;
        lastResonanceB = currentResonanceB;
        lastLinkCutoff = linkCutoff;
        lastLinkResonance = linkResonance;

        float cutoffA = cutoffASmooth.process(currentCutoffA, args.sampleTime);
        float cutoffB = cutoffBSmooth.process(currentCutoffB, args.sampleTime);
        float resonanceA = resonanceASmooth.process(currentResonanceA, args.sampleTime);
        float resonanceB = resonanceBSmooth.process(currentResonanceB, args.sampleTime);

        bool hasCoupleCv = inputs[CROSS_CV_INPUT].isConnected();
        bool hasModCv = inputs[MOD_CV_INPUT].isConnected();
        bool hasFormCv = inputs[SHIMMER_CV_INPUT].isConnected();
        bool hasModRateCv = inputs[MOD_RATE_CV_INPUT].isConnected();
        bool hasSpreadCv = inputs[SPREAD_CV_INPUT].isConnected();
        bool hasCutoffACv = inputs[CUTOFF_A_CV_INPUT].isConnected();
        bool hasCutoffBCv = inputs[CUTOFF_B_CV_INPUT].isConnected();
        bool hasResonanceACv = inputs[RESONANCE_A_CV_INPUT].isConnected();
        bool hasResonanceBCv = inputs[RESONANCE_B_CV_INPUT].isConnected();

        float cutoffAAtten = params[CUTOFF_A_ATTEN_PARAM].getValue();
        float cutoffBAtten = params[CUTOFF_B_ATTEN_PARAM].getValue();
        float resonanceAAtten = params[RESONANCE_A_ATTEN_PARAM].getValue();
        float resonanceBAtten = params[RESONANCE_B_ATTEN_PARAM].getValue();
        float coupleBase = params[CROSS_PARAM].getValue();
        float modBase = params[MOD_PARAM].getValue();
        float formBase = params[SHIMMER_PARAM].getValue();

        float coupleTarget = coupleBase;
        if (hasCoupleCv)
            coupleTarget = applyUnitCv(coupleTarget, getPolyOrMonoVoltage(inputs[CROSS_CV_INPUT], 0));
        coupleAmount = coupleSmooth.process(clamp(coupleTarget, 0.f, 1.f), args.sampleTime, PARAM_SMOOTH_TC);

        float modTarget = modBase;
        if (hasModCv)
            modTarget = applyUnitCv(modTarget, getPolyOrMonoVoltage(inputs[MOD_CV_INPUT], 0));
        modAmount = modSmooth.process(clamp(modTarget, 0.f, 1.f), args.sampleTime, PARAM_SMOOTH_TC);

        float formTarget = formBase;
        if (hasFormCv)
            formTarget = applyUnitCv(formTarget, getPolyOrMonoVoltage(inputs[SHIMMER_CV_INPUT], 0));
        formAmount = formSmooth.process(clamp(formTarget, 0.f, 1.f), args.sampleTime, PARAM_SMOOTH_TC);
        float spreadTarget = params[SHIFT_DEPTH_PARAM].getValue();
        if (hasSpreadCv)
            spreadTarget = applyUnitCv(spreadTarget, getPolyOrMonoVoltage(inputs[SPREAD_CV_INPUT], 0));
        spreadAmount = spreadSmooth.process(clamp(spreadTarget, 0.f, 1.f),
            args.sampleTime, PARAM_SMOOTH_TC);

        float modRateTarget = clamp(params[MOD_RATE_PARAM].getValue(), MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ);
        if (hasModRateCv) {
            modRateTarget = clamp(modRateTarget + getPolyOrMonoVoltage(inputs[MOD_RATE_CV_INPUT], 0) * MOD_RATE_CV_SCALE,
                                  MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ);
        }
        float modRate = modRateSmooth.process(modRateTarget, args.sampleTime);
        smoothedChaosRate = modRate;

        bool freezeGate = inputs[CHAOS_FREEZE_GATE_INPUT].getVoltage() >= 1.f;
        chaosFreezeGateActive.store(freezeGate, std::memory_order_relaxed);
        if (!isChaosFrozen()) {
            lorenzGen.process(modRate, args.sampleTime);
            driftGen.process(DRIFT_RATE_HZ, args.sampleTime);
        }

        // Values used by the CRT response display are sampled from voice zero.
        float displayCutoffA = cutoffA;
        float displayCutoffB = cutoffB;
        float displayResA = resonanceA;
        float displayResB = resonanceB;
        if (hasCutoffACv && !pitchTrackA)
            displayCutoffA = applyAttenuvertedCv(displayCutoffA, getPolyOrMonoVoltage(inputs[CUTOFF_A_CV_INPUT], 0), cutoffAAtten);
        if (hasCutoffBCv && !pitchTrackB)
            displayCutoffB = applyAttenuvertedCv(displayCutoffB, getPolyOrMonoVoltage(inputs[CUTOFF_B_CV_INPUT], 0), cutoffBAtten);
        if (hasResonanceACv)
            displayResA = applyAttenuvertedCv(displayResA, getPolyOrMonoVoltage(inputs[RESONANCE_A_CV_INPUT], 0), resonanceAAtten);
        if (hasResonanceBCv)
            displayResB = applyAttenuvertedCv(displayResB, getPolyOrMonoVoltage(inputs[RESONANCE_B_CV_INPUT], 0), resonanceBAtten);
        effectiveCutoffA = clamp(displayCutoffA, 0.f, 1.f);
        effectiveCutoffB = clamp(displayCutoffB, 0.f, 1.f);
        effectiveResonanceA = clamp(displayResA, LiquidFilter::RESONANCE_MIN, LiquidFilter::RESONANCE_MAX);
        effectiveResonanceB = clamp(displayResB, LiquidFilter::RESONANCE_MIN, LiquidFilter::RESONANCE_MAX);

        bool hasAudioA = inputs[AUDIO_A_INPUT].isConnected();
        bool hasAudioB = inputs[AUDIO_B_INPUT].isConnected();
        int channels = std::min(std::max(inputs[AUDIO_A_INPUT].getChannels(), inputs[AUDIO_B_INPUT].getChannels()),
                                shapetaker::PolyphonicProcessor::MAX_VOICES);
        float frameMaxActivity = 0.f;

        if (!hasAudioA && !hasAudioB) {
            outputs[AUDIO_A_OUTPUT].setChannels(0);
            outputs[AUDIO_B_OUTPUT].setChannels(0);
            resetInactiveVoiceState(0);
        } else {
            outputs[AUDIO_A_OUTPUT].setChannels(channels);
            outputs[AUDIO_B_OUTPUT].setChannels(channels);
            resetInactiveVoiceState(channels);

            float lorenzAxes[3] = {
                clamp(lorenzGen.getX(), -1.f, 1.f),
                clamp(lorenzGen.getY(), -1.f, 1.f),
                clamp(lorenzGen.getZ(), -1.f, 1.f)
            };
            float driftAxes[3] = {
                clamp(driftGen.getX(), -1.f, 1.f),
                clamp(driftGen.getY(), -1.f, 1.f),
                clamp(driftGen.getZ(), -1.f, 1.f)
            };
            float spreadOctaves = spreadAmount * SPREAD_MAX_OCTAVES;
            float stereoSpreadRatio = rack::dsp::exp2_taylor5(spreadOctaves * 0.5f);
            float resonanceChaosSpan = (LiquidFilter::RESONANCE_MAX - LiquidFilter::RESONANCE_MIN)
                                        * CHAOS_RESONANCE_RANGE;
            float character = getCharacterScale();

            for (int c = 0; c < channels; c++) {
                float audioA = hasAudioA ? getPolyOrMonoVoltage(inputs[AUDIO_A_INPUT], c)
                                         : getPolyOrMonoVoltage(inputs[AUDIO_B_INPUT], c);
                float audioB = hasAudioB ? getPolyOrMonoVoltage(inputs[AUDIO_B_INPUT], c)
                                         : getPolyOrMonoVoltage(inputs[AUDIO_A_INPUT], c);
                if (!std::isfinite(audioA)) audioA = 0.f;
                if (!std::isfinite(audioB)) audioB = 0.f;
                frameMaxActivity = std::max(frameMaxActivity, std::fabs(audioA) + std::fabs(audioB));

                VoiceState& vs = voiceState[c];
                auto voiceAmount = [&](shapetaker::FastSmoother& smoother, float base, Input& cv, bool hasCv) {
                    float amount = base;
                    if (hasCv)
                        amount = applyUnitCv(amount, getPolyOrMonoVoltage(cv, c));
                    return smoother.process(clamp(amount, 0.f, 1.f), args.sampleTime, PARAM_SMOOTH_TC);
                };
                float voiceCouple = voiceAmount(vs.coupleSmoother, coupleBase, inputs[CROSS_CV_INPUT], hasCoupleCv);
                float voiceMod = voiceAmount(vs.modSmoother, modBase, inputs[MOD_CV_INPUT], hasModCv);
                float voiceForm = voiceAmount(vs.formSmoother, formBase, inputs[SHIMMER_CV_INPUT], hasFormCv);

                float polarity = (c >= 3) ? -1.f : 1.f;
                float chaosA = lorenzAxes[c % 3] * polarity * voiceMod;
                float chaosB = lorenzAxes[(c + 1) % 3] * polarity * voiceMod;
                float chaosField = lorenzAxes[(c + 2) % 3] * polarity * voiceMod;
                float voiceDrift = driftAxes[c % 3] * polarity;

                auto laneCutoffHz = [&](LaneState& lane, float baseCutoff, Input& cutoffCv,
                                        bool hasCv, float atten, bool pitchTrack, float chaos) {
                    float cutoffNorm = baseCutoff;
                    if (hasCv && !pitchTrack)
                        cutoffNorm = applyAttenuvertedCv(cutoffNorm, getPolyOrMonoVoltage(cutoffCv, c), atten);
                    cutoffNorm = clamp(cutoffNorm + chaos * CHAOS_CUTOFF_RANGE, 0.f, 1.f);
                    cutoffNorm = lane.cutoffSmoother.process(cutoffNorm, args.sampleTime, CUTOFF_CV_SMOOTH_TC);
                    float hz = cutoffNormalizedToHz(cutoffNorm);
                    float pitchOctTarget = (pitchTrack && hasCv)
                        ? clamp(getPolyOrMonoVoltage(cutoffCv, c) * atten, -6.f, 6.f)
                        : 0.f;
                    float pitchOct = lane.pitchOctSmoother.process(pitchOctTarget, args.sampleTime, CUTOFF_CV_SMOOTH_TC);
                    if (pitchTrack && hasCv)
                        hz *= rack::dsp::exp2_taylor5(pitchOct);
                    return hz;
                };
                auto laneResonance = [&](LaneState& lane, float baseRes, Input& resCv, bool hasCv, float atten) {
                    float res = baseRes;
                    if (hasCv)
                        res = applyAttenuvertedCv(res, getPolyOrMonoVoltage(resCv, c), atten);
                    res = clamp(res, LiquidFilter::RESONANCE_MIN, LiquidFilter::RESONANCE_MAX);
                    return lane.resonanceSmoother.process(res, args.sampleTime, RESONANCE_CV_SMOOTH_TC);
                };

                float cutoffAHz = laneCutoffHz(vs.a, cutoffA, inputs[CUTOFF_A_CV_INPUT], hasCutoffACv,
                                               cutoffAAtten, pitchTrackA, chaosA);
                float cutoffBHz = laneCutoffHz(vs.b, cutoffB, inputs[CUTOFF_B_CV_INPUT], hasCutoffBCv,
                                               cutoffBAtten, pitchTrackB, chaosB);
                float voiceResA = laneResonance(vs.a, resonanceA, inputs[RESONANCE_A_CV_INPUT], hasResonanceACv, resonanceAAtten);
                float voiceResB = laneResonance(vs.b, resonanceB, inputs[RESONANCE_B_CV_INPUT], hasResonanceBCv, resonanceBAtten);

                if (chaosDestination >= CHAOS_CUTOFF_RESONANCE) {
                    voiceResA = clamp(voiceResA + chaosField * resonanceChaosSpan,
                                      LiquidFilter::RESONANCE_MIN, LiquidFilter::RESONANCE_MAX);
                    voiceResB = clamp(voiceResB - chaosField * resonanceChaosSpan,
                                      LiquidFilter::RESONANCE_MIN, LiquidFilter::RESONANCE_MAX);
                }
                if (chaosDestination == CHAOS_FULL_FIELD) {
                    voiceForm = clamp(voiceForm + chaosField * CHAOS_FORM_RANGE, 0.f, 1.f);
                    voiceCouple = clamp(voiceCouple + chaosField * CHAOS_COUPLE_RANGE * voiceCouple, 0.f, 1.f);
                }
                float voiceFormA = voiceForm;
                float voiceFormB = opposedForm ? 1.f - voiceForm : voiceForm;

                float microSpread = voiceCutoffSpreadMult[c];
                float driftMult = 1.f + voiceDrift * DRIFT_AMOUNT;
                cutoffAHz = clamp(cutoffAHz * microSpread * driftMult / stereoSpreadRatio,
                                  CUTOFF_HZ_BASE, args.sampleRate * NYQUIST_HEADROOM_RATIO);
                cutoffBHz = clamp(cutoffBHz * microSpread * driftMult * stereoSpreadRatio,
                                  CUTOFF_HZ_BASE, args.sampleRate * NYQUIST_HEADROOM_RATIO);

                if (routingMode == ROUTING_MID_SIDE) {
                    float mid = (audioA + audioB) * 0.5f;
                    float side = (audioA - audioB) * 0.5f;
                    audioA = mid;
                    audioB = side;
                }

                float couplingGain = COUPLE_MAX_GAIN * smoothstep01(voiceCouple);
                auto saturateCoupling = [](float x) {
                    return shapetaker::dsp::OscillatorHelper::fastTanh(x * COUPLE_SAT_SCALE) * COUPLE_SAT_SWING;
                };
                float inputA = audioA + saturateCoupling(vs.coupledOutputB) * couplingGain;
                float inputB = audioB + saturateCoupling(vs.coupledOutputA) * couplingGain;
                float processedA = 0.f;
                float processedB = 0.f;

                if (routingMode == ROUTING_SERIAL_A_B) {
                    processedA = vs.a.filter.process(inputA, cutoffAHz, voiceResA,
                                                     EXPERIMENT_INPUT_DRIVE, voiceFormA, character);
                    processedB = vs.b.filter.process(inputB + processedA * 0.72f, cutoffBHz, voiceResB,
                                                     EXPERIMENT_INPUT_DRIVE, voiceFormB, character);
                } else if (routingMode == ROUTING_SERIAL_B_A) {
                    processedB = vs.b.filter.process(inputB, cutoffBHz, voiceResB,
                                                     EXPERIMENT_INPUT_DRIVE, voiceFormB, character);
                    processedA = vs.a.filter.process(inputA + processedB * 0.72f, cutoffAHz, voiceResA,
                                                     EXPERIMENT_INPUT_DRIVE, voiceFormA, character);
                } else {
                    processedA = vs.a.filter.process(inputA, cutoffAHz, voiceResA,
                                                     EXPERIMENT_INPUT_DRIVE, voiceFormA, character);
                    processedB = vs.b.filter.process(inputB, cutoffBHz, voiceResB,
                                                     EXPERIMENT_INPUT_DRIVE, voiceFormB, character);
                }

                processedA = shapetaker::dsp::AudioProcessor::processDCBlock(
                    processedA, vs.a.dcLastIn, vs.a.dcLastOut, cachedDCCoeff);
                processedB = shapetaker::dsp::AudioProcessor::processDCBlock(
                    processedB, vs.b.dcLastIn, vs.b.dcLastOut, cachedDCCoeff);
                processedA = softOutputGuard(processedA);
                processedB = softOutputGuard(processedB);
                vs.coupledOutputA = processedA;
                vs.coupledOutputB = processedB;

                if (routingMode == ROUTING_MID_SIDE) {
                    float mid = processedA;
                    float side = processedB;
                    processedA = softOutputGuard(mid + side);
                    processedB = softOutputGuard(mid - side);
                }
                outputs[AUDIO_A_OUTPUT].setVoltage(processedA, c);
                outputs[AUDIO_B_OUTPUT].setVoltage(processedB, c);
            }
        }

        float currentActivity = inputActivity.load(std::memory_order_relaxed);
        float activityRate = (frameMaxActivity > currentActivity) ? 50.f : 1.f;
        currentActivity += (frameMaxActivity - currentActivity) * activityRate * args.sampleTime;
        inputActivity.store(currentActivity, std::memory_order_relaxed);

    }

    void onRandomize() override {
        std::mt19937 rng(rack::random::u32());
        std::uniform_real_distribution<float> cutoffDist(0.2f, 0.9f);
        std::uniform_real_distribution<float> resDist(0.1f, 0.6f);
        std::uniform_real_distribution<float> charDist(0.f, 0.7f);
        std::uniform_real_distribution<float> spreadDist(0.f, 0.65f);
        std::uniform_real_distribution<float> rateDist(0.2f, 0.8f);
        std::uniform_int_distribution<int>    linkDist(0, 1);
        std::uniform_int_distribution<int>    routingDist(ROUTING_PARALLEL, ROUTING_MID_SIDE);

        params[CUTOFF_A_PARAM].setValue(cutoffDist(rng));
        params[CUTOFF_B_PARAM].setValue(cutoffDist(rng));
        params[RESONANCE_A_PARAM].setValue(resDist(rng));
        params[RESONANCE_B_PARAM].setValue(resDist(rng));
        params[CROSS_PARAM].setValue(charDist(rng));
        params[MOD_PARAM].setValue(charDist(rng));
        params[SHIMMER_PARAM].setValue(charDist(rng));
        params[MOD_RATE_PARAM].setValue(rateDist(rng));
        params[LINK_CUTOFF_PARAM].setValue((float)linkDist(rng));
        params[LINK_RESONANCE_PARAM].setValue((float)linkDist(rng));
        params[SHIFT_DEPTH_PARAM].setValue(spreadDist(rng));
        routingMode = routingDist(rng);
        opposedForm = linkDist(rng) != 0;
        chaosFrozen = false;
    }
};

// Chaos visualizer widget — extracted to separate files
#include "involution/chaos_visualizer.hpp"
#include "involution/chaos_visualizer_impl.hpp"

struct InvolutionWidget : ShapetakerModuleWidget {
    InvolutionWidget(Involution* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Involution.svg")));
        auto wearOverlay = new shapetaker::ui::PanelWearOverlay();
        wearOverlay->box = Rect(Vec(0, 0), box.size);
        addChild(wearOverlay);

        using LayoutHelper = shapetaker::ui::LayoutHelper;
        LayoutHelper::ScrewPositions::addStandardScrews<ScrewJetBlack>(this, box.size.x);
        addAlchemicalBadge(this, 19); // Yin Yang

        auto centerPx = LayoutHelper::createCenterPxHelper(
            asset::plugin(pluginInstance, "res/panels/Involution.svg"));

        auto addKnobWithShadow = [this](app::ParamWidget* knob) {
            ::addKnobWithShadow(this, knob);
        };

        // ---- Chaos visualizer ----
        // Add before controls so the inset screen and its shadows sit behind hardware.
        ChaosVisualizer* chaosViz = new ChaosVisualizer(module);
        Vec screenCenter = centerPx("chaos-visualizer", 45.085f, 56.000f);
        chaosViz->box.pos = Vec(screenCenter.x - 86.5f, screenCenter.y - 69.f);
        addChild(chaosViz);

        // ---- Filter A/B cutoff (extra large vintage) ----
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageXLarge>(
            centerPx("cutoff-a-knob", 20.585f, 22.500f), module, Involution::CUTOFF_A_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageXLarge>(
            centerPx("cutoff-b-knob", 69.585f, 22.500f), module, Involution::CUTOFF_B_PARAM));

        // ---- Filter A/B resonance ----
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(
            centerPx("resonance-a-knob", 12.585f, 60.500f), module, Involution::RESONANCE_A_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(
            centerPx("resonance-b-knob", 77.585f, 60.500f), module, Involution::RESONANCE_B_PARAM));

        // ---- Link switches ----
        addParam(createParamCentered<ShapetakerDarkToggleOffPos4>(
            centerPx("link-cutoff-switch", 45.085f, 15.000f), module, Involution::LINK_CUTOFF_PARAM));
        addParam(createParamCentered<ShapetakerDarkToggleOffPos4>(
            centerPx("link-resonance-switch", 45.085f, 28.500f), module, Involution::LINK_RESONANCE_PARAM));

        // ---- Attenuverters ----
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(
            centerPx("cutoff-a-attenuverter", 9.335f, 42.500f), module, Involution::CUTOFF_A_ATTEN_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(
            centerPx("resonance-a-attenuverter", 12.585f, 80.500f), module, Involution::RESONANCE_A_ATTEN_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(
            centerPx("cutoff-b-attenuverter", 80.835f, 42.500f), module, Involution::CUTOFF_B_ATTEN_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(
            centerPx("resonance-b-attenuverter", 77.585f, 80.500f), module, Involution::RESONANCE_B_ATTEN_PARAM));

        // ---- Filter field controls: Couple / Chaos Depth / Form ----
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(
            centerPx("coupling-knob", 12.585f, 98.000f), module, Involution::CROSS_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(
            centerPx("chaos-depth-knob", 58.335f, 85.500f), module, Involution::MOD_PARAM));
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(
            centerPx("form-knob", 77.585f, 98.000f), module, Involution::SHIMMER_PARAM));

        // ---- Symmetric A/B cutoff spread ----
        addKnobWithShadow(createParamCentered<ShapetakerKnobVintageSmallMedium>(
            centerPx("spread-knob", 31.835f, 85.500f), module, Involution::SHIFT_DEPTH_PARAM));

        // ---- Mod Rate (attenuverter-style small knob) ----
        addKnobWithShadow(createParamCentered<ShapetakerAttenuverterOscilloscope>(
            centerPx("chaos-rate-knob", 45.085f, 98.000f), module, Involution::MOD_RATE_PARAM));

        // ---- CV inputs ----
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("cutoff-a-cv-input",    29.835f, 42.500f), module, Involution::CUTOFF_A_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("resonance-a-cv-input", 29.835f, 70.500f), module, Involution::RESONANCE_A_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("cutoff-b-cv-input",    60.335f, 42.500f), module, Involution::CUTOFF_B_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("resonance-b-cv-input", 60.335f, 70.500f), module, Involution::RESONANCE_B_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("coupling-cv-input",  50.335f, 114.5f), module, Involution::CROSS_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("chaos-depth-cv-input", 39.835f, 114.5f), module, Involution::MOD_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("form-cv-input",  60.835f, 114.5f), module, Involution::SHIMMER_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("chaos-rate-cv-input", 29.335f, 114.5f), module, Involution::MOD_RATE_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("spread-cv-input", 28.835f, 84.355f), module, Involution::SPREAD_CV_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("chaos-freeze-gate-input", 61.335f, 84.355f), module, Involution::CHAOS_FREEZE_GATE_INPUT));

        // ---- Audio I/O ----
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("audio-a-input",  8.335f, 114.5f), module, Involution::AUDIO_A_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(
            centerPx("audio-b-input",  18.835f, 114.5f), module, Involution::AUDIO_B_INPUT));
        constexpr float kStereoOutputHaloMm = 9.9f;
        addOutputWithHalo<ShapetakerBNCPort>(
            this, centerPx("audio-a-output", 71.335f, 114.5f), module, Involution::AUDIO_A_OUTPUT, kStereoOutputHaloMm);
        addOutputWithHalo<ShapetakerBNCPort>(
            this, centerPx("audio-b-output", 81.835f, 114.5f), module, Involution::AUDIO_B_OUTPUT, kStereoOutputHaloMm);
    }

    void appendContextMenu(Menu* menu) override {
        ModuleWidget::appendContextMenu(menu);
        auto* inv = dynamic_cast<Involution*>(module);
        if (!inv) return;

        menu->addChild(new MenuSeparator);
        using DisplayTheme = shapetaker::ui::ThemeManager::DisplayTheme;
        DisplayTheme::addThemeMenu(
            menu,
            [=] { return inv->useSharedChaosTheme; },
            [=](bool follow) {
                inv->useSharedChaosTheme = follow;
                inv->sharedChaosThemeRevisionSeen = DisplayTheme::getSharedThemeRevision();
            },
            [=] { return inv->chaosTheme; },
            [=](int theme) { inv->chaosTheme = theme; });

        menu->addChild(new MenuSeparator);
        const char* routingNames[] = {"Parallel stereo", "Serial A into B", "Serial B into A", "Mid / Side"};
        menu->addChild(createSubmenuItem("Filter routing", routingNames[clamp(inv->routingMode, 0, 3)], [=](Menu* subMenu) {
            for (int mode = Involution::ROUTING_PARALLEL; mode <= Involution::ROUTING_MID_SIDE; ++mode) {
                subMenu->addChild(createCheckMenuItem(routingNames[mode], "",
                    [=] { return inv->routingMode == mode; },
                    [=] { inv->routingMode = mode; }));
            }
        }));
        menu->addChild(createCheckMenuItem(
            "Opposed Form (reverse B)", "",
            [=] { return inv->opposedForm; },
            [=] { inv->opposedForm = !inv->opposedForm; }));

        const char* destinationNames[] = {"Cutoff", "Cutoff + resonance", "Full filter field"};
        menu->addChild(createSubmenuItem("Chaos destination",
            destinationNames[clamp(inv->chaosDestination, 0, 2)], [=](Menu* subMenu) {
                for (int destination = Involution::CHAOS_CUTOFF; destination <= Involution::CHAOS_FULL_FIELD; ++destination) {
                    subMenu->addChild(createCheckMenuItem(destinationNames[destination], "",
                        [=] { return inv->chaosDestination == destination; },
                        [=] { inv->chaosDestination = destination; }));
                }
            }));

        const char* characterNames[] = {"Still", "Liquid", "Volatile"};
        menu->addChild(createSubmenuItem("Filter character",
            characterNames[clamp(inv->characterMode, 0, 2)], [=](Menu* subMenu) {
                for (int character = Involution::CHARACTER_STILL; character <= Involution::CHARACTER_VOLATILE; ++character) {
                    subMenu->addChild(createCheckMenuItem(characterNames[character], "",
                        [=] { return inv->characterMode == character; },
                        [=] { inv->characterMode = character; }));
                }
            }));
        menu->addChild(createCheckMenuItem(
            "Freeze chaos field", "",
            [=] { return inv->chaosFrozen; },
            [=] { inv->chaosFrozen = !inv->chaosFrozen; }));

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Cutoff CV Mode"));
        menu->addChild(createCheckMenuItem(
            "Filter A 1V/oct tracking",
            "",
            [=] { return inv->pitchTrackA; },
            [=] { inv->pitchTrackA = !inv->pitchTrackA; }));
        menu->addChild(createCheckMenuItem(
            "Filter B 1V/oct tracking",
            "",
            [=] { return inv->pitchTrackB; },
            [=] { inv->pitchTrackB = !inv->pitchTrackB; }));
    }
};

Model* modelInvolution = createModel<Involution, InvolutionWidget>("Involution");
