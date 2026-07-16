#pragma once
#include <rack.hpp>
#include <functional>
#include <string>

using namespace rack;

namespace shapetaker {
namespace ui {

// ============================================================================
// CONTEXT MENU SLIDER HELPERS
// ============================================================================

// Generic Quantity implementation using lambdas for flexibility
template<typename TModule, typename SetterFunc, typename GetterFunc>
class LambdaQuantity : public Quantity {
private:
    TModule* module;
    SetterFunc setter;
    GetterFunc getter;
    float minValue;
    float maxValue;
    float defaultValue;
    float displayScale;
    std::string label;
    std::string unit;

public:
    LambdaQuantity(
        TModule* mod,
        SetterFunc setterFunc,
        GetterFunc getterFunc,
        float minVal,
        float maxVal,
        float defVal,
        float dispScale,
        const std::string& lbl,
        const std::string& unt
    ) : module(mod),
        setter(setterFunc),
        getter(getterFunc),
        minValue(minVal),
        maxValue(maxVal),
        defaultValue(defVal),
        displayScale(dispScale),
        label(lbl),
        unit(unt) {}

    void setValue(float v) override {
        if (module) {
            setter(module, clamp(v, minValue, maxValue));
        }
    }

    float getValue() override {
        return module ? getter(module) : defaultValue;
    }

    float getMinValue() override { return minValue; }
    float getMaxValue() override { return maxValue; }
    float getDefaultValue() override { return defaultValue; }

    float getDisplayValue() override {
        return getValue() * displayScale;
    }

    void setDisplayValue(float v) override {
        setValue(v / displayScale);
    }

    std::string getLabel() override { return label; }
    std::string getUnit() override { return unit; }
};

// Generic Slider that uses LambdaQuantity
template<typename TModule, typename SetterFunc, typename GetterFunc>
struct LambdaSlider : rack::ui::Slider {
    LambdaSlider(
        TModule* module,
        SetterFunc setter,
        GetterFunc getter,
        float minVal,
        float maxVal,
        float defVal,
        float displayScale,
        const std::string& label,
        const std::string& unit,
        float width = 200.f
    ) {
        quantity = new LambdaQuantity<TModule, SetterFunc, GetterFunc>(
            module, setter, getter, minVal, maxVal, defVal, displayScale, label, unit
        );
        box.size.x = width;
    }
};

// ============================================================================
// CONVENIENCE FACTORY FUNCTIONS
// ============================================================================

// Create a percentage slider (0-100%, stored as 0.0-1.0)
template<typename TModule, typename SetterFunc, typename GetterFunc>
rack::ui::Slider* createPercentageSlider(
    TModule* module,
    SetterFunc setter,
    GetterFunc getter,
    const std::string& label,
    float defaultValue = 0.f,
    float width = 200.f
) {
    return new LambdaSlider<TModule, SetterFunc, GetterFunc>(
        module, setter, getter,
        0.f, 1.f, defaultValue, 100.f,  // Display as 0-100%
        label, "%", width
    );
}

// Create a generic float slider with custom range and unit
template<typename TModule, typename SetterFunc, typename GetterFunc>
rack::ui::Slider* createFloatSlider(
    TModule* module,
    SetterFunc setter,
    GetterFunc getter,
    float minVal,
    float maxVal,
    float defaultVal,
    const std::string& label,
    const std::string& unit = "",
    float displayScale = 1.f,
    float width = 200.f
) {
    return new LambdaSlider<TModule, SetterFunc, GetterFunc>(
        module, setter, getter,
        minVal, maxVal, defaultVal, displayScale,
        label, unit, width
    );
}

// Custom quantity that displays in dB but stores linearly
template<typename TModule, typename SetterFunc, typename GetterFunc>
class DbQuantity : public Quantity {
private:
    TModule* module;
    SetterFunc setter;
    GetterFunc getter;
    float minLin, maxLin, defLin;
    std::string label;

public:
    DbQuantity(TModule* m, SetterFunc s, GetterFunc g, float minL, float maxL, float defL, const std::string& lbl)
        : module(m), setter(s), getter(g), minLin(minL), maxLin(maxL), defLin(defL), label(lbl) {}

    void setValue(float linearVal) override {
        if (module) setter(module, clamp(linearVal, minLin, maxLin));
    }
    float getValue() override { return module ? getter(module) : defLin; }
    float getMinValue() override { return minLin; }
    float getMaxValue() override { return maxLin; }
    float getDefaultValue() override { return defLin; }
    float getDisplayValue() override {
        float lin = getValue();
        return (lin > 0.f) ? 20.f * std::log10(lin) : -100.f;
    }
    void setDisplayValue(float db) override {
        setValue(std::pow(10.f, db / 20.f));
    }
    std::string getLabel() override { return label; }
    std::string getUnit() override { return " dB"; }
};

// Create a decibel slider (stored as linear gain, displayed as dB)
template<typename TModule, typename SetterFunc, typename GetterFunc>
rack::ui::Slider* createDecibelSlider(
    TModule* module,
    SetterFunc setter,
    GetterFunc getter,
    float minDb,
    float maxDb,
    float defaultDb,
    const std::string& label,
    float width = 200.f
) {
    // Convert dB range to linear range for storage
    float minLin = std::pow(10.f, minDb / 20.f);
    float maxLin = std::pow(10.f, maxDb / 20.f);
    float defLin = std::pow(10.f, defaultDb / 20.f);

    auto* slider = new rack::ui::Slider();
    slider->quantity = new DbQuantity<TModule, SetterFunc, GetterFunc>(module, setter, getter, minLin, maxLin, defLin, label);
    slider->box.size.x = width;

    return slider;
}

}} // namespace shapetaker::ui
