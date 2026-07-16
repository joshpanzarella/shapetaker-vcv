#pragma once
#include <rack.hpp>
#include <algorithm>
#include <cmath>

using namespace rack;

namespace shapetaker {
namespace dsp {

// ============================================================================
// SIDECHAIN DETECTION
// ============================================================================

/**
 * Sidechain Detector - Advanced Envelope Follower
 * 
 * This class implements a sophisticated envelope follower that tracks
 * the amplitude of the sidechain signal with separate attack and release times.
 */
class SidechainDetector {
private:
    float envelope = 0.0f;
    float attack_coeff = 0.0f;
    float release_coeff = 0.0f;
    
    // Internal sample rate tracking
    float sample_rate = 44100.0f;
    
public:
    /**
     * Set the timing parameters for the envelope follower
     * @param attack_ms Attack time in milliseconds (0.1 - 100)
     * @param release_ms Release time in milliseconds (1 - 1000) 
     * @param sr Sample rate in Hz
     */
    void setTiming(float attack_ms, float release_ms, float sr = 44100.0f) {
        sample_rate = sr;
        
        // Convert milliseconds to coefficient values
        // Using exponential decay formula: coeff = exp(-1 / (time_constant * sample_rate))
        attack_coeff = expf(-1.0f / (attack_ms * 0.001f * sample_rate));
        release_coeff = expf(-1.0f / (release_ms * 0.001f * sample_rate));
        
        // Clamp coefficients to prevent instability
        attack_coeff = rack::math::clamp(attack_coeff, 0.0f, 0.999f);
        release_coeff = rack::math::clamp(release_coeff, 0.0f, 0.999f);
    }
    
    /**
     * Process a single sample through the envelope follower
     * @param input Input signal sample (should be pre-scaled to 0.0-1.0 range)
     * @return Current envelope value (0.0 - 1.0)
     */
    float process(float input) {
        // Caller (Chiaroscuro) already delivers input in [0, 1]; skip redundant abs/clamp.
        float target = input;

        if (target > envelope) {
            // Attack phase - rising envelope
            envelope = target + (envelope - target) * attack_coeff;
        } else {
            // Release phase - falling envelope
            envelope = target + (envelope - target) * release_coeff;
        }
        
        // Ensure envelope decays to true zero when input is zero
        if (target < 0.0001f && envelope < 0.001f) {
            envelope = 0.0f;
        }
        
        // Clamp output to valid range
        envelope = rack::math::clamp(envelope, 0.0f, 1.0f);
        
        return envelope;
    }
    
    /**
     * Get the current envelope value without processing new input
     * @return Current envelope level
     */
    float getEnvelope() const { 
        return envelope; 
    }
    
    /**
     * Reset the envelope to zero (useful for initialization)
     */
    void reset() {
        envelope = 0.0f;
    }
    
    /**
     * Get the current sample rate
     */
    float getSampleRate() const {
        return sample_rate;
    }
};

// ============================================================================
// DISTORTION EFFECTS
// ============================================================================

/**
 * Distortion Engine - Collection of Intense Distortion Algorithms
 * 
 * This class provides 6 different distortion types ranging from
 * aggressive clipping to complex wave manipulation.
 */
class DistortionEngine {
private:
    float phase = 0.0f;          // For oscillator-based effects
    float sample_rate = 44100.0f; // Current sample rate
    float prev_input = 0.0f;     // For feedback effects

    // Sample-rate reduction state for bit crush
    int crushCounter = 0;        // Downsample hold counter
    int crushHold = 1;           // Samples to hold
    float crushSample = 0.0f;    // Last held sample
    int rateScale = 1;           // Host-rate samples per engine-rate sample group

    // DC blocking filter state (high-pass at ~10 Hz)
    float dcBlocker_x1 = 0.0f;
    float dcBlocker_y1 = 0.0f;
    float dcOut_x1 = 0.0f;
    float dcOut_y1 = 0.0f;

    // Pre-emphasis/de-emphasis filter state
    float preEmph_lp = 0.0f;
    float deEmph_lp = 0.0f;

    // Precomputed DC blocker coefficient (sample-rate dependent)
    float dcBlockR = 0.99857f; // Default for 44.1kHz, updated in setSampleRate()

    // Precomputed pre/de-emphasis filter alpha (2.5 kHz shelf, sample-rate dependent)
    float preEmphAlpha = 0.f; // Updated in setSampleRate()

    // Separate feedback state for destroy() so it doesn't share with waveFold()
    float destroyFb = 0.f;

    // Dither noise generator state for bit crush
    unsigned int ditherSeed = 1;

public:
    enum Type {
        HARD_CLIP = 0,  // Aggressive limiting with harsh harmonics
        TUBE_SAT = 1,   // Asymmetric tube-style saturation
        WAVE_FOLD = 2,  // Multi-stage wave folding
        BIT_CRUSH = 3,  // Bit depth + sample rate reduction
        DESTROY = 4,    // Hybrid destruction algorithm
        RING_MOD = 5    // Ring modulation with internal oscillator
    };
    
    /**
     * Set the sample rate for the distortion engine
     * @param sr Sample rate in Hz
     */
    void setSampleRate(float sr) {
        sample_rate = sr;
        crushCounter = 0;
        const float dcCutoffHz = 10.0f;
        dcBlockR = rack::math::clamp(1.0f - (2.0f * (float)M_PI * dcCutoffHz / sample_rate), 0.9f, 0.9999f);
        // Pre/de-emphasis shelf at 2.5 kHz — same formula, cached once per SR change
        preEmphAlpha = 1.0f - expf(-2.0f * (float)M_PI * 2500.f / sample_rate);
    }

    void setRateScale(int scale) {
        rateScale = std::max(1, scale);
    }

    void seedDither(unsigned int seed) {
        ditherSeed = seed ? seed : 1u;
    }
    
    /**
     * Reset internal state (useful for feedback-based algorithms)
     */
    void reset() {
        phase = 0.0f;
        prev_input = 0.0f;
        destroyFb = 0.0f;
        crushCounter = 0;
        crushHold = 1;
        crushSample = 0.0f;
        dcBlocker_x1 = 0.0f;
        dcBlocker_y1 = 0.0f;
        dcOut_x1 = 0.0f;
        dcOut_y1 = 0.0f;
        preEmph_lp = 0.0f;
        deEmph_lp = 0.0f;
    }
    
    /**
     * Process audio through the selected distortion algorithm
     * @param input Input audio sample (-10V to +10V typical)
     * @param drive Distortion amount (0.0 - 1.0)
     * @param type Distortion algorithm to use
     * @return Processed audio sample
     */
    float process(float input, float drive, Type type) {
        drive = rack::math::clamp(drive, 0.0f, 1.0f);

        // If drive is negligible, return a truly transparent signal. Still update
        // internal state so raising drive from zero does not start from stale filters.
        if (drive < 0.0001f) {
            prev_input *= 0.99f; // Slowly decay feedback state
            dcBlock(input);
            dcBlockOut(input);
            preEmphasis(input, 0.0f);
            deEmphasis(input, 0.0f);
            return input;
        }

        // DC blocking on input (high-pass ~10 Hz)
        float clean = dcBlock(input);

        // Pre-emphasis: boost highs before distortion (gives more "analog" character)
        float emphasized = preEmphasis(clean, drive);

        // Apply distortion algorithm
        float distorted;
        switch(type) {
            case HARD_CLIP:
                distorted = hardClip(emphasized, drive);
                break;
            case WAVE_FOLD:
                distorted = waveFold(emphasized, drive);
                break;
            case BIT_CRUSH:
                distorted = bitCrush(emphasized, drive);
                break;
            case DESTROY:
                distorted = destroy(emphasized, drive);
                break;
            case RING_MOD:
                distorted = ringMod(emphasized, drive);
                break;
            case TUBE_SAT:
                distorted = tubeSat(emphasized, drive);
                break;
            default:
                distorted = emphasized;
                break;
        }

        float centered = dcBlockOut(distorted);

        // De-emphasis: cut highs after distortion to compensate for pre-emphasis
        float deemphasized = deEmphasis(centered, drive);

        return deemphasized;
    }
    
    /**
     * Get the name of a distortion type
     * @param type Distortion type enum
     * @return Human-readable name
     */
    static const char* getTypeName(Type type) {
        switch(type) {
            case HARD_CLIP: return "Hard Clip";
            case TUBE_SAT: return "Tube Sat";
            case WAVE_FOLD: return "Wave Fold";
            case BIT_CRUSH: return "Bit Crush";
            case DESTROY: return "Destroy";
            case RING_MOD: return "Ring Mod";
            default: return "Unknown";
        }
    }
    
private:
    /**
     * DC blocking filter (1st order high-pass at ~10 Hz)
     * Removes DC offset that can accumulate from asymmetric distortion.
     * Coefficient dcBlockR is precomputed in setSampleRate() for correct
     * behavior at any sample rate (44.1k, 48k, 96k, 192k, etc.).
     */
    float dcBlock(float input) {
        float output = input - dcBlocker_x1 + dcBlockR * dcBlocker_y1;
        dcBlocker_x1 = input;
        dcBlocker_y1 = output;
        return output;
    }

    float dcBlockOut(float input) {
        float output = input - dcOut_x1 + dcBlockR * dcOut_y1;
        dcOut_x1 = input;
        dcOut_y1 = output;
        return output;
    }

    // Fast sine approximation for a phase in [0, 1) (normalized to one cycle).
    // Good to ~0.1% error — sufficient for ring mod and chaos terms.
    static inline float fastSin2Pi(float phase) {
        // Bring to [0, 1)
        phase -= std::floor(phase);
        float x = phase * 2.0f - 1.0f;          // map to [-1, 1]
        float absX = x < 0.f ? -x : x;
        float y = 4.0f * x * (1.0f - absX);     // parabola
        float absY = y < 0.f ? -y : y;
        return y * (0.225f * (absY - 1.0f) + 1.0f); // smooth
    }

    // Pre-emphasis: boost highs before distortion (2.5 kHz shelf, up to +6 dB).
    // alpha is precomputed in setSampleRate() — no expf() per sample.
    float preEmphasis(float input, float drive) {
        float boost = 1.0f + drive;
        preEmph_lp += preEmphAlpha * (input - preEmph_lp);
        float highpass = input - preEmph_lp;
        return input + highpass * (boost - 1.0f);
    }

    // De-emphasis: mirror cut after distortion to restore spectral balance.
    float deEmphasis(float input, float drive) {
        float boost = 1.0f + drive;
        deEmph_lp += preEmphAlpha * (input - deEmph_lp);
        float highpass = input - deEmph_lp;
        return input - highpass * (1.0f - 1.0f / boost);
    }

    /**
     * Simple pseudo-random number generator for dithering
     */
    float dither() {
        // TPDF (Triangular Probability Density Function) dither
        ditherSeed = (ditherSeed * 1664525u + 1013904223u);
        float r1 = (float)(ditherSeed & 0x7FFFFFFF) / 2147483648.0f;
        ditherSeed = (ditherSeed * 1664525u + 1013904223u);
        float r2 = (float)(ditherSeed & 0x7FFFFFFF) / 2147483648.0f;
        return (r1 + r2 - 1.0f) * 0.5f; // -0.5 to +0.5 range
    }

    /**
     * Monotonic asymmetric tube saturation curve.
     * Soft saturation with triode-like characteristics.
     */
    float triodeCurve(float x, float bias) {
        // Soft triode model: (x+bias) / (1 + |x+bias|)
        float biased = x + bias;
        float saturated = biased / (1.0f + fabsf(biased));
        float offset = bias / (1.0f + fabsf(bias));
        return saturated - offset;
    }

    /**
     * Smooth folding function with cubic interpolation
     */
    float smoothFold(float x) {
        // Multi-stage folding with soft boundaries
        for (int i = 0; i < 3; ++i) {
            if (x > 1.0f) {
                float dist = x - 1.0f;
                x = 1.0f - dist * (1.0f - 0.2f * dist); // Subtle cubic-ish fold
            } else if (x < -1.0f) {
                float dist = -1.0f - x;
                x = -1.0f + dist * (1.0f - 0.2f * dist);
            }
        }
        return rack::math::clamp(x, -1.2f, 1.2f);
    }

    /**
     * Hard clipping with a soft-start knee
     * Transitions from musical saturation to aggressive clipping.
     */
    float hardClip(float input, float drive) {
        // Input gain
        float gain = 1.0f + drive * 18.0f;
        float x = input * gain;
        
        // Threshold reduces with drive
        float threshold = 1.0f - drive * 0.7f;
        threshold = std::max(threshold, 0.15f);
        
        // Soft knee saturation before hard clip
        float knee = threshold * 0.5f;
        float absX = fabsf(x);
        
        float output;
        if (absX < threshold - knee) {
            output = x;
        } else if (absX > threshold + knee) {
            output = std::copysign(threshold, x);
        } else {
            // Cubic knee
            float t = (absX - (threshold - knee)) / (2.0f * knee);
            float s = t * t * (3.0f - 2.0f * t);
            output = std::copysign(threshold - knee + knee * s, x);
        }
        
        // Normalize
        return output / threshold;
    }
    
    /**
     * Wave folding with asymmetric bias and internal feedback
     */
    float waveFold(float input, float drive) {
        float bias = drive * 0.2f;
        float gain = 1.0f + drive * 8.0f;
        float x = (input + bias) * gain;
        
        // Folding logic
        x = smoothFold(x);
        
        // Subtle feedback for movement
        float feedback = drive * 0.15f;
        x = x + prev_input * feedback;
        prev_input = x;
        
        return triodeCurve(x * 0.8f, 0.0f);
    }
    
    /**
     * Bit crush with dithered quantization and sample hold
     */
    float bitCrush(float input, float drive) {
        float bits = rack::math::crossfade(16.0f, 3.5f, drive);
        float scale = std::pow(2.0f, bits - 1.0f);
        
        // Dither
        float ditherAmt = 1.0f / scale;
        float dithered = input + dither() * ditherAmt;
        
        // Quantize
        float quantized = std::round(dithered * scale) / scale;
        
        // Sample rate reduction
        int hold = (1 + (int)(drive * drive * 48.0f)) * rateScale;
        if (++crushCounter >= hold) {
            crushCounter = 0;
            crushSample = quantized;
        }
        
        return crushSample;
    }
    
    // Destruction: fold → clip → chaotic feedback.
    // Uses its own destroyFb state so waveFold's prev_input is not entangled.
    float destroy(float input, float drive) {
        float x = input;

        x = waveFold(x, drive * 0.5f);
        x = hardClip(x, drive * 0.5f);

        float fb = 0.3f + drive * 0.4f;
        // Chaos: phase expressed as normalized cycle so fastSin2Pi can be used
        float chaosPhase = (destroyFb * 4.0f + drive * 10.0f) * (1.0f / (2.0f * (float)M_PI));
        float chaos = fastSin2Pi(chaosPhase) * drive * 0.2f;
        x = x + destroyFb * fb + chaos;
        destroyFb = triodeCurve(x * 0.5f, 0.0f);

        return destroyFb;
    }

    // Ring modulation with morphing carrier (sine → tri → square).
    // Frequency: 20 Hz at drive=0, ~2 kHz at drive=1.
    // Uses fastSin2Pi and rack::dsp::exp2_taylor5 instead of std::sin/std::pow.
    float ringMod(float input, float drive) {
        // std::pow(100, drive) == 2^(drive * log2(100)) == 2^(drive * 6.64386)
        float freq = 20.0f * rack::dsp::exp2_taylor5(drive * 6.64386f);
        phase += freq / sample_rate;   // phase in cycles [0, 1)
        if (phase >= 1.0f) phase -= 1.0f;

        float tri = 2.0f * std::abs(2.0f * (phase - 0.5f)) - 1.0f;

        float carrier;
        if (drive < 0.5f) {
            float sine = fastSin2Pi(phase);
            carrier = rack::math::crossfade(sine, tri, drive * 2.0f);
        } else {
            float sq = phase < 0.5f ? 1.0f : -1.0f;
            carrier = rack::math::crossfade(tri, sq, (drive - 0.5f) * 2.0f);
        }

        float modulated = input * carrier;
        return triodeCurve(modulated * (1.0f + drive * 2.0f), 0.0f);
    }
    
    /**
     * Advanced triode tube saturation model
     */
    float tubeSat(float input, float drive) {
        float gain = 1.0f + drive * 12.0f;
        float x = input * gain;
        
        // Bias shift with drive
        float bias = drive * 0.4f;
        
        // Stage 1: Asymmetric triode saturation
        float saturated = triodeCurve(x, bias);
        
        // Stage 2: Second stage for more compression
        saturated = triodeCurve(saturated * 1.5f, -bias * 0.5f);
        
        return saturated;
    }
};

}} // namespace shapetaker::dsp
