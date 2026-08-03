#pragma once

#include "../utilities.hpp"

namespace shapetaker {
namespace involution {

/**
 * DSP components specific to the Involution module
 */

/**
 * Resonant allpass feedback phaser: 6-stage APF chain with feedback loop.
 * The phaser center frequency is modulated by the same chaos LFO that sweeps
 * the filter cutoff, creating a unified liquid/organic spectral movement.
 */
class AllpassStage {
public:
    float x1 = 0.f;
    float y1 = 0.f;
    // y[n] = c * x[n] + x[n-1] - c * y[n-1]
    float process(float x, float c) {
        float y = c * (x - y1) + x1 + 1e-18f;
        x1 = x;
        y1 = y;
        return y;
    }
    void reset() { x1 = y1 = 0.f; }
};

class AllpassPhaser {
public:
    static constexpr float kFeedbackClamp = 12.f;
    static constexpr float kDryWetAverage = 0.5f;
    static constexpr int NUM_STAGES = 6;
    AllpassStage stages[NUM_STAGES];
    float feedbackMem = 0.f;

    // 1st-order APF coefficient for given center frequency
    static float apfCoeff(float centerHz, float sampleRate) {
        float x = (float)M_PI * centerHz / sampleRate;
        // Pade approximant for tan(x) accurate enough for a phaser:
        // tan(x) ≈ x * (15 - x^2) / (15 - 6x^2)
        // It provides a smooth, fast approximation avoiding std::tan.
        float x2 = x * x;
        float g = x * (15.f - x2) / (15.f - 6.f * x2);
        return (g - 1.f) / (g + 1.f);
    }

    // Returns 0.5*(dry + APF_out): unity gain at DC/Nyquist, notches at sweep freq.
    // feedback sharpens the notches; clamp protects against runaway at high resonance.
    float process(float input, float c, float fb) {
        float x = input + fb * rack::math::clamp(feedbackMem, -kFeedbackClamp, kFeedbackClamp);
        for (int i = 0; i < NUM_STAGES; i++) {
            x = stages[i].process(x, c);
        }
        feedbackMem = x;
        return kDryWetAverage * (input + x);
    }

    void reset() {
        for (int i = 0; i < NUM_STAGES; i++) stages[i].reset();
        feedbackMem = 0.f;
    }
};

/**
 * Single-sideband frequency shifter using a 4-pair IIR Hilbert approximation.
 * Shifts all frequencies by a constant Hz offset (not pitch-proportional).
 * Run channel A with +shiftHz and channel B with -shiftHz for a counter-rotating
 * stereo image: spectral content drifts in opposite directions, producing slow
 * beating at low amounts and dramatic inharmonic textures at higher settings.
 *
 * The Hilbert pair uses two 4-stage first-order allpass chains selected for
 * base-rate or high-rate engines. The high-rate table is the base table
 * frequency-scaled with the bilinear allpass pole mapping, preserving the
 * 44.1/48 kHz audio-band transition region around 88.2/96 kHz instead of
 * letting it shift upward with normalized frequency. The allpass transfer is:
 *   H(z) = (a - z^-1) / (1 - a·z^-1)
 * which gives unity gain at all frequencies, only shifting phase.
 */
class FrequencyShifter {
private:
    static constexpr int kHilbertStages = 4;
    static constexpr float kHighRateThreshold = 88200.f;

    struct Allpass1 {
        float x1 = 0.f, y1 = 0.f;
        float process(float x, float a) {
            float y = a * (x - y1) + x1 + 1e-18f;
            x1 = x; y1 = y;
            return y;
        }
        void reset() { x1 = y1 = 0.f; }
    };

    Allpass1 stagesI[kHilbertStages];
    Allpass1 stagesQ[kHilbertStages];
    float phase = 0.f;
    float sampleRate = 44100.f;
    const float* coeffI = nullptr;
    const float* coeffQ = nullptr;

    static const float* baseCoeffI() {
        static const float c[kHilbertStages] = {0.0721f, 0.3699f, 0.6899f, 0.9150f};
        return c;
    }

    static const float* baseCoeffQ() {
        static const float c[kHilbertStages] = {0.2040f, 0.5200f, 0.8192f, 0.9720f};
        return c;
    }

    static const float* highRateCoeffI() {
        // Bilinear frequency scaling of baseCoeffI by 0.5:
        // k=(1-a)/(1+a), k'=0.5*k, a'=(1-k')/(1+k').
        static const float c[kHilbertStages] = {0.395918f, 0.626042f, 0.831920f, 0.956577f};
        return c;
    }

    static const float* highRateCoeffQ() {
        // Bilinear frequency scaling of baseCoeffQ by 0.5 for 88.2/96 kHz engines.
        static const float c[kHilbertStages] = {0.503121f, 0.727273f, 0.905320f, 0.985901f};
        return c;
    }

public:
    FrequencyShifter() {
        setSampleRate(sampleRate);
    }

    void setSampleRate(float sr) {
        sr = std::max(sr, 1.f);
        const float* newI = (sr >= kHighRateThreshold) ? highRateCoeffI() : baseCoeffI();
        const float* newQ = (sr >= kHighRateThreshold) ? highRateCoeffQ() : baseCoeffQ();
        bool changed = std::abs(sr - sampleRate) > 0.01f || newI != coeffI || newQ != coeffQ;
        sampleRate = sr;
        coeffI = newI;
        coeffQ = newQ;
        if (changed) {
            reset();
        }
    }

    // shiftHz > 0 shifts spectrum up; shiftHz < 0 shifts down
    float process(float input, float shiftHz, float* dryAligned = nullptr) {
        float I = input;
        float Q = input;
        for (int i = 0; i < kHilbertStages; i++) {
            I = stagesI[i].process(I, coeffI[i]);
            Q = stagesQ[i].process(Q, coeffQ[i]);
        }

        if (dryAligned) {
            *dryAligned = I;
        }

        phase += shiftHz / sampleRate;
        phase -= std::floor(phase);

        // Upper sideband: I·cos(φ) - Q·sin(φ)
        float sinP = dsp::OscillatorHelper::fastSin2Pi(phase);
        float cosP = dsp::OscillatorHelper::fastSin2Pi(phase + 0.25f);
        return I * cosP - Q * sinP;
    }

    void reset() {
        for (int i = 0; i < kHilbertStages; i++) {
            stagesI[i].reset();
            stagesQ[i].reset();
        }
        phase = 0.f;
    }
};

/**
 * Lorenz strange attractor — chaotic modulation source with three decorrelated
 * axes (x, y, z) for per-voice diversity.
 *
 * Parameters: σ=10, ρ=28, β=8/3 (classical Lorenz butterfly).
 * Advances by `rate * sampleTime` time units per sample so that rate=1 Hz
 * produces roughly one chaotic orbit per second.
 */
class LorenzAttractor {
private:
    static constexpr float kSigma = 10.f;
    static constexpr float kRho   = 28.f;
    static constexpr float kBeta  = 8.f / 3.f;
    // Normalize to ≈ ±1 (empirical bounds for classical parameters)
    static constexpr float kNormX      = 1.f / 20.f;
    static constexpr float kNormY      = 1.f / 28.f;
    static constexpr float kNormZ      = 1.f / 27.f;
    static constexpr float kNormZBias  = 28.f;  // center of Z range [1..55]

    float x = 0.1f, y = 0.f, z = 0.f;

public:
    // Each axis returns ≈ [-1, 1]; values occasionally exceed range during
    // transient passages — callers should clamp if needed.
    float getX() const { return x * kNormX; }
    float getY() const { return y * kNormY; }
    float getZ() const { return (z - kNormZBias) * kNormZ; }

    void process(float rate, float sampleTime) {
        float dt = rate * sampleTime;
        float dx = kSigma * (y - x) * dt;
        float dy = (x * (kRho - z) - y) * dt;
        float dz = (x * y - kBeta * z) * dt;
        x += dx; y += dy; z += dz;
    }

    void reset() { x = 0.1f; y = 0.f; z = 0.f; }
};

} // namespace involution
} // namespace shapetaker
