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

    static inline float fastTanh(float x) {
        float x2 = x * x;
        if (x2 > 9.0f) {
            return x > 0.0f ? 1.0f : -1.0f;
        }
        float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
        float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
        return a / b;
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

        // Rising edge at phase = 0 — post-edge correction
        if (phase < dt) {
            output -= polyBLEP(phase / dt);
        }
        // Rising edge at phase = 0 — pre-edge correction (phase wrapping from ~1 to ~0)
        if (phase > 1.f - dt) {
            output -= polyBLEP((phase - 1.f) / dt);
        }
        // Falling edge at phase = pulseWidth — post-edge correction
        if (phase > pulseWidth && phase < pulseWidth + dt) {
            output += polyBLEP((phase - pulseWidth) / dt);
        }
        // Falling edge at phase = pulseWidth — pre-edge correction
        if (phase > pulseWidth - dt && phase < pulseWidth) {
            output += polyBLEP((phase - pulseWidth) / dt);
        }

        return output;
    }
};

}} // namespace shapetaker::dsp
