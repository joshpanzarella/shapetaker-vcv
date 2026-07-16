#pragma once
#include <rack.hpp>
#include <cmath>
#include <algorithm>

using namespace rack;

namespace shapetaker {
namespace dsp {

// ============================================================================
// ENVELOPE UTILITIES
// ============================================================================

// Full ADSR envelope generator
class EnvelopeGenerator {
public:
    enum Stage {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };

private:
    Stage currentStage = IDLE;
    float currentLevel = 0.f;
    float attackTime = 0.01f;
    float decayTime = 0.1f;
    float sustainLevel = 0.7f;
    float releaseTime = 0.1f;

    float attackCoef = 0.f;
    float attackBase = 0.f;
    float decayCoef = 0.f;
    float decayBase = 0.f;
    float releaseCoef = 0.f;
    float releaseBase = 0.f;

    bool gateHigh = false;

    // exp(-5) ≈ 0.0067, so the envelope reaches 99.3% of its target in exactly
    // `time` seconds — the parameter means what the label says, not the RC τ.
    static float calcCoef(float time, float sampleRate) {
        time = std::max(time, 1e-6f);
        sampleRate = std::max(sampleRate, 1.f);
        return std::exp(-5.f / (time * sampleRate));
    }

    static float calcBase(float target, float coef) {
        return (1.f - coef) * target;
    }

public:
    void setAttack(float seconds, float sampleRate) {
        attackTime = std::max(seconds, 0.f);
        float coef = calcCoef(attackTime, sampleRate);
        attackCoef = coef;
        attackBase = calcBase(1.f, coef);
    }
    
    void setDecay(float seconds, float sampleRate) {
        decayTime = std::max(seconds, 0.f);
        float coef = calcCoef(decayTime, sampleRate);
        decayCoef = coef;
        decayBase = calcBase(sustainLevel, coef);
    }
    
    void setSustain(float level) {
        sustainLevel = rack::math::clamp(level, 0.f, 1.f);
        decayBase = calcBase(sustainLevel, decayCoef);
    }
    
    void setRelease(float seconds, float sampleRate) {
        releaseTime = std::max(seconds, 0.f);
        float coef = calcCoef(releaseTime, sampleRate);
        releaseCoef = coef;
        releaseBase = 0.f;
    }
    
    void gate(bool high) {
        if (high && !gateHigh) {
            // Gate on: start attack
            trigger();
        } else if (!high && gateHigh) {
            // Gate off: start release
            currentStage = RELEASE;
            if (releaseTime <= 1e-6f) {
                currentLevel = 0.f;
                currentStage = IDLE;
            }
        }
        gateHigh = high;
    }

    void trigger() {
        currentStage = ATTACK;
        gateHigh = true;
        if (attackTime <= 1e-6f) {
            currentLevel = 1.f;
            currentStage = DECAY;
        }
    }
    
    float process() {
        switch (currentStage) {
            case IDLE:
                currentLevel = 0.f;
                break;
                
            case ATTACK:
                currentLevel = attackBase + currentLevel * attackCoef;
                if (currentLevel >= 0.999f) {
                    currentLevel = 1.f;
                    currentStage = DECAY;
                    if (decayTime <= 1e-6f) {
                        currentLevel = sustainLevel;
                        currentStage = gateHigh ? SUSTAIN : RELEASE;
                    }
                }
                break;
                
            case DECAY:
                currentLevel = decayBase + currentLevel * decayCoef;
                if (currentLevel <= sustainLevel + 1e-3f) {
                    currentLevel = sustainLevel;
                    currentStage = gateHigh ? SUSTAIN : RELEASE;
                    if (!gateHigh && releaseTime <= 1e-6f) {
                        currentLevel = 0.f;
                        currentStage = IDLE;
                    }
                }
                break;
                
            case SUSTAIN:
                currentLevel = sustainLevel;
                break;
                
            case RELEASE:
                currentLevel = releaseBase + currentLevel * releaseCoef;
                if (currentLevel <= 1e-3f) {
                    currentLevel = 0.f;
                    currentStage = IDLE;
                }
                break;
        }
        
        return currentLevel;
    }
    
    void reset() {
        currentStage = IDLE;
        currentLevel = 0.f;
        gateHigh = false;
    }

    Stage getCurrentStage() const { return currentStage; }
    float getCurrentLevel() const { return currentLevel; }
    bool isActive() const { return currentStage != IDLE; }
};

// Simple trigger/gate utilities
class TriggerHelper {
public:
    // Process momentary trigger from parameter
    static bool processTrigger(Param& param, float& lastValue) {
        float value = param.getValue();
        bool triggered = (value > 0.5f && lastValue <= 0.5f);
        lastValue = value;
        return triggered;
    }
    
    // Legacy: Process trigger with SchmittTrigger, param value, input, and threshold  
    static bool processTrigger(rack::dsp::SchmittTrigger& trigger, float paramValue, Input& input, float threshold = 1.f) {
        float combinedValue = paramValue;
        if (input.isConnected()) {
            combinedValue += input.getVoltage();
        }
        return trigger.process(combinedValue, threshold);
    }
    
    // Process toggle button
    static bool processToggle(Param& param, bool& lastPressed) {
        float value = param.getValue();
        bool pressed = (value > 0.5f);
        bool triggered = pressed && !lastPressed;
        lastPressed = pressed;
        return triggered;
    }
    
    // Process toggle with value and state (backward compatibility)
    static bool processToggle(float value, bool& lastPressed, bool& state) {
        bool pressed = (value > 0.5f);
        bool triggered = pressed && !lastPressed;
        if (triggered) {
            state = !state;
        }
        lastPressed = pressed;
        return triggered;
    }
    
    // Legacy: Process toggle with SchmittTrigger, param value, and state
    static bool processToggle(rack::dsp::SchmittTrigger& trigger, float paramValue, bool& state) {
        bool triggered = trigger.process(paramValue);
        if (triggered) {
            state = !state;
        }
        return triggered;
    }
    
    // CV trigger detection with hysteresis
    static bool processCVTrigger(Input& input, bool& lastState, float threshold = 1.f) {
        float voltage = input.getVoltage();
        bool currentState = voltage > threshold;
        bool triggered = currentState && !lastState;
        lastState = currentState;
        return triggered;
    }
};

}} // namespace shapetaker::dsp
