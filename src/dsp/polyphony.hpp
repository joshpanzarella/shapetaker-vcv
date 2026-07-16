#pragma once
#include <rack.hpp>
#include <algorithm>
#include <array>
#include <initializer_list>

using namespace rack;

namespace shapetaker {
namespace dsp {

// ============================================================================
// POLYPHONY MANAGEMENT UTILITIES
// ============================================================================

/**
 * Standard polyphony configuration for Shapetaker modules
 * Provides consistent voice limits and channel management
 */
class PolyphonicProcessor {
public:
    static constexpr int MAX_VOICES = 16;  // Standard VCV Rack polyphony limit
    
private:
    int currentChannels = 1;
    
public:
    /**
     * Calculate the number of active polyphonic voices from an input
     * @param input Input port to analyze
     * @return Number of channels (1 to MAX_VOICES)
     */
    int getChannelCount(Input& input) {
        return std::min(std::max(input.getChannels(), 1), MAX_VOICES);
    }
    
    /**
     * Calculate the number of active polyphonic voices from multiple inputs
     * Uses the maximum channel count from all provided inputs
     * @param inputs Vector of input references to analyze
     * @return Number of channels (1 to MAX_VOICES)
     */
    int getChannelCount(const std::vector<std::reference_wrapper<Input>>& inputs) {
        int maxChannels = 1;
        for (Input& input : inputs) {
            if (input.isConnected()) {
                maxChannels = std::max(maxChannels, input.getChannels());
            }
        }
        return std::min(maxChannels, MAX_VOICES);
    }

    int getChannelCount(std::initializer_list<std::reference_wrapper<Input>> inputs) {
        int maxChannels = 1;
        for (auto& inputRef : inputs) {
            Input& input = inputRef.get();
            if (input.isConnected()) {
                maxChannels = std::max(maxChannels, input.getChannels());
            }
        }
        return std::min(maxChannels, MAX_VOICES);
    }
    
    /**
     * Update channel count and configure output channels accordingly
     * @param input Primary input to determine channel count from
     * @param outputs Vector of outputs to configure
     * @return Number of active channels
     */
    int updateChannels(Input& input, const std::vector<std::reference_wrapper<Output>>& outputs) {
        currentChannels = getChannelCount(input);
        
        // Configure all outputs to match
        for (Output& output : outputs) {
            output.setChannels(currentChannels);
        }
        
        return currentChannels;
    }
    
    /**
     * Update channel count from multiple inputs and configure outputs
     * @param inputs Vector of inputs to analyze
     * @param outputs Vector of outputs to configure  
     * @return Number of active channels
     */
    int updateChannels(const std::vector<std::reference_wrapper<Input>>& inputs,
                      const std::vector<std::reference_wrapper<Output>>& outputs) {
        currentChannels = getChannelCount(inputs);
        
        // Configure all outputs to match
        for (Output& output : outputs) {
            output.setChannels(currentChannels);
        }
        
        return currentChannels;
    }

    int updateChannels(std::initializer_list<std::reference_wrapper<Input>> inputs,
                      std::initializer_list<std::reference_wrapper<Output>> outputs) {
        currentChannels = getChannelCount(inputs);

        for (auto& outputRef : outputs) {
            outputRef.get().setChannels(currentChannels);
        }

        return currentChannels;
    }
    
    /**
     * Get the current number of active channels
     * @return Current channel count
     */
    int getCurrentChannels() const {
        return currentChannels;
    }
    
    /**
     * Get the maximum supported voice count
     * @return MAX_VOICES constant
     */
    static constexpr int getMaxVoices() {
        return MAX_VOICES;
    }
};

/**
 * Template helper for managing per-voice arrays
 * Automatically handles initialization and provides safe access
 */
template<typename T, int SIZE = PolyphonicProcessor::MAX_VOICES>
class VoiceArray {
private:
    std::array<T, SIZE> voices;
    
public:
    VoiceArray() {
        // Initialize all voices to default value
        for (auto& voice : voices) {
            voice = T{};
        }
    }
    
    /**
     * Access voice by index (safe, clamped to valid range)
     * @param channel Voice index (0 to SIZE-1)
     * @return Reference to voice data
     */
    T& operator[](int channel) {
        return voices[rack::math::clamp(channel, 0, SIZE - 1)];
    }
    
    const T& operator[](int channel) const {
        return voices[rack::math::clamp(channel, 0, SIZE - 1)];
    }
    
    /**
     * Get raw array for iteration
     * @return Pointer to first element
     */
    T* data() { return voices.data(); }
    const T* data() const { return voices.data(); }
    
    /**
     * Get array size
     * @return Number of voices in array
     */
    constexpr size_t size() const { return SIZE; }
    
    /**
     * Reset all voices to default state
     */
    void reset() {
        for (auto& voice : voices) {
            voice = T{};
        }
    }
    
    /**
     * Apply a function to all voices
     * @param func Function to apply (takes T& as parameter)
     */
    template<typename Func>
    void forEach(Func&& func) {
        for (auto& voice : voices) {
            func(voice);
        }
    }
    
    /**
     * Apply a function to active voices only
     * @param channels Number of active channels
     * @param func Function to apply (takes T& and int channel as parameters)
     */
    template<typename Func>
    void forEachActive(int channels, Func&& func) {
        channels = rack::math::clamp(channels, 0, SIZE);
        for (int ch = 0; ch < channels; ch++) {
            func(voices[ch], ch);
        }
    }
    
    /**
     * Apply a function to all voices with index
     * @param func Function to apply (takes T& and int channel as parameters)
     */
    template<typename Func>
    void forEachWithIndex(Func&& func) {
        for (int ch = 0; ch < SIZE; ch++) {
            func(voices[ch], ch);
        }
    }
};

/**
 * Helper for objects that need sample rate updates across multiple voices
 * Manages applying sample rate changes to arrays of DSP objects
 */
class SampleRateManager {
public:
    /**
     * Apply sample rate update to a VoiceArray of objects
     * @param voiceArray VoiceArray containing DSP objects
     * @param sampleRate New sample rate
     */
    template<typename T, int SIZE>
    static void updateSampleRate(VoiceArray<T, SIZE>& voiceArray, float sampleRate) {
        voiceArray.forEach([sampleRate](T& obj) {
            // Note: This assumes T has a setSampleRate method
            // Call sites should ensure this is true
            obj.setSampleRate(sampleRate);
        });
    }
    
    /**
     * Apply sample rate update to two VoiceArrays
     */
    template<typename T1, int SIZE1, typename T2, int SIZE2>
    static void updateSampleRate(float sampleRate, VoiceArray<T1, SIZE1>& voiceArray1, VoiceArray<T2, SIZE2>& voiceArray2) {
        updateSampleRate(voiceArray1, sampleRate);
        updateSampleRate(voiceArray2, sampleRate);
    }
    
    /**
     * Apply sample rate update to three VoiceArrays
     */
    template<typename T1, int SIZE1, typename T2, int SIZE2, typename T3, int SIZE3>
    static void updateSampleRate(float sampleRate, VoiceArray<T1, SIZE1>& voiceArray1, VoiceArray<T2, SIZE2>& voiceArray2, VoiceArray<T3, SIZE3>& voiceArray3) {
        updateSampleRate(voiceArray1, sampleRate);
        updateSampleRate(voiceArray2, sampleRate);
        updateSampleRate(voiceArray3, sampleRate);
    }
};

/**
 * Convenience aliases for common voice array types
 */
using FloatVoices = VoiceArray<float>;
using IntVoices = VoiceArray<int>; 
using BoolVoices = VoiceArray<bool>;

// Template deduction for DSP object arrays
template<typename T>
using DSPVoices = VoiceArray<T>;

}} // namespace shapetaker::dsp
