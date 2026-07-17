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

    // Sample-rate reduction state for bit crush
    int crushCounter = 0;        // Downsample hold counter
    int crushHold = 1;           // Samples to hold
    float crushSample = 0.0f;    // Last held sample
    int rateScale = 1;           // Host-rate samples per engine-rate sample group

    // Tube sag envelope (program-dependent bias/gain shift for TUBE_SAT)
    float sagEnv = 0.0f;
    float sagAttackCoeff = 0.01f;  // Updated in setSampleRate()
    float sagReleaseCoeff = 0.0005f;

    // Sample/hold chaos state for DESTROY, gated by an input envelope so the
    // feedback/chaos loop dies out when the input goes silent
    int chaosCounter = 0;
    int chaosHold = 32;
    float chaosTarget = 0.0f;
    float chaosState = 0.0f;
    float destroyEnv = 0.0f;
    float destroyAttackCoeff = 0.02f;   // Updated in setSampleRate()
    float destroyReleaseCoeff = 0.0003f;

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
        // Tube sag: ~5 ms attack, ~150 ms release
        sagAttackCoeff = 1.0f - expf(-1.0f / (0.005f * sample_rate));
        sagReleaseCoeff = 1.0f - expf(-1.0f / (0.150f * sample_rate));
        // Destroy input gate: ~1 ms attack, ~80 ms release
        destroyAttackCoeff = 1.0f - expf(-1.0f / (0.001f * sample_rate));
        destroyReleaseCoeff = 1.0f - expf(-1.0f / (0.080f * sample_rate));
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
        destroyFb = 0.0f;
        crushCounter = 0;
        crushHold = 1;
        crushSample = 0.0f;
        sagEnv = 0.0f;
        chaosCounter = 0;
        chaosHold = 32;
        chaosTarget = 0.0f;
        chaosState = 0.0f;
        destroyEnv = 0.0f;
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
            destroyFb *= 0.99f; // Slowly decay feedback state
            sagEnv *= 0.999f;
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
    // Good to ~0.1% error — sufficient for ring mod and fold shaping.
    // The parabola approximates sin(pi*(2*phase-1)) = -sin(2*pi*phase), so the
    // result is negated to return true sin — waveFold() relies on this sign
    // matching triFold().
    static inline float fastSin2Pi(float phase) {
        // Bring to [0, 1)
        phase -= std::floor(phase);
        float x = phase * 2.0f - 1.0f;          // map to [-1, 1]
        float absX = x < 0.f ? -x : x;
        float y = 4.0f * x * (1.0f - absX);     // parabola
        float absY = y < 0.f ? -y : y;
        return -y * (0.225f * (absY - 1.0f) + 1.0f); // smooth, sign-corrected
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

    // Fast tanh approximation (rational Padé-style) — brighter, harder knee
    // than the old x/(1+|x|) curve, exact to float precision for |x| < 3.
    static inline float fastTanh(float x) {
        float x2 = x * x;
        if (x2 > 9.0f) {
            return x > 0.0f ? 1.0f : -1.0f;
        }
        float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
        float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
        return a / b;
    }

    // One triode-ish gain stage. Positive swings saturate early and softly
    // (grid conduction); negative swings run further into a harder cutoff
    // rail. Both halves have unity slope at the origin, so the stage is
    // click-free; the bias response is subtracted to keep idle DC at zero.
    static inline float tubeStage(float x, float bias) {
        const float kPos = 0.7f;  // soft rail at +1/kPos
        const float kNeg = 1.6f;  // hard rail at -1/kNeg
        float b = x + bias;
        float y = (b >= 0.0f) ? fastTanh(kPos * b) * (1.0f / kPos)
                              : fastTanh(kNeg * b) * (1.0f / kNeg);
        float y0 = (bias >= 0.0f) ? fastTanh(kPos * bias) * (1.0f / kPos)
                                  : fastTanh(kNeg * bias) * (1.0f / kNeg);
        return y - y0;
    }

    // True reflective wavefolder: periodic triangle map, identity on [-1, 1],
    // folds any input back into [-1, 1] with unlimited fold count.
    static inline float triFold(float x) {
        float t = (x + 1.0f) * 0.25f;
        t -= std::floor(t);
        return 1.0f - 4.0f * fabsf(t - 0.5f);
    }

    // PolyBLEP residual for band-limiting carrier edges (t in samples/dt units)
    static inline float polyBlep(float t) {
        if (t >= 0.0f && t < 1.0f) {
            return t + t - t * t - 1.0f;
        }
        if (t > -1.0f && t < 0.0f) {
            return t + t + t * t + 1.0f;
        }
        return 0.0f;
    }

    /**
     * Hard clipping with a soft-start knee.
     * The knee narrows with drive: musical saturation at the bottom of the
     * knob, razor-edged rail clipping at the top.
     */
    float hardClip(float input, float drive) {
        // Input gain
        float gain = 1.0f + drive * 18.0f;
        float x = input * gain;

        // Threshold reduces with drive
        float threshold = 1.0f - drive * 0.7f;
        threshold = std::max(threshold, 0.15f);

        // Soft knee saturation before hard clip; knee narrows as drive rises
        float knee = threshold * (0.6f - 0.45f * drive);
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
     * Wave folding: true reflective folds (unlimited fold count), with the
     * fold tips morphing from rounded (sine map) at low drive to sharp
     * (triangle map) when pushed. A drive-scaled bias skews the folds for
     * even harmonics. Squared gain taper keeps the low knob gentle while the
     * top reaches deep multi-fold territory.
     */
    float waveFold(float input, float drive) {
        float gain = 1.0f + drive * drive * 14.0f;
        float bias = drive * 0.35f;
        float x = input * gain + bias;

        // Both maps have period 4 and matching extrema, so the crossfade
        // only changes the tip shape, not the fold positions.
        float sharp = triFold(x);
        float round = fastSin2Pi(x * 0.25f);
        float folded = rack::math::crossfade(sharp, round, 0.65f - 0.4f * drive);

        // Light glue saturation; outer DC blocker removes the bias offset.
        return fastTanh(folded * 1.1f) * 0.95f;
    }

    /**
     * Bit crush with dithered quantization and sample hold.
     * The hold interval jitters at higher drives so the decimation grinds
     * instead of ringing at one fixed frequency.
     */
    float bitCrush(float input, float drive) {
        float bits = rack::math::crossfade(16.0f, 3.0f, drive);
        float scale = std::pow(2.0f, bits - 1.0f);

        // Dither
        float ditherAmt = 1.0f / scale;
        float dithered = input + dither() * ditherAmt;

        // Quantize
        float quantized = std::round(dithered * scale) / scale;

        // Sample rate reduction
        if (++crushCounter >= crushHold) {
            crushCounter = 0;
            crushSample = quantized;
            // Choose the next hold interval, jittered by up to ±40% at full drive
            float hold = (1.0f + drive * drive * 48.0f) * (float)rateScale;
            hold *= 1.0f + dither() * 0.8f * drive;
            crushHold = std::max(1, (int)hold);
        }

        return crushSample;
    }

    // Destruction: deep asymmetric folding into chaotic feedback into hard
    // clipping. The chaos is a slewed sample/hold voltage — a broken-machine
    // stagger rather than a periodic warble — and the raw clipped signal is
    // the output, so the top of the knob is genuinely vicious.
    float destroy(float input, float drive) {
        // Input envelope gates the feedback/chaos loop: destruction reacts to
        // the signal and dies with it instead of free-running.
        float level = fabsf(input);
        float envCoeff = (level > destroyEnv) ? destroyAttackCoeff : destroyReleaseCoeff;
        destroyEnv += (level - destroyEnv) * envCoeff;
        float gate = rack::math::clamp(destroyEnv * 3.0f, 0.0f, 1.0f);

        float x = waveFold(input, 0.35f + drive * 0.5f);

        // Signal feedback plus slewed random-target chaos injection
        float fb = 0.25f + drive * 0.45f;
        if (++chaosCounter >= chaosHold) {
            chaosCounter = 0;
            chaosTarget = dither() * 2.0f * drive;
            chaosHold = (24 + (int)((dither() + 0.5f) * 120.0f)) * rateScale;
        }
        chaosState += 0.05f * (chaosTarget - chaosState);
        x += (destroyFb * fb + chaosState) * gate;

        float y = hardClip(x, 0.3f + drive * 0.5f) * 0.85f;
        destroyFb = fastTanh(y * 0.8f);
        return y;
    }

    // Ring modulation with morphing carrier (sine → tri → square).
    // Frequency: 20 Hz at drive=0, ~2 kHz at drive=1. The square component
    // is polyBLEP band-limited so the top of the sweep shimmers instead of
    // spraying inharmonic alias tones.
    float ringMod(float input, float drive) {
        // std::pow(100, drive) == 2^(drive * log2(100)) == 2^(drive * 6.64386)
        float freq = 20.0f * rack::dsp::exp2_taylor5(drive * 6.64386f);
        float dt = freq / sample_rate;
        phase += dt;   // phase in cycles [0, 1)
        if (phase >= 1.0f) phase -= 1.0f;

        float tri = 2.0f * std::abs(2.0f * (phase - 0.5f)) - 1.0f;

        float carrier;
        if (drive < 0.5f) {
            float sine = fastSin2Pi(phase);
            carrier = rack::math::crossfade(sine, tri, drive * 2.0f);
        } else {
            float sq = phase < 0.5f ? 1.0f : -1.0f;
            // Band-limit both square edges (rising at 0, falling at 0.5)
            if (phase < dt) {
                sq += polyBlep(phase / dt);
            } else if (phase > 1.0f - dt) {
                sq += polyBlep((phase - 1.0f) / dt);
            }
            if (phase > 0.5f && phase < 0.5f + dt) {
                sq -= polyBlep((phase - 0.5f) / dt);
            } else if (phase > 0.5f - dt && phase < 0.5f) {
                sq -= polyBlep((phase - 0.5f) / dt);
            }
            carrier = rack::math::crossfade(tri, sq, (drive - 0.5f) * 2.0f);
        }

        float modulated = input * carrier;
        return fastTanh(modulated * (1.0f + drive * 2.0f));
    }

    /**
     * Two-stage asymmetric tube saturation with program-dependent sag.
     * Each stage clips its halves differently (soft grid conduction versus
     * hard cutoff), and the second stage is inverted so the asymmetry bites
     * the opposite half — even harmonics from both directions. A slow
     * envelope on the first stage eases gain and deepens bias on hot
     * passages, so sustained material breathes instead of sitting still.
     */
    float tubeSat(float input, float drive) {
        // Sag eases input gain by up to ~25% and pushes bias when hot
        float sagGain = 1.0f / (1.0f + 0.35f * drive * sagEnv);
        float gain = (1.0f + drive * 14.0f) * sagGain;
        float bias = drive * (0.3f + 0.15f * sagEnv);

        float s1 = tubeStage(input * gain, bias);

        float level = fabsf(s1);
        float sagCoeff = (level > sagEnv) ? sagAttackCoeff : sagReleaseCoeff;
        sagEnv += (level - sagEnv) * sagCoeff;

        // Inverted second stage flips which half saturates hard
        float s2 = tubeStage(-1.35f * s1, bias * 0.6f);
        return -s2 * 0.9f;
    }
};

}} // namespace shapetaker::dsp
