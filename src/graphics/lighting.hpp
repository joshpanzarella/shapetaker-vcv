#pragma once
#include <rack.hpp>
#include <cmath>

using namespace rack;

namespace shapetaker {
namespace graphics {

// ============================================================================
// LIGHTING UTILITIES
// ============================================================================

// RGB color structure for consistent color handling
struct RGBColor {
    float r, g, b;
    
    RGBColor(float red = 0.f, float green = 0.f, float blue = 0.f) : r(red), g(green), b(blue) {}
    
    NVGcolor toNVG(float alpha = 1.f) const {
        return nvgRGBAf(r, g, b, alpha);
    }
    
    // Operator overloads for color math
    RGBColor operator*(float brightness) const {
        return RGBColor(r * brightness, g * brightness, b * brightness);
    }
    
    RGBColor operator+(const RGBColor& other) const {
        return RGBColor(r + other.r, g + other.g, b + other.b);
    }
    
    RGBColor operator-(const RGBColor& other) const {
        return RGBColor(r - other.r, g - other.g, b - other.b);
    }
};

class LightingHelper {
public:
    // Chiaroscuro-inspired color progression (dramatic light/dark contrast)
    static RGBColor getChiaroscuroColor(float value, float maxBrightness = 1.f) {
        value = clamp(value, 0.f, 1.f);
        maxBrightness = clamp(maxBrightness, 0.f, 1.f);
        
        if (value < 0.5f) {
            // 0.0 to 0.5: Dark purple to bright blue-white
            float red = value * 0.6f * maxBrightness;
            float green = value * maxBrightness;
            float blue = maxBrightness;
            return RGBColor(red, green, blue);
        } else {
            // 0.5 to 1.0: Bright blue-purple to dark purple
            float red = maxBrightness;
            float green = 2.0f * (1.0f - value) * maxBrightness;
            float blue = maxBrightness * (1.7f - value * 0.7f);
            return RGBColor(red, green, blue);
        }
    }
    
    // Set RGB lights on a module
    static void setRGBLight(Module* module, int lightId, const RGBColor& color) {
        if (module) {
            module->lights[lightId].setBrightness(color.r);
            module->lights[lightId + 1].setBrightness(color.g);
            module->lights[lightId + 2].setBrightness(color.b);
        }
    }
    
    // VU meter color progression (green -> yellow -> red)
    static RGBColor getVUColor(float level) {
        level = clamp(level, 0.f, 1.f);
        if (level < 0.7f) {
            return RGBColor(0.f, level / 0.7f, 0.f); // Green
        } else if (level < 0.9f) {
            float blend = (level - 0.7f) / 0.2f;
            return RGBColor(blend, 1.f, 0.f); // Green to yellow
        } else {
            float blend = (level - 0.9f) / 0.1f;
            return RGBColor(1.f, 1.f - blend, 0.f); // Yellow to red
        }
    }
    
    // Transmutation-specific colors
    static RGBColor getTealColor() { return RGBColor(0.0f, 0.604f, 0.478f); } // #009A7A
    static RGBColor getPurpleColor() { return RGBColor(0.435f, 0.122f, 0.718f); } // #6F1FB7
    
    // Mix two colors with specified blend factor
    static RGBColor mixColors(const RGBColor& a, const RGBColor& b, float blend) {
        blend = clamp(blend, 0.f, 1.f);
        return RGBColor(
            a.r * (1.f - blend) + b.r * blend,
            a.g * (1.f - blend) + b.g * blend,
            a.b * (1.f - blend) + b.b * blend
        );
    }
    
    // Generate warm/cold color variations
    static RGBColor warmColor(const RGBColor& base, float warmth = 0.1f) {
        return RGBColor(
            clamp(base.r + warmth, 0.f, 1.f),
            base.g,
            clamp(base.b - warmth * 0.5f, 0.f, 1.f)
        );
    }
    
    static RGBColor coldColor(const RGBColor& base, float coldness = 0.1f) {
        return RGBColor(
            clamp(base.r - coldness * 0.5f, 0.f, 1.f),
            base.g,
            clamp(base.b + coldness, 0.f, 1.f)
        );
    }
    
    // HSV to RGB conversion
    static RGBColor hsvToRGB(float h, float s, float v) {
        h = fmod(h, 360.f) / 60.f;
        s = clamp(s, 0.f, 1.f);
        v = clamp(v, 0.f, 1.f);
        
        int i = (int)floor(h);
        float f = h - i;
        float p = v * (1.f - s);
        float q = v * (1.f - s * f);
        float t = v * (1.f - s * (1.f - f));
        
        switch (i % 6) {
            case 0: return RGBColor(v, t, p);
            case 1: return RGBColor(q, v, p);
            case 2: return RGBColor(p, v, t);
            case 3: return RGBColor(p, q, v);
            case 4: return RGBColor(t, p, v);
            case 5: return RGBColor(v, p, q);
            default: return RGBColor();
        }
    }
};

}} // namespace shapetaker::graphics
