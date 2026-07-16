#pragma once

#include <rack.hpp>
#include <string>

namespace shapetaker {
namespace dsp {

/**
 * Helper class for standardized parameter configuration
 * Provides common parameter types and configurations used across Shapetaker modules
 */
class ParameterHelper {
public:
    /**
     * Standard parameter configurations
     */
    
    // Gain/Level parameters (0-100%)
    static void configGain(rack::engine::Module* module, int paramId, const std::string& label, float defaultValue = 0.0f) {
        module->configParam(paramId, 0.0f, 1.0f, defaultValue, label, "%", 0.0f, 100.0f);
    }
    
    // VCA Gain with typical 0-200% range
    static void configVCAGain(rack::engine::Module* module, int paramId, const std::string& label = "VCA Gain", float defaultValue = 0.0f) {
        module->configParam(paramId, 0.0f, 2.0f, defaultValue, label, "%", 0.0f, 100.0f);
    }
    
    // Attenuverter (-100% to +100%)
    static void configAttenuverter(rack::engine::Module* module, int paramId, const std::string& label, float defaultValue = 0.0f) {
        module->configParam(paramId, -1.0f, 1.0f, defaultValue, label, "%", 0.0f, 100.0f);
    }
    
    // Drive/Distortion (0-100%)  
    static void configDrive(rack::engine::Module* module, int paramId, const std::string& label = "Drive", float defaultValue = 0.0f) {
        module->configParam(paramId, 0.0f, 1.0f, defaultValue, label, "%", 0.0f, 100.0f);
    }
    
    // Mix/Blend parameter (0-100%)
    static void configMix(rack::engine::Module* module, int paramId, const std::string& label = "Mix", float defaultValue = 0.0f) {
        module->configParam(paramId, 0.0f, 1.0f, defaultValue, label, "%", 0.0f, 100.0f);
    }
    
    // Frequency parameter with exponential scaling
    static void configFrequency(rack::engine::Module* module, int paramId, const std::string& label, 
                               float minHz, float maxHz, float defaultHz, float baseHz = 261.626f) {
        float minExp = std::log2(minHz / baseHz);
        float maxExp = std::log2(maxHz / baseHz);
        float defaultExp = std::log2(defaultHz / baseHz);
        module->configParam(paramId, minExp, maxExp, defaultExp, label, " Hz", 2.f, baseHz);
    }
    
    // Standard audio frequency range (20Hz - 20kHz, default middle C)
    static void configAudioFrequency(rack::engine::Module* module, int paramId, const std::string& label, float defaultHz = 261.626f) {
        configFrequency(module, paramId, label, 20.0f, 20000.0f, defaultHz);
    }
    
    // LFO frequency range (0.1Hz - 50Hz, default 1Hz)
    static void configLFOFrequency(rack::engine::Module* module, int paramId, const std::string& label, float defaultHz = 1.0f) {
        configFrequency(module, paramId, label, 0.1f, 50.0f, defaultHz);
    }
    
    // Resonance/Q parameter
    static void configResonance(rack::engine::Module* module, int paramId, const std::string& label = "Resonance", 
                               float minQ = 0.707f, float maxQ = 10.0f, float defaultQ = 0.707f) {
        module->configParam(paramId, minQ, maxQ, defaultQ, label);
    }
    
    // BPM parameter with snap
    static void configBPM(rack::engine::Module* module, int paramId, const std::string& label = "BPM",
                         float minBPM = 20.0f, float maxBPM = 200.0f, float defaultBPM = 120.0f) {
        module->configParam(paramId, minBPM, maxBPM, defaultBPM, label, " BPM");
        module->paramQuantities[paramId]->snapEnabled = true;
    }
    
    // Sequence length with snap
    static void configLength(rack::engine::Module* module, int paramId, const std::string& label,
                            int minLength = 1, int maxLength = 64, int defaultLength = 16) {
        module->configParam(paramId, (float)minLength, (float)maxLength, (float)defaultLength, label);
        module->paramQuantities[paramId]->snapEnabled = true;
    }
    
    // Momentary button
    static void configButton(rack::engine::Module* module, int paramId, const std::string& label) {
        module->configParam(paramId, 0.0f, 1.0f, 0.0f, label);
    }
    
    // Toggle switch
    static void configToggle(rack::engine::Module* module, int paramId, const std::string& label, bool defaultValue = false) {
        module->configParam(paramId, 0.0f, 1.0f, defaultValue ? 1.0f : 0.0f, label);
    }
    
    // Multi-position switch with labels
    static void configSwitch(rack::engine::Module* module, int paramId, const std::string& label,
                            const std::vector<std::string>& options, int defaultOption = 0) {
        module->configSwitch(paramId, 0.0f, (float)(options.size() - 1), (float)defaultOption, label, options);
    }
    
    // Discrete parameter with snap and no smoothing (for type selectors, etc.)
    static void configDiscrete(rack::engine::Module* module, int paramId, const std::string& label,
                              int minValue, int maxValue, int defaultValue) {
        module->configParam(paramId, (float)minValue, (float)maxValue, (float)defaultValue, label);
        module->paramQuantities[paramId]->snapEnabled = true;
        module->paramQuantities[paramId]->smoothEnabled = false;
    }
    
    // Pan parameter (-100% L to +100% R, 0% = center)
    static void configPan(rack::engine::Module* module, int paramId, const std::string& label = "Pan", float defaultValue = 0.0f) {
        module->configParam(paramId, -1.0f, 1.0f, defaultValue, label, "%", 0.0f, 100.0f);
    }
    
    // Envelope time parameter (exponential scaling in seconds)
    static void configTime(rack::engine::Module* module, int paramId, const std::string& label,
                          float minSeconds = 0.001f, float maxSeconds = 10.0f, float defaultSeconds = 0.1f) {
        float minLog = std::log10(minSeconds);
        float maxLog = std::log10(maxSeconds);
        float defaultLog = std::log10(defaultSeconds);
        module->configParam(paramId, minLog, maxLog, defaultLog, label, " s", 10.0f, 1.0f);
    }
    
    /**
     * Parameter value utilities
     */
    
    // Set parameter value programmatically
    static void setParameterValue(rack::engine::Module* module, int paramId, float value) {
        if (module->paramQuantities[paramId]) {
            module->paramQuantities[paramId]->setValue(value);
        }
    }
    
    // Get parameter value with optional CV modulation
    static float getParameterValue(rack::engine::Module* module, int paramId, 
                                  rack::engine::Input* cvInput = nullptr, float cvScale = 0.1f) {
        float value = module->params[paramId].getValue();
        
        if (cvInput && cvInput->isConnected()) {
            value += cvInput->getVoltage() * cvScale;
        }
        
        return value;
    }
    
    // Get clamped parameter value with CV
    static float getClampedParameterValue(rack::engine::Module* module, int paramId,
                                        float minValue, float maxValue,
                                        rack::engine::Input* cvInput = nullptr, float cvScale = 0.1f) {
        float value = getParameterValue(module, paramId, cvInput, cvScale);
        return rack::math::clamp(value, minValue, maxValue);
    }
    
    /**
     * Common I/O configurations
     */
    
    // Standard audio input
    static void configAudioInput(rack::engine::Module* module, int inputId, const std::string& label) {
        module->configInput(inputId, label);
    }
    
    // Standard audio output  
    static void configAudioOutput(rack::engine::Module* module, int outputId, const std::string& label) {
        module->configOutput(outputId, label);
    }
    
    // CV input (±5V or ±10V)
    static void configCVInput(rack::engine::Module* module, int inputId, const std::string& label) {
        module->configInput(inputId, label);
    }
    
    // Gate/Trigger input
    static void configGateInput(rack::engine::Module* module, int inputId, const std::string& label) {
        module->configInput(inputId, label);
    }
    
    // Clock input
    static void configClockInput(rack::engine::Module* module, int inputId, const std::string& label) {
        module->configInput(inputId, label);
    }
    
    // Polyphonic CV output
    static void configPolyCVOutput(rack::engine::Module* module, int outputId, const std::string& label) {
        module->configOutput(outputId, label);
    }
    
    // Polyphonic gate output
    static void configPolyGateOutput(rack::engine::Module* module, int outputId, const std::string& label) {
        module->configOutput(outputId, label);
    }
};

/**
 * Common parameter configurations as constants for consistency
 */
namespace StandardParams {
    // Common ranges
    constexpr float GAIN_MIN = 0.0f;
    constexpr float GAIN_MAX = 1.0f;
    constexpr float GAIN_DEFAULT = 0.0f;
    
    constexpr float ATTENUVERTER_MIN = -1.0f;
    constexpr float ATTENUVERTER_MAX = 1.0f;
    constexpr float ATTENUVERTER_DEFAULT = 0.0f;
    
    constexpr float RESONANCE_MIN = 0.707f;
    constexpr float RESONANCE_MAX = 10.0f;
    constexpr float RESONANCE_DEFAULT = 0.707f;
    
    constexpr float BPM_MIN = 20.0f;
    constexpr float BPM_MAX = 200.0f;
    constexpr float BPM_DEFAULT = 120.0f;
    
    // Common CV scaling factors
    constexpr float CV_SCALE_1V = 0.1f;     // 10V → 1.0
    constexpr float CV_SCALE_5V = 0.2f;     // 5V → 1.0  
    constexpr float CV_SCALE_OCT = 1.0f;    // 1V/oct scaling
}

} // namespace dsp
} // namespace shapetaker