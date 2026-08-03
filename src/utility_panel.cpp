#include "plugin.hpp"
#include "ui/layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

struct UtilityPanel : Module {
    static constexpr int MIN_WIDTH_HP = 2;
    static constexpr int MAX_WIDTH_HP = 64;
    static constexpr int PLACEMENT_WIDTH_HP = 2;
    static constexpr int DEFAULT_WIDTH_HP = 12;

    int panelWidthHp = DEFAULT_WIDTH_HP;
    bool loadedFromJson = false;

    UtilityPanel() {
        config(0, 0, 0, 0);
    }

    void setPanelWidthHp(int hp) {
        panelWidthHp = clamp(hp, MIN_WIDTH_HP, MAX_WIDTH_HP);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelWidthHp", json_integer(panelWidthHp));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        loadedFromJson = true;
        json_t* widthJ = json_object_get(rootJ, "panelWidthHp");
        if (widthJ) {
            setPanelWidthHp(json_integer_value(widthJ));
        }
    }
};

struct UtilityPanelWidget;

struct UtilityPanelCenterScrew : ScrewJetBlack {
    UtilityPanelWidget* moduleWidget = nullptr;
    bool bottom = false;

    UtilityPanelCenterScrew(UtilityPanelWidget* moduleWidget, bool bottom)
        : moduleWidget(moduleWidget), bottom(bottom) {
    }

    void step() override;
};

struct UtilityPanelResizeHandle : widget::OpaqueWidget {
    UtilityPanelWidget* moduleWidget = nullptr;
    bool right = false;
    int dragStartHp = UtilityPanel::DEFAULT_WIDTH_HP;
    float dragAccumPx = 0.f;

    UtilityPanelResizeHandle(UtilityPanelWidget* moduleWidget, bool right)
        : moduleWidget(moduleWidget), right(right) {
        box.size = Vec(14.f, RACK_GRID_HEIGHT);
    }

    void step() override;

    void draw(const DrawArgs& args) override {
        const float edgeX = right ? box.size.x - 1.0f : 1.0f;

        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 7));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, edgeX, 0.f);
        nvgLineTo(args.vg, edgeX, box.size.y);
        nvgStrokeColor(args.vg, nvgRGBA(235, 216, 170, 42));
        nvgStrokeWidth(args.vg, 1.f);
        nvgStroke(args.vg);
    }

    void onButton(const event::Button& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this);
        }
    }

    void onDoubleClick(const event::DoubleClick& e) override;
    void onDragStart(const event::DragStart& e) override;
    void onDragMove(const event::DragMove& e) override;
    void onDragEnd(const event::DragEnd& e) override;
};

struct UtilityPanelWidget : ModuleWidget {
    static constexpr float BG_TEXTURE_ASPECT = 2048.f / 3238.f;
    static constexpr float BG_OFFSET_OPACITY = 0.35f;
    static constexpr int BG_DARKEN_ALPHA = 18;
    static constexpr int AUTO_FIT_PLACEMENT_FRAMES = 24;

    int autoFitPlacementFrames = AUTO_FIT_PLACEMENT_FRAMES;

    UtilityPanelWidget(UtilityPanel* module) {
        setModule(module);
        const bool loadedFromJson = module && module->loadedFromJson;
        const int initialHp = loadedFromJson
            ? module->panelWidthHp
            : UtilityPanel::PLACEMENT_WIDTH_HP;
        applyPanelWidthHp(initialHp, false, false);
        if (module && !loadedFromJson) {
            module->setPanelWidthHp(initialHp);
        }

        addChild(new UtilityPanelCenterScrew(this, false));
        addChild(new UtilityPanelCenterScrew(this, true));

        addChild(new UtilityPanelResizeHandle(this, false));
        addChild(new UtilityPanelResizeHandle(this, true));
    }

    bool applyPanelWidthHp(int hp, bool keepRightEdge, bool updateModule) {
        hp = clamp(hp, UtilityPanel::MIN_WIDTH_HP, UtilityPanel::MAX_WIDTH_HP);
        const float newWidth = hp * RACK_GRID_WIDTH;
        const float oldWidth = box.size.x;

        if (std::fabs(newWidth - oldWidth) < 0.001f && box.size.y > 0.f) {
            if (updateModule) {
                if (auto* utility = dynamic_cast<UtilityPanel*>(module)) {
                    utility->setPanelWidthHp(hp);
                }
            }
            return true;
        }

        const Vec oldPos = box.pos;
        const Vec oldSize = box.size;
        const float rightEdge = oldPos.x + oldWidth;
        Vec newPos = oldPos;
        if (keepRightEdge && oldWidth > 0.f) {
            newPos.x = rightEdge - newWidth;
        }

        setSize(Vec(newWidth, RACK_GRID_HEIGHT));
        setPosition(newPos);

        app::RackWidget* rackWidget = (APP && APP->scene) ? APP->scene->rack : nullptr;
        if (rackWidget && !rackWidget->requestModulePos(this, newPos)) {
            setSize(oldSize);
            setPosition(oldPos);
            return false;
        }

        if (updateModule) {
            if (auto* utility = dynamic_cast<UtilityPanel*>(module)) {
                utility->setPanelWidthHp(hp);
            }
        }
        return true;
    }

    bool resizeByDelta(float deltaX, bool dragRightEdge) {
        const float currentWidth = box.size.x > 0.f ? box.size.x : UtilityPanel::DEFAULT_WIDTH_HP * RACK_GRID_WIDTH;
        const float proposedWidth = currentWidth + (dragRightEdge ? deltaX : -deltaX);
        const int targetHp = clamp(static_cast<int>(std::round(proposedWidth / RACK_GRID_WIDTH)),
            UtilityPanel::MIN_WIDTH_HP, UtilityPanel::MAX_WIDTH_HP);
        return applyPanelWidthHp(targetHp, !dragRightEdge, true);
    }

    bool resizeFromDrag(int startHp, float deltaX, bool dragRightEdge) {
        startHp = clamp(startHp, UtilityPanel::MIN_WIDTH_HP, UtilityPanel::MAX_WIDTH_HP);
        const float startWidth = startHp * RACK_GRID_WIDTH;
        const float proposedWidth = startWidth + (dragRightEdge ? deltaX : -deltaX);
        const int targetHp = clamp(static_cast<int>(std::round(proposedWidth / RACK_GRID_WIDTH)),
            UtilityPanel::MIN_WIDTH_HP, UtilityPanel::MAX_WIDTH_HP);
        return applyPanelWidthHp(targetHp, !dragRightEdge, true);
    }

    bool isMountedInRack(app::RackWidget* rackWidget) {
        if (!rackWidget) {
            return false;
        }
        for (app::ModuleWidget* mw : rackWidget->getModules()) {
            if (mw == this) {
                return true;
            }
        }
        return false;
    }

    bool fitToSurroundingGap() {
        app::RackWidget* rackWidget = (APP && APP->scene) ? APP->scene->rack : nullptr;
        if (!rackWidget) {
            return false;
        }

        const float rowTolerance = RACK_GRID_HEIGHT * 0.25f;
        const float centerX = box.pos.x + box.size.x * 0.5f;
        float leftEdge = -std::numeric_limits<float>::infinity();
        float rightEdge = std::numeric_limits<float>::infinity();

        for (app::ModuleWidget* mw : rackWidget->getModules()) {
            if (!mw || mw == this) {
                continue;
            }
            if (std::fabs(mw->box.pos.y - box.pos.y) > rowTolerance) {
                continue;
            }

            const float moduleLeft = mw->box.pos.x;
            const float moduleRight = mw->box.pos.x + mw->box.size.x;
            if (moduleRight <= centerX && moduleRight > leftEdge) {
                leftEdge = moduleRight;
            }
            if (moduleLeft >= centerX && moduleLeft < rightEdge) {
                rightEdge = moduleLeft;
            }
        }

        if (!std::isfinite(leftEdge) && std::isfinite(rightEdge) && centerX <= rightEdge) {
            leftEdge = 0.f;
        }

        if (!std::isfinite(leftEdge) || !std::isfinite(rightEdge) || rightEdge <= leftEdge) {
            return false;
        }

        const int gapHp = clamp(static_cast<int>(std::round((rightEdge - leftEdge) / RACK_GRID_WIDTH)),
            UtilityPanel::MIN_WIDTH_HP, UtilityPanel::MAX_WIDTH_HP);
        const float targetWidth = gapHp * RACK_GRID_WIDTH;
        const Vec oldPos = box.pos;
        const Vec oldSize = box.size;
        const Vec targetPos(std::round(leftEdge / RACK_GRID_WIDTH) * RACK_GRID_WIDTH, box.pos.y);

        setSize(Vec(targetWidth, RACK_GRID_HEIGHT));
        setPosition(targetPos);
        if (!rackWidget->requestModulePos(this, targetPos)) {
            setSize(oldSize);
            setPosition(oldPos);
            return false;
        }

        if (auto* utility = dynamic_cast<UtilityPanel*>(module)) {
            utility->setPanelWidthHp(gapHp);
        }
        return true;
    }

    void autoFitAfterPlacement() {
        if (autoFitPlacementFrames <= 0) {
            return;
        }

        auto* utility = dynamic_cast<UtilityPanel*>(module);
        if (!utility || utility->loadedFromJson) {
            autoFitPlacementFrames = 0;
            return;
        }

        app::RackWidget* rackWidget = (APP && APP->scene) ? APP->scene->rack : nullptr;
        if (!isMountedInRack(rackWidget)) {
            return;
        }

        const bool fitted = fitToSurroundingGap();
        autoFitPlacementFrames--;
        if (fitted) {
            autoFitPlacementFrames = 0;
            return;
        }

    }

    void step() override {
        ModuleWidget::step();
        autoFitAfterPlacement();
    }

    void draw(const DrawArgs& args) override {
        std::shared_ptr<Image> bg = panelBackgroundImage();
        if (bg) {
            constexpr float inset = 2.0f;
            constexpr float textureAspect = BG_TEXTURE_ASPECT;
            float tileH = box.size.y + inset * 2.f;
            float tileW = tileH * textureAspect;
            float x = -inset;
            float y = -inset;

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

        constexpr float frame = 1.0f;
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
        nvgRect(args.vg, frame, frame, box.size.x - 2.f * frame, box.size.y - 2.f * frame);
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
    }

    void onDoubleClick(const event::DoubleClick& e) override {
        fitToSurroundingGap();
        e.consume(this);
    }

    void appendContextMenu(ui::Menu* menu) override {
        ModuleWidget::appendContextMenu(menu);
        menu->addChild(new ui::MenuSeparator());
        menu->addChild(createMenuItem("Fit to surrounding gap", "", [=] {
            fitToSurroundingGap();
        }));
    }
};

void UtilityPanelCenterScrew::step() {
    if (moduleWidget) {
        const float edgeInset = shapetaker::ui::LayoutHelper::ScrewPositions::EDGE_INSET;
        const float x = std::round((moduleWidget->box.size.x - box.size.x) * 0.5f);
        const float y = bottom ? (RACK_GRID_HEIGHT - box.size.y - edgeInset) : edgeInset;
        if (box.pos.x != x || box.pos.y != y) {
            box.pos = Vec(x, y);
        }
    }
    ScrewJetBlack::step();
}

void UtilityPanelResizeHandle::step() {
    if (moduleWidget) {
        box.pos = right ? Vec(moduleWidget->box.size.x - box.size.x, 0.f) : Vec(0.f, 0.f);
        box.size.y = moduleWidget->box.size.y;
    }
    widget::OpaqueWidget::step();
}

void UtilityPanelResizeHandle::onDragStart(const event::DragStart& e) {
    dragAccumPx = 0.f;
    if (moduleWidget) {
        if (auto* utility = dynamic_cast<UtilityPanel*>(moduleWidget->module)) {
            dragStartHp = utility->panelWidthHp;
        } else {
            dragStartHp = clamp(static_cast<int>(std::round(moduleWidget->box.size.x / RACK_GRID_WIDTH)),
                UtilityPanel::MIN_WIDTH_HP, UtilityPanel::MAX_WIDTH_HP);
        }
    }
    widget::OpaqueWidget::onDragStart(e);
}

void UtilityPanelResizeHandle::onDragMove(const event::DragMove& e) {
    if (!moduleWidget || e.button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }
    dragAccumPx += e.mouseDelta.x;
    moduleWidget->resizeFromDrag(dragStartHp, dragAccumPx, right);
}

void UtilityPanelResizeHandle::onDragEnd(const event::DragEnd& e) {
    dragAccumPx = 0.f;
    widget::OpaqueWidget::onDragEnd(e);
}

void UtilityPanelResizeHandle::onDoubleClick(const event::DoubleClick& e) {
    if (moduleWidget) {
        moduleWidget->fitToSurroundingGap();
    }
    e.consume(this);
}

Model* modelUtilityPanel = createModel<UtilityPanel, UtilityPanelWidget>("UtilityPanel");
