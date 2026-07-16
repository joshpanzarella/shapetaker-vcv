#pragma once
// Shapetaker Utilities - Reorganized by Function
// This header provides unified access to all Shapetaker utility functions

// DSP Utilities
#include "dsp/filters.hpp"
#include "dsp/parameters.hpp"
#include "dsp/oscillators.hpp"
#include "dsp/envelopes.hpp"
#include "dsp/audio.hpp"
#include "dsp/effects.hpp"
#include "dsp/polyphony.hpp"
#include "dsp/delays.hpp"
#include "dsp/pitch.hpp"

// Graphics Utilities
#include "graphics/drawing.hpp"
#include "graphics/lighting.hpp"
#include "graphics/effects.hpp"

// UI Utilities
#include "ui/widgets.hpp"
#include "ui/helpers.hpp"
#include "ui/layout.hpp"
#include "ui/theme.hpp"

// Backward compatibility aliases for existing code
// These will be removed in a future version

namespace shapetaker {
    // Legacy aliases for existing shapetaker:: usage
    using BiquadFilter = dsp::BiquadFilter;
    using MorphingFilter = dsp::MorphingFilter;
    using OscillatorHelper = dsp::OscillatorHelper;
    using EnvelopeGenerator = dsp::EnvelopeGenerator;
    using TriggerHelper = dsp::TriggerHelper;
    using AudioProcessor = dsp::AudioProcessor;
    using SidechainDetector = dsp::SidechainDetector;
    using DistortionEngine = dsp::DistortionEngine;
    using PolyphonicProcessor = dsp::PolyphonicProcessor;
    using SampleRateManager = dsp::SampleRateManager;
    using ParameterHelper = dsp::ParameterHelper;
    using ChorusEffect = dsp::ChorusEffect;
    using PhaserEffect = dsp::PhaserEffect;
    using ShimmerDelay = dsp::ShimmerDelay;
    using EnvelopeFollower = dsp::EnvelopeFollower;
    using FastSmoother = dsp::FastSmoother;
    using PitchHelper = dsp::PitchHelper;

    // Convenience aliases for voice arrays
    template<typename T, int SIZE = dsp::PolyphonicProcessor::MAX_VOICES>
    using VoiceArray = dsp::VoiceArray<T, SIZE>;
    using FloatVoices = dsp::FloatVoices;
    using IntVoices = dsp::IntVoices;
    using BoolVoices = dsp::BoolVoices;
    
    using RGBColor = graphics::RGBColor;
    using LightingHelper = graphics::LightingHelper;
    using ThemeManager = ui::ThemeManager;
    
    // UI widget aliases
    using LargeJewelLED = ui::LargeJewelLED;
    using SmallJewelLED = ui::SmallJewelLED;
    using VUMeterWidget = ui::VUMeterWidget;
    using VintageVUMeterWidget = ui::VintageVUMeterWidget;
    using VisualizerWidget = ui::VisualizerWidget;
    using ButtonHelper = ui::ButtonHelper;
    using CVHelper = ui::CVHelper;
}

// Global convenience functions (backward compatibility)
namespace st {
    // Forward drawing functions to new graphics namespace
    using shapetaker::graphics::drawVoiceCountDots;
    using shapetaker::graphics::drawAlchemicalSymbol;
    using shapetaker::graphics::isValidSymbolId;
    using shapetaker::graphics::drawVignettePatinaScratches;
    using shapetaker::graphics::wrapText;
    
    // Symbol constants
    // Total number of available alchemical symbols
    constexpr int SymbolCount = 100;
}

// Global convenience functions for backward compatibility
using shapetaker::graphics::wrapText;

// TODO: Remove these legacy includes in future version
// For now, they provide compatibility during transition
