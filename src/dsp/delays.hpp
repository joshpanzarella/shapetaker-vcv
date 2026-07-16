#pragma once

#include <rack.hpp>
#include <cmath>
#include "oscillators.hpp"

namespace shapetaker {
namespace dsp {

/**
 * Simple chorus effect with LFO-modulated delay.
 *
 * The tap sits on a fixed 8 ms base; depth scales only the LFO swing (up to
 * +-6 ms). Previously depth scaled the base delay too, which put low settings
 * in comb-filter territory and made depth automation glide the pitch.
 * The modulated read uses 4-point Catmull-Rom interpolation.
 */
class ChorusEffect {
private:
    float* delayBuffer = nullptr;
    int bufferSize = 0;
    int writeIndex = 0;
    float lfoPhase = 0.f;  // cycles [0, 1)

public:
    ~ChorusEffect() {
        delete[] delayBuffer;
    }

    void setSampleRate(float sampleRate) {
        if (delayBuffer) {
            delete[] delayBuffer;
        }
        bufferSize = (int)(sampleRate * 0.1f); // Max delay of 100ms
        delayBuffer = new float[bufferSize]();
    }

    float process(float input, float rate, float depth, float mix, float sampleRate) {
        if (!delayBuffer) {
            return input;
        }

        lfoPhase += rate / sampleRate;
        if (lfoPhase >= 1.f) {
            lfoPhase -= 1.f;
        }

        float lfo = OscillatorHelper::fastSin2Pi(lfoPhase);
        float delayMs = 8.f + 6.f * depth * lfo;
        float delayInSamples = delayMs * sampleRate * 0.001f;
        delayInSamples = rack::math::clamp(delayInSamples, 3.f, (float)(bufferSize - 4));

        int intDelay = (int)delayInSamples;
        float frac = delayInSamples - (float)intDelay;

        // Catmull-Rom taps: larger delay = older sample
        auto at = [&](int d) {
            int idx = writeIndex - d;
            while (idx < 0) {
                idx += bufferSize;
            }
            return delayBuffer[idx];
        };
        float y0 = at(intDelay - 1);
        float y1 = at(intDelay);
        float y2 = at(intDelay + 1);
        float y3 = at(intDelay + 2);
        float f2 = frac * frac;
        float f3 = f2 * frac;
        float delayedSample = 0.5f * ((2.f * y1) +
            (-y0 + y2) * frac +
            (2.f * y0 - 5.f * y1 + 4.f * y2 - y3) * f2 +
            (-y0 + 3.f * y1 - 3.f * y2 + y3) * f3);

        delayBuffer[writeIndex] = input;
        writeIndex++;
        if (writeIndex >= bufferSize) {
            writeIndex = 0;
        }

        return input * (1.f - mix) + delayedSample * mix;
    }

    void reset() {
        if (delayBuffer) {
            for (int i = 0; i < bufferSize; i++) {
                delayBuffer[i] = 0.f;
            }
        }
        writeIndex = 0;
        lfoPhase = 0.f;
    }
};

/**
 * 6-stage phaser using cascaded all-pass filters
 */
class PhaserEffect {
private:
    static const int NUM_STAGES = 6;
    class AllPassFilter {
        float x1 = 0.f, y1 = 0.f;
        float a1 = 0.f;
    public:
        void setCoefficient(float freq, float sampleRate) {
            float omega = 2.f * M_PI * freq / sampleRate;
            float tan_half = std::tan(omega * 0.5f);
            a1 = (tan_half - 1.f) / (tan_half + 1.f);
        }
        
        float process(float input) {
            float output = a1 * input + x1 - a1 * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() {
            x1 = y1 = 0.f;
        }
    };
    
    AllPassFilter stages[NUM_STAGES];
    AllPassFilter feedbackFilter;
    float feedbackMemory = 0.f;
    float dcBlocker = 0.f;

public:
    void reset() {
        for (int i = 0; i < NUM_STAGES; i++) {
            stages[i].reset();
        }
        feedbackFilter.reset();
        feedbackMemory = 0.f;
        dcBlocker = 0.f;
    }

    float process(float input, float centerFreq, float feedback, float mix, float sampleRate) {
        // Clamp parameters to safe ranges
        centerFreq = rack::math::clamp(centerFreq, 100.f, sampleRate * 0.35f);
        feedback = rack::math::clamp(feedback, 0.f, 0.7f);
        mix = rack::math::clamp(mix, 0.f, 1.f);

        // High-pass filter the feedback to prevent low-frequency buildup
        feedbackFilter.setCoefficient(80.f, sampleRate);
        float filteredFeedback = feedbackFilter.process(feedbackMemory);

        // Apply soft limiting to feedback
        filteredFeedback = std::tanh(filteredFeedback * 0.5f) * 2.f;

        // Add controlled feedback
        float signal = input + filteredFeedback * feedback * 0.3f;

        // DC blocking filter
        float dcBlocked = signal - dcBlocker;
        dcBlocker += dcBlocked * 0.001f;
        signal = dcBlocked;

        // Process through all 6 all-pass stages
        for (int i = 0; i < NUM_STAGES; i++) {
            float stageFreq = centerFreq * std::pow(2.f, (i - 2.5f) * 0.15f);
            stageFreq = rack::math::clamp(stageFreq, 100.f, sampleRate * 0.35f);

            stages[i].setCoefficient(stageFreq, sampleRate);
            signal = stages[i].process(signal);

            // Soft limiting after each stage
            signal = std::tanh(signal * 0.8f) * 1.25f;
        }

        // Store output for feedback with limiting
        feedbackMemory = rack::math::clamp(signal, -5.f, 5.f);

        // Mix dry and wet signals
        return input * (1.f - mix) + signal * mix;
    }
};

/**
 * Shimmer delay line with harmonic content
 */
class ShimmerDelay {
private:
    static const int MAX_DELAY = 4800; // 100ms at 48kHz
    float buffer[MAX_DELAY] = {};
    int writePos = 0;

public:
    float process(float input, float delayTime, float feedback, float shimmer) {
        int delaySamples = (int)(delayTime * 48000.f);
        delaySamples = rack::math::clamp(delaySamples, 1, MAX_DELAY - 1);

        int readPos = (writePos - delaySamples + MAX_DELAY) % MAX_DELAY;
        float delayed = buffer[readPos];

        // Add harmonic content for shimmer
        if (shimmer > 0.f) {
            delayed += std::sin(delayed * M_PI * 2.f) * shimmer * 0.3f;
        }

        buffer[writePos] = input + delayed * feedback;
        writePos = (writePos + 1) % MAX_DELAY;

        return delayed;
    }

    void reset() {
        for (int i = 0; i < MAX_DELAY; i++) {
            buffer[i] = 0.f;
        }
        writePos = 0;
    }
};

/**
 * Simple envelope follower for dynamic effects
 */
class EnvelopeFollower {
private:
    float envelope = 0.f;

public:
    float process(float input, float attack, float release, float sampleTime) {
        float inputLevel = std::abs(input);
        
        if (inputLevel > envelope) {
            // Attack phase
            float attackCoeff = std::exp(-sampleTime / attack);
            envelope = inputLevel + (envelope - inputLevel) * attackCoeff;
        } else {
            // Release phase
            float releaseCoeff = std::exp(-sampleTime / release);
            envelope = inputLevel + (envelope - inputLevel) * releaseCoeff;
        }
        
        return envelope;
    }

    void reset() {
        envelope = 0.f;
    }
};

/**
 * Fast parameter smoother for real-time control
 */
class FastSmoother {
private:
    float value = 0.f;
    bool initialized = false;

public:
    float process(float target, float sampleTime, float timeConstant) {
        if (!initialized) {
            value = target;
            initialized = true;
            return value;
        }
        
        if (timeConstant < 1e-6f) {
            timeConstant = 1e-6f;
        }

        float alpha = sampleTime / (timeConstant + sampleTime);
        value += alpha * (target - value);
        return value;
    }

    float process(float target, float sampleTime) {
        // Default to very fast smoothing (1ms time constant)
        return process(target, sampleTime, 0.001f);
    }

    void reset(float initialValue = 0.f) {
        value = initialValue;
        initialized = false;
    }

    float getValue() const {
        return value;
    }
};

} // namespace dsp
} // namespace shapetaker
