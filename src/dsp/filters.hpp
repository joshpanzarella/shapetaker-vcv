#pragma once
#include <rack.hpp>
#include <cmath>

using namespace rack;

namespace shapetaker {
namespace dsp {

// ============================================================================
// FILTER UTILITIES
// ============================================================================

// Simple one-pole low-pass filter for lightweight smoothing
class OnePoleLowpass {
public:
    float z1 = 0.f;

    static float computeAlpha(float cutoff, float sampleRate) {
        if (cutoff <= 0.f || sampleRate <= 0.f || !std::isfinite(cutoff) || !std::isfinite(sampleRate)) {
            return 0.f;
        }
        float dt = 1.f / sampleRate;
        float RC = 1.f / (2.f * M_PI * cutoff);
        return dt / (RC + dt);
    }

    float processWithAlpha(float input, float alpha) {
        if (alpha <= 0.f || !std::isfinite(alpha)) {
            return z1;
        }
        z1 += alpha * (input - z1) + 1e-18f;
        return z1;
    }

    inline float processFast(float input, float alpha) {
        z1 += alpha * (input - z1) + 1e-18f;
        return z1;
    }

    float process(float input, float cutoff, float sampleRate) {
        return processWithAlpha(input, computeAlpha(cutoff, sampleRate));
    }

    void reset() {
        z1 = 0.f;
    }
};

// Generic biquad filter with multiple filter types
class BiquadFilter {
public:
    enum Type {
        LOWPASS,
        HIGHPASS,
        BANDPASS,
        NOTCH,
        ALLPASS
    };

protected:
    float x1 = 0.f, x2 = 0.f;
    float y1 = 0.f, y2 = 0.f;
    float a0 = 1.f, a1 = 0.f, a2 = 0.f;
    float b1 = 0.f, b2 = 0.f;
    
    // Cache for coefficient optimization
    float lastFreq = -1.f, lastQ = -1.f;
    Type lastType = LOWPASS;

public:
    void reset() {
        x1 = x2 = y1 = y2 = 0.f;
    }
    
    float process(float input) {
        float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
        
        // Stability check
        if (!std::isfinite(output) || std::abs(output) > 10000.f) {
            reset();
            return input; // Pass through when unstable
        }
        
        // Anti-denormal offset
        output += 1e-18f;
        
        x2 = x1; x1 = input;
        y2 = y1; y1 = output;
        
        return output;
    }
    
    void setParameters(Type type, float freq, float Q, float sampleRate) {
        // Only recalculate if parameters changed significantly
        const float epsilon = 1e-6f;
        if (type == lastType && 
            std::abs(freq - lastFreq) < epsilon && 
            std::abs(Q - lastQ) < epsilon) {
            return;
        }
        
        lastType = type;
        lastFreq = freq;
        lastQ = Q;
        
        freq = clamp(freq, 1.f, sampleRate * 0.49f);
        Q = clamp(Q, 0.1f, 30.f);
        
        float omega = 2.f * M_PI * freq / sampleRate;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float alpha = sin_omega / (2.f * Q);
        
        float norm = 1.f / (1.f + alpha);
        
        switch (type) {
            case LOWPASS:
                a0 = ((1.f - cos_omega) / 2.f) * norm;
                a1 = (1.f - cos_omega) * norm;
                a2 = a0;
                break;
                
            case HIGHPASS:
                a0 = ((1.f + cos_omega) / 2.f) * norm;
                a1 = -(1.f + cos_omega) * norm;
                a2 = a0;
                break;
                
            case BANDPASS:
                a0 = alpha * norm;
                a1 = 0.f;
                a2 = -alpha * norm;
                break;
                
            case NOTCH:
                a0 = norm;
                a1 = -2.f * cos_omega * norm;
                a2 = norm;
                break;
                
            case ALLPASS:
                a0 = (1.f - alpha) * norm;
                a1 = -2.f * cos_omega * norm;
                a2 = (1.f + alpha) * norm;
                break;
        }
        
        b1 = (-2.f * cos_omega) * norm;
        b2 = (1.f - alpha) * norm;
    }
};

// Enhanced morphing filter with coefficient-level blending
class MorphingFilter : public BiquadFilter {
private:
    // Cache previous parameters for optimization
    float lastFreq = -1.f, lastResonance = -1.f, lastMorph = -1.f;
    
public:
    /**
     * Advanced morphing between LP->BP->HP with coefficient blending
     * @param freq Filter frequency in Hz
     * @param resonance Filter Q (resonance)
     * @param morph 0.0 = lowpass, 0.5 = bandpass, 1.0 = highpass
     * @param sampleRate Current sample rate
     */
    void setMorphingFilter(float freq, float resonance, float morph, float sampleRate) {
        // Only recalculate if parameters have changed significantly
        const float epsilon = 1e-6f;
        if (std::abs(freq - lastFreq) < epsilon && 
            std::abs(resonance - lastResonance) < epsilon && 
            std::abs(morph - lastMorph) < epsilon) {
            return; // No change, skip recalculation
        }
        
        // Cache current values
        lastFreq = freq;
        lastResonance = resonance;
        lastMorph = morph;
        
        freq = clamp(freq, 1.f, sampleRate * 0.45f);
        resonance = clamp(resonance, 0.1f, 30.f);
        morph = clamp(morph, 0.f, 1.f);
        
        float omega = 2.f * M_PI * freq / sampleRate;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float alpha = sin_omega / (2.f * resonance);
        
        float norm = 1.f / (1.f + alpha);
        
        // Calculate coefficients for each filter type
        // Lowpass coefficients
        float lp_a0 = ((1.f - cos_omega) / 2.f) * norm;
        float lp_a1 = (1.f - cos_omega) * norm;
        float lp_a2 = lp_a0;
        
        // Bandpass coefficients
        float bp_a0 = alpha * norm;
        float bp_a1 = 0.f;
        float bp_a2 = -bp_a0;
        
        // Highpass coefficients
        float hp_a0 = ((1.f + cos_omega) / 2.f) * norm;
        float hp_a1 = -(1.f + cos_omega) * norm;
        float hp_a2 = hp_a0;
        
        // Morph between filter types using coefficient blending
        if (morph < 0.5f) {
            // Morph between lowpass and bandpass
            float blend = morph * 2.f;
            a0 = lp_a0 * (1.f - blend) + bp_a0 * blend;
            a1 = lp_a1 * (1.f - blend) + bp_a1 * blend;
            a2 = lp_a2 * (1.f - blend) + bp_a2 * blend;
        } else {
            // Morph between bandpass and highpass
            float blend = (morph - 0.5f) * 2.f;
            a0 = bp_a0 * (1.f - blend) + hp_a0 * blend;
            a1 = bp_a1 * (1.f - blend) + hp_a1 * blend;
            a2 = bp_a2 * (1.f - blend) + hp_a2 * blend;
        }
        
        b1 = (-2.f * cos_omega) * norm;
        b2 = (1.f - alpha) * norm;
    }
    
    /**
     * Set as stable highpass filter with fixed Q for feedback paths
     */
    void setStableHighpass(float freq, float sampleRate) {
        freq = clamp(freq, 1.f, sampleRate * 0.45f);
        
        float omega = 2.f * M_PI * freq / sampleRate;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        
        // Use low, stable Q to prevent resonance buildup
        float Q = 0.5f;
        float alpha = sin_omega / (2.f * Q);
        
        float norm = 1.f / (1.f + alpha);
        
        // Standard highpass coefficients
        a0 = ((1.f + cos_omega) / 2.f) * norm;
        a1 = -(1.f + cos_omega) * norm;
        a2 = a0;
        b1 = (-2.f * cos_omega) * norm;
        b2 = (1.f - alpha) * norm;
        
        // Reset cache
        lastFreq = lastResonance = lastMorph = -1.f;
    }
    
    /**
     * Set as allpass filter for phaser stages
     */
    void setAllpass(float freq, float sampleRate) {
        freq = clamp(freq, 1.f, sampleRate * 0.45f);
        
        float omega = 2.f * M_PI * freq / sampleRate;
        float tan_half_omega = std::tan(omega / 2.f);
        float norm = 1.f / (1.f + tan_half_omega);
        
        a0 = (1.f - tan_half_omega) * norm;
        a1 = -2.f * norm;
        a2 = (1.f + tan_half_omega) * norm;
        b1 = a1;
        b2 = a0;
        
        // Reset cache
        lastFreq = lastResonance = lastMorph = -1.f;
    }
    
    void reset() {
        BiquadFilter::reset();
        lastFreq = lastResonance = lastMorph = -1.f;
    }
};

}} // namespace shapetaker::dsp
