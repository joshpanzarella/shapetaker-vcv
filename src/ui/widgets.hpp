#pragma once
#include <rack.hpp>
#include <nanovg.h>
#include <cmath>
#include <vector>
#include "../graphics/drawing.hpp"
#include "../graphics/lighting.hpp"

// Forward declaration so bezels can load bundled assets without including plugin.hpp here
extern Plugin* pluginInstance;

using namespace rack;

namespace shapetaker {
namespace ui {

// ============================================================================
// PANEL OVERLAYS
// ============================================================================

// Full-module randomized wear for a cohesive vintage look. Add early so it
// sits beneath the controls, with its box sized to the panel.
struct PanelWearOverlay : TransparentWidget {
    unsigned int baseSeed = 0u;

    PanelWearOverlay() {
        baseSeed = rack::random::u32();
    }

    void draw(const DrawArgs& args) override {
        float w = box.size.x;
        float h = box.size.y;
        // Sparse randomized wear with paired highlight/shadow scratches and soft scuffs.
        int scratchCount = rack::math::clamp(static_cast<int>((w * h) / 4700.0f), 14, 56);
        graphics::drawMicroScratches(args, 0.0f, 0.0f, w, h, scratchCount, baseSeed, 0.915f);
    }
};

// ============================================================================
// CUSTOM LED WIDGETS
// ============================================================================

// Global brightness cap for jewel LEDs.  Lower values let the SVG facets
// show through at full drive, like a guitar-amp power jewel.
static constexpr float kJewelMaxBrightness = 0.99f;

static inline float jewelBrightness(NVGcolor color) {
    return std::max({color.r, color.g, color.b, color.a});
}

static inline void drawAmpJewelLensShade(const widget::Widget::DrawArgs& args, Vec boxSize, float lensFraction = 0.40f) {
    const float cx = boxSize.x * 0.5f;
    const float cy = boxSize.y * 0.5f;
    const float lensRadius = lensFraction * std::min(boxSize.x, boxSize.y);

    NVGpaint shade = nvgRadialGradient(args.vg, cx, cy, lensRadius * 0.10f, lensRadius,
        nvgRGBA(12, 12, 15, 142), nvgRGBA(2, 2, 4, 222));
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, cx, cy, lensRadius);
    nvgFillPaint(args.vg, shade);
    nvgFill(args.vg);

    nvgBeginPath(args.vg);
    nvgCircle(args.vg, cx, cy, lensRadius * 0.96f);
    nvgStrokeWidth(args.vg, std::max(0.65f, lensRadius * 0.12f));
    nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, 138));
    nvgStroke(args.vg);
}

static inline void drawAmpJewelLight(const widget::Widget::DrawArgs& args, Vec boxSize, NVGcolor color,
                                     float lensFraction = 0.36f, float bulbFraction = 0.46f) {
    float brightness = jewelBrightness(color);
    if (brightness <= 1e-3f)
        return;

    const float minSize = std::min(boxSize.x, boxSize.y);
    const float cx = boxSize.x * 0.5f;
    const float cy = boxSize.y * 0.5f;
    const float lensRadius = lensFraction * minSize;
    const float bulbRadius = lensRadius * (bulbFraction * 1.08f);
    const float glow = clamp(std::pow(brightness, 0.58f) * 1.70f, 0.f, 1.f);
    const float coreGlow = clamp(std::pow(brightness, 0.45f) * 1.85f, 0.f, 1.f);

    nvgSave(args.vg);
    nvgScissor(args.vg, cx - lensRadius, cy - lensRadius, lensRadius * 2.f, lensRadius * 2.f);

    // Faint inner lens tint, not a full-lens wash.
    NVGcolor tintInner = color;
    tintInner.a = 0.28f * glow;
    NVGcolor tintOuter = color;
    tintOuter.a = 0.0f;
    NVGpaint tint = nvgRadialGradient(args.vg, cx, cy, bulbRadius * 0.75f, lensRadius, tintInner, tintOuter);
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, cx, cy, lensRadius);
    nvgFillPaint(args.vg, tint);
    nvgFill(args.vg);

    // Diffused lamp glow around the bulb. This brightens the jewel without
    // making the central filament larger or washing onto the panel.
    NVGcolor diffuseInner = color;
    diffuseInner.a = 0.46f * glow;
    NVGcolor diffuseOuter = color;
    diffuseOuter.a = 0.06f * glow;
    NVGpaint diffuse = nvgRadialGradient(args.vg, cx, cy, bulbRadius * 0.48f, lensRadius * 0.94f,
        diffuseInner, diffuseOuter);
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, cx, cy, lensRadius * 0.92f);
    nvgFillPaint(args.vg, diffuse);
    nvgFill(args.vg);

    // Bright center bulb for zoomed-out readability.
    NVGcolor bulbInner = nvgRGBAf(
        std::min(color.r * 1.35f + 0.22f, 1.f),
        std::min(color.g * 1.35f + 0.22f, 1.f),
        std::min(color.b * 1.35f + 0.22f, 1.f),
        1.0f * coreGlow);
    NVGcolor bulbOuter = color;
    bulbOuter.a = 0.0f;
    NVGpaint bulb = nvgRadialGradient(args.vg, cx, cy, 0.f, bulbRadius, bulbInner, bulbOuter);
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, cx, cy, bulbRadius);
    nvgFillPaint(args.vg, bulb);
    nvgFill(args.vg);

    NVGcolor filament = nvgRGBAf(1.f, 1.f, 1.f, 0.62f * coreGlow);
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, cx, cy, std::max(1.0f, bulbRadius * 0.18f));
    nvgFillColor(args.vg, filament);
    nvgFill(args.vg);

    nvgResetScissor(args.vg);
    nvgRestore(args.vg);
}

// Base class for jewel LEDs with RGB mixing and layered effects
template<int SIZE>
class JewelLEDBase : public ModuleLightWidget {
protected:
    NVGcolor getLayeredColor(float r, float g, float b, float maxBrightness) const {
        return nvgRGBAf(r, g, b, fmaxf(r, fmaxf(g, b)) * maxBrightness);
    }
    
    void drawJewelLayers(const DrawArgs& args, float r, float g, float b, float maxBrightness) {
        if (maxBrightness < 0.01f) {
            drawOffState(args);
            return;
        }
        
        float cx = box.size.x * 0.5f;
        float cy = box.size.y * 0.5f;
        float radius = 0.32f * std::min(box.size.x, box.size.y);
        
        // Small internal bulb: the jewel lens is larger than the visible lamp.
        NVGpaint outerGlow = nvgRadialGradient(args.vg, cx, cy, radius * 0.35f, radius,
            nvgRGBAf(r, g, b, 0.22f * maxBrightness), nvgRGBAf(r, g, b, 0.0f));
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius);
        nvgFillPaint(args.vg, outerGlow);
        nvgFill(args.vg);
        
        // Layer 2: Warm center
        NVGpaint mediumRing = nvgRadialGradient(args.vg, cx, cy, 0.f, radius * 0.62f,
            nvgRGBAf(r * 1.2f, g * 1.2f, b * 1.2f, 0.58f * maxBrightness),
            nvgRGBAf(r, g, b, 0.0f));
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius * 0.62f);
        nvgFillPaint(args.vg, mediumRing);
        nvgFill(args.vg);
        
        // Layer 3: Tiny bright filament
        NVGpaint innerCore = nvgRadialGradient(args.vg, cx, cy, 0, radius * 0.22f,
            nvgRGBAf(1, 1, 1, 0.50f * maxBrightness), 
            nvgRGBAf(r * 1.4f, g * 1.4f, b * 1.4f, 0.0f));
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius * 0.22f);
        nvgFillPaint(args.vg, innerCore);
        nvgFill(args.vg);
    }
    
    // Override the default VCV Rack halo so glow stays tightly inside the lens.
    // Real guitar-amp jewel cases emit a faint rim glow but do NOT illuminate the bezel.
    void drawHalo(const DrawArgs& args) override {
        // Amp-style jewels should not spill colored light onto the panel.
    }

    void drawOffState(const DrawArgs& args) {
        float cx = box.size.x * 0.5f;
        float cy = box.size.y * 0.5f;
        float radius = 0.5f * std::min(box.size.x, box.size.y);
        
        // Dark background (no hard rim to keep bezel clean)
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius * 0.8f);
        nvgFillColor(args.vg, nvgRGBA(24, 24, 28, 160));
        nvgFill(args.vg);
    }
    
public:
    JewelLEDBase() {
        // Default; specific LEDs will set mm-based sizes in their constructors
        box.size = Vec(SIZE, SIZE);
    }
    
    void drawLight(const DrawArgs& args) override {
        if (!module) return;
        
        // Get the RGB light values from the module's lights array
        float brightness[3] = {};
        for (int i = 0; i < 3; i++) {
            if (firstLightId + i < (int)module->lights.size()) {
                brightness[i] = module->lights[firstLightId + i].getBrightness();
            }
        }
        NVGcolor color = nvgRGBAf(
            brightness[0] * kJewelMaxBrightness,
            brightness[1] * kJewelMaxBrightness,
            brightness[2] * kJewelMaxBrightness,
            fmaxf(brightness[0], fmaxf(brightness[1], brightness[2])) * kJewelMaxBrightness);
        drawAmpJewelLight(args, box.size, color, 0.36f, 0.48f);
    }
};

// Specific LED sizes
class LargeJewelLED : public JewelLEDBase<30> {
public:
    LargeJewelLED() {
        bgColor = nvgRGBA(0, 0, 0, 0);
        borderColor = nvgRGBA(0, 0, 0, 0);
        // Add RGB base colors for the MultiLightWidget
        addBaseColor(nvgRGB(255, 0, 0));   // Red
        addBaseColor(nvgRGB(0, 255, 0));   // Green  
        addBaseColor(nvgRGB(0, 0, 255));   // Blue
        // Hardware-friendly lens: 12 mm
        box.size = mm2px(Vec(12.f, 12.f));
    }
};

class SmallJewelLED : public JewelLEDBase<15> {
public:
    SmallJewelLED() {
        bgColor = nvgRGBA(0, 0, 0, 0);
        borderColor = nvgRGBA(0, 0, 0, 0);
        // Add RGB base colors for the MultiLightWidget
        addBaseColor(nvgRGB(255, 0, 0));   // Red
        addBaseColor(nvgRGB(0, 255, 0));   // Green  
        addBaseColor(nvgRGB(0, 0, 255));   // Blue
        // Hardware-friendly lens: 10 mm
        box.size = mm2px(Vec(10.f, 10.f));
    }
};

// Single-channel white small jewel LED.
// Uses one module light value (no RGB channel coupling) while keeping jewel styling.
class WhiteJewelLEDSmall : public ModuleLightWidget {
private:
    widget::SvgWidget* sw = nullptr;

public:
    WhiteJewelLEDSmall() {
        // Match existing small jewel footprint.
        box.size = Vec(10.f, 10.f);

        sw = new widget::SvgWidget;
        std::shared_ptr<Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_small.svg"));
        if (svg) {
            sw->setSvg(svg);
            // Source art is larger than this footprint; center it.
            sw->box.pos = Vec(-2.5f, -2.5f);
            addChild(sw);
        }

        // One white channel only.
        addBaseColor(nvgRGB(255, 255, 255));
    }

    void drawLight(const DrawArgs& args) override {
        float brightness = std::max({color.r, color.g, color.b, color.a});
        if (brightness <= 1e-3f) {
            return;
        }

        drawAmpJewelLight(args, box.size, color, 0.36f, 0.50f);
    }

    void drawHalo(const DrawArgs& args) override {
        // Amp-style jewel lenses do not cast a panel halo.
    }

    void draw(const DrawArgs& args) override {
        if (children.empty()) {
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 5.f, 5.f, 4.8f);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 5.f, 5.f, 3.2f);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }

        // Draw jewel body/facets first.
        widget::Widget::draw(args);

        nvgSave(args.vg);
        nvgTranslate(args.vg, -2.5f, -2.5f);
        drawAmpJewelLensShade(args, Vec(15.f, 15.f), 0.38f);
        nvgRestore(args.vg);

        // Overlay illuminated white lens.
        nvgSave(args.vg);
        nvgGlobalAlpha(args.vg, 0.95f);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        drawLight(args);
        nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
        drawHalo(args);
        nvgRestore(args.vg);
    }
};

// Jewel LED with integrated brass bezel (keeps lens smaller than the ring)
class SmallBezelJewelLED : public SmallJewelLED {
private:
    widget::SvgWidget* bezel = nullptr;

public:
    SmallBezelJewelLED() {
        // Overall footprint slightly larger so the bezel frames the lens
        box.size = mm2px(Vec(9.f, 9.f));

        bezel = new widget::SvgWidget;
        bezel->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ui/brass_bezel_small.svg")));
        bezel->box.size = box.size;
        bezel->box.pos = Vec(0.f, 0.f);
        this->addChild(bezel);
    }

    void onAdd(const widget::Widget::AddEvent& e) override {
        SmallJewelLED::onAdd(e);
        if (bezel && !bezel->parent) {
            addChild(bezel);
        }
    }

    void draw(const widget::Widget::DrawArgs& args) override {
        // Draw bezel via child; just render the lens smaller than the ring
        nvgSave(args.vg);
        const float s = 0.6f;
        nvgTranslate(args.vg, box.size.x * (1.f - s) * 0.5f, box.size.y * (1.f - s) * 0.5f);
        nvgScale(args.vg, s, s);
        SmallJewelLED::draw(args);
        nvgRestore(args.vg);
    }
};

// Medium-sized LEDs (20px) for transmutation module
class MediumJewelLED : public JewelLEDBase<20> {
public:
    MediumJewelLED() {
        bgColor = nvgRGBA(0, 0, 0, 0);
        borderColor = nvgRGBA(0, 0, 0, 0);
        // Add RGB base colors for the MultiLightWidget
        addBaseColor(nvgRGB(255, 0, 0));   // Red
        addBaseColor(nvgRGB(0, 255, 0));   // Green  
        addBaseColor(nvgRGB(0, 0, 255));   // Blue
        // Hardware-friendly lens: 12 mm (matches large for prominent use)
        box.size = mm2px(Vec(12.f, 12.f));
    }
};

/**
 * Compact indicator light with an engraved brass bezel
 * Used for stage position LEDs on Torsion (gives hardware depth around the emitter)
 */
template<typename LIGHT = WhiteLight>
class BrassBezelSmallLight : public LIGHT {
private:
    widget::SvgWidget* bezel = nullptr;

public:
    BrassBezelSmallLight() {
        // Larger footprint for Torsion stage indicators.
        this->box.size = mm2px(Vec(7.0f, 7.0f));
        this->bgColor = nvgRGBA(0, 0, 0, 0);
        this->borderColor = nvgRGBA(0, 0, 0, 0);

        bezel = new widget::SvgWidget;
        bezel->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ui/brass_bezel_small.svg")));
        bezel->box.size = this->box.size;
        bezel->box.pos = Vec(0.f, 0.f);
        this->addChild(bezel);
    }

    void drawLight(const widget::Widget::DrawArgs& args) override {
        nvgSave(args.vg);

        // Keep the lit lens well inside the bezel aperture.
        const float minSize = std::min(this->box.size.x, this->box.size.y);
        const float centerX = this->box.size.x * 0.5f;
        const float centerY = this->box.size.y * 0.5f;
        const float cx = centerX;
        const float cy = centerY;
        const float radius = 0.26f * minSize;        // slightly larger lens within the bezel
        const float innerRadius = radius * 0.58f;
        const float aperture = 0.3f * minSize;       // clip box roughly matching the bezel opening

        // Clip lens drawing so nothing bleeds past the aperture
        nvgScissor(args.vg, cx - aperture, cy - aperture, aperture * 2.f, aperture * 2.f);

        // Determine brightness from the incoming light color; fall back to alpha if channels are zero
        float brightness = std::max({this->color.a, this->color.r, this->color.g, this->color.b});

        // Always draw a neutral “unlit” lens so all stages look consistent when off
        NVGcolor unlitOuter = nvgRGBAf(0.82f, 0.82f, 0.82f, 0.04f);
        NVGcolor unlitInner = nvgRGBAf(1.f, 1.f, 1.f, 0.08f);
        NVGpaint unlitPaint = nvgRadialGradient(args.vg, cx, cy, innerRadius * 0.9f, radius,
            unlitInner, unlitOuter);
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius);
        nvgFillPaint(args.vg, unlitPaint);
        nvgFill(args.vg);

        if (brightness > 1e-3f) {
            NVGcolor color = this->color;
            color.a = brightness * kJewelMaxBrightness;
            drawAmpJewelLight(args, this->box.size, color, 0.30f, 0.52f);
        }

        nvgResetScissor(args.vg);
        nvgRestore(args.vg);
    }

    void drawHalo(const widget::Widget::DrawArgs& args) override {
        // Brass-bezel indicators should read as small lamps, not panel glows.
    }

    void draw(const widget::Widget::DrawArgs& args) override {
        // Draw lens first, then bezel over the top to cover any overhang.
        drawAmpJewelLensShade(args, this->box.size, 0.30f);
        drawLight(args);
        drawHalo(args);
        widget::Widget::draw(args);
    }

    void drawLayer(const widget::Widget::DrawArgs& args, int layer) override {
        // Suppress LightWidget's default layer-1 light pass since this widget
        // already renders lens/halo explicitly in draw().
        if (layer == 1) {
            return;
        }
        widget::Widget::drawLayer(args, layer);
    }
};

// Teal-colored LED for Sequence A (pre-configured for teal color)
class TealJewelLEDMedium : public MediumJewelLED {
public:
    TealJewelLEDMedium() {
        // Override with teal color only
        baseColors.clear();
        addBaseColor(nvgRGB(0, 154, 122));   // Teal (#009A7A)
    }
};

// Purple-colored LED for Sequence B (pre-configured for purple color)
class PurpleJewelLEDMedium : public MediumJewelLED {
public:
    PurpleJewelLEDMedium() {
        // Override with purple color only
        baseColors.clear();
        addBaseColor(nvgRGB(111, 31, 183));  // Purple (#6F1FB7)
    }
};

// ============================================================================
// MEASUREMENT/DISPLAY WIDGETS
// ============================================================================

// VU meter with configurable face and needle graphics
class VUMeterWidget : public widget::Widget {
private:
    Module* module = nullptr;
    int paramId = -1;
    int lightId = -1;
    std::string faceSvgPath;
    std::string needleSvgPath;
    
public:
    VUMeterWidget(Module* m, int pId, int lId, const std::string& faceSvg, const std::string& needleSvg)
        : module(m), paramId(pId), lightId(lId), faceSvgPath(faceSvg), needleSvgPath(needleSvg) {
        box.size = Vec(60, 60); // Default size
    }
    
    void draw(const DrawArgs& args) override {
        // Draw VU meter face
        std::shared_ptr<window::Svg> faceSvg = APP->window->loadSvg(faceSvgPath);
        if (faceSvg && faceSvg->handle) {
            nvgSave(args.vg);
            nvgScale(args.vg, box.size.x / faceSvg->handle->width, box.size.y / faceSvg->handle->height);
            svgDraw(args.vg, faceSvg->handle);
            nvgRestore(args.vg);
        }
        
        // Draw needle based on parameter value
        if (module && paramId >= 0) {
            float value = module->params[paramId].getValue();
            drawNeedle(args, value);
        }
        
        // Update lighting if applicable
        if (module && lightId >= 0) {
            float level = module->params[paramId].getValue();
            auto color = graphics::LightingHelper::getVUColor(level);
            graphics::LightingHelper::setRGBLight(module, lightId, color);
        }
    }
    
private:
    void drawNeedle(const DrawArgs& args, float value) {
        float angle = rescale(value, 0.f, 1.f, -45.f, 45.f); // -45° to +45°
        Vec center = box.size.mult(0.5f);
        
        nvgSave(args.vg);
        nvgTranslate(args.vg, center.x, center.y);
        nvgRotate(args.vg, angle * M_PI / 180.f);
        
        // Draw needle as a line
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, 0);
        nvgLineTo(args.vg, 0, -box.size.y * 0.35f);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGB(220, 220, 220));
        nvgStroke(args.vg);
        
        nvgRestore(args.vg);
    }
};

// Vintage VU meter using a single SVG file with integrated meter and needle
class VintageVUMeterWidget : public widget::Widget {
private:
    Module* module = nullptr;
    int lightId = -1;
    std::string svgPath;
    float* screenBrightness = nullptr;

public:
    VintageVUMeterWidget(Module* m, int lId, const std::string& svg)
        : module(m), lightId(lId), svgPath(svg) {
        box.size = Vec(50, 50); // Default size
    }

    void setScreenBrightnessPointer(float* brightness) {
        screenBrightness = brightness;
    }

    void draw(const DrawArgs& args) override {
        // Draw the vintage VU meter SVG face (always, including module browser).
        std::shared_ptr<window::Svg> svg = APP->window->loadSvg(svgPath);
        if (svg && svg->handle) {
            nvgSave(args.vg);
            float sx = box.size.x / svg->handle->width;
            float sy = box.size.y / svg->handle->height;
            nvgScale(args.vg, sx, sy);
            svgDraw(args.vg, svg->handle);
            nvgRestore(args.vg);
        }

        if (module) {
            // Keep the warm screen illumination below the inverse-brightened
            // marks so mid brightness does not wash out the dial text.
            drawBacklightGlow(args);
        }

        drawScreenBrightnessOverlay(args);

        if (!module) return;

        // Draw animated needle based on VU level
        if (lightId >= 0 && lightId < (int)module->lights.size()) {
            float value = clamp(module->lights[lightId].getBrightness(), 0.f, 1.f);
            drawVUNeedle(args, value);
        }
    }
private:



    float getScreenBrightness() const {
        return screenBrightness ? clamp(*screenBrightness, 0.f, 1.f) : 1.f;
    }

    void drawScreenBrightnessOverlay(const DrawArgs& args) {
        float brightness = getScreenBrightness();
        float dim = 1.f - brightness;
        if (dim <= 0.002f) {
            return;
        }

        auto smoothstep = [](float edge0, float edge1, float x) {
            float t = clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
            return t * t * (3.f - 2.f * t);
        };

        float faceDimAlpha = std::pow(dim, 3.80f);
        float darkMarksAlpha = smoothstep(0.20f, 0.28f, brightness);
        float marksGlowAlpha = 1.f - smoothstep(0.22f, 0.38f, brightness);
        drawMeterSvgOverlay(args, "res/meters/vintage_vu_face_dim.svg", clamp(faceDimAlpha, 0.f, 1.f));
        drawMeterSvgOverlay(args, "res/meters/vintage_vu_marks_dark.svg", darkMarksAlpha);
        drawMeterSvgOverlay(args, "res/meters/vintage_vu_marks_glow.svg", marksGlowAlpha);
    }

    void drawMeterSvgOverlay(const DrawArgs& args, const char* svgAssetPath, float alpha) {
        if (alpha <= 0.002f) {
            return;
        }

        std::shared_ptr<window::Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, svgAssetPath));
        if (!svg || !svg->handle) {
            return;
        }

        nvgSave(args.vg);
        nvgGlobalAlpha(args.vg, alpha);
        nvgScale(args.vg, box.size.x / svg->handle->width, box.size.y / svg->handle->height);
        svgDraw(args.vg, svg->handle);
        nvgRestore(args.vg);
    }



    void drawBacklightGlow(const DrawArgs& args) {
        float w = box.size.x;
        float h = box.size.y;
        float cx = w * 0.5f;
        float cy = h * 0.45f;  // Center on the meter face
        float radius = std::max(w, h) * 0.38f;
        float brightness = getScreenBrightness();

        // Warm circular backlight glow — gradient fades to transparent naturally
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius);
        NVGpaint glow = nvgRadialGradient(args.vg,
            cx, cy, 0, radius,
            nvgRGBA(255, 220, 140, (int)std::round(115.f * std::pow(brightness, 1.45f))),
            nvgRGBA(255, 220, 140, 0));
        nvgFillPaint(args.vg, glow);
        nvgFill(args.vg);
    }

    void drawVUNeedle(const DrawArgs& args, float normalized) {
        normalized = clamp(normalized, 0.f, 1.f);

        float angle;
        // Angles derived from SVG tick mark circumcenter at (130.85, 183.77) in viewBox coords.
        // Arc center → tick positions yield the exact CW-from-vertical angles for each dB mark.
        constexpr float kAngleMin = 139.3f;  // -20 dB
        constexpr float kAngleMid =  73.1f;  //   0 VU
        constexpr float kAngleMax =  39.1f;  //  +3 dB

        if (normalized <= 0.5f) {
            float t = rescale(normalized, 0.f, 0.5f, 0.f, 1.f);
            angle = kAngleMin - t * (kAngleMin - kAngleMid);
        } else {
            float t = rescale(normalized, 0.5f, 1.f, 0.f, 1.f);
            angle = kAngleMid - t * (kAngleMid - kAngleMax);
        }

        Vec center = box.size.mult(0.5f);
        // Pivot matches SVG arc center: x≈0.505×w (≈centre), y=183.77/271.04×h≈0.678×h.
        Vec pivotPoint = Vec(center.x, box.size.y * 0.678f);
        float needleLength = box.size.y * 0.35f;

        nvgSave(args.vg);
        nvgTranslate(args.vg, pivotPoint.x, pivotPoint.y);
        nvgRotate(args.vg, (90.f - angle) * M_PI / 180.0f);

        NVGcolor bodyColor = nvgRGBA(12, 12, 12, 245);
        NVGcolor highlightColor = nvgRGBA(45, 45, 45, 180);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, 0);
        nvgLineTo(args.vg, 0, -needleLength);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, bodyColor);
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 1.0f, -needleLength * 0.2f);
        nvgLineTo(args.vg, 1.0f, -needleLength);
        nvgStrokeWidth(args.vg, 0.9f);
        nvgStrokeColor(args.vg, highlightColor);
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, 0.f, 0.f, needleLength * 0.08f);
        nvgFillColor(args.vg, nvgRGBA(30, 30, 30, 255));
        nvgFill(args.vg);
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, 0.f, 0.f, needleLength * 0.05f);
        nvgFillColor(args.vg, highlightColor);
        nvgFill(args.vg);

        nvgRestore(args.vg);
    }
};

// Base class for oscilloscope-style visualizers with CRT effects
class VisualizerWidget : public Widget {
protected:
    Module* module = nullptr;
    std::vector<float> waveform;
    int maxSamples = 512;
    float timeScale = 1.0f;
    graphics::RGBColor traceColor = graphics::RGBColor(0, 1, 0.5f); // Default green
    
public:
    VisualizerWidget(Module* m) : module(m) {
        box.size = Vec(200, 100);
        waveform.resize(maxSamples, 0.f);
    }
    
    void setTraceColor(const graphics::RGBColor& color) { traceColor = color; }
    void setTimeScale(float scale) { timeScale = scale; }
    
    virtual void updateWaveform() = 0; // Implement in derived classes
    
    void draw(const DrawArgs& args) override {
        if (!module) return;
        
        updateWaveform();
        
        // Draw CRT background
        drawCRTBackground(args);
        
        // Draw waveform trace
        drawWaveform(args);
        
        // Add phosphor glow effect
        drawPhosphorEffect(args);
    }
    
private:
    void drawCRTBackground(const DrawArgs& args) {
        // Dark CRT background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(5, 10, 5));
        nvgFill(args.vg);
        
        // Grid lines
        nvgStrokeColor(args.vg, nvgRGBA(0, 80, 0, 40));
        nvgStrokeWidth(args.vg, 0.5f);
        
        // Vertical grid
        for (float x = 0; x < box.size.x; x += box.size.x / 8) {
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x, 0);
            nvgLineTo(args.vg, x, box.size.y);
            nvgStroke(args.vg);
        }
        
        // Horizontal grid
        for (float y = 0; y < box.size.y; y += box.size.y / 6) {
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, y);
            nvgLineTo(args.vg, box.size.x, y);
            nvgStroke(args.vg);
        }
    }
    
    void drawWaveform(const DrawArgs& args) {
        if (waveform.empty()) return;
        
        nvgBeginPath(args.vg);
        for (size_t i = 0; i < waveform.size(); i++) {
            float x = rescale(i, 0, waveform.size() - 1, 0, box.size.x);
            float y = rescale(waveform[i], -1.f, 1.f, box.size.y, 0);
            
            if (i == 0) {
                nvgMoveTo(args.vg, x, y);
            } else {
                nvgLineTo(args.vg, x, y);
            }
        }
        
        nvgStrokeColor(args.vg, traceColor.toNVG());
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);
    }
    
    void drawPhosphorEffect(const DrawArgs& args) {
        // Add subtle phosphor glow along the trace
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        
        nvgBeginPath(args.vg);
        for (size_t i = 0; i < waveform.size(); i++) {
            float x = rescale(i, 0, waveform.size() - 1, 0, box.size.x);
            float y = rescale(waveform[i], -1.f, 1.f, box.size.y, 0);
            
            if (i == 0) {
                nvgMoveTo(args.vg, x, y);
            } else {
                nvgLineTo(args.vg, x, y);
            }
        }
        
        nvgStrokeColor(args.vg, nvgRGBAf(traceColor.r, traceColor.g, traceColor.b, 0.3f));
        nvgStrokeWidth(args.vg, 3.0f);
        nvgStroke(args.vg);
        
        nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
    }
};

struct Trimpot : app::SvgKnob {
    Trimpot() {
        minAngle = -0.5 * M_PI;
        maxAngle = 0.5 * M_PI;
        setSvg(APP->window->loadSvg(asset::system("res/ComponentLibrary/Trimpot.svg")));
        box.size = mm2px(Vec(6, 6));
    }
};

struct VUCalibrationKnob : app::SvgKnob {
    VUCalibrationKnob() {
        minAngle = -0.5f * M_PI;
        maxAngle = 0.5f * M_PI;
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/vintage_vu_calibration.svg")));
        // Match the inner circle's proportional size in the meter
        // Inner circle diameter: 23.57 SVG units out of 259.09 meter width = 4.0mm
        float sizeMm = 23.57f / 259.0896f * (46.0f * 259.0896f / 271.0356f);
        box.size = mm2px(Vec(sizeMm, sizeMm));
    }
};

}} // namespace shapetaker::ui
