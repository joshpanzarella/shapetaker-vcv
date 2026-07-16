#pragma once
#include <rack.hpp>
#include <nanovg.h>

using namespace rack;

namespace shapetaker {
namespace graphics {

// ============================================================================
// VISUAL EFFECTS UTILITIES  
// ============================================================================

// Vintage panel aging effects
class VintageEffects {
public:
    // Draw subtle vignette around panel edges
    static void drawVignette(const widget::Widget::DrawArgs& args, float x, float y, 
                           float w, float h, float intensity = 0.3f) {
        NVGpaint vignette = nvgRadialGradient(args.vg, x + w * 0.5f, y + h * 0.5f,
                                            fminf(w, h) * 0.3f, fminf(w, h) * 0.8f,
                                            nvgRGBAf(0, 0, 0, 0), nvgRGBAf(0, 0, 0, intensity));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, x, y, w, h);
        nvgFillPaint(args.vg, vignette);
        nvgFill(args.vg);
    }
    
    // Draw patina (greenish aging) overlay
    static void drawPatina(const widget::Widget::DrawArgs& args, float x, float y, 
                          float w, float h, float intensity = 0.1f) {
        NVGpaint patina = nvgLinearGradient(args.vg, x, y, x + w, y + h,
                                          nvgRGBAf(0.086f, 0.118f, 0.071f, intensity), 
                                          nvgRGBAf(0.196f, 0.157f, 0.086f, intensity * 0.7f));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, x, y, w, h);
        nvgFillPaint(args.vg, patina);
        nvgFill(args.vg);
    }
    
    // Draw random micro-scratches
    static void drawScratches(const widget::Widget::DrawArgs& args, float x, float y, 
                            float w, float h, int count = 8, unsigned int seed = 12345u) {
        auto rnd = [&]() mutable {
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; 
            return (seed & 0xFFFF) / 65535.f; 
        };
        
        nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 1, 0.05f));
        nvgStrokeWidth(args.vg, 0.5f);
        
        for (int i = 0; i < count; ++i) {
            float x1 = x + rnd() * w;
            float y1 = y + rnd() * h;
            float dx = (rnd() - 0.5f) * w * 0.15f;
            float dy = (rnd() - 0.5f) * h * 0.15f;
            
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x1, y1);
            nvgLineTo(args.vg, x1 + dx, y1 + dy);
            nvgStroke(args.vg);
        }
    }
};

// CRT/TV display effects
class CRTEffects {
public:
    // Draw CRT-style phosphor glow
    static void drawPhosphorGlow(const widget::Widget::DrawArgs& args, Vec center, 
                               float radius, NVGcolor color, float intensity = 1.f) {
        // Multiple glow layers for realistic phosphor effect
        for (int layer = 0; layer < 3; layer++) {
            float layerRadius = radius * (1.f + layer * 0.3f);
            float layerAlpha = intensity * (0.8f - layer * 0.2f);
            
            NVGpaint glow = nvgRadialGradient(args.vg, center.x, center.y,
                                            layerRadius * 0.2f, layerRadius,
                                            nvgRGBAf(color.r, color.g, color.b, layerAlpha),
                                            nvgRGBAf(color.r, color.g, color.b, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, center.x, center.y, layerRadius);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);
        }
    }
    
    // Draw TV static effect
    static void drawTVStatic(const widget::Widget::DrawArgs& args, float x, float y,
                           float w, float h, float intensity = 0.1f, unsigned int seed = 0) {
        // Generate random noise pattern
        auto rnd = [&]() mutable {
            seed = seed * 1103515245 + 12345;
            return ((seed >> 16) & 0x7fff) / 32767.f;
        };
        
        int pixelSize = 2;
        for (int px = 0; px < w; px += pixelSize) {
            for (int py = 0; py < h; py += pixelSize) {
                if (rnd() < intensity) {
                    float brightness = rnd();
                    nvgFillColor(args.vg, nvgRGBAf(brightness, brightness, brightness, 0.3f));
                    nvgBeginPath(args.vg);
                    nvgRect(args.vg, x + px, y + py, pixelSize, pixelSize);
                    nvgFill(args.vg);
                }
            }
        }
    }
    
    // Draw color separation/chromatic aberration
    static void drawChromaticAberration(const widget::Widget::DrawArgs& args, 
                                       const std::string& text, Vec pos, 
                                       float fontSize, float separation = 1.f) {
        nvgFontSize(args.vg, fontSize);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        
        // Red channel (offset left)
        nvgFillColor(args.vg, nvgRGBAf(1, 0, 0, 0.7f));
        nvgText(args.vg, pos.x - separation, pos.y, text.c_str(), nullptr);
        
        // Blue channel (offset right)  
        nvgFillColor(args.vg, nvgRGBAf(0, 0, 1, 0.7f));
        nvgText(args.vg, pos.x + separation, pos.y, text.c_str(), nullptr);
        
        // Green channel (center)
        nvgFillColor(args.vg, nvgRGBAf(0, 1, 0, 1.f));
        nvgText(args.vg, pos.x, pos.y, text.c_str(), nullptr);
    }
};

}} // namespace shapetaker::graphics