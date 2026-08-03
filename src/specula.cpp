#include "plugin.hpp"
#include "ui/menu_helpers.hpp"
#include "ui/widgets.hpp"

#include <algorithm>
#include <cmath>

struct Specula : Module {
    // Specula is an inline monitor: whatever arrives has to leave intact, so the
    // pass-through has to reach as wide as Rack itself can. This used to be 6 —
    // the plugin's usual voice budget — which silently dropped channels 7-16 on
    // their way to the output. A voice budget is the right call for a module
    // that *processes* voices; a meter that quietly narrows the cable running
    // through it is just a hole in the patch.
    static constexpr int MAX_CHANNELS = rack::engine::PORT_MAX_CHANNELS;

    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        LEFT_INPUT,
        RIGHT_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LEFT_VU_LIGHT,
        RIGHT_VU_LIGHT,
        NUM_LIGHTS
    };

    dsp::VuMeter2 vuMeterLeft;
    dsp::VuMeter2 vuMeterRight;
    float leftNeedleDisplay = 0.f;
    float rightNeedleDisplay = 0.f;
    float meterScreenBrightness = 0.62f;

    Specula() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        vuMeterLeft.mode = dsp::VuMeter2::PEAK;
        vuMeterRight.mode = dsp::VuMeter2::PEAK;
        // Slower ballistic response for analog-style needle movement.
        vuMeterLeft.lambda = 5.f;
        vuMeterRight.lambda = 5.f;

        shapetaker::ui::LabelFormatter::normalizeModuleControls(this);
    }

    void process(const ProcessArgs& args) override {
        passThroughAudio(LEFT_INPUT, LEFT_OUTPUT);
        passThroughAudio(RIGHT_INPUT, RIGHT_OUTPUT);

        float leftPeak = getPeakVoltage(inputs[LEFT_INPUT]);
        float rightPeak = getPeakVoltage(inputs[RIGHT_INPUT]);

        // sqrt(2): maps -3 dBFS → 0 VU and 0 dBFS → +3 VU.
        constexpr float calibration = 1.4142f;
        float leftNeedle = computeNeedleNormalized(
            args.sampleTime, leftPeak, calibration, vuMeterLeft);
        float rightNeedle = computeNeedleNormalized(
            args.sampleTime, rightPeak, calibration, vuMeterRight);

        lights[LEFT_VU_LIGHT].setBrightness(applyNeedleBallistics(args.sampleTime, leftNeedle, leftNeedleDisplay));
        lights[RIGHT_VU_LIGHT].setBrightness(applyNeedleBallistics(args.sampleTime, rightNeedle, rightNeedleDisplay));
    }

private:
    void passThroughAudio(int inputId, int outputId) {
        int channels = std::min(inputs[inputId].getChannels(), MAX_CHANNELS);
        outputs[outputId].setChannels(channels);
        for (int c = 0; c < channels; ++c) {
            outputs[outputId].setVoltage(inputs[inputId].getVoltage(c), c);
        }
    }

    float getPeakVoltage(Input& input) {
        int channels = std::min(input.getChannels(), MAX_CHANNELS);
        float peak = 0.f;
        for (int c = 0; c < channels; ++c) {
            peak = std::max(peak, std::fabs(input.getVoltage(c)));
        }
        return peak;
    }

    float computeNeedleNormalized(float deltaTime,
                                  float peakVoltage,
                                  float calibration,
                                  dsp::VuMeter2& meter) {
        // Calibrate for standard Rack audio levels: 10 Vpp (5 V peak) ~= 0 VU at default calibration.
        float cal = rack::math::clamp(calibration, 0.5f, 2.f);
        float reference = 5.f / cal;
        meter.process(deltaTime, peakVoltage / reference);

        float amplitude = std::max(meter.v, 1e-6f);
        float db = rack::dsp::amplitudeToDb(amplitude);
        return dbToNeedleNormalized(db);
    }

    float dbToNeedleNormalized(float db) const {
        // Dial model: -20dB (left) -> 0dB mark (center) -> +3dB clip edge (right).
        constexpr float kDbMin = -20.f;
        constexpr float kDbZero = 0.f;
        constexpr float kDbClip = 3.f;
        float clampedDb = rack::math::clamp(db, kDbMin, kDbClip);
        if (clampedDb <= kDbZero) {
            return rack::math::rescale(clampedDb, kDbMin, kDbZero, 0.f, 0.5f);
        }
        return rack::math::rescale(clampedDb, kDbZero, kDbClip, 0.5f, 1.f);
    }

    float applyNeedleBallistics(float deltaTime, float target, float& state) const {
        // Fast attack, slower release.
        constexpr float attackTau = 0.015f;
        constexpr float releaseTau = 0.45f;
        float tau = (target > state) ? attackTau : releaseTau;
        float alpha = 1.f - std::exp(-deltaTime / std::max(tau, 1e-4f));
        state += (target - state) * alpha;
        return rack::math::clamp(state, 0.f, 1.f);
    }

public:
    void setMeterScreenBrightness(float value) {
        meterScreenBrightness = rack::math::clamp(value, 0.f, 1.f);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "meterScreenBrightness", json_real(meterScreenBrightness));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* brightnessJ = json_object_get(rootJ, "meterScreenBrightness");
        if (brightnessJ) {
            setMeterScreenBrightness(json_number_value(brightnessJ));
        }
    }
};

struct SpeculaWidget : ModuleWidget {
    struct VUMeterInsetFrame : Widget {
        void draw(const DrawArgs& args) override {
            NVGcontext* vg = args.vg;
            float x = 0.f;
            float y = 0.f;
            float w = box.size.x;
            float h = box.size.y;
            float r = std::min(w, h) * 0.105f;

            nvgSave(vg);

            NVGpaint castShadow = nvgBoxGradient(
                vg, x + 0.45f, y + 0.85f, w - 0.1f, h - 0.05f, r, 2.2f,
                nvgRGBA(0, 0, 0, 70),
                nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + 0.05f, y + 0.25f, w + 0.55f, h + 0.75f, r + 0.25f);
            nvgFillPaint(vg, castShadow);
            nvgFill(vg);

            NVGpaint base = nvgLinearGradient(
                vg, x, y, x, y + h,
                nvgRGBA(255, 238, 204, 12),
                nvgRGBA(0, 0, 0, 36));
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, w, h, r);
            nvgFillPaint(vg, base);
            nvgFill(vg);

            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + 0.15f, y + 0.15f, w - 0.3f, h - 0.3f, r - 0.1f);
            nvgStrokeWidth(vg, 0.45f);
            nvgStrokeColor(vg, nvgRGBA(255, 238, 204, 22));
            nvgStroke(vg);

            nvgBeginPath(vg);
            nvgMoveTo(vg, x + r * 0.92f, y + 0.38f);
            nvgLineTo(vg, x + w - r * 0.92f, y + 0.38f);
            nvgStrokeWidth(vg, 0.5f);
            nvgStrokeColor(vg, nvgRGBA(255, 248, 226, 28));
            nvgStroke(vg);

            nvgBeginPath(vg);
            nvgMoveTo(vg, x + w - 0.34f, y + r);
            nvgLineTo(vg, x + w - 0.34f, y + h - r);
            nvgStrokeWidth(vg, 0.55f);
            nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 78));
            nvgStroke(vg);

            nvgBeginPath(vg);
            nvgMoveTo(vg, x + r * 0.92f, y + h - 0.34f);
            nvgLineTo(vg, x + w - r * 0.92f, y + h - 0.34f);
            nvgStrokeWidth(vg, 0.55f);
            nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 92));
            nvgStroke(vg);

            nvgRestore(vg);
        }
    };

    // Match the uniform Clairaudient/Tessellation/Transmutation/Torsion leather treatment
    void draw(const DrawArgs& args) override {
        std::shared_ptr<Image> bg = panelBackgroundImage();
        if (bg) {
            // Keep leather grain density consistent across panel widths via fixed-height tiling.
            constexpr float inset = 2.0f;
            constexpr float textureAspect = 2048.f / 3238.f;  // panel_background.png
            float tileH = box.size.y + inset * 2.f;
            float tileW = tileH * textureAspect;
            float x = -inset;
            float y = -inset;

            nvgSave(args.vg);

            // Base tile pass
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
            NVGpaint paintA = nvgImagePattern(args.vg, x, y, tileW, tileH, 0.f, bg->handle, 1.0f);
            nvgFillPaint(args.vg, paintA);
            nvgFill(args.vg);

            // Offset low-opacity pass to soften seam visibility
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
            NVGpaint paintB = nvgImagePattern(args.vg, x + tileW * 0.5f, y, tileW, tileH, 0.f, bg->handle, 0.35f);
            nvgFillPaint(args.vg, paintB);
            nvgFill(args.vg);

            // Slight darkening to match existing module tone
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 18));
            nvgFill(args.vg);

            nvgRestore(args.vg);
        }
        ModuleWidget::draw(args);

        // Draw a black inner frame to fully mask any edge tinting
        constexpr float frame = 1.0f;
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
        nvgRect(args.vg, frame, frame, box.size.x - 2.f * frame, box.size.y - 2.f * frame);
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
    }

    SpeculaWidget(Specula* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Specula.svg")));
        auto wearOverlay = new shapetaker::ui::PanelWearOverlay();
        wearOverlay->box = Rect(Vec(0, 0), box.size);
        addChild(wearOverlay);

        using LayoutHelper = shapetaker::ui::LayoutHelper;
        LayoutHelper::ScrewPositions::addStandardScrews<ScrewJetBlack>(this, box.size.x);
        addAlchemicalBadge(this, 73); // Labrys (double axe)

        // Parse SVG panel for precise positioning
        shapetaker::ui::LayoutHelper::PanelSVGParser parser(asset::plugin(pluginInstance, "res/panels/Specula.svg"));
        auto centerPx = shapetaker::ui::LayoutHelper::createCenterPxHelper(parser);

        Rect leftMeterRect = parser.rectMm("left_vu_meter", 6.367703f, 14.433204f, 38.064594f, 39.764595f);
        Rect rightMeterRect = parser.rectMm("right_vu_meter", 6.367703f, 62.969048f, 38.064594f, 39.764595f);
        constexpr float kMeterScale = 1.10f;
        auto scaleRectFromCenter = [](const Rect& rect, float scale) {
            Vec center = rect.pos.plus(rect.size.div(2.f));
            Vec scaledSize = rect.size.mult(scale);
            return Rect(center.minus(scaledSize.div(2.f)), scaledSize);
        };
        leftMeterRect = scaleRectFromCenter(leftMeterRect, kMeterScale);
        rightMeterRect = scaleRectFromCenter(rightMeterRect, kMeterScale);

        auto addMeterInset = [&](const Rect& meterRect) {
            Rect frameRect = meterRect;
            Vec padding = Vec(0.02f, 0.02f);
            frameRect.pos = frameRect.pos.minus(padding);
            frameRect.size = frameRect.size.plus(padding.mult(2.f));

            auto* frame = new VUMeterInsetFrame();
            frame->box.pos = mm2px(frameRect.pos);
            frame->box.size = mm2px(frameRect.size);
            addChild(frame);
        };

        addMeterInset(leftMeterRect);
        auto* leftMeter = new shapetaker::ui::VintageVUMeterWidget(
            module, Specula::LEFT_VU_LIGHT, asset::plugin(pluginInstance, "res/meters/vintage_vu.svg"));
        if (module) {
            leftMeter->setScreenBrightnessPointer(&module->meterScreenBrightness);
        }
        leftMeter->box.size = mm2px(leftMeterRect.size);
        leftMeter->box.pos = mm2px(leftMeterRect.pos);
        addChild(leftMeter);

        addMeterInset(rightMeterRect);
        auto* rightMeter = new shapetaker::ui::VintageVUMeterWidget(
            module, Specula::RIGHT_VU_LIGHT, asset::plugin(pluginInstance, "res/meters/vintage_vu.svg"));
        if (module) {
            rightMeter->setScreenBrightnessPointer(&module->meterScreenBrightness);
        }
        rightMeter->box.size = mm2px(rightMeterRect.size);
        rightMeter->box.pos = mm2px(rightMeterRect.pos);
        addChild(rightMeter);

        // Use SVG positioning for inputs and outputs
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("left_input", 9.3099117f, 114.5f), module, Specula::LEFT_INPUT));
        addInput(createInputCentered<ShapetakerBNCPort>(centerPx("right_input", 20.391472f, 114.5f), module, Specula::RIGHT_INPUT));

        constexpr float kSpeculaOutputHaloMm = 9.9f;
        addOutputWithHalo<ShapetakerBNCPort>(this, centerPx("left_output", 31.473032f, 114.5f), module, Specula::LEFT_OUTPUT, kSpeculaOutputHaloMm);
        addOutputWithHalo<ShapetakerBNCPort>(this, centerPx("right_output", 42.554592f, 114.5f), module, Specula::RIGHT_OUTPUT, kSpeculaOutputHaloMm);
    }

    void appendContextMenu(Menu* menu) override {
        ModuleWidget::appendContextMenu(menu);
        auto* specula = dynamic_cast<Specula*>(module);
        if (!specula) {
            return;
        }

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Meter Display"));
        menu->addChild(shapetaker::ui::createPercentageSlider(
            specula,
            [](Specula* m, float value) { m->setMeterScreenBrightness(value); },
            [](Specula* m) { return m->meterScreenBrightness; },
            "Screen brightness",
            0.62f));
    }
};

Model* modelSpecula = createModel<Specula, SpeculaWidget>("Specula");
