#pragma once
#include "../plugin.hpp"
#include "rack.hpp"
#include <algorithm>

/**
 * LiquidFilter - 6th-order filter with liquid, resonant character
 *
 * Three cascaded 2-pole SVF stages (k=2.0) with global ladder-style feedback:
 * - k=2.0 is held constant — reducing it shifts the -180° phase crossing to a
 *   higher-gain frequency, cutting the max stable feedbackAmount below 2 and
 *   causing pumping oscillation.  Resonance comes entirely from global feedback.
 * - Feedback is 2nd-order HP'd at HP_CUTOFF_RATIO of the filter cutoff
 *   (-12dB/oct), so bass below the resonant region is strongly protected;
 *   clamped HP_CUTOFF_MIN_HZ–HP_CUTOFF_MAX_HZ.
 * - lastFeedback is tanh-limited to ±FEEDBACK_TANH_SWING — prevents integrator
 *   runaway and gives the feedback loop extra "spring" for an elastic, liquid
 *   character.
 * - The top of the resonance travel crosses into tanh-stabilized
 *   self-oscillation (EXT_RESONANCE_BOOST) instead of stopping at the legacy
 *   feedback ceiling.
 * - Dual-envelope cutoff breathing: a fast input follower (ENV_ATTACK_TC /
 *   ENV_RELEASE_TC) opens the cutoff on transients; a slow output follower
 *   (OUT_ENV_ATTACK_TC / OUT_ENV_RELEASE_TC) adds a secondary "bloom" as the
 *   resonant peak itself builds then decays.  The two envelopes create a
 *   multi-stage release — the resonance seeks, detunes slightly, and settles —
 *   which is the defining liquid quality of vintage analog ladder filters
 *   reacting to their own current draw.
 * - Tighter inter-stage saturation prevents amplitude buildup through the cascade
 *   and adds organic harmonic compression.
 * - 4x oversampling for alias suppression.
 */
class LiquidFilter {
public:
    // Parameter bounds — referenced by the host module's configParam/clamp calls
    static constexpr float RESONANCE_MIN = 0.707f;
    static constexpr float RESONANCE_MAX = 1.6f;

private:
    // -------------------------------------------------------------------------
    // DSP tuning constants
    // -------------------------------------------------------------------------
    static constexpr float EXT_RESONANCE_BOOST  = 0.65f;   // top-end resonance loop boost into self-oscillation
    static constexpr float FEEDBACK_SCALE       = 2.08f;   // feedbackAmount multiplier
    static constexpr float BREATH_CUTOFF_SCALE  = 0.28f;   // max cutoff shift from input breath (28%)
    static constexpr float BLOOM_CUTOFF_SCALE   = 0.095f;  // max cutoff shift from output bloom (9.5%)
    static constexpr float ENV_ATTACK_TC        = 0.003f;  // 3ms   — input transient attack
    static constexpr float ENV_RELEASE_TC       = 0.120f;  // 120ms — input breath release
    static constexpr float OUT_ENV_ATTACK_TC    = 0.010f;  // 10ms  — output bloom attack
    static constexpr float OUT_ENV_RELEASE_TC   = 0.250f;  // 250ms — output bloom release
    static constexpr float RES_ENV_ATTACK_TC    = 0.016f;  // 16ms  — resonant body attack
    static constexpr float RES_ENV_RELEASE_TC   = 0.210f;  // 210ms — resonant body release
    static constexpr float GRIP_ATTACK_TC       = 0.020f;  // 20ms  — cutoff sweep grip attack
    static constexpr float GRIP_RELEASE_TC      = 0.180f;  // 180ms — cutoff sweep grip release
    static constexpr float RIPPLE_LP_HZ         = 50.f;    // pre-filter rectifier ripple before envelope followers
    static constexpr float FEEDBACK_TANH_SWING  = 2.85f;   // ±V limit on tanh-clamped feedback
    static constexpr float FEEDBACK_PRESCALE    = 0.58f;   // pre-tanh scale on stage3 LP output
    static constexpr float FEEDBACK_DAMP_HZ     = 9500.f;  // analog-style HF loss in resonance feedback
    static constexpr float SIGNAL_HEADROOM      = 12.f;    // ±V headroom throughout the signal path
    static constexpr float OUT_KNEE             = 8.f;     // exact-linear output range before peak limiting
    static constexpr float INPUT_PEAK_NORM      = 10.f;    // normalise input envelope to 10V peak = 1.0
    static constexpr float SVF_K                = 2.f;     // critically-damped SVF damping coefficient
    static constexpr float HP_CUTOFF_RATIO      = 0.16f;   // feedback HP as fraction of filter cutoff
    static constexpr float HP_CUTOFF_MIN_HZ     = 30.f;    // lower bound for feedback HP
    static constexpr float HP_CUTOFF_MAX_HZ     = 140.f;   // upper bound for feedback HP
    static constexpr float SAT_DRIVE_PRE         = 0.30f;  // saturation drive growth post-injection
    static constexpr float SAT_DRIVE_INTER      = 0.42f;  // saturation drive growth between stages
    static constexpr float SAT_DRIVE_POST       = 0.18f;  // saturation drive growth post-cascade
    static constexpr float BREATH_RESONANCE_DAMP = 0.24f; // how much breath modulation scales back at max resonance
    static constexpr float CHARACTER_DRIVE_BASE  = 1.18f; // baseline circuit push at panel drive=1
    static constexpr float CHARACTER_DRIVE_RESONANCE = 0.95f; // extra push as resonance rises
    static constexpr float CHARACTER_BIAS        = 0.022f; // tiny asymmetric transistor-like offset
    static constexpr float LIQUID_PULL_SCALE     = 0.24f;  // resonant body pulls cutoff down as it blooms
    static constexpr float LIQUID_BLOOM_GAIN     = 0.34f;  // extra feedback emphasis from resonant body
    static constexpr float SWEEP_GRIP_PULL       = 0.13f;  // extra downward pull during descending cutoff sweeps
    static constexpr float SWEEP_GRIP_BLOOM      = 0.24f;  // feedback focus during descending cutoff sweeps
    static constexpr float INPUT_DRIVE_SENS      = 0.42f;  // hot input pushes the circuit drive
    static constexpr float INPUT_FEEDBACK_SENS   = 0.16f;  // hot input focuses feedback at resonance
    static constexpr float INPUT_BREATH_SENS     = 0.18f;  // hot input increases cutoff breath
    static constexpr float BASS_RECOVERY_GAIN    = 0.40f;  // restores body lost to high resonance
    static constexpr float BASS_RECOVERY_MAX_HZ  = 260.f;  // keeps recovery in true low end
    static constexpr float BASS_RECOVERY_MIN_HZ  = 28.f;
    static constexpr float STAGE_SAT_BIAS        = 0.03f;  // even harmonics in inter-stage saturation

    // Three 2-pole SVF stages
    struct SVF2Pole {
        float ic1eq = 0.f;
        float ic2eq = 0.f;
        float lastV1 = 0.f;  // Bandpass output
        float lastV2 = 0.f;  // Lowpass output (read for the feedback tap)
        float lastHigh = 0.f;

        float process(float input, float g, float k) {
            float v1 = (ic1eq + g * (input - ic2eq)) / (1.f + g * (g + k));
            float v2 = ic2eq + g * v1;

            ic1eq = 2.f * v1 - ic1eq + 1e-18f;
            ic2eq = 2.f * v2 - ic2eq + 1e-18f;

            lastV1 = v1;
            lastV2 = v2;
            lastHigh = input - k * v1 - v2;

            return v2;
        }

        void reset() {
            ic1eq = ic2eq = 0.f;
            lastV1 = lastV2 = lastHigh = 0.f;
        }
    };

    SVF2Pole stage1, stage2, stage3;

    // VCV Rack's built-in oversampling
    static const int OVERSAMPLE_FACTOR = 4;
    static const int OVERSAMPLE_QUALITY = 8;
    rack::dsp::Decimator<OVERSAMPLE_FACTOR, OVERSAMPLE_QUALITY> decimator;
    rack::dsp::Upsampler<OVERSAMPLE_FACTOR, OVERSAMPLE_QUALITY> upsampler;

    float baseSampleRate = 48000.f;
    float oversampledRate = 48000.f * OVERSAMPLE_FACTOR;

    // Global feedback state (ladder-style resonance)
    float lastFeedback = 0.f;

    // Input envelope follower: fast attack, medium release
    // Tracks the incoming signal level to open the cutoff on transients.
    float signalEnvelope  = 0.f;
    float envAttackCoeff  = 0.f;
    float envReleaseCoeff = 0.f;

    // Output envelope follower: slow attack, slow release ("bloom")
    // Tracks the filter output level — when the resonant peak builds up,
    // the cutoff shifts slightly, detuning the peak and creating the
    // liquid "seeking-and-settling" motion of vintage ladder filters.
    float outputEnvelope     = 0.f;
    float outEnvAttackCoeff  = 0.f;
    float outEnvReleaseCoeff = 0.f;
    float resonanceEnvelope  = 0.f;
    float resEnvAttackCoeff  = 0.f;
    float resEnvReleaseCoeff = 0.f;
    float sweepGripEnvelope  = 0.f;
    float gripAttackCoeff    = 0.f;
    float gripReleaseCoeff   = 0.f;
    float previousCutoff     = 0.f;
    float bassRecoveryLP     = 0.f;
    float bassRecoveryLP2    = 0.f;
    float envRippleLP        = 0.f;
    float outRippleLP        = 0.f;
    float resRippleLP        = 0.f;
    float rippleCoeff        = 0.f;
    float fbDampLP           = 0.f;
    float fbDampAlpha        = 0.f;

    // 2nd-order HP on the feedback path: two cascaded 1-pole LP states.
    // Subtracting only the HP'd feedback from the input preserves bass.
    // Two poles give -12dB/oct below the HP cutoff (vs -6dB/oct with one pole),
    // strongly protecting bass even at very high resonance settings.
    float hpFeedbackLP1 = 0.f;
    float hpFeedbackLP2 = 0.f;

    struct DriveParams {
        float inputMult;
        float biasOffset;
        float postBiasOffset;
        float makeupWet;
        float dry;
    };

    void computeDriveParams(float drive, DriveParams& p) {
        drive = rack::math::clamp(drive, 1.f, 9.f);
        float driveMix = rack::math::clamp((drive - 1.f) / 8.f, 0.f, 1.f);
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

        p.inputMult = lerp(1.f, drive * 0.9f + 0.2f, driveMix);
        p.biasOffset = CHARACTER_BIAS * SIGNAL_HEADROOM * driveMix;
        p.postBiasOffset = CHARACTER_BIAS * SIGNAL_HEADROOM * driveMix * 0.65f;
        float makeup = lerp(1.f, 1.0f / (drive * 0.5f + 0.5f), driveMix);
        float wet = lerp(0.35f, 0.95f, driveMix);
        p.makeupWet = wet * makeup;
        p.dry = 1.f - wet;
    }

    float driveSaturateFast(float input, const DriveParams& p) {
        float driven = input * p.inputMult + p.biasOffset;
        float normalized = driven / SIGNAL_HEADROOM;
        normalized = rack::math::clamp(normalized, -2.8f, 2.8f);
        float abs_x = std::abs(normalized);
        if (abs_x > 1.5f) {
            normalized = (normalized > 0.f) ? 1.f : -1.f;
        } else if (abs_x >= 0.5f) {
            float sign = normalized > 0.f ? 1.f : -1.f;
            normalized = sign * (-0.5f * abs_x * abs_x + 1.5f * abs_x - 0.125f);
        }
        float shaped = normalized * SIGNAL_HEADROOM - p.postBiasOffset;
        float result = p.dry * input + p.makeupWet * shaped;
        return rack::math::clamp(result, -SIGNAL_HEADROOM, SIGNAL_HEADROOM);
    }

    float filterSaturateFast(float input, float driveMult) {
        return shapetaker::dsp::OscillatorHelper::fastTanh(input * driveMult) / driveMult;
    }

    float saturateAsym(float input, float driveMult, float bias) {
        float b = bias;
        return (shapetaker::dsp::OscillatorHelper::fastTanh(input * driveMult + b)
                - shapetaker::dsp::OscillatorHelper::fastTanh(b)) / driveMult;
    }

    static inline float softLimitOut(float x) {
        float ax = std::abs(x);
        if (ax <= OUT_KNEE) {
            return x;
        }
        float range = SIGNAL_HEADROOM - OUT_KNEE;
        float y = OUT_KNEE + range * shapetaker::dsp::OscillatorHelper::fastTanh((ax - OUT_KNEE) / range);
        return std::copysign(y, x);
    }

public:
    LiquidFilter() : decimator(0.9f), upsampler(0.9f) {
        oversampledRate    = baseSampleRate * OVERSAMPLE_FACTOR;
        envAttackCoeff     = std::exp(-1.f / (baseSampleRate * ENV_ATTACK_TC));
        envReleaseCoeff    = std::exp(-1.f / (baseSampleRate * ENV_RELEASE_TC));
        outEnvAttackCoeff  = std::exp(-1.f / (baseSampleRate * OUT_ENV_ATTACK_TC));
        outEnvReleaseCoeff = std::exp(-1.f / (baseSampleRate * OUT_ENV_RELEASE_TC));
        resEnvAttackCoeff  = std::exp(-1.f / (baseSampleRate * RES_ENV_ATTACK_TC));
        resEnvReleaseCoeff = std::exp(-1.f / (baseSampleRate * RES_ENV_RELEASE_TC));
        gripAttackCoeff    = std::exp(-1.f / (baseSampleRate * GRIP_ATTACK_TC));
        gripReleaseCoeff   = std::exp(-1.f / (baseSampleRate * GRIP_RELEASE_TC));
        rippleCoeff        = 1.f - std::exp(-2.f * static_cast<float>(M_PI) * RIPPLE_LP_HZ / baseSampleRate);
        fbDampAlpha        = 1.f - std::exp(-2.f * static_cast<float>(M_PI) * FEEDBACK_DAMP_HZ / oversampledRate);
        reset();
    }

    void setSampleRate(float sr) {
        bool rateChanged = std::abs(sr - baseSampleRate) > 0.01f;
        baseSampleRate     = sr;
        oversampledRate    = sr * OVERSAMPLE_FACTOR;
        envAttackCoeff     = std::exp(-1.f / (sr * ENV_ATTACK_TC));
        envReleaseCoeff    = std::exp(-1.f / (sr * ENV_RELEASE_TC));
        outEnvAttackCoeff  = std::exp(-1.f / (sr * OUT_ENV_ATTACK_TC));
        outEnvReleaseCoeff = std::exp(-1.f / (sr * OUT_ENV_RELEASE_TC));
        resEnvAttackCoeff  = std::exp(-1.f / (sr * RES_ENV_ATTACK_TC));
        resEnvReleaseCoeff = std::exp(-1.f / (sr * RES_ENV_RELEASE_TC));
        gripAttackCoeff    = std::exp(-1.f / (sr * GRIP_ATTACK_TC));
        gripReleaseCoeff   = std::exp(-1.f / (sr * GRIP_RELEASE_TC));
        rippleCoeff        = 1.f - std::exp(-2.f * static_cast<float>(M_PI) * RIPPLE_LP_HZ / sr);
        fbDampAlpha        = 1.f - std::exp(-2.f * static_cast<float>(M_PI) * FEEDBACK_DAMP_HZ / oversampledRate);
        if (rateChanged) {
            reset();
        }
    }

    void reset() {
        stage1.reset();
        stage2.reset();
        stage3.reset();
        decimator.reset();
        upsampler.reset();
        lastFeedback = 0.f;
        hpFeedbackLP1 = 0.f;
        hpFeedbackLP2 = 0.f;
        signalEnvelope = 0.f;
        outputEnvelope = 0.f;
        resonanceEnvelope = 0.f;
        sweepGripEnvelope = 0.f;
        previousCutoff = 0.f;
        bassRecoveryLP = 0.f;
        bassRecoveryLP2 = 0.f;
        envRippleLP = 0.f;
        outRippleLP = 0.f;
        resRippleLP = 0.f;
        fbDampLP = 0.f;
    }

    float process(float input, float cutoff, float resonance, float drive = 1.f,
                  float form = 0.f, float character = 1.f) {
        // Safety checks
        if (!std::isfinite(input)) return 0.f;
        if (oversampledRate <= 0.f) return input;

        // Clamp parameters to safe ranges
        cutoff    = rack::math::clamp(cutoff,    1.f, oversampledRate * 0.45f);
        resonance = rack::math::clamp(resonance, 0.1f, 10.f);
        drive     = rack::math::clamp(drive,     0.1f, 10.f);
        form      = rack::math::clamp(form, 0.f, 1.f);
        character = rack::math::clamp(character, 0.f, 1.75f);

        // Map resonance to normalized 0..1
        const float resonanceRange = std::max(RESONANCE_MAX - RESONANCE_MIN, 0.001f);
        float resonanceClamped    = rack::math::clamp(resonance, RESONANCE_MIN, RESONANCE_MAX);
        // Branch experiment: let the top of the control travel cross into
        // tanh-stabilized self-oscillation instead of stopping at the legacy cap.
        float resonanceNormalized = rack::math::clamp(
            (resonanceClamped - RESONANCE_MIN) / resonanceRange, 0.f, 1.f);

        // Global feedback amount.
        // The x^0.75 shaping (vs linear) pulls resonance/growl earlier into
        // the knob range, so the filter bites before the control reaches the top.
        float sqrtRes = std::sqrt(resonanceNormalized);
        float res75   = sqrtRes * std::sqrt(sqrtRes);  // x^0.75 via two sqrt calls
        float feedbackAmount = res75 * FEEDBACK_SCALE;
        float osc = rack::math::clamp((resonanceNormalized - 0.8f) * 5.f, 0.f, 1.f);
        osc = osc * osc * (3.f - 2.f * osc);
        feedbackAmount *= 1.f + osc * EXT_RESONANCE_BOOST;
        float liquidAmount   = res75;
        float inputPush = signalEnvelope * signalEnvelope;

        // ====================================================================
        // DUAL-ENVELOPE CUTOFF BREATHING
        // ====================================================================
        // Input follower: fast attack / medium release.
        // Opens the cutoff on incoming transients, then exhales over ~120ms.
        {
            envRippleLP += rippleCoeff * (std::abs(input) * (1.f / INPUT_PEAK_NORM) - envRippleLP);
            float envIn = envRippleLP;
            float envCoeff = (envIn > signalEnvelope) ? envAttackCoeff : envReleaseCoeff;
            signalEnvelope += (1.f - envCoeff) * (envIn - signalEnvelope);
            signalEnvelope  = rack::math::clamp(signalEnvelope, 0.f, 1.f);
            inputPush = signalEnvelope * signalEnvelope;
        }
        float circuitDrive = rack::math::clamp(
            drive * (CHARACTER_DRIVE_BASE + resonanceNormalized * CHARACTER_DRIVE_RESONANCE)
                * (1.f + inputPush * INPUT_DRIVE_SENS * character
                    * (0.45f + 0.55f * liquidAmount)),
            1.f,
            9.f);
        // outputEnvelope holds the previous cycle's tracked output level (0-1).
        // It is updated after decimation (below) so this cycle uses last cycle's
        // value — a 1-sample delay that avoids an algebraic loop.  The bloom
        // effect is too slow (OUT_ENV_RELEASE_TC) to be sensitive to 1-sample jitter.

        // Input breath: up to BREATH_CUTOFF_SCALE cutoff shift at full signal (≈3.2 semitones).
        // Scale back with resonance: PWM and sync produce dense transients that trigger the
        // breath follower continuously, causing rapid cutoff modulation that interacts badly
        // with the near-oscillating loop.  At max resonance the shift is ~60% of its base value.
        // Output bloom: up to BLOOM_CUTOFF_SCALE additional shift as resonance builds (≈1 semitone)
        if (previousCutoff <= 0.f) {
            previousCutoff = cutoff;
        }
        float fallingOctavesPerSecond = 0.f;
        if (cutoff < previousCutoff) {
            float ratio = previousCutoff / std::max(cutoff, 1.f);
            fallingOctavesPerSecond = (ratio - 1.f) * 1.44269504089f * baseSampleRate;
        }
        previousCutoff = cutoff;
        float gripTarget = rack::math::clamp(fallingOctavesPerSecond * 0.055f, 0.f, 1.f) * liquidAmount;
        float gripCoeff = (gripTarget > sweepGripEnvelope) ? gripAttackCoeff : gripReleaseCoeff;
        sweepGripEnvelope += (1.f - gripCoeff) * (gripTarget - sweepGripEnvelope);
        sweepGripEnvelope = rack::math::clamp(sweepGripEnvelope, 0.f, 1.f);

        float breathScale = BREATH_CUTOFF_SCALE
            * (1.f - resonanceNormalized * BREATH_RESONANCE_DAMP)
            * (1.f + inputPush * INPUT_BREATH_SENS)
            * character;
        float liquidPull = (resonanceEnvelope * liquidAmount * LIQUID_PULL_SCALE
            + sweepGripEnvelope * SWEEP_GRIP_PULL) * character;
        float breathCutoff = cutoff * (1.f + signalEnvelope * breathScale
                                           + outputEnvelope * BLOOM_CUTOFF_SCALE * character);
        breathCutoff *= (1.f - liquidPull);
        breathCutoff = rack::math::clamp(breathCutoff, 1.f, oversampledRate * 0.45f);

        // Filter coefficients at oversampled rate.
        // breathCutoff carries the envelope-modulated cutoff for elasticity.
        // These coefficients are constant across the oversampled block, so compute them once outside the loop.
        float _gx = M_PI * breathCutoff / oversampledRate;
        float _gx2 = _gx * _gx;
        float g = _gx * (15.f - _gx2) / (15.f - 6.f * _gx2);
        g = rack::math::clamp(g, 0.f, 0.99f);

        // 2nd-order HP on feedback at HP_CUTOFF_RATIO of filter cutoff (-12dB/oct).
        // e.g. cutoff=400Hz → HP at 80Hz; 40Hz is then -24dB down in the feedback
        // signal.  Clamped HP_CUTOFF_MIN_HZ–HP_CUTOFF_MAX_HZ to stay well below
        // the musical midrange and protect more of the bass spectrum.
        float HP_CUTOFF_HZ = rack::math::clamp(
            breathCutoff * HP_CUTOFF_RATIO, HP_CUTOFF_MIN_HZ, HP_CUTOFF_MAX_HZ);
        float hpAlpha = rack::math::clamp(
            (2.f * static_cast<float>(M_PI) * HP_CUTOFF_HZ) / oversampledRate,
            0.f, 0.99f);

        // Upsample
        float upsampledBuffer[OVERSAMPLE_FACTOR];
        upsampler.process(input, upsampledBuffer);

        float oversampledOutputs[OVERSAMPLE_FACTOR];

        // Precompute values constant across oversampling loop
        DriveParams dParams;
        computeDriveParams(circuitDrive, dParams);
        
        float drivePreMult = rack::math::clamp(1.0f + feedbackAmount * SAT_DRIVE_PRE, 0.1f, SIGNAL_HEADROOM) / SIGNAL_HEADROOM;
        float driveInterMult = rack::math::clamp(1.0f + feedbackAmount * SAT_DRIVE_INTER, 0.1f, SIGNAL_HEADROOM) / SIGNAL_HEADROOM;
        float drivePostMult = rack::math::clamp(1.f + feedbackAmount * SAT_DRIVE_POST, 0.1f, SIGNAL_HEADROOM) / SIGNAL_HEADROOM;
        
        float feedbackFocus = 1.f
            + (resonanceEnvelope * liquidAmount * LIQUID_BLOOM_GAIN
            + sweepGripEnvelope * SWEEP_GRIP_BLOOM
            + inputPush * liquidAmount * INPUT_FEEDBACK_SENS) * character;
        float feedbackFocusMult = FEEDBACK_PRESCALE * feedbackFocus;

        for (int i = 0; i < OVERSAMPLE_FACTOR; i++) {
            float x = upsampledBuffer[i];

            // Pre-filter drive saturation
            x = driveSaturateFast(x, dParams);

            // ================================================================
            // GLOBAL FEEDBACK TOPOLOGY (ladder-style)
            // ================================================================
            {
                // Cascade two 1-pole LPs to form a 2nd-order HP
                hpFeedbackLP1 += hpAlpha * (lastFeedback - hpFeedbackLP1) + 1e-18f;
                float hp1 = lastFeedback - hpFeedbackLP1;
                hpFeedbackLP2 += hpAlpha * (hp1 - hpFeedbackLP2) + 1e-18f;
                float feedbackHPed = hp1 - hpFeedbackLP2;
                x = x - feedbackHPed * feedbackAmount;
            }

            // Post-injection saturation
            x = filterSaturateFast(x, drivePreMult);

            // Cascade three critically-damped 2-pole stages (k=SVF_K).
            x = stage1.process(x, g, SVF_K);
            float band = stage1.lastV1 * 1.35f;
            float high = stage1.lastHigh;

            // Inter-stage saturation: slight bias grows even harmonics with resonance.
            float stageBias = STAGE_SAT_BIAS * resonanceNormalized;
            x = saturateAsym(x, driveInterMult, stageBias);

            x = stage2.process(x, g, SVF_K);
            x = saturateAsym(x, driveInterMult, stageBias);

            x = stage3.process(x, g, SVF_K);
            float low = x;

            // Store damped LP integrator state for feedback.
            fbDampLP += fbDampAlpha * (stage3.lastV2 - fbDampLP) + 1e-18f;
            lastFeedback = shapetaker::dsp::OscillatorHelper::fastTanh(fbDampLP * feedbackFocusMult) * FEEDBACK_TANH_SWING;

            // FORM continuously moves through the actual filter responses:
            // deep 6-pole lowpass -> resonant bandpass -> 2-pole highpass.
            // The cascade still runs in every mode so its global resonance and
            // liquid feedback remain part of the same instrument.
            if (form < 0.5f) {
                float t = form * 2.f;
                t = t * t * (3.f - 2.f * t);
                x = low + (band - low) * t;
            } else {
                float t = (form - 0.5f) * 2.f;
                t = t * t * (3.f - 2.f * t);
                x = band + (high - band) * t;
            }

            // Post-cascade saturation
            x = filterSaturateFast(x, drivePostMult);

            oversampledOutputs[i] = x;
        }

        // Downsample back to base rate
        float output = decimator.process(oversampledOutputs);

        // High resonance naturally pulls energy out of the passband low end.
        // Recover only a low-passed slice of the input, scaled by resonance and
        // signal level, so the filter keeps body without turning into a dry blend.
        {
            float recoveryHz = rack::math::clamp(
                std::min(breathCutoff * 0.55f, BASS_RECOVERY_MAX_HZ),
                BASS_RECOVERY_MIN_HZ,
                baseSampleRate * 0.45f);
            float recoveryAlpha = rack::math::clamp(
                (2.f * static_cast<float>(M_PI) * recoveryHz) / baseSampleRate,
                0.f,
                0.99f);
            bassRecoveryLP += recoveryAlpha * (input - bassRecoveryLP) + 1e-18f;
            bassRecoveryLP2 += recoveryAlpha * (bassRecoveryLP - bassRecoveryLP2) + 1e-18f;
            float recoveryDrive = 0.35f + 0.65f * inputPush;
            float recoveryFormTrim = 1.f - form * form * (3.f - 2.f * form);
            float recoveryAmount = BASS_RECOVERY_GAIN * liquidAmount * recoveryDrive
                * recoveryFormTrim * character;
            output += bassRecoveryLP2 * recoveryAmount;
        }

        // Update output bloom envelope for next cycle.
        // Tracks filter output level (normalized to 0-1 at ±SIGNAL_HEADROOM peak).
        // Slow attack ignores transients; slow release holds the bloom long enough
        // to create the liquid "seeking-and-settling" motion.
        {
            outRippleLP += rippleCoeff * (std::abs(output) * (1.f / SIGNAL_HEADROOM) - outRippleLP);
            float envOut = outRippleLP;
            float outCoeff = (envOut > outputEnvelope) ? outEnvAttackCoeff : outEnvReleaseCoeff;
            outputEnvelope += (1.f - outCoeff) * (envOut - outputEnvelope);
            outputEnvelope  = rack::math::clamp(outputEnvelope, 0.f, 1.f);

            resRippleLP += rippleCoeff * (std::abs(lastFeedback) * (1.f / FEEDBACK_TANH_SWING) - resRippleLP);
            float envRes = rack::math::clamp(resRippleLP, 0.f, 1.f);
            float resCoeff = (envRes > resonanceEnvelope) ? resEnvAttackCoeff : resEnvReleaseCoeff;
            resonanceEnvelope += (1.f - resCoeff) * (envRes - resonanceEnvelope);
            resonanceEnvelope = rack::math::clamp(resonanceEnvelope, 0.f, 1.f);
        }

        // Soft output limiter: exact-linear below OUT_KNEE, tanh-shaped only for
        // resonant peaks above normal Rack levels. This preserves clean passband
        // gain while retaining the same smooth ceiling at ±SIGNAL_HEADROOM.
        output = softLimitOut(output);

        if (!std::isfinite(output)) {
            reset();
            return 0.f;
        }

        return output;
    }
};
