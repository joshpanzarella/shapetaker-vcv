#pragma once
#include <rack.hpp>
#include <history.hpp>
#include <nanovg.h>

using namespace rack;

extern Plugin* pluginInstance;

// Include reorganized utilities
#include "utilities.hpp"
#include "ui/label_formatter.hpp"

extern Model* modelClairaudient;

/**
 * Base knob class with universal fallback indicator support.
 * ONLY draws a procedural indicator if the knob uses blank_indicator.svg.
 * Knobs with custom indicators (set via setSvg after construction) are automatically skipped.
 */
struct ShapetakerKnobBase : app::SvgKnob {
    std::shared_ptr<Svg> blankIndicatorSvg;  // Cache the blank indicator SVG for comparison
    float indicatorRadiusFraction = 0.80f;   // Position on skirt - closer to center
    float indicatorSizeFraction = 0.05f;     // Smaller, more subtle indicator

    void checkForFallbackIndicator(const std::string& svgPath) {
        // Load and cache the blank indicator SVG for later comparison
        if (svgPath.find("blank_indicator.svg") != std::string::npos) {
            blankIndicatorSvg = Svg::load(svgPath);
        }
    }

    bool shouldDrawFallback() const {
        // Check at draw time if the current SVG is still the blank indicator
        // This handles cases where setSvg() is called after construction to set a custom indicator
        if (!blankIndicatorSvg) return false;
        if (!sw || !sw->svg) return false;
        return (sw->svg == blankIndicatorSvg);
    }

    void drawFallbackIndicator(const DrawArgs& args) {
        // Skip if not using blank indicator (i.e., has a custom indicator)
        if (!shouldDrawFallback()) return;

        // Get the current knob value and convert to angle
        float value = 0.f;
        if (getParamQuantity()) {
            value = getParamQuantity()->getScaledValue();
        }
        float angle = rack::math::rescale(value, 0.f, 1.f, minAngle, maxAngle);

        // Calculate indicator position on the skirt
        Vec center = box.size.div(2);
        float knobRadius = std::min(center.x, center.y);
        float indicatorDistance = knobRadius * indicatorRadiusFraction;
        float indicatorSize = knobRadius * indicatorSizeFraction;

        // Calculate indicator position
        float x = center.x + indicatorDistance * std::sin(angle);
        float y = center.y - indicatorDistance * std::cos(angle);

        // Draw small filled circle indicator using beige color from panel text (#c8c8b6)
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, x, y, indicatorSize);
        nvgFillColor(args.vg, nvgRGB(0xc8, 0xc8, 0xb6));  // Beige color matching panel aesthetics
        nvgFill(args.vg);
    }
};

/**
 * Jet black screw - procedurally drawn for a sleek modern look.
 */
struct ScrewJetBlack : widget::Widget {
    ScrewJetBlack() {
        box.size = Vec(17, 17);
    }

    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        float cx = box.size.x / 2.f;
        float cy = box.size.y / 2.f;
        float r = std::min(cx, cy) * 0.80f;

        // Soft contact shadow lifts the screw off the panel without a large halo.
        NVGpaint shadow = nvgRadialGradient(vg, cx + r * 0.10f, cy + r * 0.12f, r * 0.48f, r * 1.06f,
            nvgRGBA(0, 0, 0, 82), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgCircle(vg, cx + r * 0.04f, cy + r * 0.05f, r * 1.04f);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);

        // Beveled black body.
        NVGpaint body = nvgRadialGradient(vg, cx - r * 0.28f, cy - r * 0.32f, r * 0.08f, r * 0.82f,
            nvgRGB(34, 34, 32), nvgRGB(0, 0, 0));
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r);
        nvgFillPaint(vg, body);
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r);
        nvgStrokeWidth(vg, 1.15f);
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgStroke(vg);

        // Upper rim catches a little warm panel light.
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.98f);
        nvgStrokeWidth(vg, 0.55f);
        nvgStrokeColor(vg, nvgRGBA(238, 220, 187, 18));
        nvgStroke(vg);

        // Inner recess, slightly lighter near the top-left and darker at the edge.
        NVGpaint recess = nvgRadialGradient(vg, cx - r * 0.18f, cy - r * 0.20f, r * 0.08f, r * 0.72f,
            nvgRGB(25, 25, 24), nvgRGB(7, 7, 8));
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.68f);
        nvgFillPaint(vg, recess);
        nvgFill(vg);

        // Cross slot shadow.
        nvgBeginPath(vg);
        nvgRoundedRect(vg, cx - r * 0.52f, cy - r * 0.07f, r * 1.04f, r * 0.18f, r * 0.045f);
        nvgRoundedRect(vg, cx - r * 0.07f, cy - r * 0.52f, r * 0.18f, r * 1.04f, r * 0.045f);
        nvgFillColor(vg, nvgRGB(1, 1, 2));
        nvgFill(vg);

        // Subtle lower-right slot bevel keeps the cross visible when zoomed out.
        nvgBeginPath(vg);
        nvgMoveTo(vg, cx - r * 0.44f, cy + r * 0.10f);
        nvgLineTo(vg, cx + r * 0.44f, cy + r * 0.10f);
        nvgMoveTo(vg, cx + r * 0.10f, cy - r * 0.44f);
        nvgLineTo(vg, cx + r * 0.10f, cy + r * 0.44f);
        nvgStrokeWidth(vg, 0.5f);
        nvgStrokeColor(vg, nvgRGBA(255, 245, 220, 20));
        nvgStroke(vg);

        // Small specular nick, restrained so the screw still reads as black.
        nvgBeginPath(vg);
        nvgCircle(vg, cx - r * 0.28f, cy - r * 0.30f, r * 0.13f);
        nvgFillColor(vg, nvgRGBA(255, 248, 228, 34));
        nvgFill(vg);
    }
};

/**
 * Knob shadow widget - draws a realistic drop shadow beneath knobs.
 * Used by all Shapetaker modules for consistent visual depth.
 */
struct KnobShadowWidget : widget::TransparentWidget {
    float padding = 6.f;
    float alpha = 0.32f;
    float offset = 0.f;
    float verticalScale = 0.7f;

    KnobShadowWidget(const Vec& knobSize, float paddingPx, float alpha_) {
        padding = paddingPx;
        alpha = alpha_;
        box.size = knobSize.plus(Vec(padding * 2.f, padding * 2.f));
        offset = padding * 0.58f;
        verticalScale = 0.65f;
    }

    void draw(const DrawArgs& args) override {
        Vec center = box.size.div(2.f);
        float outerR = std::max(box.size.x, box.size.y) * 0.5f;
        float innerR = std::max(0.f, outerR - padding);

        nvgSave(args.vg);
        nvgTranslate(args.vg, center.x, center.y + offset);
        nvgScale(args.vg, 1.f, verticalScale);

        NVGpaint paint = nvgRadialGradient(
            args.vg,
            0.f,
            0.f,
            innerR * 0.25f,
            outerR,
            nvgRGBAf(0.f, 0.f, 0.f, alpha),
            nvgRGBAf(0.f, 0.f, 0.f, 0.f));
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, 0.f, 0.f, outerR);
        nvgFillPaint(args.vg, paint);
        nvgFill(args.vg);

        nvgRestore(args.vg);
    }
};

/**
 * Helper function to add a knob with shadow to a module widget.
 * Usage: addKnobWithShadow(this, knob) instead of addParam(knob)
 */
inline void addKnobWithShadow(ModuleWidget* widget, app::ParamWidget* knob) {
    if (!knob || !widget) return;

    float diameter = std::max(knob->box.size.x, knob->box.size.y);
    // Subtle shadow: reduced padding and alpha to avoid bleeding into nearby text
    float padding = std::max(6.f, std::min(diameter * 0.25f, 14.f));
    float alpha = std::max(0.22f, std::min(0.38f, 0.18f + diameter * 0.004f));

    auto* shadow = new KnobShadowWidget(knob->box.size, padding, alpha);
    shadow->offset = padding * 0.45f;  // Subtle vertical offset
    shadow->box.pos = knob->box.pos.minus(Vec(padding, padding));
    widget->addChild(shadow);
    widget->addParam(knob);
}

inline void drawPanelContactShadow(NVGcontext* vg, Vec size, float radius, float alpha = 0.24f, float yOffset = 1.2f) {
    float cx = size.x * 0.5f;
    float cy = size.y * 0.5f + yOffset;
    float rx = std::max(1.f, size.x * 0.56f);
    float ry = std::max(1.f, size.y * 0.48f);

    nvgSave(vg);
    nvgTranslate(vg, cx, cy);
    nvgScale(vg, rx / ry, 1.f);
    NVGpaint paint = nvgRadialGradient(vg, 0.f, 0.f, ry * 0.30f, ry,
        nvgRGBAf(0.f, 0.f, 0.f, alpha * 0.55f), nvgRGBAf(0.f, 0.f, 0.f, 0.f));
    nvgBeginPath(vg);
    nvgCircle(vg, 0.f, 0.f, ry);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
    nvgRestore(vg);
}

inline void drawPanelCircularSeat(NVGcontext* vg, Vec size, float alpha = 0.22f) {
    float cx = size.x * 0.5f;
    float cy = size.y * 0.5f;
    float r = std::min(size.x, size.y) * 0.48f;

    NVGpaint shadow = nvgRadialGradient(vg, cx + r * 0.08f, cy + r * 0.12f, r * 0.50f, r * 1.18f,
        nvgRGBAf(0.f, 0.f, 0.f, alpha), nvgRGBAf(0.f, 0.f, 0.f, 0.f));
    nvgBeginPath(vg);
    nvgCircle(vg, cx + r * 0.04f, cy + r * 0.06f, r * 1.14f);
    nvgFillPaint(vg, shadow);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, r * 1.01f);
    nvgStrokeWidth(vg, 0.8f);
    nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 70));
    nvgStroke(vg);
}

struct ShapetakerCKSS : rack::componentlibrary::CKSS {
    void draw(const DrawArgs& args) override {
        drawPanelContactShadow(args.vg, box.size, 2.2f, 0.22f, 1.0f);
        rack::componentlibrary::CKSS::draw(args);
    }
};

struct ShapetakerCKSSThree : rack::componentlibrary::CKSSThree {
    void draw(const DrawArgs& args) override {
        drawPanelContactShadow(args.vg, box.size, 2.2f, 0.22f, 1.0f);
        rack::componentlibrary::CKSSThree::draw(args);
    }
};

struct ShapetakerPJ301MPort : rack::componentlibrary::PJ301MPort {
    void draw(const DrawArgs& args) override {
        drawPanelCircularSeat(args.vg, box.size, 0.18f);
        rack::componentlibrary::PJ301MPort::draw(args);
    }
};

// Shadow helpers currently disabled per request (leave hooks for future tuning).
inline void applyCircularShadow(app::SvgKnob* knob, float, float, float = 0.f, float = 0.f) {
    if (!knob || !knob->shadow) return;
    knob->shadow->visible = false;
}

// Hexagonal attenuverter shadow helper (disabled).
inline void applyHexShadow(app::SvgKnob* knob, float = 0.f, float = 0.f, float = 0.f, float = 0.f) {
    if (!knob || !knob->shadow) return;
    knob->shadow->visible = false;
}

// ============================================================================
// CHARRED KNOBS (for testing alternative aesthetics)
// ============================================================================

// ============================================================================
// VINTAGE CHUNKY KNOBS (tactile oscilloscope style)
// ============================================================================

struct ShapetakerKnobVintageSmall : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerKnobVintageSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_rotate.svg")));

        bg = new widget::SvgWidget;
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_background.svg")));
        nativeSize = bg->box.size;
        if (fb && tw) fb->addChildBelow(bg, tw);
        auto* fg = new widget::SvgWidget;
        fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_fg.svg")));
        if (fb && tw) fb->addChildAbove(fg, tw);

        // Small: 12mm
        box.size = mm2px(Vec(15.f, 15.f));
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

struct ShapetakerKnobVintageSmallMedium : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerKnobVintageSmallMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_rotate.svg")));

        bg = new widget::SvgWidget;
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_background.svg")));
        nativeSize = bg->box.size;
        if (fb && tw) fb->addChildBelow(bg, tw);
        auto* fg = new widget::SvgWidget;
        fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_fg.svg")));
        if (fb && tw) fb->addChildAbove(fg, tw);

        // Small-Medium: 15mm (between 12mm small and 18mm medium)
        box.size = mm2px(Vec(15.f, 15.f));
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

struct ShapetakerKnobVintageMedium : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerKnobVintageMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        // Load the rotating indicator
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_rotate.svg")));

        // Load and add the static background
        bg = new widget::SvgWidget;
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_background.svg")));
        nativeSize = bg->box.size;
        if (fb && tw) fb->addChildBelow(bg, tw);
        auto* fg = new widget::SvgWidget;
        fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_fg.svg")));
        if (fb && tw) fb->addChildAbove(fg, tw);

        // Medium: 19.8mm (18mm + 10%)
        box.size = mm2px(Vec(19.8f, 19.8f));
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

struct ShapetakerKnobVintageLarge : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerKnobVintageLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_rotate.svg")));

        bg = new widget::SvgWidget;
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_background.svg")));
        nativeSize = bg->box.size;
        if (fb && tw) fb->addChildBelow(bg, tw);
        auto* fg = new widget::SvgWidget;
        fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_fg.svg")));
        if (fb && tw) fb->addChildAbove(fg, tw);

        // Large: 22mm
        box.size = mm2px(Vec(22.f, 22.f));
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

struct ShapetakerKnobVintageXLarge : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerKnobVintageXLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_rotate.svg")));

        bg = new widget::SvgWidget;
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_background.svg")));
        nativeSize = bg->box.size;
        if (fb && tw) fb->addChildBelow(bg, tw);
        auto* fg = new widget::SvgWidget;
        fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_knob_fg.svg")));
        if (fb && tw) fb->addChildAbove(fg, tw);

        // XLarge: 27mm (matches Davies XLarge at 1.5x)
        box.size = mm2px(Vec(27.f, 27.f));
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

struct ShapetakerKnobVintageAttenuverter : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerKnobVintageAttenuverter() {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_attenuverter_rotate.svg")));

        bg = new widget::SvgWidget;
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_attenuverter_background.svg")));
        nativeSize = bg->box.size;
        if (fb && tw) fb->addChildBelow(bg, tw);
        auto* fg = new widget::SvgWidget;
        fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_attenuverter_fg.svg")));
        if (fb && tw) fb->addChildAbove(fg, tw);

        // Attenuverter: 10mm
        box.size = mm2px(Vec(12.f, 12.f));
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float scale = std::min(sx, sy);
        float tx = (box.size.x - svgW * scale) * 0.5f;
        float ty = (box.size.y - svgH * scale) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, scale, scale);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

// ============================================================================
// DARK KNOBS (new charred aesthetic)
// ============================================================================

// ============================================================================
// DAVIES 1900H (dot indicator variants)
// ============================================================================

struct ShapetakerDavies1900hSmallDot : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerDavies1900hSmallDot() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
        if (fb && tw) fb->addChildBelow(bg, tw);

        // Use the large Davies assets and scale down to the small size.
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/st_davies_1900h_large_indicator.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/backgrounds/st_davies_1900h_large_bg.svg")));
        nativeSize = bg->box.size;
        // Match the previous small knob footprint, then scale up 10%.
        box.size = Vec(39.6f, 39.6f);
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

struct ShapetakerDavies1900hLargeDot : app::SvgKnob {
    widget::SvgWidget* bg;

    ShapetakerDavies1900hLargeDot() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
        if (fb && tw) fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/st_davies_1900h_large_indicator.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/backgrounds/st_davies_1900h_large_bg.svg")));
    }
};

// Davies 1900h extra-large (scaled up from large) with dot indicator
struct ShapetakerDavies1900hXLargeDot : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(54.f, 54.f);

    ShapetakerDavies1900hXLargeDot() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
        if (fb && tw) fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/st_davies_1900h_large_indicator.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/backgrounds/st_davies_1900h_large_bg.svg")));
        nativeSize = bg->box.size;

        // Scale up 50% from the large size for the VCA control.
        box.size = nativeSize.mult(1.5f);
    }

    void draw(const DrawArgs& args) override {
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

// Hallicrafters-inspired vintage knob - rugged bakelite with metallic collar
// Inspired by Hallicrafters SX-28, Collins radio equipment, classic military receivers
// 54x54px - same size as Davies1900h large for drop-in replacement
struct ShapetakerKnobHallicraftersMedium : app::SvgKnob {
    widget::SvgWidget* bg;

    ShapetakerKnobHallicraftersMedium() {
        minAngle = -0.83 * M_PI;  // Match Davies1900h rotation range
        maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
        if (fb && tw) fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/st_hallicrafters_medium_indicator.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/backgrounds/st_hallicrafters_medium_bg.svg")));
    }
};

// Base for all non-momentary position switches (2–5 pos).
// Bare clicks do nothing; value only changes via click-hold-drag.
// Drag up/right to increase, down/left to decrease. 20 px per step.
struct DragOnlySvgSwitch : app::SvgSwitch {
    float _valueAtDragStart = 0.f;
    float _dragAccum        = 0.f;

    void onDragStart(const event::DragStart& e) override {
        auto* pq = getParamQuantity();
        _valueAtDragStart = pq ? pq->getValue() : 0.f;
        _dragAccum = 0.f;
        // Do NOT call parent — Switch::onDragStart would step the value on every click.
        // Drag events still fire because onButton (handled by the parent chain) consumed the press.
    }

    void onDragMove(const event::DragMove& e) override {
        if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;
        auto* pq = getParamQuantity();
        if (!pq) return;
        // Vertical: up = increase.  Horizontal: right = increase.
        _dragAccum += -e.mouseDelta.y + e.mouseDelta.x;
        const float kStep = 20.f;
        while (_dragAccum >= kStep) {
            _dragAccum -= kStep;
            pq->setValue(clamp(pq->getValue() + 1.f, pq->minValue, pq->maxValue));
        }
        while (_dragAccum <= -kStep) {
            _dragAccum += kStep;
            pq->setValue(clamp(pq->getValue() - 1.f, pq->minValue, pq->maxValue));
        }
    }

    void onDragEnd(const event::DragEnd& e) override {
        // Do NOT call parent — it expects internal state that parent's onDragStart would have set.
        // Manually push an undo entry if the value actually changed.
        auto* pq = getParamQuantity();
        if (pq && pq->module && pq->getValue() != _valueAtDragStart) {
            auto* h = new history::ParamChange;
            h->name = "move switch";
            h->moduleId = pq->module->id;
            h->paramId = pq->paramId;
            h->oldValue = _valueAtDragStart;
            h->newValue = pq->getValue();
            APP->history->push(h);
        }
    }

    void draw(const DrawArgs& args) override {
        drawPanelContactShadow(args.vg, box.size, 2.6f, 0.24f, 1.1f);
        app::SvgSwitch::draw(args);
    }
};

// Dark slide toggle - Befaco size (9.5x10.7mm) with black body and grey lever
struct ShapetakerDarkToggle : DragOnlySvgSwitch {
    ShapetakerDarkToggle() {
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_off.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_on.svg")));
        shadow->opacity = 0.0;
    }
};

// Two-position dark toggle variant:
// off -> pos4
struct ShapetakerDarkToggleOffPos4 : DragOnlySvgSwitch {
    ShapetakerDarkToggleOffPos4() {
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_off.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_pos4.svg")));
        shadow->opacity = 0.0;
    }
};

// Five-position variant for vertical dark toggle travel:
// off -> pos1 -> on -> pos3 -> pos4
struct ShapetakerDarkToggleFivePos : DragOnlySvgSwitch {
    ShapetakerDarkToggleFivePos() {
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_off.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_pos1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_on.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_pos3.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_pos4.svg")));
        shadow->opacity = 0.0;
    }
};

// Four-position variant for mode selection:
// off -> pos1 -> on -> pos4
struct ShapetakerDarkToggleFourPos : DragOnlySvgSwitch {
    ShapetakerDarkToggleFourPos() {
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_off.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_pos1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_on.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_pos4.svg")));
        shadow->opacity = 0.0;
    }
};

// Three-position variant for panel mode switches:
// off -> on -> pos4
struct ShapetakerDarkToggleThreePos : DragOnlySvgSwitch {
    ShapetakerDarkToggleThreePos() {
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_off.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_on.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/switches/dark_toggle_pos4.svg")));
        shadow->opacity = 0.0;
    }
};

struct ShapetakerVintageTripleSwitch : app::Switch {
    float _valueAtDragStart = 0.f;
    float _dragAccum        = 0.f;

    ShapetakerVintageTripleSwitch() {
        momentary = false;
        // Match the footprint of the brass 2-way toggle
        box.size = mm2px(Vec(12.f, 12.f));
    }

    void onDragStart(const event::DragStart& e) override {
        auto* pq = getParamQuantity();
        _valueAtDragStart = pq ? pq->getValue() : 0.f;
        _dragAccum = 0.f;
        // Do NOT call parent — Switch::onDragStart steps the value on click.
    }
    void onDragMove(const event::DragMove& e) override {
        if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;
        auto* pq = getParamQuantity();
        if (!pq) return;
        _dragAccum += -e.mouseDelta.y + e.mouseDelta.x;
        const float kStep = 20.f;
        while (_dragAccum >= kStep) {
            _dragAccum -= kStep;
            pq->setValue(clamp(pq->getValue() + 1.f, pq->minValue, pq->maxValue));
        }
        while (_dragAccum <= -kStep) {
            _dragAccum += kStep;
            pq->setValue(clamp(pq->getValue() - 1.f, pq->minValue, pq->maxValue));
        }
    }
    void onDragEnd(const event::DragEnd& e) override {
        auto* pq = getParamQuantity();
        if (pq && pq->module && pq->getValue() != _valueAtDragStart) {
            auto* h = new history::ParamChange;
            h->name = "move switch";
            h->moduleId = pq->module->id;
            h->paramId = pq->paramId;
            h->oldValue = _valueAtDragStart;
            h->newValue = pq->getValue();
            APP->history->push(h);
        }
    }

    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        float w = box.size.x;
        float h = box.size.y;

        drawPanelContactShadow(vg, box.size, h * 0.32f, 0.24f, 1.0f);

        nvgSave(vg);

        // Drop shadow
        nvgBeginPath(vg);
        float shadowRadius = h * 0.32f;
        nvgRoundedRect(vg, w * 0.08f, h * 0.06f, w * 0.84f, h * 0.88f, shadowRadius);
        NVGpaint shadowPaint = nvgBoxGradient(vg, w * 0.5f, h * 0.5f, w * 0.7f, h * 0.8f, shadowRadius, w * 0.3f,
            nvgRGBA(0, 0, 0, 80), nvgRGBA(0, 0, 0, 0));
        nvgFillPaint(vg, shadowPaint);
        nvgFill(vg);

        // Base plate
        float radius = h * 0.28f;
        float inset = w * 0.08f;
        float insetY = h * 0.05f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, inset, insetY, w - inset * 2.f, h - insetY * 2.f, radius);
        NVGpaint basePaint = nvgLinearGradient(vg, inset, insetY, inset, h - insetY,
            nvgRGBA(52, 54, 58, 255),
            nvgRGBA(32, 33, 36, 255));
        nvgFillPaint(vg, basePaint);
        nvgFill(vg);

        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 200));
        nvgStrokeWidth(vg, 0.8f);
        nvgStroke(vg);

        // Inner bevel
        nvgBeginPath(vg);
        nvgRoundedRect(vg, inset + 0.6f, insetY + 0.5f, (w - inset * 2.f) - 1.2f, (h - insetY * 2.f) - 1.0f, radius * 0.85f);
        nvgStrokeColor(vg, nvgRGBA(75, 77, 82, 180));
        nvgStrokeWidth(vg, 0.5f);
        nvgStroke(vg);

        // Top sheen
        nvgBeginPath(vg);
        nvgRoundedRect(vg, inset + 0.8f, insetY + 0.7f, (w - inset * 2.f) - 1.6f, (h - insetY * 2.f) * 0.45f, radius * 0.7f);
        NVGpaint sheen = nvgLinearGradient(vg, inset, insetY, inset, insetY + (h - insetY * 2.f) * 0.4f,
            nvgRGBA(255, 255, 255, 25),
            nvgRGBA(255, 255, 255, 0));
        nvgFillPaint(vg, sheen);
        nvgFill(vg);

        // Screws
        auto drawScrew = [&](float x, float y) {
            float sr = w * 0.11f;
            nvgBeginPath(vg);
            nvgCircle(vg, x + 0.15f, y + 0.15f, sr * 0.95f);
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 60));
            nvgFill(vg);

            nvgBeginPath(vg);
            nvgCircle(vg, x, y, sr);
            NVGpaint screwPaint = nvgRadialGradient(vg, x - sr * 0.3f, y - sr * 0.3f,
                sr * 0.1f, sr * 1.1f,
                nvgRGBA(195, 192, 185, 255),
                nvgRGBA(85, 85, 85, 255));
            nvgFillPaint(vg, screwPaint);
            nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 180));
            nvgStrokeWidth(vg, 0.35f);
            nvgStroke(vg);

            nvgBeginPath(vg);
            nvgRect(vg, x - sr * 0.65f, y - sr * 0.12f, sr * 1.3f, sr * 0.24f);
            nvgFillColor(vg, nvgRGBA(30, 30, 30, 200));
            nvgFill(vg);
        };
        drawScrew(inset + (w - inset * 2.f) * 0.32f, insetY + (h - insetY * 2.f) * 0.28f);
        drawScrew(inset + (w - inset * 2.f) * 0.68f, insetY + (h - insetY * 2.f) * 0.72f);

        // Angle across three states (-30°, 0°, +30°)
        float state = 0.f;
        if (auto* pq = getParamQuantity()) {
            state = rack::math::clamp(pq->getValue(), 0.f, 2.f);
        }
        float theta = 24.f * (M_PI / 180.f);
        float t = rack::math::rescale(state, 0.f, 2.f, -1.f, 1.f);
        float angle = t * theta;

        float pivotX = w * 0.5f;
        float pivotY = insetY + (h - insetY * 2.f) * 0.60f;
        nvgTranslate(vg, pivotX, pivotY);
        nvgRotate(vg, angle);

        // Lever shadow
        nvgBeginPath(vg);
        float stemWidth = w * 0.22f;
        float stemLength = (h - insetY * 2.f) * 0.78f;
        nvgRoundedRect(vg, -stemWidth * 0.5f + 0.3f, -stemLength + 0.3f, stemWidth, stemLength, stemWidth * 0.4f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 50));
        nvgFill(vg);

        // Lever stem - brass finish
        nvgBeginPath(vg);
        nvgRoundedRect(vg, -stemWidth * 0.5f, -stemLength, stemWidth, stemLength, stemWidth * 0.4f);
        NVGpaint leverPaint = nvgLinearGradient(vg, -stemWidth * 0.5f, -stemLength, stemWidth * 0.5f, -stemLength,
            nvgRGBA(217, 191, 121, 255),
            nvgRGBA(125, 95, 40, 255));
        nvgFillPaint(vg, leverPaint);
        nvgFill(vg);

        nvgStrokeColor(vg, nvgRGBA(235, 215, 155, 180));
        nvgStrokeWidth(vg, 0.4f);
        nvgStroke(vg);

        // Lever tip - brass cap
        float tipR = stemWidth * 0.92f;
        nvgBeginPath(vg);
        nvgCircle(vg, 0.2f, -stemLength + 0.2f, tipR);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 60));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgCircle(vg, 0.f, -stemLength, tipR);
        NVGpaint tipPaint = nvgRadialGradient(vg,
            -tipR * 0.35f, -stemLength - tipR * 0.35f,
            tipR * 0.15f, tipR * 1.15f,
            nvgRGBA(189, 152, 70, 255),
            nvgRGBA(96, 71, 29, 255));
        nvgFillPaint(vg, tipPaint);
        nvgFill(vg);

        nvgStrokeColor(vg, nvgRGBA(72, 52, 22, 220));
        nvgStrokeWidth(vg, 0.4f);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgCircle(vg, -tipR * 0.3f, -stemLength - tipR * 0.3f, tipR * 0.35f);
        nvgFillColor(vg, nvgRGBA(255, 235, 180, 50));
        nvgFill(vg);

        nvgRestore(vg);

        app::Switch::draw(args);
    }
};

// Note: LED glow for shuttle switches lives in the SVGs themselves (panel LEDs).

struct ShapetakerBNCPort : app::SvgPort {
    ShapetakerBNCPort() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/ports/st_bnc_connector.svg")));
        // Aged-nickel BNC sized just above Rack's stock PJ301M port (8.03 mm):
        // near-parity in mixed racks, with a little vintage heft kept.
        box.size = mm2px(Vec(8.5f, 8.5f));
    }
    void draw(const DrawArgs& args) override {
        drawPanelCircularSeat(args.vg, box.size, 0.20f);
        // Scale the SVG to fit the current box (SVG viewBox is 20x20)
        const float svgSize = 20.f;
        float s = std::min(box.size.x, box.size.y) / svgSize;
        float tx = (box.size.x - svgSize * s) * 0.5f;
        float ty = (box.size.y - svgSize * s) * 0.5f;
        nvgSave(args.vg);
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgPort::draw(args);
        nvgRestore(args.vg);
    }
};

struct OutputJackHaloWidget : Widget {
    explicit OutputJackHaloWidget(float diameterMm = 9.9f) {
        box.size = mm2px(Vec(diameterMm, diameterMm));
    }

    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        const float cx = box.size.x * 0.5f;
        const float cy = box.size.y * 0.5f;
        const float r = std::min(cx, cy);

        nvgSave(vg);

        NVGpaint contact = nvgRadialGradient(
            vg, cx, cy + r * 0.08f, r * 0.42f, r * 1.04f,
            nvgRGBA(0, 0, 0, 72),
            nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.98f);
        nvgFillPaint(vg, contact);
        nvgFill(vg);

        NVGpaint brassBase = nvgRadialGradient(
            vg, cx - r * 0.26f, cy - r * 0.30f, r * 0.16f, r * 0.94f,
            nvgRGBA(212, 166, 89, 112),
            nvgRGBA(122, 93, 54, 126));
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.97f);
        nvgFillPaint(vg, brassBase);
        nvgFill(vg);

        NVGpaint brass = nvgLinearGradient(
            vg, cx - r * 0.72f, cy - r * 0.82f, cx + r * 0.70f, cy + r * 0.86f,
            nvgRGBA(212, 166, 89, 246),
            nvgRGBA(122, 93, 54, 242));
        // Main brass band: pushed outward and widened so a clear ring stays
        // visible around the jack (the halo sits underneath the port).
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.90f);
        nvgStrokeWidth(vg, r * 0.15f);
        nvgStrokePaint(vg, brass);
        nvgStroke(vg);

        auto fillLitAnnulus = [&](NVGpaint paint) {
            nvgBeginPath(vg);
            nvgCircle(vg, cx, cy, r * 0.97f);
            nvgCircle(vg, cx, cy, r * 0.64f);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, paint);
            nvgFill(vg);
        };

        // Diagonal bevel light: common Rack-style top-left catch light with
        // lower-right falloff, clipped to the visible output indicator ring.
        NVGpaint directionalLight = nvgLinearGradient(
            vg, cx - r * 0.72f, cy - r * 0.86f, cx + r * 0.78f, cy + r * 0.78f,
            nvgRGBA(255, 246, 218, 78),
            nvgRGBA(255, 246, 218, 0));
        fillLitAnnulus(directionalLight);

        NVGpaint directionalShade = nvgLinearGradient(
            vg, cx - r * 0.46f, cy - r * 0.58f, cx + r * 0.84f, cy + r * 0.92f,
            nvgRGBA(0, 0, 0, 0),
            nvgRGBA(24, 14, 5, 86));
        fillLitAnnulus(directionalShade);

        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.83f);
        nvgStrokeWidth(vg, r * 0.024f);
        nvgStrokeColor(vg, nvgRGBA(248, 232, 200, 120));
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.965f);
        nvgStrokeWidth(vg, r * 0.024f);
        nvgStrokeColor(vg, nvgRGBA(122, 93, 54, 210));
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r * 0.62f);
        nvgStrokeWidth(vg, r * 0.032f);
        nvgStrokeColor(vg, nvgRGBA(168, 132, 78, 214));
        nvgStroke(vg);

        constexpr int kTicks = 12;
        for (int i = 0; i < kTicks; ++i) {
            float a = (float)i * (2.f * (float)M_PI / (float)kTicks);
            float ca = std::cos(a);
            float sa = std::sin(a);
            float inner = (i % 3 == 0) ? r * 0.68f : r * 0.74f;
            float outer = r * 0.93f;
            nvgBeginPath(vg);
            nvgMoveTo(vg, cx + ca * inner, cy + sa * inner);
            nvgLineTo(vg, cx + ca * outer, cy + sa * outer);
            nvgStrokeWidth(vg, (i % 3 == 0) ? r * 0.027f : r * 0.015f);
            nvgStrokeColor(vg, (i % 3 == 0)
                ? nvgRGBA(212, 166, 89, 198)
                : nvgRGBA(122, 93, 54, 186));
            nvgStroke(vg);
        }

        // Subtle abstract output triangle, kept small so the halo remains the primary shape.
        nvgBeginPath(vg);
        nvgMoveTo(vg, cx - r * 0.19f, cy + r * 0.89f);
        nvgLineTo(vg, cx, cy + r * 1.08f);
        nvgLineTo(vg, cx + r * 0.19f, cy + r * 0.89f);
        nvgClosePath(vg);
        nvgFillColor(vg, nvgRGBA(168, 132, 78, 198));
        nvgFill(vg);

        nvgRestore(vg);
    }
};

static inline void addOutputJackHalo(ModuleWidget* w, Vec center, float diameterMm = 9.9f) {
    auto* halo = new OutputJackHaloWidget(diameterMm);
    halo->box.pos = center.minus(halo->box.size.div(2.f));
    w->addChild(halo);
}

template <typename TPort>
static inline void addOutputWithHalo(ModuleWidget* w, Vec center, Module* module, int outputId, float diameterMm = 9.9f) {
    addOutputJackHalo(w, center, diameterMm);
    w->addOutput(createOutputCentered<TPort>(center, module, outputId));
}

struct ShapetakerAttenuverterOscilloscope : app::SvgKnob {
    widget::SvgWidget* bg;
    Vec nativeSize = Vec(100.f, 100.f);
    ShapetakerAttenuverterOscilloscope() {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        // Rotating indicator only
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_attenuverter_rotate.svg")));

        // Stationary background (gradient stays fixed while the indicator rotates)
        bg = new widget::SvgWidget;
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_attenuverter_background.svg")));
        auto* fg = new widget::SvgWidget;
        fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/indicators/vintage_attenuverter_fg.svg")));
        if (fb && tw) fb->addChildAbove(fg, tw);
        nativeSize = bg->box.size;

        // Add background below the rotating transform widget (stationary)
        if (fb && tw) {
            fb->addChildBelow(bg, tw);
        }

        // Target: Attenuverter = 10 mm
        box.size = mm2px(Vec(10.f, 10.f));
        applyHexShadow(this, 0.94f, 0.08f, 0.20f, 0.65f);
    }
    void draw(const DrawArgs& args) override {
        drawPanelCircularSeat(args.vg, box.size, 0.18f);
        nvgSave(args.vg);
        float svgW = std::max(1.f, nativeSize.x);
        float svgH = std::max(1.f, nativeSize.y);
        float sx = box.size.x / svgW;
        float sy = box.size.y / svgH;
        float s = std::min(sx, sy);
        float tx = (box.size.x - svgW * s) * 0.5f;
        float ty = (box.size.y - svgH * s) * 0.5f;
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);
        app::SvgKnob::draw(args);
        nvgRestore(args.vg);
    }
};

// Shadow behavior is now integrated into the base Shapetaker knob classes above.

// Shapetaker vintage momentary button using a single SVG with a pressed overlay
struct ShapetakerVintageMomentary : app::SvgSwitch {
    ShapetakerVintageMomentary() {
        momentary = true;
        // Use the same SVG for both frames; we add a pressed overlay in draw()
        auto svgUp = Svg::load(asset::plugin(pluginInstance, "res/buttons/vintage_momentary_button.svg"));
        addFrame(svgUp);
        addFrame(svgUp);
        if (shadow) shadow->visible = false;
        // 9 x 9 mm footprint (hardware-friendly)
        box.size = mm2px(Vec(9.f, 9.f));
    }
    void draw(const DrawArgs& args) override {
        drawPanelCircularSeat(args.vg, box.size, 0.22f);
        nvgSave(args.vg);
        // Incoming SVG viewBox is 100x100; scale to our box
        const float s = box.size.x / 100.f;
        // Simulate a mechanical press: nudge downward and darken slightly when active
        const bool pressed = (getParamQuantity() && getParamQuantity()->getValue() > 0.5f);
        if (pressed) {
            nvgTranslate(args.vg, 0.f, 0.9f * s);
        }
        nvgScale(args.vg, s, s);
        app::SvgSwitch::draw(args);
        if (pressed) {
            // Warm backlight glow — radial from center, warm cream fading to transparent
            NVGpaint glow = nvgRadialGradient(args.vg, 50.f, 50.f, 5.f, 35.f,
                nvgRGBA(250, 245, 240, 232), nvgRGBA(230, 225, 220, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 35.f);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);

            NVGpaint core = nvgRadialGradient(args.vg, 50.f, 50.f, 0.f, 17.f,
                nvgRGBA(255, 255, 255, 104), nvgRGBA(245, 240, 235, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 17.f);
            nvgFillPaint(args.vg, core);
            nvgFill(args.vg);
        }
        nvgRestore(args.vg);
    }
};

// Lighted version of the vintage momentary button
struct ShapetakerVintageMomentaryLight : ShapetakerVintageMomentary {
    Module* module = nullptr;
    int lightId = -1;
    float lightGain = 1.55f;
    float lightAlpha = 245.f;
    float lightCoreAlpha = 130.f;
    float lightCoreRadius = 21.f;

    void draw(const DrawArgs& args) override {
        // Get light brightness
        float brightness = 0.f;
        if (module && lightId >= 0) {
            brightness = clamp(module->lights[lightId].getBrightness() * lightGain, 0.f, 1.f);
        }

        drawPanelCircularSeat(args.vg, box.size, 0.22f);
        nvgSave(args.vg);
        // Incoming SVG viewBox is 100x100; scale to our box
        const float s = box.size.x / 100.f;
        // Simulate a mechanical press: nudge downward and darken slightly when active
        const bool pressed = (getParamQuantity() && getParamQuantity()->getValue() > 0.5f);
        if (pressed) {
            nvgTranslate(args.vg, 0.f, 0.9f * s);
        }
        nvgScale(args.vg, s, s);

        app::SvgSwitch::draw(args);

        // Warm backlight glow when light is active — same feel as the trig button press
        if (brightness > 0.f) {
            float lamp = std::pow(brightness, 0.68f);
            NVGpaint glow = nvgRadialGradient(args.vg, 50.f, 50.f, 5.f, 35.f,
                nvgRGBA(255, 255, 255, (int)(lamp * lightAlpha)), nvgRGBA(245, 240, 235, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 35.f);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);

            if (lightCoreAlpha > 0.f && lightCoreRadius > 0.f) {
                NVGpaint core = nvgRadialGradient(args.vg, 50.f, 50.f, 0.f, lightCoreRadius,
                    nvgRGBA(255, 255, 255, (int)(lamp * lightCoreAlpha)),
                    nvgRGBA(245, 240, 235, 0));
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, 50.f, 50.f, lightCoreRadius);
                nvgFillPaint(args.vg, core);
                nvgFill(args.vg);
            }
        }

        if (pressed) {
            // Warm backlight glow — radial from center, warm cream fading to transparent
            NVGpaint glow = nvgRadialGradient(args.vg, 50.f, 50.f, 5.f, 35.f,
                nvgRGBA(255, 232, 170, 232), nvgRGBA(255, 211, 92, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 35.f);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);

            NVGpaint core = nvgRadialGradient(args.vg, 50.f, 50.f, 0.f, 17.f,
                nvgRGBA(255, 245, 190, 104), nvgRGBA(255, 190, 84, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 17.f);
            nvgFillPaint(args.vg, core);
            nvgFill(args.vg);
        }
        nvgRestore(args.vg);
    }
};

// High-contrast white-lamp momentary (Evocation's button, shared).
struct ShapetakerWhiteMomentaryLight : ShapetakerVintageMomentaryLight {
    void draw(const DrawArgs& args) override {
        float brightness = 0.f;
        if (module && lightId >= 0) {
            brightness = clamp(module->lights[lightId].getBrightness() * lightGain, 0.f, 1.f);
        }

        drawPanelCircularSeat(args.vg, box.size, 0.22f);
        nvgSave(args.vg);
        const float s = box.size.x / 100.f;
        const bool pressed = (getParamQuantity() && getParamQuantity()->getValue() > 0.5f);
        if (pressed) {
            nvgTranslate(args.vg, 0.f, 0.9f * s);
        }
        nvgScale(args.vg, s, s);

        app::SvgSwitch::draw(args);

        if (brightness > 0.f) {
            float lamp = std::pow(brightness, 0.68f);
            NVGpaint glow = nvgRadialGradient(args.vg, 50.f, 50.f, 5.f, 35.f,
                nvgRGBA(255, 255, 255, (int)(lamp * lightAlpha)), nvgRGBA(245, 240, 235, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 35.f);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);

            if (lightCoreAlpha > 0.f && lightCoreRadius > 0.f) {
                NVGpaint core = nvgRadialGradient(args.vg, 50.f, 50.f, 0.f, lightCoreRadius,
                    nvgRGBA(255, 255, 255, (int)(lamp * lightCoreAlpha)),
                    nvgRGBA(245, 240, 235, 0));
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, 50.f, 50.f, lightCoreRadius);
                nvgFillPaint(args.vg, core);
                nvgFill(args.vg);
            }
        }

        if (pressed) {
            NVGpaint glow = nvgRadialGradient(args.vg, 50.f, 50.f, 5.f, 35.f,
                nvgRGBA(255, 255, 252, 210), nvgRGBA(235, 232, 226, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 35.f);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);

            NVGpaint core = nvgRadialGradient(args.vg, 50.f, 50.f, 0.f, 17.f,
                nvgRGBA(255, 255, 255, 118), nvgRGBA(240, 238, 232, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 17.f);
            nvgFillPaint(args.vg, core);
            nvgFill(args.vg);
        }
        nvgRestore(args.vg);
    }
};

// Latching vintage button with built-in amber backlight
struct ShapetakerVintageLatchLED : app::SvgSwitch {
    ShapetakerVintageLatchLED() {
        momentary = false;
        auto svgUp = Svg::load(asset::plugin(pluginInstance, "res/buttons/vintage_momentary_button.svg"));
        addFrame(svgUp);
        addFrame(svgUp);
        if (shadow) shadow->visible = false;
        box.size = mm2px(Vec(9.f, 9.f));
    }
    void draw(const DrawArgs& args) override {
        drawPanelCircularSeat(args.vg, box.size, 0.22f);
        nvgSave(args.vg);
        const float s = box.size.x / 100.f;
        bool active = getParamQuantity() && getParamQuantity()->getValue() > 0.5f;
        if (active) {
            nvgTranslate(args.vg, 0.f, 0.8f * s);
        }
        nvgScale(args.vg, s, s);
        app::SvgSwitch::draw(args);
        if (active) {
            // Warm backlight glow — matches momentary button press feel
            NVGpaint glow = nvgRadialGradient(args.vg, 50.f, 50.f, 5.f, 35.f,
                nvgRGBA(255, 232, 170, 244), nvgRGBA(255, 211, 92, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 35.f);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);

            NVGpaint core = nvgRadialGradient(args.vg, 50.f, 50.f, 0.f, 18.f,
                nvgRGBA(255, 245, 190, 128), nvgRGBA(255, 190, 84, 0));
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 50.f, 50.f, 18.f);
            nvgFillPaint(args.vg, core);
            nvgFill(args.vg);
        }
        nvgRestore(args.vg);
    }
};

struct RingLight : app::ModuleLightWidget {
    float ringThickness = 4.f;
    float glowThickness = 2.f;
    float innerRadiusOverride = -1.f;
    float outerRadiusOverride = -1.f;

    RingLight() {
        box.size = mm2px(Vec(11.f, 11.f));
        color = nvgRGB(0x2e, 0xea, 0xd8);
    }

    void draw(const DrawArgs& args) override {
        float brightness = module ? module->lights[firstLightId].getBrightness() : 0.f;
        if (brightness <= 0.f) return;

        float cx = box.size.x * 0.5f;
        float cy = box.size.y * 0.5f;
        float outerRadius = outerRadiusOverride > 0.f ? outerRadiusOverride : std::min(box.size.x, box.size.y) * 0.5f;
        float innerRadius = innerRadiusOverride >= 0.f ? innerRadiusOverride : std::max(0.f, outerRadius - ringThickness);
        outerRadius = std::max(innerRadius + 1.f, outerRadius);

        NVGcolor ringColor = color;
        ringColor.a *= brightness;

        // Draw outer glow
        if (glowThickness > 0.f) {
            NVGcolor transparent = nvgRGBAf(ringColor.r, ringColor.g, ringColor.b, 0.f);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cx, cy, outerRadius + glowThickness);
            nvgCircle(args.vg, cx, cy, innerRadius);
            nvgPathWinding(args.vg, NVG_HOLE);
            NVGpaint glow = nvgRadialGradient(args.vg, cx, cy, innerRadius, outerRadius + glowThickness, ringColor, transparent);
            nvgFillPaint(args.vg, glow);
            nvgFill(args.vg);
        }

        // Draw crisp ring stroke
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, (innerRadius + outerRadius) * 0.5f);
        nvgStrokeColor(args.vg, ringColor);
        nvgStrokeWidth(args.vg, std::max(1.f, outerRadius - innerRadius));
        nvgStroke(args.vg);
    }
};

// Alchemical-styled momentary buttons for REST/TIE to match symbol buttons
struct ShapetakerRestMomentary : app::SvgSwitch {
    ShapetakerRestMomentary() {
        momentary = true;
        if (shadow) shadow->visible = false;
        // 9 x 9 mm footprint
        box.size = mm2px(Vec(9.f, 9.f));
    }
    void draw(const DrawArgs& args) override {
        drawPanelContactShadow(args.vg, box.size, 3.0f, 0.22f, 1.1f);
        // Background — match AlchemicalSymbolWidget normal state and bevels
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGBA(40, 40, 40, 100));
        nvgFill(args.vg);
        nvgStrokeColor(args.vg, nvgRGBA(100, 100, 100, 150));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);

        // Inner shadow ring
        float inset = 1.0f;
        float rOuter = 3.0f;
        float rInner = std::max(0.0f, rOuter - 1.0f);
        NVGpaint innerShadow = nvgBoxGradient(
            args.vg,
            inset, inset,
            box.size.x - inset * 2.0f,
            box.size.y - inset * 2.0f,
            rInner, 3.5f,
            nvgRGBA(0, 0, 0, 50), nvgRGBA(0, 0, 0, 0)
        );
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillPaint(args.vg, innerShadow);
        nvgFill(args.vg);

        // Top highlight
        nvgSave(args.vg);
        nvgScissor(args.vg, 0, 0, box.size.x, std::min(6.0f, box.size.y));
        NVGpaint topHi = nvgLinearGradient(args.vg, 0, 0, 0, 6.0f, nvgRGBA(255, 255, 255, 28), nvgRGBA(255, 255, 255, 0));
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillPaint(args.vg, topHi);
        nvgFill(args.vg);
        nvgRestore(args.vg);

        // Side highlights
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        NVGpaint leftHi = nvgLinearGradient(args.vg, inset - 0.5f, 0, inset + 4.5f, 0, nvgRGBA(255, 255, 255, 18), nvgRGBA(255, 255, 255, 0));
        nvgFillPaint(args.vg, leftHi);
        nvgFill(args.vg);
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        NVGpaint rightHi = nvgLinearGradient(args.vg, box.size.x - (inset - 0.5f), 0, box.size.x - (inset + 4.5f), 0, nvgRGBA(255, 255, 255, 12), nvgRGBA(255, 255, 255, 0));
        nvgFillPaint(args.vg, rightHi);
        nvgFill(args.vg);

        // Draw REST glyph in vintage ink
        NVGcolor ink = nvgRGBA(232, 224, 200, 230);
        float cx = box.size.x * 0.5f;
        float cy = box.size.y * 0.5f;
        float w = std::min(box.size.x, box.size.y) * 0.60f;
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, cx - w * 0.5f, cy);
        nvgLineTo(args.vg, cx + w * 0.5f, cy);
        nvgStrokeColor(args.vg, ink);
        nvgLineCap(args.vg, NVG_ROUND);
        nvgStrokeWidth(args.vg, rack::clamp(w * 0.10f, 1.0f, 2.0f));
        nvgStroke(args.vg);

        // Pressed overlay for feedback
        bool pressed = false;
        if (getParamQuantity()) pressed = getParamQuantity()->getValue() > 0.5f;
        if (pressed) {
            nvgSave(args.vg);
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 40));
            nvgFill(args.vg);
            nvgRestore(args.vg);
        }
    }
};

struct ShapetakerTieMomentary : app::SvgSwitch {
    ShapetakerTieMomentary() {
        momentary = true;
        if (shadow) shadow->visible = false;
        // 9 x 9 mm footprint
        box.size = mm2px(Vec(9.f, 9.f));
        if (shadow) shadow->visible = false;
        box.size = Vec(18.f, 18.f);
    }
    void draw(const DrawArgs& args) override {
        drawPanelContactShadow(args.vg, box.size, 3.0f, 0.22f, 1.1f);
        // Background — match AlchemicalSymbolWidget normal state and bevels
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGBA(40, 40, 40, 100));
        nvgFill(args.vg);
        nvgStrokeColor(args.vg, nvgRGBA(100, 100, 100, 150));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);

        // Inner shadow ring
        float inset = 1.0f;
        float rOuter = 3.0f;
        float rInner = std::max(0.0f, rOuter - 1.0f);
        NVGpaint innerShadow = nvgBoxGradient(
            args.vg,
            inset, inset,
            box.size.x - inset * 2.0f,
            box.size.y - inset * 2.0f,
            rInner, 3.5f,
            nvgRGBA(0, 0, 0, 50), nvgRGBA(0, 0, 0, 0)
        );
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillPaint(args.vg, innerShadow);
        nvgFill(args.vg);

        // Top highlight
        nvgSave(args.vg);
        nvgScissor(args.vg, 0, 0, box.size.x, std::min(6.0f, box.size.y));
        NVGpaint topHi = nvgLinearGradient(args.vg, 0, 0, 0, 6.0f, nvgRGBA(255, 255, 255, 28), nvgRGBA(255, 255, 255, 0));
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillPaint(args.vg, topHi);
        nvgFill(args.vg);
        nvgRestore(args.vg);

        // Side highlights
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        NVGpaint leftHi = nvgLinearGradient(args.vg, inset - 0.5f, 0, inset + 4.5f, 0, nvgRGBA(255, 255, 255, 18), nvgRGBA(255, 255, 255, 0));
        nvgFillPaint(args.vg, leftHi);
        nvgFill(args.vg);
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, inset - 0.5f, inset - 0.5f, box.size.x - (inset - 0.5f) * 2.0f, box.size.y - (inset - 0.5f) * 2.0f, rInner + 0.5f);
        nvgRoundedRect(args.vg, inset + 0.8f, inset + 0.8f, box.size.x - (inset + 0.8f) * 2.0f, box.size.y - (inset + 0.8f) * 2.0f, std::max(0.0f, rInner - 0.8f));
        nvgPathWinding(args.vg, NVG_HOLE);
        NVGpaint rightHi = nvgLinearGradient(args.vg, box.size.x - (inset - 0.5f), 0, box.size.x - (inset + 4.5f), 0, nvgRGBA(255, 255, 255, 12), nvgRGBA(255, 255, 255, 0));
        nvgFillPaint(args.vg, rightHi);
        nvgFill(args.vg);

        // Draw TIE glyph (lower arc) in vintage ink
        NVGcolor ink = nvgRGBA(232, 224, 200, 230);
        float cx = box.size.x * 0.5f;
        float cy = box.size.y * 0.52f;
        float r = std::min(box.size.x, box.size.y) * 0.32f;
        nvgBeginPath(args.vg);
        nvgArc(args.vg, cx, cy, r, M_PI * 1.15f, M_PI * 1.85f, NVG_CW);
        nvgStrokeColor(args.vg, ink);
        nvgLineCap(args.vg, NVG_ROUND);
        nvgStrokeWidth(args.vg, rack::clamp(r * 0.28f, 1.0f, 2.0f));
        nvgStroke(args.vg);

        // Pressed overlay for feedback
        bool pressed = false;
        if (getParamQuantity()) pressed = getParamQuantity()->getValue() > 0.5f;
        if (pressed) {
            nvgSave(args.vg);
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 40));
            nvgFill(args.vg);
            nvgRestore(args.vg);
        }
    }
};

// Horizontal 6-way selector for Chiaroscuro distortion types
struct ShapetakerHorizontalDistortionSelector : app::ParamWidget {
    float accumulatedDelta = 0.f;

    ShapetakerHorizontalDistortionSelector() {
        box.size = mm2px(Vec(23.0f, 6.0f));
    }

    int stepCount() {
        if (auto* pq = getParamQuantity()) {
            float range = pq->maxValue - pq->minValue;
            return std::max(2, (int)std::round(range) + 1);
        }
        return 6;
    }

    void geometry(float& left, float& right, float& trackY, float& trackH, float& knobR) const {
        knobR = box.size.y * 0.45f;
        float margin = box.size.x * 0.02f;
        left = knobR + margin;
        right = box.size.x - knobR - margin;
        trackH = box.size.y * 0.36f;
        trackY = (box.size.y - trackH) * 0.5f;
    }

    void setValueFromPos(float x) {
        auto* pq = getParamQuantity();
        if (!pq) {
            return;
        }
        float left = 0.f;
        float right = 0.f;
        float trackY = 0.f;
        float trackH = 0.f;
        float knobR = 0.f;
        geometry(left, right, trackY, trackH, knobR);
        float t = 0.f;
        if (right > left) {
            t = clamp((x - left) / (right - left), 0.f, 1.f);
        }
        int steps = stepCount();
        float value = pq->minValue + std::round(t * (steps - 1));
        pq->setValue(clamp(value, pq->minValue, pq->maxValue));
    }

    void onDragStart(const event::DragStart& e) override {
        accumulatedDelta = 0.f;
        ParamWidget::onDragStart(e);
    }

    void onDragMove(const event::DragMove& e) override {
        auto* pq = getParamQuantity();
        if (!pq) {
            return;
        }
        accumulatedDelta += e.mouseDelta.x;
        float stepThreshold = 32.f;
        if (std::fabs(accumulatedDelta) >= stepThreshold) {
            float dir = (accumulatedDelta > 0.f) ? 1.f : -1.f;
            float value = pq->getValue() + dir;
            pq->setValue(clamp(value, pq->minValue, pq->maxValue));
            accumulatedDelta = 0.f;
        }
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this);
        }
        ParamWidget::onButton(e);
    }

    void draw(const DrawArgs& args) override {
        auto* pq = getParamQuantity();
        float left = 0.f;
        float right = 0.f;
        float trackY = 0.f;
        float trackH = 0.f;
        float knobR = 0.f;
        geometry(left, right, trackY, trackH, knobR);

        NVGcontext* vg = args.vg;
        float w = box.size.x;
        float h = box.size.y;
        float cy = h * 0.5f;

        nvgSave(vg);

        // === HOUSING: recessed anodized aluminum mounting plate ===

        // Outer lip shadow (housing sits recessed into the panel)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, -1.0f, -0.5f, w + 2.0f, h + 2.0f, 4.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 50));
        nvgFill(vg);

        // Panel lip highlight above housing (light catches top edge of recess)
        nvgBeginPath(vg);
        nvgMoveTo(vg, 1.0f, -0.5f);
        nvgLineTo(vg, w - 1.0f, -0.5f);
        nvgStrokeColor(vg, nvgRGBA(95, 98, 105, 60));
        nvgStrokeWidth(vg, 0.6f);
        nvgStroke(vg);

        // Drop shadow beneath housing body (depth from panel surface)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0.5f, 2.0f, w - 1.f, h, 3.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 85));
        nvgFill(vg);

        // Main housing body
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0.f, 0.f, w, h, 3.0f);
        NVGpaint housing = nvgLinearGradient(vg, 0, 0, 0, h,
            nvgRGBA(82, 84, 90, 255),
            nvgRGBA(38, 40, 44, 255));
        nvgFillPaint(vg, housing);
        nvgFill(vg);

        // Brushed metal texture (horizontal grain)
        for (int i = 0; i < 16; i++) {
            float ly = 1.5f + (h - 3.0f) * ((float)i / 15.0f);
            nvgBeginPath(vg);
            nvgMoveTo(vg, 2.5f, ly);
            nvgLineTo(vg, w - 2.5f, ly);
            int alpha = (i % 3 == 0) ? 22 : ((i % 3 == 1) ? 12 : 8);
            nvgStrokeColor(vg, nvgRGBA(115, 118, 124, alpha));
            nvgStrokeWidth(vg, 0.3f);
            nvgStroke(vg);
        }

        // Top chamfer highlight
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3.0f, 0.8f);
        nvgLineTo(vg, w - 3.0f, 0.8f);
        nvgStrokeColor(vg, nvgRGBA(140, 145, 150, 80));
        nvgStrokeWidth(vg, 0.7f);
        nvgStroke(vg);

        // Bottom shadow edge (housing bottom is darker)
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3.0f, h - 0.5f);
        nvgLineTo(vg, w - 3.0f, h - 0.5f);
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 70));
        nvgStrokeWidth(vg, 0.7f);
        nvgStroke(vg);

        // Housing edge bevel (dark outline)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0.f, 0.f, w, h, 3.0f);
        nvgStrokeColor(vg, nvgRGBA(12, 12, 15, 220));
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);

        // Inner highlight (inset border for recessed plate look)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 1.0f, 1.0f, w - 2.0f, h - 2.0f, 2.5f);
        nvgStrokeColor(vg, nvgRGBA(100, 103, 110, 35));
        nvgStrokeWidth(vg, 0.5f);
        nvgStroke(vg);

        // === MACHINED CHANNEL GROOVE (deep recess) ===
        float channelH = h * 0.32f;
        float channelY = cy - channelH * 0.5f;
        float channelLeft = left - knobR * 0.35f;
        float channelRight = right + knobR * 0.35f;
        float channelW = channelRight - channelLeft;
        float channelR = channelH * 0.45f;

        // Outer shadow halo (soft shadow around the groove for depth)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, channelLeft - 1.5f, channelY - 1.5f,
            channelW + 3.f, channelH + 3.f, channelR + 1.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 50));
        nvgFill(vg);

        // Inner shadow (sharp dark edge at top of groove = deep cut)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, channelLeft - 0.5f, channelY - 1.0f,
            channelW + 1.f, channelH + 1.5f, channelR);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 150));
        nvgFill(vg);

        // Channel body (dark machined slot)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, channelLeft, channelY, channelW, channelH, channelR);
        NVGpaint channelPaint = nvgLinearGradient(vg, 0, channelY, 0, channelY + channelH,
            nvgRGBA(5, 5, 8, 255),
            nvgRGBA(18, 19, 24, 255));
        nvgFillPaint(vg, channelPaint);
        nvgFill(vg);

        // Channel floor specular (faint reflection on the groove floor)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, channelLeft + 2.0f, channelY + channelH * 0.55f,
            channelW - 4.0f, channelH * 0.35f, channelR * 0.5f);
        nvgFillColor(vg, nvgRGBA(55, 58, 65, 20));
        nvgFill(vg);

        // Channel top inner edge (dark bevel inside groove)
        nvgBeginPath(vg);
        nvgMoveTo(vg, channelLeft + channelR, channelY + 0.5f);
        nvgLineTo(vg, channelRight - channelR, channelY + 0.5f);
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 100));
        nvgStrokeWidth(vg, 0.5f);
        nvgStroke(vg);

        // Channel bottom lip highlight (light catches the lower edge)
        nvgBeginPath(vg);
        nvgMoveTo(vg, channelLeft + channelR, channelY + channelH);
        nvgLineTo(vg, channelRight - channelR, channelY + channelH);
        nvgStrokeColor(vg, nvgRGBA(90, 92, 100, 55));
        nvgStrokeWidth(vg, 0.5f);
        nvgStroke(vg);

        // === HORIZONTAL GUIDE RAIL (silver throttle bar) ===
        {
            float railH = channelH * 0.38f;
            float railY = cy - railH * 0.5f;
            float railInset = channelR * 0.6f;
            float railLeft = channelLeft + railInset;
            float railW = channelW - railInset * 2.0f;
            float railR = railH * 0.35f;

            // Rail shadow (sits slightly recessed)
            nvgBeginPath(vg);
            nvgRoundedRect(vg, railLeft, railY + 0.6f, railW, railH, railR);
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 90));
            nvgFill(vg);

            // Rail body — brushed silver cylinder
            nvgBeginPath(vg);
            nvgRoundedRect(vg, railLeft, railY, railW, railH, railR);
            NVGpaint railPaint = nvgLinearGradient(vg, 0, railY, 0, railY + railH,
                nvgRGBA(170, 175, 185, 180),
                nvgRGBA(85, 88, 98, 180));
            nvgFillPaint(vg, railPaint);
            nvgFill(vg);

            // Top specular highlight (cylindrical reflection)
            nvgBeginPath(vg);
            nvgRoundedRect(vg, railLeft + 1.5f, railY + 0.4f,
                railW - 3.0f, railH * 0.30f, railR * 0.5f);
            nvgFillColor(vg, nvgRGBA(220, 225, 235, 55));
            nvgFill(vg);

            // Bottom edge shadow
            nvgBeginPath(vg);
            nvgMoveTo(vg, railLeft + railR, railY + railH - 0.3f);
            nvgLineTo(vg, railLeft + railW - railR, railY + railH - 0.3f);
            nvgStrokeColor(vg, nvgRGBA(30, 32, 38, 90));
            nvgStrokeWidth(vg, 0.4f);
            nvgStroke(vg);

            // Subtle top edge line
            nvgBeginPath(vg);
            nvgMoveTo(vg, railLeft + railR, railY + 0.3f);
            nvgLineTo(vg, railLeft + railW - railR, railY + 0.3f);
            nvgStrokeColor(vg, nvgRGBA(200, 205, 215, 40));
            nvgStrokeWidth(vg, 0.3f);
            nvgStroke(vg);
        }

        // Parameter values for handle positioning
        int steps = stepCount();
        float value = pq ? pq->getValue() : 0.f;
        float minV = pq ? pq->minValue : 0.f;
        float maxV = pq ? pq->maxValue : (float)(steps - 1);

        // === SLIDER HANDLE (3D extruded knurled tab) ===
        float t = (maxV > minV) ? (value - minV) / (maxV - minV) : 0.f;
        float handleCX = math::rescale(t, 0.f, 1.f, left, right);

        float handleW = h * 0.62f;
        float handleH = h * 0.92f;
        float handleX = handleCX - handleW * 0.5f;
        float handleY = cy - handleH * 0.5f;
        float handleR = 2.0f;

        // Soft drop shadow (handle floats above housing)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, handleX - 0.5f, handleY + 2.5f, handleW + 1.0f, handleH, handleR + 0.5f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 60));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, handleX + 0.5f, handleY + 1.8f, handleW, handleH, handleR);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 100));
        nvgFill(vg);

        // Handle base layer (darker underside showing handle thickness)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, handleX - 0.3f, handleY + 0.8f, handleW + 0.6f, handleH, handleR + 0.3f);
        nvgFillColor(vg, nvgRGBA(60, 58, 52, 255));
        nvgFill(vg);

        // Handle body - brushed stainless steel top face
        nvgBeginPath(vg);
        nvgRoundedRect(vg, handleX, handleY, handleW, handleH - 1.0f, handleR);
        NVGpaint handlePaint = nvgLinearGradient(vg, handleX, handleY, handleX, handleY + handleH,
            nvgRGBA(205, 202, 194, 255),
            nvgRGBA(125, 122, 115, 255));
        nvgFillPaint(vg, handlePaint);
        nvgFill(vg);

        // Left edge highlight (directional light from upper-left)
        nvgBeginPath(vg);
        nvgMoveTo(vg, handleX + 0.5f, handleY + handleR);
        nvgLineTo(vg, handleX + 0.5f, handleY + handleH - handleR - 1.0f);
        nvgStrokeColor(vg, nvgRGBA(225, 222, 215, 70));
        nvgStrokeWidth(vg, 0.6f);
        nvgStroke(vg);

        // Right edge shadow (opposite side from light)
        nvgBeginPath(vg);
        nvgMoveTo(vg, handleX + handleW - 0.5f, handleY + handleR);
        nvgLineTo(vg, handleX + handleW - 0.5f, handleY + handleH - handleR - 1.0f);
        nvgStrokeColor(vg, nvgRGBA(30, 28, 24, 80));
        nvgStrokeWidth(vg, 0.6f);
        nvgStroke(vg);

        // Top face specular highlight (bright spot on raised surface)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, handleX + 1.5f, handleY + 0.8f,
            handleW - 3.0f, handleH * 0.18f, handleR * 0.5f);
        nvgFillColor(vg, nvgRGBA(240, 238, 232, 65));
        nvgFill(vg);

        // Secondary specular (wider, dimmer reflection below)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, handleX + 2.0f, handleY + handleH * 0.22f,
            handleW - 4.0f, handleH * 0.12f, 1.0f);
        nvgFillColor(vg, nvgRGBA(220, 218, 212, 25));
        nvgFill(vg);

        // Handle edge outline (crisp machined edge)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, handleX, handleY, handleW, handleH - 1.0f, handleR);
        nvgStrokeColor(vg, nvgRGBA(42, 40, 36, 210));
        nvgStrokeWidth(vg, 0.9f);
        nvgStroke(vg);

        // Knurling/grip lines (machined horizontal grooves)
        int gripLines = 5;
        float gripZoneTop = cy - (handleH - 1.0f) * 0.28f;
        float gripZoneH = (handleH - 1.0f) * 0.56f;
        float gripSpacing = gripZoneH / (float)(gripLines - 1);
        float gripInset = 2.5f;
        for (int g = 0; g < gripLines; g++) {
            float gy = gripZoneTop + g * gripSpacing;
            // Dark groove (cut into metal)
            nvgBeginPath(vg);
            nvgMoveTo(vg, handleX + gripInset, gy);
            nvgLineTo(vg, handleX + handleW - gripInset, gy);
            nvgStrokeColor(vg, nvgRGBA(40, 38, 32, 155));
            nvgStrokeWidth(vg, 0.6f);
            nvgStroke(vg);
            // Light ridge below (sharp edge catches light)
            nvgBeginPath(vg);
            nvgMoveTo(vg, handleX + gripInset, gy + 0.6f);
            nvgLineTo(vg, handleX + handleW - gripInset, gy + 0.6f);
            nvgStrokeColor(vg, nvgRGBA(215, 212, 205, 50));
            nvgStrokeWidth(vg, 0.35f);
            nvgStroke(vg);
        }

        nvgRestore(vg);
    }
};

// Blade-style 6-way selector for Chiaroscuro distortion types
struct ShapetakerBladeDistortionSelector : app::ParamWidget {
    float accumulatedDelta = 0.f;
    bool drawBase = true;
    bool drawDetents = true;

    ShapetakerBladeDistortionSelector() {
        box.size = mm2px(Vec(24.15f, 7.82f));
    }

    int stepCount() {
        if (auto* pq = getParamQuantity()) {
            float range = pq->maxValue - pq->minValue;
            return std::max(2, (int)std::round(range) + 1);
        }
        return 6;
    }

    void geometry(float& left, float& right, float& slotY, float& slotH) const {
        float margin = box.size.x * 0.09f;
        left = margin;
        right = box.size.x - margin;
        slotH = 3.0f;
        // Keep slot at original panel position: the widget is centered at SVG's
        // 43.2mm but box is now taller. Offset from the original 3.4mm layout.
        float oldBoxH = mm2px(3.4f);
        float oldSlotY = (oldBoxH - slotH) * 0.62f;
        slotY = oldSlotY + (box.size.y - oldBoxH) * 0.5f;
    }

    float detentAngleForIndex(int index, int steps) const {
        if (steps == 6) {
            // 6 distinct angles for each position
            static const float angles[6] = {-25.0f, -15.0f, -5.0f, 5.0f, 15.0f, 25.0f};
            index = std::max(0, std::min(index, 5));
            return angles[index];
        }
        if (steps <= 1) {
            return 0.0f;
        }
        float t = (float) index / (float) (steps - 1);
        return math::rescale(t, 0.0f, 1.0f, -25.0f, 25.0f);
    }

    void setValueFromPos(float x) {
        auto* pq = getParamQuantity();
        if (!pq) {
            return;
        }
        float left = 0.f;
        float right = 0.f;
        float slotY = 0.f;
        float slotH = 0.f;
        geometry(left, right, slotY, slotH);
        float t = 0.f;
        if (right > left) {
            t = clamp((x - left) / (right - left), 0.f, 1.f);
        }
        int steps = stepCount();
        float value = pq->minValue + std::round(t * (steps - 1));
        pq->setValue(clamp(value, pq->minValue, pq->maxValue));
    }

    void onDragStart(const event::DragStart& e) override {
        accumulatedDelta = 0.f;
        ParamWidget::onDragStart(e);
    }

    void onDragMove(const event::DragMove& e) override {
        auto* pq = getParamQuantity();
        if (!pq) {
            return;
        }
        accumulatedDelta += e.mouseDelta.x;
        float stepThreshold = 28.f;
        if (std::fabs(accumulatedDelta) >= stepThreshold) {
            float dir = (accumulatedDelta > 0.f) ? 1.f : -1.f;
            float value = pq->getValue() + dir;
            pq->setValue(clamp(value, pq->minValue, pq->maxValue));
            accumulatedDelta = 0.f;
        }
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this);
        }
        ParamWidget::onButton(e);
    }

    void draw(const DrawArgs& args) override {
        auto* pq = getParamQuantity();
        float left = 0.f;
        float right = 0.f;
        float slotY = 0.f;
        float slotH = 0.f;
        geometry(left, right, slotY, slotH);

        NVGcontext* vg = args.vg;
        float w = box.size.x;
        float h = box.size.y;

        nvgSave(vg);

        int steps = stepCount();

        // SVG override: render position SVG for mapped detents
        {
            float minV_ = pq ? pq->minValue : 0.f;
            float maxV_ = pq ? pq->maxValue : (float)(steps - 1);
            float val_  = pq ? pq->getValue() : 0.f;
            float t_    = (maxV_ > minV_) ? (val_ - minV_) / (maxV_ - minV_) : 0.f;
            int di = (steps <= 1) ? 0 : (int)std::round(clamp(t_, 0.0f, 1.0f) * (steps - 1));

            static const char* svgPaths[6] = {
                "res/switches/five_pos_sw_pos1.svg",
                "res/switches/five_pos_sw_pos2.svg",
                "res/switches/five_pos_sw_pos3.svg",
                "res/switches/five_pos_sw_pos4.svg",
                "res/switches/five_pos_sw_pos5.svg",
                "res/switches/five_pos_sw_pos5.svg",  // placeholder until pos6 is authored
            };
            const char* svgPath = (di >= 0 && di < 6) ? svgPaths[di] : nullptr;

            if (svgPath) {
                auto svg = APP->window->loadSvg(asset::plugin(pluginInstance, svgPath));
                if (svg && svg->handle) {
                    float svgW = svg->handle->width;
                    float svgH = svg->handle->height;
                    if (svgW > 0.f && svgH > 0.f) {
                        float scale = std::min(w / svgW, h / svgH);
                        nvgTranslate(vg, (w - svgW * scale) * 0.5f, (h - svgH * scale) * 0.5f);
                        nvgScale(vg, scale, scale);
                        svgDraw(vg, svg->handle);
                    }
                }
            }
        }

        nvgRestore(vg);
    }
};
// Horizontal 4-position selector using five_pos_sw SVG frames (pos1/2/4/5).
// Skips the two-bar centre pos3; positions map to the four oscillator
// interaction modes on Torsion (and any other 4-step param).
struct ShapetakerFivePosSwitch : app::ParamWidget {
    float accumulatedDelta = 0.f;

    ShapetakerFivePosSwitch() {
        box.size = mm2px(Vec(24.15f, 7.82f));
    }

    int stepCount() {
        if (auto* pq = getParamQuantity()) {
            float range = pq->maxValue - pq->minValue;
            return std::max(2, (int)std::round(range) + 1);
        }
        return 4;
    }

    void setValueFromPos(float x) {
        auto* pq = getParamQuantity();
        if (!pq) return;
        float margin = box.size.x * 0.09f;
        float left   = margin;
        float right  = box.size.x - margin;
        float t = (right > left) ? clamp((x - left) / (right - left), 0.f, 1.f) : 0.f;
        int steps = stepCount();
        float value = pq->minValue + std::round(t * (steps - 1));
        pq->setValue(clamp(value, pq->minValue, pq->maxValue));
    }

    void onDragStart(const event::DragStart& e) override {
        accumulatedDelta = 0.f;
        ParamWidget::onDragStart(e);
    }

    void onDragMove(const event::DragMove& e) override {
        auto* pq = getParamQuantity();
        if (!pq) return;
        accumulatedDelta += e.mouseDelta.x;
        float stepThreshold = 28.f;
        if (std::fabs(accumulatedDelta) >= stepThreshold) {
            float dir = (accumulatedDelta > 0.f) ? 1.f : -1.f;
            float value = pq->getValue() + dir;
            pq->setValue(clamp(value, pq->minValue, pq->maxValue));
            accumulatedDelta = 0.f;
        }
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            // Consume to start drag, but do not jump to the clicked X position.
            // Value changes only via onDragMove (click-hold-drag).
            e.consume(this);
        }
        ParamWidget::onButton(e);
    }

    void draw(const DrawArgs& args) override {
        auto* pq = getParamQuantity();
        NVGcontext* vg = args.vg;
        float w = box.size.x;
        float h = box.size.y;

        int steps   = stepCount();
        float minV_ = pq ? pq->minValue : 0.f;
        float maxV_ = pq ? pq->maxValue : (float)(steps - 1);
        float val_  = pq ? pq->getValue() : 0.f;
        float t_    = (maxV_ > minV_) ? (val_ - minV_) / (maxV_ - minV_) : 0.f;
        int di = (steps <= 1) ? 0 : (int)std::round(clamp(t_, 0.0f, 1.0f) * (steps - 1));

        // Four-step parameters skip the two-bar center frame. Five-step
        // parameters use the full set, allowing layered editors to expose a
        // fifth mode without changing the established four-way appearance.
        static const char* svgPaths[5] = {
            "res/switches/five_pos_sw_pos1.svg",
            "res/switches/five_pos_sw_pos2.svg",
            "res/switches/five_pos_sw_pos3.svg",
            "res/switches/five_pos_sw_pos4.svg",
            "res/switches/five_pos_sw_pos5.svg",
        };
        int frame = di;
        if (steps == 4 && di >= 2) frame = di + 1;
        const char* svgPath = (frame >= 0 && frame < 5) ? svgPaths[frame] : nullptr;

        drawPanelContactShadow(vg, box.size, 3.4f, 0.20f, 1.0f);
        nvgSave(vg);
        if (svgPath) {
            auto svg = APP->window->loadSvg(asset::plugin(pluginInstance, svgPath));
            if (svg && svg->handle) {
                float svgW = svg->handle->width;
                float svgH = svg->handle->height;
                if (svgW > 0.f && svgH > 0.f) {
                    float scale = std::min(w / svgW, h / svgH);
                    nvgTranslate(vg, (w - svgW * scale) * 0.5f, (h - svgH * scale) * 0.5f);
                    nvgScale(vg, scale, scale);
                    svgDraw(vg, svg->handle);
                }
            }
        }
        nvgRestore(vg);
    }
};

// 2-position toggle using five_pos_sw pos1 (off/left) and pos5 (on/max/right).
// Same 24.15x7.82mm footprint as ShapetakerFivePosSwitch.
struct ShapetakerFivePosToggle : app::ParamWidget {
    float accumulatedDelta = 0.f;

    ShapetakerFivePosToggle() {
        box.size = mm2px(Vec(24.15f, 7.82f));
    }

    void onDragStart(const event::DragStart& e) override {
        accumulatedDelta = 0.f;
        ParamWidget::onDragStart(e);
    }

    void onDragMove(const event::DragMove& e) override {
        auto* pq = getParamQuantity();
        if (!pq) return;
        accumulatedDelta += e.mouseDelta.x;
        if (std::fabs(accumulatedDelta) >= 28.f) {
            float value = (accumulatedDelta > 0.f) ? pq->maxValue : pq->minValue;
            pq->setValue(value);
            accumulatedDelta = 0.f;
        }
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this); // drag-only: consume for drag init, no click-to-flip
        }
        ParamWidget::onButton(e);
    }

    void draw(const DrawArgs& args) override {
        auto* pq = getParamQuantity();
        bool on = pq && (pq->getValue() > (pq->minValue + pq->maxValue) * 0.5f);
        const char* svgPath = on
            ? "res/switches/five_pos_sw_pos5.svg"
            : "res/switches/five_pos_sw_pos1.svg";
        float w = box.size.x;
        float h = box.size.y;
        drawPanelContactShadow(args.vg, box.size, 3.4f, 0.20f, 1.0f);
        nvgSave(args.vg);
        auto svg = APP->window->loadSvg(asset::plugin(pluginInstance, svgPath));
        if (svg && svg->handle) {
            float svgW = svg->handle->width;
            float svgH = svg->handle->height;
            if (svgW > 0.f && svgH > 0.f) {
                float scale = std::min(w / svgW, h / svgH);
                nvgTranslate(args.vg, (w - svgW * scale) * 0.5f, (h - svgH * scale) * 0.5f);
                nvgScale(args.vg, scale, scale);
                svgDraw(args.vg, svg->handle);
            }
        }
        nvgRestore(args.vg);
    }
};

// VUMeterWidget moved to shapetakerWidgets.hpp (namespace shapetaker)

// Legacy JewelLED variants removed in favor of shapetakerWidgets.hpp LEDs
/* struct JewelLED : ModuleLightWidget {
    JewelLED() {
        // Set a fixed size
        box.size = Vec(25, 25);
        
        // Try to load the jewel SVG, fallback to simple shape if it fails
        widget::SvgWidget* sw = new widget::SvgWidget;
        std::shared_ptr<Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_simple.svg"));
        
        if (svg) {
            // SVG loaded successfully
            sw->setSvg(svg);
            addChild(sw);
        }
        
        // Set up proper RGB color mixing like RedGreenBlueLight
        // Based on Chiaroscuro code: Red increases, Green decreases, Blue = 0
        addBaseColor(nvgRGB(0xff, 0x00, 0x00)); // Red channel
        addBaseColor(nvgRGB(0x00, 0xff, 0x00)); // Green channel  
        addBaseColor(nvgRGB(0x00, 0x00, 0xff)); // Blue channel (unused but needed for RGB)
    }
    
    void step() override {
        ModuleLightWidget::step();
        
        // Override the color mixing to get proper green-to-red transition
        if (module) {
            float r = module->lights[firstLightId + 0].getBrightness(); // Red increases with intensity
            float g = module->lights[firstLightId + 1].getBrightness(); // Green decreases with intensity
            float b = module->lights[firstLightId + 2].getBrightness(); // Blue (always 0)
            
            // Set the color directly for better control
            color = nvgRGBAf(r, g, b, fmaxf(r, g));
        }
    }
    
    void draw(const DrawArgs& args) override {
        // If SVG didn't load, draw a simple jewel shape
        if (children.empty()) {
            // Draw chrome bezel
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 12.5, 12.5, 12);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);
            
            // Draw inner ring
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 12.5, 12.5, 8);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }
        
        // Call parent draw for lighting effects
        ModuleLightWidget::draw(args);
    }
}; */

struct JewelLEDSmall : ModuleLightWidget {
    widget::SvgWidget* sw = nullptr;

    void drawHalo(const DrawArgs& args) override {
        // Amp-style jewel lenses do not cast a panel halo.
    }

    JewelLEDSmall() {
        // Set a smaller size to reduce glow radius
        box.size = Vec(10, 10);

        // Try to load the jewel SVG, fallback to simple shape if it fails
        sw = new widget::SvgWidget;
        std::shared_ptr<Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_small.svg"));

        if (svg) {
            // SVG loaded successfully
            sw->setSvg(svg);
            // Center the SVG within the smaller box (SVG is ~15px, box is 10px)
            sw->box.pos = Vec(-2.5, -2.5);
            addChild(sw);
        }

        // Set up proper RGB color mixing like RedGreenBlueLight
        addBaseColor(nvgRGB(0xff, 0x00, 0x00)); // Red channel
        addBaseColor(nvgRGB(0x00, 0xff, 0x00)); // Green channel
        addBaseColor(nvgRGB(0x00, 0x00, 0xff)); // Blue channel
    }

    void step() override {
        ModuleLightWidget::step();

        if (module) {
            float r = module->lights[firstLightId + 0].getBrightness();
            float g = module->lights[firstLightId + 1].getBrightness();
            float b = module->lights[firstLightId + 2].getBrightness();

            color = nvgRGBAf(r, g, b, fmaxf(r, g));
        }
    }

    void drawLight(const DrawArgs& args) override {
        shapetaker::ui::drawAmpJewelLight(args, box.size, color, 0.36f, 0.50f);
    }

    void draw(const DrawArgs& args) override {
        if (children.empty()) {
            // Fallback drawing if SVG fails to load (centered on 5,5 instead of 7.5,7.5)
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 5, 5, 4.8);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 5, 5, 3.2);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }
        // Draw SVG children first
        widget::Widget::draw(args);
        nvgSave(args.vg);
        nvgTranslate(args.vg, -2.5f, -2.5f);
        shapetaker::ui::drawAmpJewelLensShade(args, Vec(15.f, 15.f), 0.38f);
        nvgRestore(args.vg);

        // Overlay the colored light using additive blending (dimmed to preserve facets)
        nvgSave(args.vg);
        nvgGlobalAlpha(args.vg, shapetaker::ui::kJewelMaxBrightness);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        drawLight(args);
        nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
        drawHalo(args);
        nvgRestore(args.vg);
    }
};

struct JewelLEDCompact : ModuleLightWidget {
    widget::SvgWidget* sw = nullptr;

    void drawHalo(const DrawArgs& args) override {
        // Amp-style jewel lenses do not cast a panel halo.
    }

    JewelLEDCompact() {
        // Intermediate size between Small (10px) and Medium (30px)
        box.size = Vec(18.f, 18.f);

        // Use the medium SVG, scaled to fit
        sw = new widget::SvgWidget;
        std::shared_ptr<Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_medium.svg"));

        if (svg) {
            sw->setSvg(svg);
            addChild(sw);
        }

        // Set up proper RGB color mixing
        addBaseColor(nvgRGB(0xff, 0x00, 0x00));
        addBaseColor(nvgRGB(0x00, 0xff, 0x00));
        addBaseColor(nvgRGB(0x00, 0x00, 0xff));
    }

    void step() override {
        ModuleLightWidget::step();

        if (module) {
            float r = module->lights[firstLightId + 0].getBrightness();
            float g = module->lights[firstLightId + 1].getBrightness();
            float b = module->lights[firstLightId + 2].getBrightness();

            color = nvgRGBAf(r, g, b, fmaxf(fmaxf(r, g), b));
        }
    }

    void drawLight(const DrawArgs& args) override {
        shapetaker::ui::drawAmpJewelLight(args, box.size, color, 0.36f, 0.50f);
    }

    void draw(const DrawArgs& args) override {
        constexpr float svgSize = 20.f;
        float s = std::min(box.size.x, box.size.y) / svgSize;
        float tx = (box.size.x - svgSize * s) * 0.5f;
        float ty = (box.size.y - svgSize * s) * 0.5f;

        nvgSave(args.vg);
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);

        if (children.empty()) {
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 10, 10, 9.6);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 10, 10, 8.0);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }
        // Draw SVG children first
        widget::Widget::draw(args);
        nvgRestore(args.vg);

        shapetaker::ui::drawAmpJewelLensShade(args, box.size, 0.38f);

        // Overlay the colored light using additive blending (dimmed to preserve facets)
        nvgSave(args.vg);
        nvgGlobalAlpha(args.vg, shapetaker::ui::kJewelMaxBrightness);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        drawLight(args);
        nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
        drawHalo(args);
        nvgRestore(args.vg);
    }
};

struct JewelLEDMedium : ModuleLightWidget {
    widget::SvgWidget* sw = nullptr;

    void drawHalo(const DrawArgs& args) override {
        // Amp-style jewel lenses do not cast a panel halo.
    }

    JewelLEDMedium() {
        // Set a fixed size (target footprint 30x30 px)
        box.size = Vec(30.f, 30.f);

        // Try to load the jewel SVG, fallback to simple shape if it fails
        sw = new widget::SvgWidget;
        std::shared_ptr<Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_medium.svg"));

        if (svg) {
            // SVG loaded successfully
            sw->setSvg(svg);
            addChild(sw);
        }

        // Set up proper RGB color mixing like RedGreenBlueLight
        addBaseColor(nvgRGB(0xff, 0x00, 0x00));
        addBaseColor(nvgRGB(0x00, 0xff, 0x00));
        addBaseColor(nvgRGB(0x00, 0x00, 0xff));
    }

    void step() override {
        ModuleLightWidget::step();

        if (module) {
            float r = module->lights[firstLightId + 0].getBrightness();
            float g = module->lights[firstLightId + 1].getBrightness();
            float b = module->lights[firstLightId + 2].getBrightness();

            color = nvgRGBAf(r, g, b, fmaxf(fmaxf(r, g), b));
        }
    }

    void drawLight(const DrawArgs& args) override {
        shapetaker::ui::drawAmpJewelLight(args, box.size, color, 0.36f, 0.50f);
    }

    void draw(const DrawArgs& args) override {
        constexpr float svgSize = 20.f;
        float s = std::min(box.size.x, box.size.y) / svgSize;
        float tx = (box.size.x - svgSize * s) * 0.5f;
        float ty = (box.size.y - svgSize * s) * 0.5f;

        nvgSave(args.vg);
        nvgTranslate(args.vg, tx, ty);
        nvgScale(args.vg, s, s);

        if (children.empty()) {
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 10, 10, 9.6);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 10, 10, 8.0);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }
        // Draw SVG children first
        widget::Widget::draw(args);
        nvgRestore(args.vg);

        // Draw illumination outside the scaled context and clip it to the lens area.
        shapetaker::ui::drawAmpJewelLensShade(args, box.size, 0.38f);
        nvgSave(args.vg);
        nvgGlobalAlpha(args.vg, shapetaker::ui::kJewelMaxBrightness);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        drawLight(args);
        nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
        drawHalo(args);
        nvgRestore(args.vg);
    }
};

// TealJewelLEDMedium and PurpleJewelLEDMedium are now defined in shapetakerWidgets.hpp

struct JewelLEDLarge : ModuleLightWidget {
    std::shared_ptr<Svg> jewel_svg;

    JewelLEDLarge() {
        // Set a slightly larger size for the jewel case (between medium and xlarge)
        box.size = mm2px(Vec(8.f, 8.f));

        jewel_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_large.svg"));

        // Set up proper RGB color mixing like RedGreenBlueLight
        addBaseColor(nvgRGB(0xff, 0x00, 0x00)); // Red channel
        addBaseColor(nvgRGB(0x00, 0xff, 0x00)); // Green channel
        addBaseColor(nvgRGB(0x00, 0x00, 0xff)); // Blue channel
    }

    void step() override {
        ModuleLightWidget::step();

        if (module) {
            float r = module->lights[firstLightId + 0].getBrightness();
            float g = module->lights[firstLightId + 1].getBrightness();
            float b = module->lights[firstLightId + 2].getBrightness();

            color = nvgRGBAf(r, g, b, fmaxf(fmaxf(r, g), b));
        }
    }

    void drawLight(const DrawArgs& args) override {
        shapetaker::ui::drawAmpJewelLight(args, box.size, color, 0.36f, 0.50f);
    }

    void drawHalo(const DrawArgs& args) override {
        // Amp-style jewel lenses do not cast a panel halo.
    }

    void draw(const DrawArgs& args) override {
        // Draw SVG bezel/facets scaled to box.size so centering stays correct
        if (jewel_svg && jewel_svg->handle) {
            float svgW = jewel_svg->handle->width;
            float svgH = jewel_svg->handle->height;
            if (svgW > 0.f && svgH > 0.f) {
                float scale = std::min(box.size.x / svgW, box.size.y / svgH);
                float ox = (box.size.x - svgW * scale) * 0.5f;
                float oy = (box.size.y - svgH * scale) * 0.5f;
                nvgSave(args.vg);
                nvgTranslate(args.vg, ox, oy);
                nvgScale(args.vg, scale, scale);
                svgDraw(args.vg, jewel_svg->handle);
                nvgRestore(args.vg);
            }
        } else {
            // Fallback: procedural circles when SVG unavailable
            float cx = box.size.x * 0.5f;
            float cy = box.size.y * 0.5f;
            float outerRadius = 0.48f * std::min(box.size.x, box.size.y);
            float innerRadius = outerRadius * 0.67f;
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cx, cy, outerRadius);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cx, cy, innerRadius);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }
        shapetaker::ui::drawAmpJewelLensShade(args, box.size, 0.38f);

        // Overlay the colored light using additive blending (dimmed to preserve facets)
        nvgSave(args.vg);
        nvgGlobalAlpha(args.vg, shapetaker::ui::kJewelMaxBrightness);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        drawLight(args);
        nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
        drawHalo(args);
        nvgRestore(args.vg);
    }
};

struct JewelLEDXLarge : ModuleLightWidget {
    JewelLEDXLarge() {
        // Set a larger size for the jewel case
        box.size = mm2px(Vec(16.f, 16.f));

        // Try to load the jewel SVG, fallback to simple shape if it fails
        widget::SvgWidget* sw = new widget::SvgWidget;
        std::shared_ptr<Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_xlarge.svg"));

        if (svg) {
            // SVG loaded successfully
            sw->setSvg(svg);
            sw->box.size = box.size;
            sw->box.pos = Vec(0.f, 0.f);
            addChild(sw);
        }

        // Set up proper RGB color mixing like RedGreenBlueLight
        addBaseColor(nvgRGB(0xff, 0x00, 0x00)); // Red channel
        addBaseColor(nvgRGB(0x00, 0xff, 0x00)); // Green channel
        addBaseColor(nvgRGB(0x00, 0x00, 0xff)); // Blue channel
    }

    void step() override {
        ModuleLightWidget::step();

        if (module) {
            float r = module->lights[firstLightId + 0].getBrightness();
            float g = module->lights[firstLightId + 1].getBrightness();
            float b = module->lights[firstLightId + 2].getBrightness();

            color = nvgRGBAf(r, g, b, fmaxf(fmaxf(r, g), b));
        }
    }

    void drawLight(const DrawArgs& args) override {
        shapetaker::ui::drawAmpJewelLight(args, box.size, color, 0.36f, 0.50f);
    }

    void drawHalo(const DrawArgs& args) override {
        // Amp-style jewel lenses do not cast a panel halo.
    }

    void draw(const DrawArgs& args) override {
        // Draw SVG bezel/facets first
        if (children.empty()) {
            float cx = box.size.x * 0.5f;
            float cy = box.size.y * 0.5f;
            float outerRadius = 0.48f * std::min(box.size.x, box.size.y);
            float innerRadius = outerRadius * 0.67f;

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cx, cy, outerRadius);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cx, cy, innerRadius);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }
        widget::Widget::draw(args);
        shapetaker::ui::drawAmpJewelLensShade(args, box.size, 0.38f);

        // Overlay the colored light using additive blending (dimmed to preserve facets)
        nvgSave(args.vg);
        nvgGlobalAlpha(args.vg, shapetaker::ui::kJewelMaxBrightness);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        drawLight(args);
        nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
        drawHalo(args);
        nvgRestore(args.vg);
    }
};

/* struct JewelLEDHuge : ModuleLightWidget {
    JewelLEDHuge() {
        // Set a fixed size
        box.size = Vec(50, 50);
        
        // Try to load the jewel SVG, fallback to simple shape if it fails
        widget::SvgWidget* sw = new widget::SvgWidget;
        std::shared_ptr<Svg> svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/leds/jewel_led_huge.svg"));
        
        if (svg) {
            // SVG loaded successfully
            sw->setSvg(svg);
            addChild(sw);
        }
        
        // Set up proper RGB color mixing like RedGreenBlueLight
        addBaseColor(nvgRGB(0xff, 0x00, 0x00)); // Red channel
        addBaseColor(nvgRGB(0x00, 0xff, 0x00)); // Green channel  
        addBaseColor(nvgRGB(0x00, 0x00, 0xff)); // Blue channel
    }
    
    void step() override {
        ModuleLightWidget::step();
        
        if (module) {
            float r = module->lights[firstLightId + 0].getBrightness();
            float g = module->lights[firstLightId + 1].getBrightness();
            float b = module->lights[firstLightId + 2].getBrightness();
            
            color = nvgRGBAf(r, g, b, fmaxf(r, g));
        }
    }
    
    void draw(const DrawArgs& args) override {
        if (children.empty()) {
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 25, 25, 24);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0xc0, 0xc0));
            nvgFill(args.vg);
            
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, 25, 25, 16);
            nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
            nvgFill(args.vg);
        }
        
        ModuleLightWidget::draw(args);
    }
}; */

// An interface for modules that can provide data to the oscilloscope
struct IOscilloscopeSource {
    virtual ~IOscilloscopeSource() {}
    virtual const Vec* getOscilloscopeBuffer() const = 0;
    virtual int getOscilloscopeBufferIndex() const = 0;
    virtual int getOscilloscopeBufferSize() const = 0;
    // 0 = green, 1 = blue, 2 = yellow, 3 = amber
    virtual int getOscilloscopeTheme() const { return 0; }
    // 0 = X-Y Lissajous, 1 = triggered dual-trace waveform
    virtual int getOscilloscopeDisplayMode() const { return 0; }
};

struct VintageOscilloscopeWidget : widget::Widget {
    IOscilloscopeSource* source;
    std::shared_ptr<Svg> screenSvgs[shapetaker::ui::ThemeManager::DisplayTheme::THEME_COUNT];

    VintageOscilloscopeWidget(IOscilloscopeSource* source) : source(source) {}

    std::shared_ptr<Svg> getScreenSvg(shapetaker::ui::ThemeManager::DisplayTheme::Theme theme) {
        using DisplayTheme = shapetaker::ui::ThemeManager::DisplayTheme;
        int themeIndex = clamp((int)theme, 0, DisplayTheme::THEME_COUNT - 1);
        if (!screenSvgs[themeIndex]) {
            const char* screenSvg = DisplayTheme::getOscilloscopeScreenSVG(static_cast<DisplayTheme::Theme>(themeIndex));
            screenSvgs[themeIndex] = Svg::load(asset::plugin(pluginInstance, screenSvg));
        }
        return screenSvgs[themeIndex];
    }
    
    void drawLayer(const DrawArgs& args, int layer) override {
        // Layer 0: panel seating shadow (subtle drop beneath the circular screen)
        if (layer == 0) {
            NVGcontext* vg = args.vg;
            float w = box.size.x;
            float h = box.size.y;
            float cx = w * 0.5f;
            float cy = h * 0.5f + h * 0.10f; // slight downward bias like VCV knob shadows
            float r  = std::min(w, h) * 0.48f; // hug the bezel edge
            NVGpaint p = nvgRadialGradient(vg, cx, cy, r * 0.90f, r,
                                           nvgRGBA(0, 0, 0, 36), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, 0, 0, w, h);
            nvgFillPaint(vg, p);
            nvgFill(vg);
        }
        if (layer == 1) {
            NVGcontext* vg = args.vg;

            // Use centralized DisplayTheme system for consistent colors
            using DisplayTheme = shapetaker::ui::ThemeManager::DisplayTheme;
            int themeIndex = 0;
            if (source) {
                themeIndex = clamp(source->getOscilloscopeTheme(), 0, DisplayTheme::THEME_COUNT - 1);
            }
            DisplayTheme::Theme theme = static_cast<DisplayTheme::Theme>(themeIndex);

            // Get theme colors from centralized system
            NVGcolor glowInner = DisplayTheme::getGlowInnerColor(theme);
            NVGcolor glowOuter = DisplayTheme::getGlowOuterColor(theme);
            NVGcolor phosphorInner = DisplayTheme::getPhosphorInnerColor(theme);
            NVGcolor phosphorOuter = DisplayTheme::getPhosphorOuterColor(theme);
            float traceDimR, traceDimG, traceDimB;
            float traceBrightR, traceBrightG, traceBrightB;
            DisplayTheme::getTraceDimRGB(theme, traceDimR, traceDimG, traceDimB);
            DisplayTheme::getTraceBrightRGB(theme, traceBrightR, traceBrightG, traceBrightB);
            
            // --- CRT Glow Effect ---
            // Draw a soft glow behind the screen to simulate CRT phosphorescence
            nvgBeginPath(vg);
            nvgCircle(vg, box.size.x / 2.f, box.size.y / 2.f, box.size.x / 2.f);
            // A radial gradient from themed glow color to transparent
            NVGpaint glowPaint = nvgRadialGradient(
                vg,
                box.size.x / 2.f,
                box.size.y / 2.f,
                box.size.x * 0.1f,
                box.size.x * 0.5f,
                glowInner,
                glowOuter);
            nvgFillPaint(vg, glowPaint);
            nvgFill(vg);
            // --- End Glow Effect ---

            // Draw background SVG
            std::shared_ptr<Svg> bg_svg = getScreenSvg(theme);
            if (bg_svg) {
                nvgSave(vg);
                float scaleX = box.size.x / 200.f;
                float scaleY = box.size.y / 200.f;
                nvgScale(vg, scaleX, scaleY);
                bg_svg->draw(vg);
                nvgRestore(vg);
            }

            // Brass bezel overlay for Clairaudient's CRT screen (warmer vintage hardware look).
            auto drawBrassBezel = [&]() {
                float minDim = std::min(box.size.x, box.size.y);
                float cx = box.size.x * 0.5f;
                float cy = box.size.y * 0.5f;
                float outerR = minDim * 0.475f;
                // Reduced bezel ring thickness by 65%, then 20%, then 15%, then 20%, then another 15%.
                float baseBezelThickness = minDim * 0.083f;
                float bezelThickness = baseBezelThickness * 0.16184f;
                float innerR = outerR - bezelThickness;
                float lipR = innerR - bezelThickness * 0.35f;

                auto drawRing = [&](float outer, float inner) {
                    nvgBeginPath(vg);
                    nvgCircle(vg, cx, cy, outer);
                    nvgCircle(vg, cx, cy, inner);
                    nvgPathWinding(vg, NVG_HOLE);
                };

                // Recess shadow where bezel meets the panel.
                drawRing(outerR * 1.06f, outerR);
                NVGpaint seatShadow = nvgRadialGradient(vg, cx, cy, outerR * 0.9f, outerR * 1.08f,
                                                        nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 92));
                nvgFillPaint(vg, seatShadow);
                nvgFill(vg);

                // Main brass body.
                drawRing(outerR, innerR);
                NVGpaint brassBody = nvgLinearGradient(vg,
                    cx - outerR, cy - outerR,
                    cx + outerR * 0.35f, cy + outerR,
                    nvgRGBA(164, 131, 86, 246), nvgRGBA(60, 40, 20, 250));
                nvgFillPaint(vg, brassBody);
                nvgFill(vg);

                // Top catch light for curved metal.
                drawRing(outerR * 0.998f, innerR * 1.01f);
                NVGpaint catchLight = nvgLinearGradient(vg,
                    cx, cy - outerR,
                    cx, cy - outerR * 0.42f,
                    nvgRGBA(245, 224, 188, 74), nvgRGBA(0, 0, 0, 0));
                nvgFillPaint(vg, catchLight);
                nvgFill(vg);

                // Inner lip to seat the glass.
                drawRing(innerR, lipR);
                NVGpaint lipShade = nvgLinearGradient(vg,
                    cx - innerR * 0.2f, cy - innerR,
                    cx + innerR, cy + innerR,
                    nvgRGBA(14, 14, 16, 220), nvgRGBA(108, 80, 48, 120));
                nvgFillPaint(vg, lipShade);
                nvgFill(vg);

                nvgBeginPath(vg);
                nvgCircle(vg, cx, cy, outerR - 0.45f);
                nvgStrokeWidth(vg, 0.8f);
                nvgStrokeColor(vg, nvgRGBA(225, 203, 168, 34));
                nvgStroke(vg);

                nvgBeginPath(vg);
                nvgCircle(vg, cx, cy, innerR + 0.2f);
                nvgStrokeWidth(vg, 0.9f);
                nvgStrokeColor(vg, nvgRGBA(20, 14, 10, 145));
                nvgStroke(vg);
            };
            
            // --- Enhanced Spherical CRT Effect ---
            // Create a pronounced spherical bulging effect with multiple layers
            
            // Layer 1: Main spherical highlight (subdued for vintage look)
            nvgBeginPath(vg);
            nvgCircle(vg, box.size.x / 2.f, box.size.y / 2.f, box.size.x * 0.85f);
            NVGpaint mainHighlight = nvgRadialGradient(vg,
                box.size.x * 0.35f, box.size.y * 0.35f, // Offset highlight to top-left
                box.size.x * 0.05f, box.size.x * 0.6f,
                nvgRGBA(255, 255, 255, 18), // Subtle white highlight for vintage look
                nvgRGBA(255, 255, 255, 0));  // Fades to nothing
            nvgFillPaint(vg, mainHighlight);
            nvgFill(vg);

            // Layer 2: Subtle center hotspot for glass dome effect
            nvgBeginPath(vg);
            nvgCircle(vg, box.size.x * 0.38f, box.size.y * 0.38f, box.size.x * 0.15f);
            NVGpaint centerHighlight = nvgRadialGradient(vg,
                box.size.x * 0.38f, box.size.y * 0.38f,
                0, box.size.x * 0.15f,
                nvgRGBA(255, 255, 255, 30), // Muted center for aged glass
                nvgRGBA(255, 255, 255, 0));
            nvgFillPaint(vg, centerHighlight);
            nvgFill(vg);
            
            // Layer 3: Edge darkening for spherical depth
            nvgBeginPath(vg);
            nvgCircle(vg, box.size.x / 2.f, box.size.y / 2.f, box.size.x * 0.48f);
            NVGpaint edgeDarken = nvgRadialGradient(vg,
                box.size.x / 2.f, box.size.y / 2.f,
                box.size.x * 0.3f, box.size.x * 0.48f,
                nvgRGBA(0, 0, 0, 0),     // Transparent center
                nvgRGBA(0, 0, 0, 25));    // Dark edges
            nvgFillPaint(vg, edgeDarken);
            nvgFill(vg);
            
            // Layer 4: Themed phosphor glow enhancement
            nvgBeginPath(vg);
            nvgCircle(vg, box.size.x / 2.f, box.size.y / 2.f, box.size.x * 0.45f);
            NVGpaint phosphorGlow = nvgRadialGradient(
                vg,
                box.size.x / 2.f,
                box.size.y / 2.f,
                box.size.x * 0.1f,
                box.size.x * 0.45f,
                phosphorInner,
                phosphorOuter);
            nvgFillPaint(vg, phosphorGlow);
            nvgFill(vg);

            if (source) {
                nvgSave(vg);
                nvgScissor(vg, 0, 0, box.size.x, box.size.y);

                // Add margin to keep signal away from bezel edge (10% margin on each side)
                float margin = box.size.x * 0.10f;
                float screenLeft = margin;
                float screenRight = box.size.x - margin;
                float screenTop = margin;
                float screenBottom = box.size.y - margin;
                float screenWidth = screenRight - screenLeft;
                float screenHeight = screenBottom - screenTop;
                Vec screenCenter = Vec((screenLeft + screenRight) * 0.5f, (screenTop + screenBottom) * 0.5f);
                float screenRadius = std::min(screenWidth, screenHeight) * 0.5f;

                // Maximum voltage before hard clipping (prevents signal from reaching bezel)
                const float MAX_DISPLAY_VOLTAGE = 6.0f;

                // Function to map voltage to screen coordinates
                auto voltageToScreen = [&](Vec voltage) {
                    // Hard clamp voltage to prevent exceeding screen bounds
                    float clampedX = clamp(voltage.x, -MAX_DISPLAY_VOLTAGE, MAX_DISPLAY_VOLTAGE);
                    float clampedY = clamp(voltage.y, -MAX_DISPLAY_VOLTAGE, MAX_DISPLAY_VOLTAGE);

                    // Normalize voltage to -1..+1 range
                    float x_norm = clampedX / MAX_DISPLAY_VOLTAGE;
                    float y_norm = clampedY / MAX_DISPLAY_VOLTAGE;

                    // Map normalized coordinates to a circular screen area
                    float normX = x_norm;
                    float normY = y_norm;
                    float r = std::sqrt(normX * normX + normY * normY);
                    if (r > 1.f) {
                        normX /= r;
                        normY /= r;
                        r = 1.f;
                    }

                    // Slight barrel distortion to imply a spherical CRT surface
                    const float warp = 0.12f;
                    float rWarped = std::min(1.f, r * (1.f + warp * r * r));
                    if (r > 0.f) {
                        float scale = rWarped / r;
                        normX *= scale;
                        normY *= scale;
                    }

                    float screenX = screenCenter.x + normX * screenRadius;
                    float screenY = screenCenter.y - normY * screenRadius;

                    // Add "dust" artifact for a less clean, more analog look
                    float fuzz_amount = 0.4f; // pixels of random noise
                    float fuzz_x = (random::uniform() - 0.5f) * fuzz_amount;
                    float fuzz_y = (random::uniform() - 0.5f) * fuzz_amount;

                    return Vec(screenX + fuzz_x, screenY + fuzz_y);
                };
                
                // --- Vintage Persistence Drawing Logic ---
                const Vec* buffer = source->getOscilloscopeBuffer();
                const int bufferSize = source->getOscilloscopeBufferSize();
                const int currentIndex = source->getOscilloscopeBufferIndex();

                // Set line style for smooth corners
                nvgLineJoin(vg, NVG_ROUND);
                nvgLineCap(vg, NVG_ROUND);

                if (source->getOscilloscopeDisplayMode() == 1) {
                    // --- Triggered Dual-Trace Waveform Display ---
                    // Copy the ring buffer into chronological order
                    // (oldest first; the newest sample sits at currentIndex-1).
                    static thread_local std::vector<Vec> chrono;
                    chrono.resize(bufferSize);
                    for (int i = 0; i < bufferSize; i++) {
                        chrono[i] = buffer[(currentIndex + i) % bufferSize];
                    }

                    // The module buffers ~2.5 cycles in waveform mode, so a
                    // full period always follows the first trigger. Measure
                    // the actual period between two rising zero-crossings and
                    // display ~1 cycle of it, whatever the pitch.
                    const int maxWindow = (bufferSize * 3) / 5;
                    const int minWindow = bufferSize / 8;

                    // Armed below -0.05 V, fires at +0.05 V, so slow crossings
                    // trigger reliably and noise around 0 V does not.
                    auto findRisingCross = [&](int from, int to) {
                        bool armed = false;
                        for (int i = from; i < to; i++) {
                            float v = chrono[i].x;
                            if (v < -0.05f) {
                                armed = true;
                            } else if (armed && v >= 0.05f) {
                                return i;
                            }
                        }
                        return -1;
                    };

                    int start = findRisingCross(0, bufferSize - minWindow);
                    int window = maxWindow;
                    if (start < 0) {
                        start = bufferSize - maxWindow; // no trigger: newest window
                    } else {
                        int next = findRisingCross(start + minWindow, std::min(start + maxWindow, bufferSize));
                        if (next > start) {
                            int period = next - start;
                            window = period + period / 16; // slight overlap past one cycle
                        }
                        window = clamp(window, minWindow, std::min(maxWindow, bufferSize - start));
                    }

                    // Fit the trace inside the round screen: x spans +/-0.80 of
                    // the radius, +/-6 V spans +/-0.58, so the corners stay
                    // inside the circle. No barrel warp here — a waveform
                    // wants straight axes.
                    const float MAX_V = 6.f;
                    auto wavePoint = [&](int idx, float v) {
                        float t = (float)idx / (float)(window - 1);
                        float x = screenCenter.x + (t * 2.f - 1.f) * 0.80f * screenRadius;
                        float y = screenCenter.y - clamp(v, -MAX_V, MAX_V) / MAX_V * 0.58f * screenRadius;
                        float fuzz = 0.4f;
                        return Vec(x + (random::uniform() - 0.5f) * fuzz,
                                   y + (random::uniform() - 0.5f) * fuzz);
                    };

                    // Right trace first (dimmer, sits behind), then left.
                    for (int trace = 0; trace < 2; trace++) {
                        bool isLeft = (trace == 1);
                        float level = isLeft ? 1.f : 0.45f;
                        nvgBeginPath(vg);
                        for (int i = 0; i < window; i++) {
                            const Vec& v = chrono[start + i];
                            Vec p = wavePoint(i, isLeft ? v.x : v.y);
                            if (i == 0) nvgMoveTo(vg, p.x, p.y);
                            else nvgLineTo(vg, p.x, p.y);
                        }
                        nvgStrokeColor(vg, nvgRGBAf(traceDimR, traceDimG, traceDimB, 0.30f * level));
                        nvgStrokeWidth(vg, 2.2f);
                        nvgStroke(vg);
                        nvgStrokeColor(vg, nvgRGBAf(traceBrightR, traceBrightG, traceBrightB, 0.75f * level));
                        nvgStrokeWidth(vg, 0.9f);
                        nvgStroke(vg);
                    }

                    nvgRestore(vg);
                    drawBrassBezel();
                    Widget::drawLayer(args, layer);
                    return;
                }

                // --- X-Y Lissajous Display ---
                const int numChunks = 24;
                const int chunkSize = bufferSize / numChunks;
                const float DISCONTINUITY_THRESHOLD = 5.0f;

                for (int c = 0; c < numChunks; c++) {
                    float age = (float)c / (numChunks - 1);
                    float alpha = powf(1.0f - age, 1.8f);
                    alpha = clamp(alpha, 0.f, 1.f);

                        if (alpha < 0.01f) continue;

                        nvgBeginPath(vg);
                        bool firstPointInChunk = true;
                        Vec lastVoltage = Vec(0, 0);

                        for (int i = 0; i < chunkSize; i++) {
                            int point_offset = c * chunkSize + i;
                            if (point_offset >= bufferSize - 1) continue;

                            int buffer_idx = (currentIndex - 1 - point_offset + bufferSize) % bufferSize;

                            if (buffer_idx == currentIndex) {
                                firstPointInChunk = true;
                                continue;
                            }

                            Vec currentVoltage = buffer[buffer_idx];
                            Vec p = voltageToScreen(currentVoltage);

                            if (!firstPointInChunk) {
                                float deltaX = std::abs(currentVoltage.x - lastVoltage.x);
                                float deltaY = std::abs(currentVoltage.y - lastVoltage.y);
                                if (deltaX > DISCONTINUITY_THRESHOLD || deltaY > DISCONTINUITY_THRESHOLD) {
                                    firstPointInChunk = true;
                                }
                            }

                            if (firstPointInChunk) {
                                nvgMoveTo(vg, p.x, p.y);
                                firstPointInChunk = false;
                            } else {
                                nvgLineTo(vg, p.x, p.y);
                            }

                            lastVoltage = currentVoltage;
                        }

                        nvgStrokeColor(vg, nvgRGBAf(traceDimR, traceDimG, traceDimB, alpha * 0.30f));
                        nvgStrokeWidth(vg, 1.2f + alpha * 1.2f);
                        nvgStroke(vg);

                        nvgStrokeColor(vg, nvgRGBAf(traceBrightR, traceBrightG, traceBrightB, alpha * 0.65f));
                        nvgStrokeWidth(vg, 0.6f + alpha * 0.3f);
                        nvgStroke(vg);
                    }

                nvgRestore(vg);
            }

            drawBrassBezel();
        }
        Widget::drawLayer(args, layer);
    }
};

// Capacitive touch switch (brass touch pad like touch strip)
struct CapacitiveTouchSwitch : app::SvgSwitch {
    CapacitiveTouchSwitch() {
        momentary = false;
        latch = true;
        // No frames needed - visual state shown by LED
        box.size = Vec(27.2f, 27.2f);
        if (shadow) shadow->visible = false;
    }

    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float cx = box.size.x / 2.0f;
        float cy = box.size.y / 2.0f;

        // Base brass gradient
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius);
        NVGpaint base = nvgLinearGradient(args.vg, cx, cy - radius, cx, cy + radius,
            nvgRGBA(148, 122, 82, 255), // Lighter top color
            nvgRGBA(76, 60, 46, 255));  // Lighter bottom color
        nvgFillPaint(args.vg, base);
        nvgFill(args.vg);

        // Subtle center glow
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius * 0.95f);
        NVGpaint centerGlow = nvgRadialGradient(args.vg,
            cx, cy, radius * 0.1f, radius * 0.95f,
            nvgRGBA(240, 210, 130, 110), // Brighter glow
            nvgRGBA(100, 70, 38, 0));
        nvgFillPaint(args.vg, centerGlow);
        nvgFill(args.vg);

        // Edge sheen
        NVGpaint edgeSheen = nvgRadialGradient(args.vg,
            cx, cy, radius * 0.8f, radius,
            nvgRGBA(255, 235, 150, 48), // Brighter sheen
            nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius);
        nvgFillPaint(args.vg, edgeSheen);
        nvgFill(args.vg);

        // Border
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius - 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(88, 62, 36, 160));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
    }

    void onChange(const ChangeEvent& e) override {
        SvgSwitch::onChange(e);
    }
};

// Vintage slider widget matching Torsion's illuminated style
struct VintageSlider : app::SvgSlider {
    static constexpr float LED_RADIUS = 1.7f;
    static constexpr float LED_GLOW_RADIUS = 4.5f;
    // Track SVG is 12px wide; handle (12px) fills it exactly — no offset needed.
    static constexpr float TRACK_CENTER_OFFSET_X = 0.0f;
    NVGcolor ledColor = nvgRGBf(1.0f, 0.6f, 0.2f);

    VintageSlider() {
        // Set the background (track) SVG - 12x60px
        setBackgroundSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_track_small.svg"))
        );

        // Set the handle SVG - 12x18px
        setHandleSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_handle_small.svg"))
        );

        // Widget 18px wide (Befaco-comparable); track and handle align.
        // Travel: 60px track - 24px handle = 36px.
        box.size = Vec(18.f, 60.f);
        maxHandlePos = Vec(0.f, 0.f);   // top  (param min = 0)
        minHandlePos = Vec(0.f, 36.f);  // bottom (param max = 1)
    }

    void draw(const DrawArgs& args) override {
        // Art-only: the SVG track and bakelite paddle ARE the look; the old
        // code-drawn well, slot rim, and LED lamp are retired.
        app::SvgSlider::draw(args);
    }

    void drawInsetWell(const DrawArgs& args) {
        const bool large = box.size.y > 90.f;
        const float padX = large ? 1.05f : 0.85f;
        const float padY = large ? 0.9f : 0.7f;
        const float wellX = -padX;
        const float wellY = -padY;
        const float wellW = box.size.x + padX * 2.f;
        const float wellH = box.size.y + padY * 2.f;
        const float wellR = large ? 4.2f : 3.4f;

        NVGcontext* vg = args.vg;

        // A tight pocket around the rail, not a separate backing plate.
        NVGpaint outerShadow = nvgBoxGradient(vg, wellX, wellY + 0.35f, wellW, wellH, wellR, 2.4f,
            nvgRGBA(0, 0, 0, 42), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, wellX - 0.45f, wellY - 0.25f, wellW + 0.9f, wellH + 0.75f, wellR + 0.55f);
        nvgFillPaint(vg, outerShadow);
        nvgFill(vg);

        NVGpaint pocket = nvgLinearGradient(vg, wellX, wellY, wellX + wellW, wellY,
            nvgRGBA(0, 0, 0, 30), nvgRGBA(255, 238, 204, 8));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, wellX, wellY, wellW, wellH, wellR);
        nvgFillPaint(vg, pocket);
        nvgFill(vg);

        NVGpaint topDepth = nvgLinearGradient(vg, 0.f, wellY, 0.f, wellY + 18.f,
            nvgRGBA(0, 0, 0, 44), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, wellX + 0.8f, wellY + 0.8f, wellW - 1.6f, std::min(18.f, wellH * 0.18f), wellR * 0.6f);
        nvgFillPaint(vg, topDepth);
        nvgFill(vg);

        NVGpaint bottomCatch = nvgLinearGradient(vg, 0.f, wellY + wellH - 10.f, 0.f, wellY + wellH,
            nvgRGBA(255, 238, 204, 0), nvgRGBA(255, 238, 204, 6));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, wellX + 1.0f, wellY + wellH - std::min(10.f, wellH * 0.10f),
                       wellW - 2.0f, std::min(10.f, wellH * 0.10f), wellR * 0.42f);
        nvgFillPaint(vg, bottomCatch);
        nvgFill(vg);

        // Very slight outer bezel so Incantation/Torsion faders read as seated
        // metalwork without turning each slider into a separate framed control.
        nvgBeginPath(vg);
        nvgRoundedRect(vg, wellX + 0.18f, wellY + 0.18f, wellW - 0.36f, wellH - 0.36f, wellR);
        nvgStrokeWidth(vg, 0.55f);
        nvgStrokeColor(vg, nvgRGBA(176, 143, 95, large ? 42 : 34));
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgMoveTo(vg, wellX + wellR * 0.72f, wellY + 0.7f);
        nvgLineTo(vg, wellX + wellW - wellR * 0.72f, wellY + 0.7f);
        nvgMoveTo(vg, wellX + 0.7f, wellY + wellR * 0.72f);
        nvgLineTo(vg, wellX + 0.7f, wellY + wellH - wellR * 0.72f);
        nvgStrokeWidth(vg, 0.45f);
        nvgStrokeColor(vg, nvgRGBA(255, 236, 190, large ? 28 : 22));
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgMoveTo(vg, wellX + wellR * 0.72f, wellY + wellH - 0.65f);
        nvgLineTo(vg, wellX + wellW - wellR * 0.72f, wellY + wellH - 0.65f);
        nvgMoveTo(vg, wellX + wellW - 0.65f, wellY + wellR * 0.72f);
        nvgLineTo(vg, wellX + wellW - 0.65f, wellY + wellH - wellR * 0.72f);
        nvgStrokeWidth(vg, 0.45f);
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, large ? 72 : 58));
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, wellX + 0.45f, wellY + 0.45f, wellW - 0.9f, wellH - 0.9f, wellR);
        nvgStrokeWidth(vg, 0.45f);
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 54));
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgMoveTo(vg, wellX + wellW - 0.8f, wellY + wellR);
        nvgLineTo(vg, wellX + wellW - 0.8f, wellY + wellH - wellR);
        nvgStrokeWidth(vg, 0.45f);
        nvgStrokeColor(vg, nvgRGBA(255, 238, 204, 9));
        nvgStroke(vg);
    }

    // Draws a machined-edge bevel, inner depth shadows, and handle contact
    // shadows around the slot opening to make the slider look embedded in panel.
    void drawSlotRim(const DrawArgs& args) {
        // Channel geometry: x=1.5, width=9 (→ channel 1.5..10.5, center at 6)
        // in the 12px-wide track SVG.  slotY: 1 for small (60px), 2 for large.
        const float slotX = 1.5f;
        const float slotW = 9.0f;
        const float slotR = 2.5f;
        const float slotY = (box.size.y > 90.f) ? 2.0f : 1.0f;
        const float slotH = box.size.y - slotY * 2.0f;

        // ---- Outer definition stroke — crisp dark outline around slot opening ----
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, slotX - 0.3f, slotY - 0.3f,
                       slotW + 0.6f, slotH + 0.6f, slotR + 0.3f);
        nvgStrokeWidth(args.vg, 0.6f);
        nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, 100));
        nvgStroke(args.vg);

        // ---- Machined bevel rim ----
        // Top rim: light catches the upper machined lip
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, slotX + slotR * 0.4f, slotY);
        nvgLineTo(args.vg, slotX + slotW - slotR * 0.4f, slotY);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(220, 220, 220, 90));
        nvgStroke(args.vg);

        // Bottom rim: shadow at lower lip
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, slotX + slotR * 0.4f, slotY + slotH);
        nvgLineTo(args.vg, slotX + slotW - slotR * 0.4f, slotY + slotH);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, 100));
        nvgStroke(args.vg);

        // Left wall: subtle highlight from the chamfered left edge
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, slotX, slotY + slotR * 0.6f);
        nvgLineTo(args.vg, slotX, slotY + slotH - slotR * 0.6f);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(190, 190, 190, 55));
        nvgStroke(args.vg);

        // Right wall: shadow from chamfered right edge
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, slotX + slotW, slotY + slotR * 0.6f);
        nvgLineTo(args.vg, slotX + slotW, slotY + slotH - slotR * 0.6f);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, 70));
        nvgStroke(args.vg);

        // ---- Inner depth shadows — top fades out as handle approaches top so LED glow is unobstructed ----
        float topFade = 1.0f;
        if (handle) {
            topFade = rack::math::clamp(handle->box.pos.y / 18.0f, 0.0f, 1.0f);
        }
        NVGpaint topDepth = nvgLinearGradient(args.vg,
            0.f, slotY + 0.5f, 0.f, slotY + 13.0f,
            nvgRGBA(0, 0, 0, (int)(165.f * topFade)), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, slotX + 0.3f, slotY + 0.5f,
                       slotW - 0.6f, 13.0f, slotR * 0.3f);
        nvgFillPaint(args.vg, topDepth);
        nvgFill(args.vg);

        // Bottom: shadow cast from lower rim upward into the channel
        const float botY = slotY + slotH - 13.5f;
        NVGpaint botDepth = nvgLinearGradient(args.vg,
            0.f, botY, 0.f, botY + 13.0f,
            nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 130));
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, slotX + 0.3f, botY,
                       slotW - 0.6f, 13.0f, slotR * 0.3f);
        nvgFillPaint(args.vg, botDepth);
        nvgFill(args.vg);

        // ---- Handle contact shadows ----
        // Darkens the slot just above/below the handle body, showing that the
        // handle physically sits in the channel and blocks light at its edges.
        if (handle) {
            const float hTop = handle->box.pos.y;
            const float hBot = handle->box.pos.y + handle->box.size.y;
            const float contactH = 6.0f;
            const float slotEnd = slotY + slotH;

            // Shadow above handle (fades downward into slot → handle)
            if (hTop > slotY + 1.0f) {
                float shadowTop = std::max(slotY + 0.5f, hTop - contactH);
                NVGpaint topContact = nvgLinearGradient(args.vg,
                    0.f, shadowTop, 0.f, hTop,
                    nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 120));
                nvgBeginPath(args.vg);
                nvgRoundedRect(args.vg, slotX + 0.5f, shadowTop,
                               slotW - 1.0f, hTop - shadowTop, slotR * 0.2f);
                nvgFillPaint(args.vg, topContact);
                nvgFill(args.vg);
            }

            // Shadow below handle (fades upward into slot ← handle)
            if (hBot < slotEnd - 1.0f) {
                float shadowBot = std::min(slotEnd - 0.5f, hBot + contactH);
                NVGpaint botContact = nvgLinearGradient(args.vg,
                    0.f, hBot, 0.f, shadowBot,
                    nvgRGBA(0, 0, 0, 120), nvgRGBA(0, 0, 0, 0));
                nvgBeginPath(args.vg);
                nvgRoundedRect(args.vg, slotX + 0.5f, hBot,
                               slotW - 1.0f, shadowBot - hBot, slotR * 0.2f);
                nvgFillPaint(args.vg, botContact);
                nvgFill(args.vg);
            }
        }
    }

    void drawLED(const DrawArgs& args, Vec pos, float brightness) {
        brightness = rack::math::clamp(brightness, 0.f, 1.f);
        if (brightness <= 0.f) {
            return;
        }

        nvgSave(args.vg);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);

        float glowAlpha = brightness * 0.32f;
        NVGcolor glowColor = nvgRGBAf(ledColor.r, ledColor.g, ledColor.b, glowAlpha);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, pos.x, pos.y, LED_GLOW_RADIUS);
        NVGpaint glowPaint = nvgRadialGradient(
            args.vg,
            pos.x, pos.y,
            0.f,
            LED_GLOW_RADIUS,
            glowColor,
            nvgRGBAf(ledColor.r, ledColor.g, ledColor.b, 0.f)
        );
        nvgFillPaint(args.vg, glowPaint);
        nvgFill(args.vg);

        float coreAlpha = brightness * 0.95f;
        NVGcolor coreColor = nvgRGBAf(
            rack::math::clamp(ledColor.r * 1.1f, 0.f, 1.f),
            rack::math::clamp(ledColor.g * 1.1f, 0.f, 1.f),
            rack::math::clamp(ledColor.b * 0.95f, 0.f, 1.f),
            coreAlpha
        );

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, pos.x, pos.y, LED_RADIUS);
        NVGpaint corePaint = nvgRadialGradient(
            args.vg,
            pos.x, pos.y - LED_RADIUS * 0.3f,
            0.f,
            LED_RADIUS,
            nvgRGBAf(1.f, 0.92f, 0.6f, coreAlpha),
            coreColor
        );
        nvgFillPaint(args.vg, corePaint);
        nvgFill(args.vg);

        nvgRestore(args.vg);
    }
};

// Large-height variant used by modules that need the long Torsion-style throw.
struct VintageSliderLarge : VintageSlider {
    VintageSliderLarge() {
        setBackgroundSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_track_large.svg"))
        );
        setHandleSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_handle_small.svg"))
        );

        // Track 18x120px. Travel: 120 - 24 = 96px. Track and handle align exactly.
        box.size = Vec(18.f, 120.f);
        maxHandlePos = Vec(0.f, 0.f);
        minHandlePos = Vec(0.f, 96.f);
    }
};

struct VintageSliderMedium : VintageSlider {
    VintageSliderMedium() {
        setBackgroundSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_track_medium.svg"))
        );
        setHandleSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_handle_small.svg"))
        );

        // Track 18x90px, handle 18x24px. Travel: 90 - 24 = 66px.
        box.size = Vec(18.f, 90.f);
        maxHandlePos = Vec(0.f, 0.f);
        minHandlePos = Vec(0.f, 66.f);
    }
};

struct VintageSliderXLarge : VintageSlider {
    VintageSliderXLarge() {
        setBackgroundSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_track_xlarge_wide.svg"))
        );
        setHandleSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_handle_medium.svg"))
        );

        // Track 18x180px, handle 18x22px. Travel: 180 - 22 = 158px.
        box.size = Vec(18.f, 180.f);
        maxHandlePos = Vec(0.f, 0.f);
        minHandlePos = Vec(0.f, 158.f);
    }
};

// ---------------------------------------------------------------------------
// AlchemicalBadgeWidget
// Small alchemical symbol badge drawn at a fixed position in panel space.
// Add as a child widget after setPanel() so box.size is available.
// ---------------------------------------------------------------------------
struct AlchemicalBadgeWidget : Widget {
    int   symbolId;
    NVGcolor color;
    float size;

    AlchemicalBadgeWidget(int id, float sz = 6.5f,
                          NVGcolor col = nvgRGBA(190, 180, 160, 110))
        : symbolId(id), color(col), size(sz) {
        box.size = Vec(sz * 2.f, sz * 2.f);
    }

    void draw(const DrawArgs& args) override {
        shapetaker::graphics::drawAlchemicalSymbol(args, Vec(size, size), symbolId, color, size, 0.6f);
    }
};

// Helper: add a badge at the bottom-left of a module panel (left of bottom screw).
// Call after setPanel() so that box.size is valid.
static inline void addAlchemicalBadge(ModuleWidget* w, int symbolId) {
    constexpr float kSymbolSize = 4.0f;
    auto* badge = new AlchemicalBadgeWidget(symbolId, kSymbolSize);
    // Centre the symbol at (8, panelHeight - 9) — the slim column left of the bottom screw
    badge->box.pos = Vec(8.f - kSymbolSize, w->box.size.y - 9.f - kSymbolSize);
    w->addChild(badge);
}

// ---------------------------------------------------------------------------
// ShapetakerModuleWidget
// Base class for all Shapetaker module widgets.  Handles the shared leather-
// grain panel background (fixed-height tiling + seam-softening pass +
// darkening overlay) and the inner black frame that masks SVG edge tinting.
// Subclasses just call ModuleWidget::draw() as normal — the background renders
// before children and the frame renders after, producing a consistent look
// across all panels without any per-module duplication.
// ---------------------------------------------------------------------------
struct ShapetakerModuleWidget : ModuleWidget {
    static constexpr float BG_TEXTURE_ASPECT = 2880.f / 4553.f; // panel_background.png
    static constexpr float BG_INSET          = 2.0f;
    static constexpr float BG_OFFSET_OPACITY = 0.35f;
    static constexpr int   BG_DARKEN_ALPHA   = 18;
    static constexpr float FRAME_WIDTH       = 1.0f;

    void draw(const DrawArgs& args) override {
        std::shared_ptr<Image> bg = APP->window->loadImage(
            asset::plugin(pluginInstance, "res/panels/panel_background.png"));
        if (bg) {
            float tileH = box.size.y + BG_INSET * 2.f;
            float tileW = tileH * BG_TEXTURE_ASPECT;
            float x = -BG_INSET;
            float y = -BG_INSET;

            nvgSave(args.vg);

            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
            NVGpaint paintA = nvgImagePattern(args.vg, x, y, tileW, tileH, 0.f, bg->handle, 1.0f);
            nvgFillPaint(args.vg, paintA);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
            NVGpaint paintB = nvgImagePattern(args.vg, x + tileW * 0.5f, y, tileW, tileH, 0.f, bg->handle, BG_OFFSET_OPACITY);
            nvgFillPaint(args.vg, paintB);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, BG_DARKEN_ALPHA));
            nvgFill(args.vg);

            nvgRestore(args.vg);
        }

        ModuleWidget::draw(args);

        // Black inner frame masks any SVG edge colour bleed
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
        nvgRect(args.vg, FRAME_WIDTH, FRAME_WIDTH,
                box.size.x - 2.f * FRAME_WIDTH, box.size.y - 2.f * FRAME_WIDTH);
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
    }
};

// Horizontal variant of the vintage slider
struct VintageSliderHorizontal : app::SvgSlider {
    VintageSliderHorizontal() {
        // Set horizontal track
        setBackgroundSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_track_horizontal.svg"))
        );

        // Set horizontal handle
        setHandleSvg(
            Svg::load(asset::plugin(pluginInstance,
            "res/sliders/vintage_slider_handle_horizontal.svg"))
        );

        // Configure horizontal travel
        maxHandlePos = mm2px(Vec(-20, 0));  // Left position (minimum)
        minHandlePos = mm2px(Vec(20, 0));   // Right position (maximum)

        // Set widget box for horizontal orientation
        box.size = mm2px(Vec(45, 14));
    }
};
