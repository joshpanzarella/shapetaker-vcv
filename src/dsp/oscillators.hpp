#pragma once
#include <rack.hpp>
#include <cstdint>

using namespace rack;

namespace shapetaker {
namespace dsp {

// ============================================================================
// OSCILLATOR UTILITIES
// ============================================================================

class OscillatorHelper {
public:
    static float softenShapeEdges(float shape) {
        constexpr float knee = 0.02f; // 2% knee for gentle edge smoothing
        // Allow a small overshoot so CV hitting the rails doesn't hard-clip.
        if (shape < 0.f) {
            float t = (shape + knee) / knee;
            if (t <= 0.f) {
                return 0.f;
            }
            float eased = (-t * t * t + 2.f * t * t) * knee;
            return eased;
        }
        if (shape > 1.f) {
            float t = (1.f + knee - shape) / knee;
            if (t <= 0.f) {
                return 1.f;
            }
            float eased = (-t * t * t + 2.f * t * t) * knee;
            return 1.f - eased;
        }
        if (shape < knee) {
            float t = shape / knee;
            float eased = (-t * t * t + 2.f * t * t) * knee;
            return eased;
        }
        if (shape > 1.f - knee) {
            float t = (1.f - shape) / knee;
            float eased = (-t * t * t + 2.f * t * t) * knee;
            return 1.f - eased;
        }
        return shape;
    }

    // The bail-out sits at |x| > 5, not |x| > 3. The rational is still accurate
    // there (tanh(5) = 0.9999092), so returning a flat +/-1.0 at |x| = 3 used to
    // put a 0.00494 step — half a percent of full scale — into the middle of the
    // transfer curve. Any signal crossing it picked the step up as high-order
    // harmonic splatter: measured -57 dB relative to the saturated output at
    // drive 4.0, against -103 dB with the knee where it is now. Worst-case error
    // against true tanh falls from 0.004943 to 0.000096.
    //
    // For |x| <= 3 this is bit-identical to the previous version, so anything
    // that never reaches the old knee (Clairaudient's bus colour, for one) is
    // completely unaffected.
    //
    // The rational overshoots 1.0 very slightly just before the bail-out
    // (1.0000098 at x = 5), so the result is clamped rather than trusted.
    static inline float fastTanh(float x) {
        float x2 = x * x;
        if (x2 > 25.0f) {
            return x > 0.0f ? 1.0f : -1.0f;
        }
        float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
        float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
        float y = a / b;
        return y < -1.0f ? -1.0f : (y > 1.0f ? 1.0f : y);
    }

    static inline float fastSin2Pi(float phase) {
        int32_t phaseInt = static_cast<int32_t>(phase);
        float frac = phase - static_cast<float>(phaseInt);
        if (frac < 0.f) frac += 1.f;
        
        float x = frac * 2.f - 1.f; // map 0..1 to -1..1
        float absX = x < 0.f ? -x : x;
        
        // parabola
        float y = 4.f * x * (1.f - absX);
        float absY = y < 0.f ? -y : y;
        
        // smooth
        return y * (0.225f * (absY - 1.f) + 1.f);
    }

    // Accurate drop-in for fastSin2Pi: same phase convention, SAME SIGN, ~59 dB
    // more accurate, roughly 1.8x the cost.
    //
    //   fastSin2Pi:      peak error 1.1e-3  (-59 dB),  THD  -62 dBc
    //   accurateSin2Pi:  peak error 3.6e-6 (-109 dB),  THD -121 dBc
    //
    // SIGN: fastSin2Pi above returns -sin(2*pi*phase), not +sin — the parabola
    // approximates sin(pi*(2*phase-1)) and this copy never negates it. (The
    // near-identical fastSin2Pi in DistortionEngine, src/dsp/effects.hpp, DOES
    // negate, so the two are off by half a cycle.) This function reproduces the
    // OscillatorHelper sign so it can be swapped in without inverting anything.
    //
    // Use it where the sine is an *oscillator core* rather than a modulation
    // shape. It matters most in FM: a modulator's own distortion is multiplied
    // into every sideband family it generates, so fastSin2Pi's -62 dBc puts a
    // floor under the whole voice that no amount of oversampling can lift. For
    // LFOs, ring mod, chorus and wavefolding, fastSin2Pi remains the right call.
    //
    // Quarter-wave fold into [-pi/2, pi/2], then the 9th-order odd Taylor series
    // (error ~ z^11/11! = 3.7e-6 at the fold edge).
    static inline float accurateSin2Pi(float phase) {
        int32_t phaseInt = static_cast<int32_t>(phase);
        float frac = phase - static_cast<float>(phaseInt);
        if (frac < 0.f) frac += 1.f;

        float x = 4.f * frac;                   // [0, 4) in quarter-cycles
        if (x > 3.f)      x -= 4.f;             // [-1, 0)
        else if (x > 1.f) x = 2.f - x;          // [-1, 1]
        float z = x * 1.57079632679f;           // [-pi/2, pi/2]
        float z2 = z * z;
        float s = z * (1.f + z2 * (-1.66666667e-1f
                     + z2 * ( 8.33333333e-3f
                     + z2 * (-1.98412698e-4f
                     + z2 *   2.75573192e-6f))));
        return -s;                              // match fastSin2Pi's sign
    }

    // Sigmoid-morphed saw with a movable transition center. Shifting the
    // center away from 0.5 skews the wave's duty cycle (sigmoid-PWM), which
    // breaks odd-harmonic symmetry and introduces even harmonics far more
    // effectively than slope asymmetry once the sigmoid saturates. Modulating
    // range/center at audio rate produces phase-distortion-style formant
    // movement. blend: 0 = pure saw, 1 = full sigmoid. season scales the
    // subtle organic harmonic garnish.
    static float voicedSigmoidSaw(float phase, float blend, float range, float center,
                                  float lowShape, float airGain, float season) {
        float linearSaw = 2.f * phase - 1.f;
        float baseSaw = linearSaw * (1.f - 0.245f * linearSaw * linearSaw);

        float sigmoidInput = (phase - center) * range * 2.f;
        sigmoidInput += fastSin2Pi(phase * 3.f) * 0.03f * season;

        float sigmoidOutput = fastTanh(sigmoidInput);

        float result = linearSaw * (1.f - blend) + sigmoidOutput * blend;

        if (airGain > 0.f) {
            result += fastSin2Pi(phase * 7.f) * 0.008f * season * airGain;
        }

        float shaped = fastTanh(result * 1.05f) * 0.95f;

        return rack::math::crossfade(baseSaw, shaped, lowShape);
    }

    // ========================================================================
    // ANTI-ALIASING UTILITIES
    // ========================================================================

    // PolyBLEP (Polynomial Band-Limited Step) residual for one edge.
    // t = (phase - discontinuity) / dt, so t ∈ [0,1) just after the edge
    // and t ∈ (-1,0) just before it (wrap-around). Returns 0 outside [-1,1).
    static float polyBLEP(float t) {
        if (t >= 0.f && t < 1.f) {
            // Post-discontinuity: 2t - t² - 1
            return t + t - t * t - 1.f;
        }
        if (t > -1.f && t < 0.f) {
            // Pre-discontinuity (wrap-around): t² + 2t + 1
            return t + t + t * t + 1.f;
        }
        return 0.f;
    }

    // Generate PWM (Pulse Width Modulation) waveform with polyBLEP anti-aliasing.
    // Each discontinuity needs two corrections: one sample after the edge (t > 0)
    // and one sample before (t < 0, the wrap-around side). Four corrections total.
    // phase: oscillator phase [0, 1)
    // pulseWidth: duty cycle [0.05, 0.95]
    // dt: phase increment per sample (freq / sampleRate)
    static float pwmWithPolyBLEP(float phase, float pulseWidth, float dt) {
        float output = (phase < pulseWidth) ? 1.f : -1.f;

        // Rising edge at phase = 0: the residual is added, pulling the naive
        // +1 down toward the band-limited midpoint (polyBLEP(0+) = -1).
        if (phase < dt) {
            output += polyBLEP(phase / dt);
        }
        // Rising edge at phase = 0 — pre-edge correction (phase wrapping from ~1 to ~0)
        if (phase > 1.f - dt) {
            output += polyBLEP((phase - 1.f) / dt);
        }
        // Falling edge at phase = pulseWidth — post-edge correction (subtracted:
        // the naive value is -1 and must be pulled up toward the midpoint)
        if (phase > pulseWidth && phase < pulseWidth + dt) {
            output -= polyBLEP((phase - pulseWidth) / dt);
        }
        // Falling edge at phase = pulseWidth — pre-edge correction
        if (phase > pulseWidth - dt && phase < pulseWidth) {
            output -= polyBLEP((phase - pulseWidth) / dt);
        }

        return output;
    }
};

}} // namespace shapetaker::dsp
