#pragma once
#include <rack.hpp>
#include <nanovg.h>
#include <cmath>

using namespace rack;

namespace shapetaker {
namespace graphics {

// ============================================================================
// DRAWING UTILITIES
// ============================================================================

// Draw small voice-count dots arranged around a center at a given radius
// count is clamped to [0,6]. dotRadius in pixels.
void drawVoiceCountDots(const widget::Widget::DrawArgs& args, Vec center, int count,
                        float radius, float dotRadius, NVGcolor color);

// Draw alchemical symbols with various customization options
void drawAlchemicalSymbol(const widget::Widget::DrawArgs& args, Vec pos, int symbolId,
                         NVGcolor color = nvgRGBA(255,255,255,255), 
                         float size = 10.0f, float strokeWidth = 1.0f);

// Check if symbol ID is valid (0..SymbolCount-1)
bool isValidSymbolId(int symbolId);

// Get symbol count
constexpr int getSymbolCount() { return 80; }

// Draw vintage panel effects: vignette + patina + micro-scratches
void drawVignettePatinaScratches(const widget::Widget::DrawArgs& args,
                                float x, float y, float w, float h,
                                float cornerRadius = 0.0f, int scratchCount = 12,
                                NVGcolor vignette1 = nvgRGBA(0,0,0,0),
                                NVGcolor vignette2 = nvgRGBA(0,0,0,25),
                                int patinaLayers = 3, float scratchAlpha = 0.3f,
                                int scratchVariations = 4, unsigned int seed = 12345u);

// Draw subtle micro-scratches with highlight/shadow for depth.
void drawMicroScratches(const widget::Widget::DrawArgs& args,
                        float x, float y, float w, float h,
                        int scratchCount = 7, unsigned int seed = 12345u,
                        float intensity = 1.0f);

// Helper function for drawing vintage text effects
void drawVintageText(const widget::Widget::DrawArgs& args, Vec pos, 
                    const std::string& text, NVGcolor color, float fontSize,
                    bool addGlow = false, float glowRadius = 2.0f);

// Draw CRT-style scanlines effect
void drawScanlines(const widget::Widget::DrawArgs& args, float x, float y, 
                  float w, float h, float lineSpacing = 2.0f, float alpha = 0.1f);

// Draw phosphor glow effect for CRT displays
void drawPhosphorGlow(const widget::Widget::DrawArgs& args, Vec center, 
                     float radius, NVGcolor color, float intensity = 1.0f);

/**
 * Draw subtle CRT shadow mask (RGB triads) over a region
 * triadWidth is the pixel width of one R+G+B set; alpha controls overall opacity.
 */
void drawShadowMask(const widget::Widget::DrawArgs& args, float x, float y,
                   float w, float h, float triadWidth = 3.0f, float alpha = 0.04f);

/**
 * Draw soft glass reflections over a screen rectangle for depth and realism
 * intensity controls overall reflection strength.
 */
void drawGlassReflections(const widget::Widget::DrawArgs& args, float x, float y,
                         float w, float h, float intensity = 0.12f);

/**
 * Wrap text to fit within a maximum width, breaking at word boundaries
 * Returns vector of strings, each fitting within maxWidth
 */
std::vector<std::string> wrapText(const std::string& text, float maxWidth, NVGcontext* vg);

}} // namespace shapetaker::graphics
