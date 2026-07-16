#pragma once
#include <rack.hpp>
#include <cmath>

using namespace rack;

namespace shapetaker {
namespace dsp {

// ============================================================================
// PITCH AND TUNING UTILITIES
// ============================================================================

class PitchHelper {
public:
    // ========================================================================
    // QUANTIZATION - Snap values to musical intervals
    // ========================================================================

    // Quantize voltage to discrete octave steps
    // voltage: Input voltage to quantize
    // minOct: Minimum octave (default -2)
    // maxOct: Maximum octave (default +2)
    // Returns: Voltage snapped to nearest whole octave
    static float quantizeToOctave(float voltage, float minOct = -2.f, float maxOct = 2.f) {
        float clamped = rack::math::clamp(voltage, minOct, maxOct);
        return std::round(clamped);
    }

    // Quantize to semitone steps within a range
    // semitones: Input semitone offset to quantize
    // range: Maximum semitone range in both directions (default 24 = 4 octaves)
    // Returns: Semitone offset snapped to nearest semitone
    static float quantizeToSemitone(float semitones, float range = 24.f) {
        float clamped = rack::math::clamp(semitones, -range, range);
        return std::round(clamped);
    }

    // Quantize to cent steps (100 cents = 1 semitone)
    // cents: Input cent offset to quantize
    // range: Maximum cent range in both directions (default 50 = half semitone)
    // Returns: Cent offset snapped to nearest cent
    static float quantizeToCent(float cents, float range = 50.f) {
        float clamped = rack::math::clamp(cents, -range, range);
        return std::round(clamped);
    }

    // ========================================================================
    // CONVERSION - Transform between different pitch representations
    // ========================================================================

    // Convert semitones to voltage (V/Oct standard)
    // semitones: Semitone offset (12 = 1 octave)
    // Returns: Voltage offset in V/Oct
    static float semitonesToVoltage(float semitones) {
        return semitones / 12.f;
    }

    // Convert voltage to semitones (V/Oct standard)
    // voltage: Voltage offset in V/Oct
    // Returns: Semitone offset (12 per octave)
    static float voltageToSemitones(float voltage) {
        return voltage * 12.f;
    }

    // Convert cents to voltage (V/Oct standard)
    // cents: Cent offset (100 = 1 semitone, 1200 = 1 octave)
    // Returns: Voltage offset in V/Oct
    static float centsToVoltage(float cents) {
        return cents / 1200.f;
    }

    // Convert voltage to cents (V/Oct standard)
    // voltage: Voltage offset in V/Oct
    // Returns: Cent offset (100 per semitone)
    static float voltageToCents(float voltage) {
        return voltage * 1200.f;
    }

    // Convert frequency to voltage (V/Oct, with reference)
    // freq: Frequency in Hz
    // refFreq: Reference frequency (default 261.626Hz = C4)
    // Returns: Voltage in V/Oct relative to reference
    static float frequencyToVoltage(float freq, float refFreq = 261.626f) {
        if (freq <= 0.f || refFreq <= 0.f) return 0.f;
        return std::log2(freq / refFreq);
    }

    // Convert voltage to frequency (V/Oct, with reference)
    // voltage: Voltage in V/Oct
    // refFreq: Reference frequency (default 261.626Hz = C4)
    // Returns: Frequency in Hz
    static float voltageToFrequency(float voltage, float refFreq = 261.626f) {
        return refFreq * std::pow(2.f, voltage);
    }

    // ========================================================================
    // MUSICAL SCALES - Quantize to specific scale patterns
    // ========================================================================

    // Quantize voltage to chromatic scale (all 12 semitones)
    // voltage: Input voltage
    // Returns: Voltage quantized to nearest semitone
    static float quantizeChromatic(float voltage) {
        float semitones = std::round(voltage * 12.f);
        return semitones / 12.f;
    }

    // Quantize voltage to major scale (7 notes)
    // voltage: Input voltage
    // Returns: Voltage quantized to C major scale
    static float quantizeMajorScale(float voltage) {
        // Major scale intervals: 0, 2, 4, 5, 7, 9, 11 (semitones)
        static const int scaleIntervals[] = {0, 2, 4, 5, 7, 9, 11};
        static const int scaleSize = 7;

        float octaves = std::floor(voltage);
        float fractional = voltage - octaves;
        float semitones = fractional * 12.f;

        // Find nearest scale degree
        int nearestInterval = 0;
        float minDistance = 12.f;
        for (int i = 0; i < scaleSize; i++) {
            float distance = std::abs(semitones - scaleIntervals[i]);
            if (distance < minDistance) {
                minDistance = distance;
                nearestInterval = scaleIntervals[i];
            }
        }

        return octaves + nearestInterval / 12.f;
    }

    // Quantize voltage to minor scale (7 notes)
    // voltage: Input voltage
    // Returns: Voltage quantized to C minor scale
    static float quantizeMinorScale(float voltage) {
        // Natural minor scale intervals: 0, 2, 3, 5, 7, 8, 10 (semitones)
        static const int scaleIntervals[] = {0, 2, 3, 5, 7, 8, 10};
        static const int scaleSize = 7;

        float octaves = std::floor(voltage);
        float fractional = voltage - octaves;
        float semitones = fractional * 12.f;

        // Find nearest scale degree
        int nearestInterval = 0;
        float minDistance = 12.f;
        for (int i = 0; i < scaleSize; i++) {
            float distance = std::abs(semitones - scaleIntervals[i]);
            if (distance < minDistance) {
                minDistance = distance;
                nearestInterval = scaleIntervals[i];
            }
        }

        return octaves + nearestInterval / 12.f;
    }

    // Quantize voltage to pentatonic scale (5 notes)
    // voltage: Input voltage
    // Returns: Voltage quantized to C major pentatonic scale
    static float quantizePentatonic(float voltage) {
        // Major pentatonic scale intervals: 0, 2, 4, 7, 9 (semitones)
        static const int scaleIntervals[] = {0, 2, 4, 7, 9};
        static const int scaleSize = 5;

        float octaves = std::floor(voltage);
        float fractional = voltage - octaves;
        float semitones = fractional * 12.f;

        // Find nearest scale degree
        int nearestInterval = 0;
        float minDistance = 12.f;
        for (int i = 0; i < scaleSize; i++) {
            float distance = std::abs(semitones - scaleIntervals[i]);
            if (distance < minDistance) {
                minDistance = distance;
                nearestInterval = scaleIntervals[i];
            }
        }

        return octaves + nearestInterval / 12.f;
    }

    // ========================================================================
    // TUNING - Alternative tuning systems
    // ========================================================================

    // Convert voltage from 12-TET to microtonal tuning
    // voltage: Input voltage in 12-TET (12 tone equal temperament)
    // divisions: Number of divisions per octave (e.g., 24 for quarter tones)
    // Returns: Voltage quantized to microtonal system
    static float quantizeMicrotonal(float voltage, int divisions) {
        if (divisions <= 0) return voltage;
        float steps = std::round(voltage * divisions);
        return steps / divisions;
    }

    // Apply just intonation correction (perfect fifths and thirds)
    // voltage: Input voltage
    // Returns: Voltage adjusted for just intonation intervals
    static float applyJustIntonation(float voltage) {
        // Just intonation ratios for major scale (in cents from equal temperament)
        // These are the differences from 12-TET
        static const float justCents[] = {
            0.f,      // Tonic (no change)
            -11.73f,  // Major second (slightly flat)
            13.69f,   // Major third (slightly sharp)
            -15.64f,  // Perfect fourth (slightly flat)
            1.96f,    // Perfect fifth (slightly sharp)
            -13.69f,  // Major sixth (slightly flat)
            11.73f    // Major seventh (slightly sharp)
        };

        float octaves = std::floor(voltage);
        float fractional = voltage - octaves;
        int semitone = (int)std::round(fractional * 12.f) % 7;

        float adjustment = justCents[semitone] / 1200.f;  // Convert cents to voltage
        return voltage + adjustment;
    }
};

}} // namespace shapetaker::dsp
