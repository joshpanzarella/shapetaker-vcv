#pragma once
#include <rack.hpp>

using namespace rack;

namespace shapetaker {
namespace ui {

// ============================================================================
// WIDGET HELPER UTILITIES
// ============================================================================

namespace WidgetHelper {
    // Standard screw positions for VCV Rack modules
    inline Vec getTopLeftScrew(float moduleWidth) {
        return Vec(RACK_GRID_WIDTH, 0);
    }
    
    inline Vec getTopRightScrew(float moduleWidth) {
        return Vec(moduleWidth - 2 * RACK_GRID_WIDTH, 0);
    }
    
    inline Vec getBottomLeftScrew(float moduleWidth) {
        return Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH);
    }
    
    inline Vec getBottomRightScrew(float moduleWidth) {
        return Vec(moduleWidth - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH);
    }
    
    // Center a widget within a given rectangle
    inline Vec centerInRect(Vec widgetSize, Vec rectPos, Vec rectSize) {
        return rectPos.plus(rectSize.minus(widgetSize).mult(0.5f));
    }
    
    // Standard port spacing helpers
    inline Vec getPortPosition(int row, int col, Vec startPos = Vec(0, 0), 
                       Vec spacing = Vec(30, 30)) {
        return startPos.plus(Vec(col * spacing.x, row * spacing.y));
    }
    
    // Grid layout helper
    inline Vec getGridPosition(int index, int cols, Vec startPos = Vec(0, 0),
                       Vec spacing = Vec(30, 30)) {
        int row = index / cols;
        int col = index % cols;
        return getPortPosition(row, col, startPos, spacing);
    }
    
    // Convert millimeters to pixels using VCV standard
    inline float mm2px(float mm) {
        return mm * RACK_GRID_WIDTH / 5.08f; // 5.08mm per HP
    }
    
    inline Vec mm2px(Vec mm) {
        return Vec(mm2px(mm.x), mm2px(mm.y));
    }
    
    // Safe widget positioning with bounds checking
    inline Vec clampPosition(Vec pos, Vec widgetSize, Vec containerSize) {
        return Vec(
            rack::math::clamp(pos.x, 0.f, containerSize.x - widgetSize.x),
            rack::math::clamp(pos.y, 0.f, containerSize.y - widgetSize.y)
        );
    }
    
    // Create standard widget creators with consistent styling
    template<typename TWidget>
    TWidget* createCenteredWidget(Vec pos, Module* module, int paramId) {
        TWidget* widget = createWidget<TWidget>(pos);
        widget->module = module;
        widget->paramId = paramId;
        return widget;
    }
    
    // Animation helpers
    inline float easeInOut(float t) {
        return t * t * (3.f - 2.f * t); // Smooth step function
    }
    
    inline float easeInOutCubic(float t) {
        return t < 0.5f ? 4.f * t * t * t : 1.f - pow(-2.f * t + 2.f, 3.f) / 2.f;
    }
    
    // Color interpolation
    inline NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
        t = rack::math::clamp(t, 0.f, 1.f);
        return nvgRGBAf(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
    
    // Text measurement
    inline Vec measureText(NVGcontext* vg, const std::string& text, float fontSize) {
        nvgSave(vg);
        nvgFontSize(vg, fontSize);
        
        float bounds[4];
        nvgTextBounds(vg, 0, 0, text.c_str(), nullptr, bounds);
        Vec size = Vec(bounds[2] - bounds[0], bounds[3] - bounds[1]);
        
        nvgRestore(vg);
        return size;
    }
} // namespace WidgetHelper

// ============================================================================
// CONTROL HELPERS
// ============================================================================

// Button state management
class ButtonHelper {
private:
    bool lastPressed = false;
    float pressTimer = 0.f;
    
public:
    // Process momentary button with press animation timing
    bool processMomentary(Param& param, float sampleTime) {
        float value = param.getValue();
        bool pressed = (value > 0.5f);
        bool triggered = pressed && !lastPressed;
        
        if (triggered) {
            pressTimer = 0.1f; // 100ms press animation
            param.setValue(0.f); // Auto-release
        }
        
        if (pressTimer > 0.f) {
            pressTimer -= sampleTime;
        }
        
        lastPressed = pressed;
        return triggered;
    }
    
    // Process toggle button
    bool processToggle(Param& param, bool& state) {
        float value = param.getValue();
        bool pressed = (value > 0.5f);
        bool triggered = pressed && !lastPressed;
        
        if (triggered) {
            state = !state;
            param.setValue(0.f); // Reset button
        }
        
        lastPressed = pressed;
        return triggered;
    }
    
    float getPressAnimation() const {
        return rack::math::clamp(pressTimer * 10.f, 0.f, 1.f); // Fade over 100ms
    }
};

// CV input processing with visual feedback
class CVHelper {
public:
    // Process CV with LED feedback
    static float processWithLED(Input& cvInput, Module* module, int lightId,
                               float scale = 0.1f, bool bipolar = false) {
        if (!cvInput.isConnected()) {
            if (module) module->lights[lightId].setBrightness(0.f);
            return 0.f;
        }
        
        float voltage = cvInput.getVoltage();
        float normalized = bipolar ? voltage * scale * 0.2f : // -5V to +5V -> -1 to +1
                                   voltage * scale * 0.1f;    // 0V to +10V -> 0 to 1
        
        // Set LED brightness based on CV level
        if (module) {
            float brightness = bipolar ? std::abs(normalized) : normalized;
            module->lights[lightId].setBrightness(rack::math::clamp(brightness, 0.f, 1.f));
        }
        
        return normalized;
    }
};

}} // namespace shapetaker::ui