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
 * This class provides 5 different distortion types ranging from
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
    // tubeSat() bias, as a fraction of the driven amplitude — see tubeSat().
    // Higher values buy more 2nd harmonic when saturated but skew the duty
    // heavily at moderate levels; 0.10 keeps the even/odd balance near neutral
    // at ordinary drive while staying clear of HARD_CLIP when pushed.
    static constexpr float TUBE_BIAS_FRAC = 0.10f;
    static constexpr float TUBE_BIAS_SAG  = 0.05f;

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

    // Dither noise generator state for bit crush
    unsigned int ditherSeed = 1;

    // Previous fold-map input, for the ADAA secant in waveFold()
    float foldXPrev = 0.f;
    // Previous post-gain clipper input, for the ADAA secant in hardClip()
    float clipUPrev = 0.f;
    static constexpr float CLIP_ADAA_EPS = 1e-4f;

    // How much of the pre/de-emphasis pair the wavefolder gets. See process().
    static constexpr float FOLD_EMPHASIS_SCALE = 0.0f;

    // Fold gain coefficient. Was 14; ADAA on its own did not clean up the
    // default 4x oversampling because at that depth the input crosses several
    // fold periods per sample. 10 keeps genuine multi-fold territory while
    // letting the ADAA work.
    static constexpr float FOLD_GAIN = 10.0f;
    // Below this segment length the secant is numerically useless.
    static constexpr float FOLD_ADAA_EPS = 1e-4f;

public:
    enum Type {
        HARD_CLIP = 0,  // Aggressive limiting with harsh harmonics
        TUBE_SAT = 1,   // Asymmetric tube-style saturation
        WAVE_FOLD = 2,  // Multi-stage wave folding
        BIT_CRUSH = 3,  // Bit depth + sample rate reduction
        // DESTROY used to sit at 4. It was removed: it was built out of
        // waveFold() into hardClip() plus a chaos loop, so it overlapped two
        // topologies already present while contributing mostly noise, and its
        // character shifted with every change to the folder it borrowed.
        // RING_MOD takes index 4 so a patch that stored 5 clamps onto it and
        // keeps the same sound.
        RING_MOD = 4    // Ring modulation with internal oscillator
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
    }

    void setRateScale(int scale) {
        rateScale = std::max(1, scale);
    }

    // How much of the pre/de-emphasis pair a given topology gets. Declared here
    // rather than with the other members because it needs Type.
    static inline float emphasisScale(Type type) {
        return (type == WAVE_FOLD) ? FOLD_EMPHASIS_SCALE : 1.0f;
    }

    /**
     * Per-topology output trim, so the five algorithms sit at matched loudness
     * for a given drive instead of spanning 4-6 dB. The adaptive makeup in
     * Chiaroscuro hides most of that spread, but only by working harder on the
     * quiet topologies — and makeup gain amplifies whatever noise and residual
     * aliasing those topologies carry, so evening the levels up front is worth
     * doing even though the makeup would have covered it.
     *
     * A quadratic in drive rather than one constant: RING_MOD rises about
     * 4.6 dB across its drive range as the carrier sweeps up, and WAVE_FOLD
     * *fell* 3.1 dB as more of its output moved above Nyquist and was correctly
     * discarded by the decimator. A single scalar cannot follow either.
     *
     * Coefficients are least-squares fits to a 9-point drive grid, each point
     * itself averaged over six pitches from 110 Hz to 2093 Hz, targeting
     * -2.5 dB. Deliberately low order: the measured curves have wiggles of a
     * decibel or so (fold count crossing period boundaries, the ring-mod
     * carrier moving through the measurement band) but those move with input
     * frequency, so tracking them against a fixed set of tones would be
     * precision without accuracy.
     */
    static inline float outputTrim(Type type, float drive) {
        switch (type) {
            case HARD_CLIP:
                return 0.8474f + drive * (-0.1306f) + drive * drive * (+0.1215f);
            case TUBE_SAT:
                return 1.0593f + drive * (-0.2649f) + drive * drive * (+0.2331f);
            case WAVE_FOLD:
                return 1.2175f + drive * (+0.2933f) + drive * drive * (+0.1184f);
            case BIT_CRUSH:
                return 1.0588f + drive * (+0.0792f) + drive * drive * (-0.2135f);
            case RING_MOD:
                return 1.9424f + drive * (-0.9498f) + drive * drive * (+0.0024f);
            default:
                return 1.0f;
        }
    }

    void seedDither(unsigned int seed) {
        ditherSeed = seed ? seed : 1u;
    }
    
    /**
     * Reset internal state (useful for feedback-based algorithms)
     */
    void reset() {
        phase = 0.0f;
        crushCounter = 0;
        crushHold = 1;
        crushSample = 0.0f;
        sagEnv = 0.0f;
        dcBlocker_x1 = 0.0f;
        dcBlocker_y1 = 0.0f;
        dcOut_x1 = 0.0f;
        dcOut_y1 = 0.0f;
        preEmph_lp = 0.0f;
        deEmph_lp = 0.0f;
        foldXPrev = 0.0f;
        clipUPrev = 0.0f;
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
            sagEnv *= 0.999f;
            dcBlock(input);
            dcBlockOut(input);
            preEmphasis(input, 0.0f);
            deEmphasis(input, 0.0f);
            return input;
        }

        // DC blocking on input (high-pass ~10 Hz)
        float clean = dcBlock(input);

        // Pre-emphasis: boost highs before distortion (gives more "analog"
        // character). Scaled per type, because it is actively harmful to the
        // wavefolder: preEmphasis doubles everything above 2.5 kHz at full
        // drive, and a folder's fold count scales directly with amplitude, so
        // that boost doubles the fold traversal rate and with it the aliasing.
        // It spent exactly the headroom the ADAA in waveFold() buys. Every
        // other topology clips or quantises rather than folding, so the boost
        // costs them nothing and they keep it.
        const float emphDrive = drive * emphasisScale(type);
        float emphasized = preEmphasis(clean, emphDrive);

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

        // De-emphasis: cut highs after distortion to compensate for
        // pre-emphasis. Must use the same scaled drive or the pair stops
        // mirroring and the type's tonal balance shifts.
        float deemphasized = deEmphasis(centered, emphDrive);

        // Per-topology output trim. Note this sits after the early transparent
        // return above, so a bypassed engine is still bit-transparent, and
        // Chiaroscuro's own dry/wet onset crossfade covers the same drive span
        // over which the trim first applies, so there is no step at the
        // threshold.
        return deemphasized * outputTrim(type, drive);
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

    // NOTE: OscillatorHelper (src/dsp/oscillators.hpp) carries its own copies of
    // fastSin2Pi and fastTanh, and every module calls *those*. The two sines are
    // NOT interchangeable: this one returns +sin(2*pi*phase), OscillatorHelper's
    // returns -sin(2*pi*phase). Keep any fix applied to both, and mind the sign.

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
    //
    // The bail-out sits at |x| > 5, not |x| > 3. The rational is still accurate
    // there (tanh(5) = 0.9999092), so returning a flat +/-1.0 at |x| = 3 used to
    // put a 0.00494 step — half a percent of full scale — into the middle of the
    // transfer curve. Any signal crossing that threshold picked up the step as
    // high-order harmonic splatter: measured -57 dB relative to the saturated
    // output at drive 4.0, against -103 dB with the knee where it is now.
    // Below |x| = 3 this is bit-identical to the previous version.
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

    // One triode-ish gain stage. Positive swings saturate early and softly
    // (grid conduction); negative swings run further into a harder cutoff
    // rail. Both halves have unity slope at the origin, so the stage is
    // click-free; the bias response is subtracted to keep idle DC at zero.
    static inline float tubeStage(float x, float bias) {
        // Asymmetry widened from 0.7/1.6. With the stage-2 inversion removed
        // below, the two stages now compound instead of cancelling, and this
        // extra spread is what pulls the 2nd harmonic clear of HARD_CLIP.
        const float kPos = 0.62f; // soft rail at +1/kPos
        const float kNeg = 1.85f; // hard rail at -1/kNeg
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

    // Antiderivatives of the two fold maps, for the ADAA in waveFold().
    //
    // Both maps have period 4 and zero mean over a period, so their
    // antiderivatives are themselves periodic. That buys three things: no
    // period counting, no unbounded accumulation, and — because a zero-mean
    // function's periodic antiderivative differs from the true one only by a
    // zero-slope term — the definite integral stays correct even when x jumps
    // several periods in a single sample, which is exactly the hard case.
    //
    // With p = frac((x+1)/4) and dx = 4 dp, integrating the triangle gives
    //   G(p) = 2p^2 - p        for p <= 0.5
    //          3p - 2p^2 - 1   for p >  0.5
    // and G(0) = G(0.5) = G(1) = 0, so 4*G(p) is continuous across the wrap.
    static inline float triFoldInt(float x) {
        float u = (x + 1.0f) * 0.25f;
        float p = u - std::floor(u);
        float g = (p <= 0.5f) ? (2.0f * p * p - p) : (3.0f * p - 2.0f * p * p - 1.0f);
        return 4.0f * g;
    }

    // The smooth map is a true sine rather than fastSin2Pi. The parabolic
    // approximation has a discontinuous derivative at each period wrap and
    // measured as a *worse* alias source than the triangle map, which defeats
    // the point of anti-aliasing the fold at all. sin(2*pi*x/4) == sin(pi*x/2).
    static inline float sinMap(float x) {
        return std::sin((float)M_PI * x * 0.5f);
    }
    static inline float sinMapInt(float x) {
        return -std::cos((float)M_PI * x * 0.5f) * (2.0f / (float)M_PI);
    }

    // The fold map and its antiderivative, as a function of the sharp/smooth
    // morph weight.
    static inline float foldMap(float x, float t) {
        return rack::math::crossfade(triFold(x), sinMap(x), t);
    }
    static inline float foldMapInt(float x, float t) {
        return rack::math::crossfade(triFoldInt(x), sinMapInt(x), t);
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

    // The clipper's shape, on the post-gain variable. Odd function.
    static inline float clipShape(float u, float a, float b, float knee, float threshold) {
        float au = fabsf(u);
        if (au < a) return u;
        if (au > b) return std::copysign(threshold, u);
        float t = (au - a) / (2.0f * knee);
        float s = t * t * (3.0f - 2.0f * t);
        return std::copysign(a + knee * s, u);
    }

    // Antiderivative of clipShape. The shape is odd, so its antiderivative is
    // even and equals G(|u|) where G integrates from zero:
    //   linear region  G = u^2/2
    //   cubic knee     G = a^2/2 + a*(u-a) + 2*knee^2*(t^3 - t^4/2)
    //   rail           G = G(b) + threshold*(u-b)
    static inline float clipShapeInt(float u, float a, float b, float knee, float threshold) {
        float au = fabsf(u);
        if (au <= a) return 0.5f * au * au;
        if (au >= b) {
            float gb = 0.5f * a * a + 2.0f * a * knee + knee * knee;
            return gb + threshold * (au - b);
        }
        float t = (au - a) / (2.0f * knee);
        float t3 = t * t * t;
        return 0.5f * a * a + a * (au - a) + 2.0f * knee * knee * (t3 - 0.5f * t3 * t);
    }

    /**
     * Hard clipping with a soft-start knee.
     * The knee narrows with drive: musical saturation at the bottom of the
     * knob, razor-edged rail clipping at the top.
     *
     * Anti-aliased the same way as the wavefolder. A hard rail is a derivative
     * discontinuity, so point-sampling it sprays foldback: this measured -31 dB
     * at the default 4x oversampling, second only to WAVE_FOLD before that was
     * fixed, and HARD_CLIP is the most reached-for topology of the six. Both the
     * shape and its antiderivative are piecewise polynomials here, so the
     * antiderivative is exact and costs no transcendentals.
     */
    float hardClip(float input, float drive) {
        // A knee-primary voicing was tried here to spread the knob's usable
        // range and measured worse on both counts, so this is the original
        // mapping. Holding threshold at unity while sweeping the knee narrow
        // *raises* the clipping onset (a = threshold - knee), which fights the
        // rising gain: total harmonic energy went non-monotonic, dipping around
        // DIST 60% before climbing again, so the knob read as more-distorted /
        // less-distorted / more-distorted. Worse, capping the gain at 5-8x left
        // the clipper completely linear for quiet sources (THD below -140 dB at
        // a 0.15 input) — the shrinking threshold below is what lets a quiet
        // signal reach the rail at all.
        float gain = 1.0f + drive * 18.0f;
        float threshold = std::max(1.0f - drive * 0.7f, 0.15f);
        // Knee is always well clear of zero (>= 0.15 * 0.15), so the divisions
        // by knee below are safe.
        float knee = threshold * (0.6f - 0.45f * drive);
        float a = threshold - knee;
        float b = threshold + knee;

        float u = input * gain;
        float du = u - clipUPrev;
        float shaped;
        if (fabsf(du) < CLIP_ADAA_EPS) {
            shaped = clipShape(0.5f * (u + clipUPrev), a, b, knee, threshold);
        } else {
            shaped = (clipShapeInt(u, a, b, knee, threshold)
                    - clipShapeInt(clipUPrev, a, b, knee, threshold)) / du;
        }
        clipUPrev = u;

        // Normalize
        return shaped / threshold;
    }

    /**
     * Wave folding: true reflective folds (unlimited fold count), with the
     * fold tips morphing from rounded (sine map) at low drive to sharp
     * (triangle map) when pushed. A drive-scaled bias skews the folds for
     * even harmonics; the outer DC blocker strips the resulting offset. Squared
     * gain taper keeps the low knob gentle while the top reaches deep
     * multi-fold territory.
     *
     * A biasScale argument used to sit here so the removed DESTROY topology
     * could gate the skew bias down — feeding a biased fold into a high-gain
     * clipper pinned that clipper to a constant rail whenever the input was too
     * small to swing the fold through zero. Nothing needs it now.
     */
    float waveFold(float input, float drive) {
        float gain = 1.0f + drive * drive * FOLD_GAIN;
        float bias = drive * 0.35f;
        float x = input * gain + bias;
        // Both maps have period 4 and matching extrema, so the crossfade only
        // changes the tip shape, not the fold positions.
        float t = rack::math::clamp(0.65f - 0.4f * drive, 0.0f, 1.0f);

        // First-order ADAA. Point-sampling this map is what made WAVE_FOLD the
        // dirtiest topology by a wide margin: at full drive the input crosses
        // several fold periods per sample, and the foldback measured *louder
        // than the signal itself* at 1x and 2x oversampling, -17 dB at the
        // default 4x. Averaging the map over the segment actually traversed
        // this sample, rather than sampling one point on it, took the 4x figure
        // to about -37 dB and made 1x usable. Oversampling alone could not fix
        // this and neither could a milder fold gain; it needed both.
        //
        // Both antiderivatives are recomputed each sample from the *current*
        // map rather than caching the previous sample's value, so a moving drive
        // cannot mix two different maps into one difference.
        float folded;
        float dx = x - foldXPrev;
        if (fabsf(dx) < FOLD_ADAA_EPS) {
            // Degenerate segment — the secant is numerically useless here, so
            // fall back to the midpoint value.
            folded = foldMap(0.5f * (x + foldXPrev), t);
        } else {
            folded = (foldMapInt(x, t) - foldMapInt(foldXPrev, t)) / dx;
        }
        foldXPrev = x;

        // Light glue saturation; outer DC blocker removes the bias offset. The
        // tanh stays outside the ADAA: tanh(F(x)) has no closed-form
        // antiderivative, and it is a gentle curve on an already-bounded
        // signal, so it generates little of the aliasing.
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
        // Bias is a fraction of the *driven* amplitude, not an absolute offset.
        //
        // This is what keeps TUBE_SAT distinguishable from HARD_CLIP when both
        // are pushed hard. Unequal rails alone cannot do it: drive a symmetric
        // input into full saturation and you get a 50%-duty wave with plateaus
        // at +A and -B, whose only asymmetry is a DC offset of (A-B)/2 — and the
        // outer DC blocker removes exactly that, leaving a symmetric square with
        // the same shape as a hard clip. Measured, the two became almost
        // identical: 2nd harmonic -40.8 dB and duty 49.8% at the hot operating
        // point, against hard clip's mathematically zero even harmonics.
        //
        // What survives DC blocking is *duty* asymmetry, and that comes from a
        // bias applied before saturation. An absolute 0.3 was swamped once the
        // post-gain signal reached ~42, shifting the zero crossing under 1%.
        // Scaling with gain makes the duty shift level-independent.
        float bias = drive * (TUBE_BIAS_FRAC + TUBE_BIAS_SAG * sagEnv) * gain;

        float s1 = tubeStage(input * gain, bias);

        float level = fabsf(s1);
        float sagCoeff = (level > sagEnv) ? sagAttackCoeff : sagReleaseCoeff;
        sagEnv += (level - sagEnv) * sagCoeff;

        // Second stage no longer inverts. Feeding it -1.35*s1 applied stage
        // one's asymmetry to the *flipped* signal, so the two stages undid each
        // other — and the harder it was driven the more complete the
        // cancellation, with the 2nd harmonic falling to -81 dB at full drive.
        // That left TUBE_SAT harmonically indistinguishable from HARD_CLIP:
        // both were pure odd-harmonic saturators with near-identical H3/H5, and
        // with the makeup gain matching their loudness there was nothing left
        // to tell them apart. Compounding the stages restores the even-harmonic
        // warmth the topology is named for.
        float s2 = tubeStage(1.35f * s1, bias * 0.6f);
        return s2 * 0.9f;
    }
};

}} // namespace shapetaker::dsp
