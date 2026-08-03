// ChaosVisualizer implementation
// This file is included in involution.cpp after the Involution struct is fully defined

// Chaos visualizer screen color theme palette
struct ChaosParticleRange {
    float rBase, rT, rAura;
    float gBase, gT, gAura;
    float bBase, bT, bAura;
};

struct ChaosThemePalette {
    NVGcolor bgInner;          // Screen background radial gradient inner
    NVGcolor bgOuter;          // Screen background radial gradient outer
    NVGcolor hotspotInner;     // Center hotspot glow (with alpha)
    NVGcolor gridColor;        // Grid line color
    NVGcolor outerGlowInner;   // CRT outer glow inner
    NVGcolor outerGlowOuter;   // CRT outer glow outer
    NVGcolor innerGlowInner;   // CRT inner glow inner
    NVGcolor innerGlowOuter;   // CRT inner glow outer
    ChaosParticleRange hueRanges[3]; // Particle color per hue sector
    NVGcolor ledBaseColors[3]; // Jewel LED base colors (aura, orbit, tide)
    NVGcolor ledGlowColor;     // Rate glow halo/core color
};

static const ChaosThemePalette CHAOS_THEMES[] = {
    // 0: Phosphor - Green terminal CRT spectrum (default)
    {
        nvgRGB(8, 14, 8),                    // bgInner
        nvgRGB(3, 6, 3),                     // bgOuter
        nvgRGBA(14, 26, 14, 120),            // hotspotInner
        nvgRGBA(0, 200, 60, 20),             // gridColor
        nvgRGBA(0, 140, 40, 20),             // outerGlowInner (reduced alpha)
        nvgRGBA(0, 40, 10, 0),              // outerGlowOuter
        nvgRGBA(0, 200, 60, 30),             // innerGlowInner (reduced alpha)
        nvgRGBA(0, 60, 18, 0),              // innerGlowOuter
        {   // hueRanges
            {0, 0, 0,     255, 0, 0,       80, 80, 40},   // Range 0: green w/ blue
            {0, 80, 30,   255, 0, 0,      160, -80, 0},   // Range 1: green→teal
            {80, -80, 30, 200, -60, 0,     40, 0, 0}      // Range 2: teal→green
        },
        {nvgRGB(80, 220, 60), nvgRGB(40, 200, 120), nvgRGB(60, 180, 160)}, // ledBaseColors
        nvgRGB(0, 255, 80)     // ledGlowColor
    },
    // 1: Lunar - Cool cyan spectrum (more cyan, less blue for better visibility)
    {
        nvgRGB(8, 12, 22),                   // bgInner
        nvgRGB(2, 4, 10),                    // bgOuter
        nvgRGBA(16, 22, 38, 120),            // hotspotInner
        nvgRGBA(0, 200, 255, 20),            // gridColor (more cyan)
        nvgRGBA(0, 140, 200, 20),            // outerGlowInner (reduced alpha)
        nvgRGBA(0, 35, 50, 0),              // outerGlowOuter (more cyan)
        nvgRGBA(0, 200, 240, 30),            // innerGlowInner (reduced alpha)
        nvgRGBA(0, 50, 70, 0),              // innerGlowOuter (cyan tint)
        {   // hueRanges
            {0, 120, 20,  200, 60, 20,   255, 0, 0},     // Range 0: cyan→aqua
            {100, -40, 20, 230, -60, 0,   255, 0, 0},     // Range 1: aqua→lunar
            {0, 80, 20, 200, -60, 0,   255, 0, 0}      // Range 2: lunar→cyan
        },
        {nvgRGB(0, 200, 255), nvgRGB(100, 220, 255), nvgRGB(170, 210, 255)}, // ledBaseColors
        nvgRGB(0, 230, 255)  // ledGlowColor (bright cyan)
    },
    // 2: Solar - Warm yellow/gold spectrum
    {
        nvgRGB(16, 14, 6),                   // bgInner (yellow-tinted)
        nvgRGB(6, 5, 2),                     // bgOuter
        nvgRGBA(26, 24, 10, 120),            // hotspotInner
        nvgRGBA(220, 200, 60, 20),           // gridColor (bright yellow)
        nvgRGBA(180, 160, 40, 20),           // outerGlowInner (reduced alpha)
        nvgRGBA(50, 45, 10, 0),             // outerGlowOuter
        nvgRGBA(220, 200, 80, 30),           // innerGlowInner (reduced alpha)
        nvgRGBA(70, 60, 20, 0),             // innerGlowOuter
        {   // hueRanges
            {255, 0, 0,   220, 100, 40,    80, 80, 0},    // Range 0: gold→yellow
            {255, -50, 30, 255, 0, 0,     120, -60, 0},   // Range 1: yellow→amber
            {220, -80, 30, 180, 0, 0,      60, 0, 0}      // Range 2: amber→gold
        },
        {nvgRGB(255, 230, 80), nvgRGB(255, 210, 100), nvgRGB(255, 200, 120)}, // ledBaseColors
        nvgRGB(255, 230, 80)   // ledGlowColor
    },
    // 3: Amber - Warm CRT orange/amber spectrum
    {
        nvgRGB(16, 10, 6),                   // bgInner
        nvgRGB(6, 4, 2),                     // bgOuter
        nvgRGBA(26, 16, 10, 120),            // hotspotInner
        nvgRGBA(200, 120, 0, 20),            // gridColor
        nvgRGBA(140, 80, 0, 20),             // outerGlowInner (reduced alpha)
        nvgRGBA(40, 20, 0, 0),              // outerGlowOuter
        nvgRGBA(200, 120, 0, 30),            // innerGlowInner (reduced alpha)
        nvgRGBA(60, 35, 0, 0),              // innerGlowOuter
        {   // hueRanges
            {255, 0, 0,    80, 120, 40,    0, 20, 0},     // Range 0: red→amber
            {255, 0, 0,   200, -100, 0,   20, -20, 0},    // Range 1: gold→orange
            {255, -105, 30, 40, 0, 0,      0, 0, 0}       // Range 2: orange→red
        },
        {nvgRGB(220, 100, 40), nvgRGB(200, 170, 50), nvgRGB(255, 140, 60)}, // ledBaseColors
        nvgRGB(255, 160, 0)    // ledGlowColor
    }
};

static const int NUM_CHAOS_THEMES = 4;
static const char* const CHAOS_THEME_NAMES[] = {"Phosphor", "Lunar", "Solar", "Amber"};

namespace {
constexpr float kFallbackRefreshHz = 60.f;
constexpr float kBezelThinScale = 0.4624f;
constexpr float kPanelCutoutScale = 1.09f;
constexpr float kScanlineThickness = 0.5f;
constexpr int kScanlineCount = 20;
constexpr float kResonanceBaseline = 0.707f;
constexpr float kResonanceActivityScale = 2.0f;
constexpr float kMinTotalActivity = 0.55f;
constexpr int kMinSquares = 80;
constexpr int kMaxSquares = 220;
constexpr float kDiamondBoundary = 0.9f;
constexpr float kAngleOrbitScale = 3.4f;
constexpr float kAngleOrbitDepth = 0.6f;
constexpr float kTimeSpinRate = 0.3f;
constexpr float kChaosSpinA = 1.0f;
constexpr float kChaosSpinB = 0.8f;

inline float chaosScreenHalfForSize(const Vec& size) {
    float bezelOuterHalf = std::min(size.x, size.y) * 0.9f * 0.5f;
    float bezelInnerHalf = bezelOuterHalf - bezelOuterHalf * 0.10f * kBezelThinScale;
    float lipInnerHalf = bezelInnerHalf - bezelInnerHalf * 0.07f * kBezelThinScale;
    return lipInnerHalf * 0.985f;
}

inline bool isInsideChaosScreen(const Vec& pos, const Vec& size) {
    Vec center = size.mult(0.5f);
    float screenHalf = chaosScreenHalfForSize(size);
    return std::fabs(pos.x - center.x) + std::fabs(pos.y - center.y) <= screenHalf;
}
}

inline float getVisualizerDeltaTime() {
    float refreshHz = (APP && APP->window) ? APP->window->getMonitorRefreshRate() : kFallbackRefreshHz;
    if (refreshHz <= 0.f) {
        refreshHz = kFallbackRefreshHz;
    }
    return 1.f / refreshHz;
}

inline void ChaosVisualizer::onButton(const event::Button& e) {
    if (module && e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS
            && isInsideChaosScreen(e.pos, box.size)) {
        module->chaosFrozen = !module->chaosFrozen;
        e.consume(this);
        return;
    }
    Widget::onButton(e);
}

inline void ChaosVisualizer::step() {
    Widget::step();
    float deltaTime = getVisualizerDeltaTime();

    if (module) {
        float characterMotion = 1.f;
        if (module->characterMode == Involution::CHARACTER_STILL)
            characterMotion = 0.55f;
        else if (module->characterMode == Involution::CHARACTER_VOLATILE)
            characterMotion = 1.65f;
        if (!module->isChaosFrozen())
            time += deltaTime * characterMotion;

        float activity = module->inputActivity.load(std::memory_order_relaxed) * 0.1f;
        // Snappy attack so the visualizer ignites quickly; slower decay back to idle.
        float alphaAttack  = 8.0f * deltaTime;
        float alphaRelease = 1.5f * deltaTime;
        float alpha = (activity > visualInputActivity) ? alphaAttack : alphaRelease;
        visualInputActivity += (activity - visualInputActivity) * alpha;

        // All parameter-driven phases are fully gated by signal presence so they
        // only animate when audio is actually flowing through the module.
        float signalGate = clamp(visualInputActivity, 0.f, 1.5f);
        float resonanceFieldMotion = module->chaosDestination >= Involution::CHAOS_CUTOFF_RESONANCE
            ? 1.f : 0.18f;
        float fullFieldMotion = module->chaosDestination == Involution::CHAOS_FULL_FIELD
            ? 1.f : 0.28f;

        float rawChaosRate = clamp(module->smoothedChaosRate, Involution::MOD_RATE_MIN_HZ, Involution::MOD_RATE_MAX_HZ);
        float smoothedChaosRate = visualChaosRateSmoother.process(rawChaosRate, deltaTime);
        if (!module->isChaosFrozen())
            chaosPhase += smoothedChaosRate * signalGate * characterMotion * deltaTime;

        float smoothedFilterMorph = visualFilterMorphSmoother.process(
            module->formAmount, deltaTime);
        if (!module->isChaosFrozen()) {
            filterMorphPhase += (smoothedFilterMorph + 0.1f) * 0.5f
                * signalGate * fullFieldMotion * characterMotion * deltaTime;
        }

        visualOrbitSmoother.process(module->params[Involution::MOD_PARAM].getValue(), deltaTime);
        visualTideSmoother.process(module->params[Involution::SHIMMER_PARAM].getValue(), deltaTime);

        float smoothedCutoffA = visualCutoffASmoother.process(module->effectiveCutoffA, deltaTime);
        float smoothedCutoffB = visualCutoffBSmoother.process(module->effectiveCutoffB, deltaTime);
        if (!module->isChaosFrozen()) {
            cutoffPhase += (smoothedCutoffA + smoothedCutoffB) * 0.2f
                * signalGate * characterMotion * deltaTime;
        }

        float smoothedResonanceA = visualResonanceASmoother.process(module->effectiveResonanceA, deltaTime);
        float smoothedResonanceB = visualResonanceBSmoother.process(module->effectiveResonanceB, deltaTime);
        float avgResonance = (smoothedResonanceA + smoothedResonanceB) * 0.5f;
        float resonanceActivity = (avgResonance - kResonanceBaseline) * kResonanceActivityScale;
        resonanceActivity = std::max(resonanceActivity, 0.0f);
        if (!module->isChaosFrozen()) {
            resonancePhase += resonanceActivity * 0.4f * signalGate
                * resonanceFieldMotion * characterMotion * deltaTime;
        }

        // Rotation has two layers: a very slow idle drift (always present, shows
        // the display is alive) plus signal-driven motion. Serial B>A reverses
        // the field, while Mid/Side produces a faster counterbalanced orbit.
        constexpr float kIdleSpinRate = 0.04f;  // ~1 full rev per 2.5 min
        float routingSpin = 1.f;
        if (module->routingMode == Involution::ROUTING_SERIAL_A_B)
            routingSpin = 0.82f;
        else if (module->routingMode == Involution::ROUTING_SERIAL_B_A)
            routingSpin = -0.82f;
        else if (module->routingMode == Involution::ROUTING_MID_SIDE)
            routingSpin = 1.28f;
        if (!module->isChaosFrozen()) {
            inputRotationPhase += (kIdleSpinRate * (routingSpin < 0.f ? -1.f : 1.f)
                + kTimeSpinRate * signalGate * routingSpin * characterMotion) * deltaTime;
        }
    } else {
        time += deltaTime;
    }
}

inline void ChaosVisualizer::drawLayer(const DrawArgs& args, int layer) {
    if (layer != 1) return;

    // Resolve current color theme
    int themeIdx = module
        ? module->getEffectiveChaosTheme()
        : shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeIndex();
    const ChaosThemePalette& t = CHAOS_THEMES[themeIdx];

    NVGcontext* vg = args.vg;
    float width = box.size.x;
    float height = box.size.y;
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;
    float diamondSize = std::min(width, height) * 0.9f;
    float bezelOuterHalf = diamondSize * 0.5f;
    float baseBezelThickness = bezelOuterHalf * 0.10f;
    float bezelThickness = baseBezelThickness * kBezelThinScale;
    float bezelInnerHalf = bezelOuterHalf - bezelThickness;
    float baseLipThickness = bezelInnerHalf * 0.07f;
    float lipThickness = baseLipThickness * kBezelThinScale;
    float lipInnerHalf = bezelInnerHalf - lipThickness;
    float screenHalf = chaosScreenHalfForSize(box.size);
    float screenSize = screenHalf * 2.f;

    auto addDiamond = [&](float halfSize) {
        nvgMoveTo(vg, centerX, centerY - halfSize);
        nvgLineTo(vg, centerX + halfSize, centerY);
        nvgLineTo(vg, centerX, centerY + halfSize);
        nvgLineTo(vg, centerX - halfSize, centerY);
        nvgClosePath(vg);
    };

    // Panel cutout: broad, uneven occlusion makes the whole assembly sit below
    // the copper/leather surface instead of floating above it.
    nvgBeginPath(vg);
    addDiamond(bezelOuterHalf * kPanelCutoutScale);
    addDiamond(bezelOuterHalf * 0.985f);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint cutoutShadow = nvgRadialGradient(vg, centerX, centerY,
        bezelOuterHalf * 0.92f, bezelOuterHalf * kPanelCutoutScale,
        nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 76));
    nvgFillPaint(vg, cutoutShadow);
    nvgFill(vg);

    nvgBeginPath(vg);
    addDiamond(bezelOuterHalf * 1.045f);
    addDiamond(bezelOuterHalf * 0.99f);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint directionalPocket = nvgLinearGradient(vg,
        centerX - bezelOuterHalf, centerY - bezelOuterHalf,
        centerX + bezelOuterHalf * 0.65f, centerY + bezelOuterHalf,
        nvgRGBA(0, 0, 0, 58), nvgRGBA(255, 230, 190, 12));
    nvgFillPaint(vg, directionalPocket);
    nvgFill(vg);

    // Recess shadow where the bezel sits in the panel.
    nvgBeginPath(vg);
    addDiamond(bezelOuterHalf * 1.035f);
    addDiamond(bezelOuterHalf);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint recessShadow = nvgRadialGradient(vg, centerX, centerY, bezelOuterHalf * 0.82f, bezelOuterHalf * 1.12f,
                                              nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 118));
    nvgFillPaint(vg, recessShadow);
    nvgFill(vg);

    // Main bezel ring: warm bakelite/brass blend inspired by the Specula VU housing.
    nvgBeginPath(vg);
    addDiamond(bezelOuterHalf);
    addDiamond(bezelInnerHalf);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint bezelBody = nvgLinearGradient(vg,
        centerX - bezelOuterHalf, centerY - bezelOuterHalf,
        centerX + bezelOuterHalf * 0.35f, centerY + bezelOuterHalf,
        nvgRGBA(134, 108, 82, 248), nvgRGBA(33, 25, 20, 252));
    nvgFillPaint(vg, bezelBody);
    nvgFill(vg);

    // Fine top catch-light to push the bezel curvature.
    nvgBeginPath(vg);
    addDiamond(bezelOuterHalf * 0.995f);
    addDiamond(bezelInnerHalf * 1.01f);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint topCatch = nvgLinearGradient(vg,
        centerX, centerY - bezelOuterHalf,
        centerX, centerY - bezelOuterHalf * 0.45f,
        nvgRGBA(236, 214, 186, 78), nvgRGBA(0, 0, 0, 0));
    nvgFillPaint(vg, topCatch);
    nvgFill(vg);

    // Inner lip ring where the glass drops into the housing.
    nvgBeginPath(vg);
    addDiamond(bezelInnerHalf);
    addDiamond(lipInnerHalf);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint lipShade = nvgLinearGradient(vg,
        centerX - bezelInnerHalf * 0.2f, centerY - bezelInnerHalf,
        centerX + bezelInnerHalf, centerY + bezelInnerHalf,
        nvgRGBA(6, 6, 8, 238), nvgRGBA(68, 52, 38, 154));
    nvgFillPaint(vg, lipShade);
    nvgFill(vg);

    nvgBeginPath(vg);
    addDiamond(bezelInnerHalf);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(214, 190, 160, 32));
    nvgStroke(vg);

    // Inner gasket so panel texture never shows between bezel lip and screen.
    nvgBeginPath(vg);
    addDiamond(lipInnerHalf);
    addDiamond(screenHalf);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillColor(vg, nvgRGBA(8, 8, 10, 240));
    nvgFill(vg);

    // Tight aperture shadow from the raised lip onto the glass below it.
    nvgBeginPath(vg);
    addDiamond(screenHalf * 1.02f);
    addDiamond(screenHalf * 0.90f);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint apertureOcclusion = nvgLinearGradient(vg,
        centerX - screenHalf * 0.65f, centerY - screenHalf,
        centerX + screenHalf * 0.5f, centerY + screenHalf,
        nvgRGBA(0, 0, 0, 205), nvgRGBA(0, 0, 0, 58));
    nvgFillPaint(vg, apertureOcclusion);
    nvgFill(vg);

    // Diamond screen background with backlit effect
    nvgBeginPath(vg);
    addDiamond(screenHalf);

    NVGpaint backlitPaint = nvgRadialGradient(vg, centerX, centerY, 0, screenSize * 0.6f,
                                             t.bgInner, t.bgOuter);
    nvgFillPaint(vg, backlitPaint);
    nvgFill(vg);

    // Add center hotspot
    nvgBeginPath(vg);
    nvgMoveTo(vg, centerX, centerY - screenSize/4);
    nvgLineTo(vg, centerX + screenSize/4, centerY);
    nvgLineTo(vg, centerX, centerY + screenSize/4);
    nvgLineTo(vg, centerX - screenSize/4, centerY);
    nvgClosePath(vg);
    NVGcolor hotspotOuter = t.hotspotInner;
    hotspotOuter.a = 0.0f;
    NVGpaint centerGlow = nvgRadialGradient(vg, centerX, centerY, 0, screenSize * 0.25f,
                                           t.hotspotInner, hotspotOuter);
    nvgFillPaint(vg, centerGlow);
    nvgFill(vg);

    // Screen-edge falloff keeps the CRT plane visually behind the lip.
    nvgBeginPath(vg);
    addDiamond(screenHalf * 0.995f);
    NVGpaint screenInsetShade = nvgRadialGradient(vg, centerX, centerY,
        screenHalf * 0.52f, screenHalf * 1.02f,
        nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 118));
    nvgFillPaint(vg, screenInsetShade);
    nvgFill(vg);

    // Draw diamond grid lines
    nvgStrokeColor(vg, t.gridColor);
    nvgStrokeWidth(vg, 0.5f);

    float halfSize = screenSize / 2.0f;

    // Horizontal lines
    for (int i = -2; i <= 2; i++) {
        if (i == 0) continue;
        float y = centerY + i * screenSize * 0.15f;
        float width = halfSize * (1.0f - std::fabs(y - centerY) / halfSize);

        nvgBeginPath(vg);
        nvgMoveTo(vg, centerX - width, y);
        nvgLineTo(vg, centerX + width, y);
        nvgStroke(vg);
    }

    // Vertical lines
    for (int i = -2; i <= 2; i++) {
        if (i == 0) continue;
        float x = centerX + i * screenSize * 0.15f;
        float height = halfSize * (1.0f - std::fabs(x - centerX) / halfSize);

        nvgBeginPath(vg);
        nvgMoveTo(vg, x, centerY - height);
        nvgLineTo(vg, x, centerY + height);
        nvgStroke(vg);
    }

    if (module) {
        // Only show pattern if output cables are connected (module has power)
        bool outputsConnected = module->outputs[Involution::AUDIO_A_OUTPUT].isConnected() ||
                               module->outputs[Involution::AUDIO_B_OUTPUT].isConnected();

        if (outputsConnected) {
            float deltaTime = getVisualizerDeltaTime();

            float rawChaosAmount = visualChaosAmountSmoother.process(
                module->coupleAmount, deltaTime);
            float filterMorph = visualFilterMorphSmoother.getValue();
            float orbitAmount = visualOrbitSmoother.getValue();
            float tideAmount = visualTideSmoother.getValue();
            float cutoffA = visualCutoffASmoother.process(module->effectiveCutoffA, deltaTime);
            float cutoffB = visualCutoffBSmoother.process(module->effectiveCutoffB, deltaTime);
            float resonanceA = visualResonanceASmoother.process(module->effectiveResonanceA, deltaTime);
            float resonanceB = visualResonanceBSmoother.process(module->effectiveResonanceB, deltaTime);

            // Visual density is driven by resonance and chaos depth so the screen
            // stays active without requiring Couple. Coupling adds a final bonus.
            float avgResVis = (resonanceA + resonanceB) * 0.5f;
            float resNorm = clamp((avgResVis - LiquidFilter::RESONANCE_MIN) /
                                  (LiquidFilter::RESONANCE_MAX - LiquidFilter::RESONANCE_MIN),
                                  0.f, 1.f);
            // 0.3 floor ensures minimum dot size and brightness even at zero resonance
            float chaosAmount = clamp(rawChaosAmount * 0.4f + resNorm * 0.8f + orbitAmount * 0.3f + 0.3f,
                                      0.f, 1.f);

            drawSquareChaos(vg, centerX, centerY, screenHalf * 0.92f, chaosAmount, chaosPhase,
                          filterMorph, orbitAmount, tideAmount,
                          cutoffA, cutoffB, resonanceA, resonanceB,
                          filterMorphPhase, cutoffPhase, resonancePhase);
        }

    }

    // CRT effects (bounded to the screen aperture so they never exceed the bezel).
    float effectOuterHalf = screenHalf * 0.995f;
    float effectMidHalf = screenHalf * 0.965f;
    float effectInnerHalf = screenHalf * 0.88f;

    nvgBeginPath(vg);
    addDiamond(effectOuterHalf);
    nvgClosePath(vg);
    NVGpaint outerGlow = nvgRadialGradient(vg, centerX, centerY, screenSize * 0.35f, screenSize * 0.55f,
                                          t.outerGlowInner, t.outerGlowOuter);
    nvgFillPaint(vg, outerGlow);
    nvgFill(vg);

    nvgBeginPath(vg);
    addDiamond(effectMidHalf);
    nvgClosePath(vg);
    NVGpaint innerGlow = nvgRadialGradient(vg, centerX, centerY, screenSize * 0.25f, screenSize * 0.38f,
                                          t.innerGlowInner, t.innerGlowOuter);
    nvgFillPaint(vg, innerGlow);
    nvgFill(vg);

    nvgBeginPath(vg);
    addDiamond(effectInnerHalf);
    nvgClosePath(vg);
    NVGpaint bulgeHighlight = nvgRadialGradient(vg,
        centerX - screenSize * 0.15f, centerY - screenSize * 0.15f,
        screenSize * 0.05f, screenSize * 0.4f,
        nvgRGBA(255, 255, 255, 25), nvgRGBA(255, 255, 255, 0));
    nvgFillPaint(vg, bulgeHighlight);
    nvgFill(vg);

    // Scanlines
    nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 40));
    nvgStrokeWidth(vg, kScanlineThickness);
    for (int i = 0; i < kScanlineCount; i++) {
        float y = centerY - screenSize/2 + (i / static_cast<float>(kScanlineCount - 1)) * screenSize;
        float lineWidth = screenSize * (1.0f - 2.0f * std::fabs(y - centerY) / screenSize);
        if (lineWidth > 0) {
            nvgBeginPath(vg);
            nvgMoveTo(vg, centerX - lineWidth/2, y);
            nvgLineTo(vg, centerX + lineWidth/2, y);
            nvgStroke(vg);
        }
    }

    // Vignette
    nvgBeginPath(vg);
    nvgMoveTo(vg, centerX, centerY - screenSize/2);
    nvgLineTo(vg, centerX + screenSize/2, centerY);
    nvgLineTo(vg, centerX, centerY + screenSize/2);
    nvgLineTo(vg, centerX - screenSize/2, centerY);
    nvgClosePath(vg);
    NVGpaint vignette = nvgRadialGradient(vg, centerX, centerY, screenSize * 0.2f, screenSize * 0.5f,
                                         nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 30));
    nvgFillPaint(vg, vignette);
    nvgFill(vg);

    // Final bezel cap: guarantees no screen/effect pixel can visually bleed onto bezel metal.
    nvgBeginPath(vg);
    addDiamond(bezelInnerHalf);
    addDiamond(lipInnerHalf);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint lipShadeCap = nvgLinearGradient(vg,
        centerX - bezelInnerHalf * 0.2f, centerY - bezelInnerHalf,
        centerX + bezelInnerHalf, centerY + bezelInnerHalf,
        nvgRGBA(6, 6, 8, 238), nvgRGBA(68, 52, 38, 154));
    nvgFillPaint(vg, lipShadeCap);
    nvgFill(vg);

    nvgBeginPath(vg);
    addDiamond(bezelInnerHalf);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(214, 190, 160, 32));
    nvgStroke(vg);

    nvgBeginPath(vg);
    addDiamond(lipInnerHalf);
    addDiamond(screenHalf);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillColor(vg, nvgRGBA(4, 4, 6, 250));
    nvgFill(vg);

    nvgBeginPath(vg);
    addDiamond(screenHalf * 1.015f);
    addDiamond(screenHalf * 0.91f);
    nvgPathWinding(vg, NVG_HOLE);
    NVGpaint finalApertureShadow = nvgLinearGradient(vg,
        centerX - screenHalf * 0.75f, centerY - screenHalf,
        centerX + screenHalf * 0.55f, centerY + screenHalf,
        nvgRGBA(0, 0, 0, 190), nvgRGBA(0, 0, 0, 28));
    nvgFillPaint(vg, finalApertureShadow);
    nvgFill(vg);
}

inline void ChaosVisualizer::drawSquareChaos(NVGcontext* vg, float cx, float cy, float maxRadius,
                    float chaosAmount, float chaosPhase, float auraAmount,
                    float orbitAmount, float tideAmount,
                    float cutoffA, float cutoffB, float resonanceA, float resonanceB,
                    float filterMorphPhase, float cutoffPhase, float resonancePhase) {

    float coupleAmount = module ? clamp(module->coupleAmount, 0.f, 1.f) : 0.f;
    float spreadAmount = module ? clamp(module->spreadAmount, 0.f, 1.f) : 0.f;
    bool opposedForm = module && module->opposedForm;
    float formOpposition = opposedForm ? std::fabs(auraAmount - 0.5f) * 2.f : 0.f;
    float visualFormAverage = opposedForm ? 0.5f : auraAmount;
    int routing = module ? module->routingMode : Involution::ROUTING_PARALLEL;
    int destination = module ? module->chaosDestination : Involution::CHAOS_FULL_FIELD;
    float characterDynamics = 1.f;
    if (module && module->characterMode == Involution::CHARACTER_STILL)
        characterDynamics = 0.58f;
    else if (module && module->characterMode == Involution::CHARACTER_VOLATILE)
        characterDynamics = 1.55f;
    float resonanceField = destination >= Involution::CHAOS_CUTOFF_RESONANCE ? 1.f : 0.22f;
    float fullField = destination == Involution::CHAOS_FULL_FIELD ? 1.f : 0.35f;

    float totalActivity = chaosAmount + (cutoffA + cutoffB) * 0.2f
        + spreadAmount * 0.16f + coupleAmount * 0.12f;

    float avgResonance = (resonanceA + resonanceB) * 0.5f;
    float resonanceActivity = (avgResonance - kResonanceBaseline) * kResonanceActivityScale;
    resonanceActivity = std::max(resonanceActivity, 0.0f);
    totalActivity += resonanceActivity * 0.3f;

    totalActivity = std::max(totalActivity, kMinTotalActivity);

    int baseSquares = kMinSquares + (int)(visualFormAverage * 20)
        + (int)(formOpposition * 10.f) + (int)(spreadAmount * 14.f)
        + (int)(coupleAmount * 10.f);
    int resonanceSquares = (int)(resonanceActivity * 80);
    int activitySquares = (int)(totalActivity * 120);
    int numSquares = baseSquares + activitySquares + resonanceSquares;
    numSquares = clamp(numSquares, kMinSquares, kMaxSquares);

    int themeIdx = module
        ? module->getEffectiveChaosTheme()
        : shapetaker::ui::ThemeManager::DisplayTheme::getSharedThemeIndex();
    const ChaosParticleRange* ranges = CHAOS_THEMES[themeIdx].hueRanges;

    for (int i = 0; i < numSquares; i++) {
        float particleT = i / (float)numSquares;
        float lanePolarity = (i & 1) ? 1.f : -1.f;
        float particleForm = opposedForm
            ? (lanePolarity < 0.f ? auraAmount : 1.f - auraAmount)
            : auraAmount;
        float angle = (i / (float)numSquares) * 2.0f * M_PI * (kAngleOrbitScale + orbitAmount * kAngleOrbitDepth);

        angle += inputRotationPhase;
        angle += chaosPhase * kChaosSpinA;
        angle += chaosPhase * kChaosSpinB;
        angle += filterMorphPhase;
        angle += cutoffPhase;
        angle += resonancePhase;
        angle += sinf(i * 0.17f + chaosPhase * 2.f) * coupleAmount * 0.48f;
        angle += lanePolarity * formOpposition * 0.32f;
        if (routing == Involution::ROUTING_SERIAL_A_B)
            angle += particleT * 0.62f;
        else if (routing == Involution::ROUTING_SERIAL_B_A)
            angle -= particleT * 0.62f;
        else if (routing == Involution::ROUTING_MID_SIDE)
            angle += sinf(angle * 2.f) * 0.24f;

        // Power-curve distribution (t^1.5) concentrates particles toward center
        // and reduces pile-up at the diamond boundary clamp.
        float baseRadius = particleT * sqrtf(particleT) * maxRadius;
        float radiusVar = sinf(time * (3.0f + tideAmount * 1.5f) + i * 0.2f)
            * maxRadius * 0.10f * chaosAmount * (1.f + orbitAmount * 0.4f)
            * characterDynamics * (0.72f + fullField * 0.28f);
        float resonancePulse = sinf(time * 4.0f + i * 0.5f) * maxRadius * 0.15f
            * resonanceActivity * resonanceField * characterDynamics;
        float spreadOffset = lanePolarity * spreadAmount * maxRadius * 0.085f
            * (0.25f + particleT * 0.75f);
        float formContour = sinf(angle * 2.f + filterMorphPhase) * maxRadius
            * 0.045f * visualFormAverage * fullField;
        formContour += cosf(angle * 2.f - filterMorphPhase) * maxRadius
            * 0.055f * lanePolarity * formOpposition * fullField;
        float radius = baseRadius + radiusVar + resonancePulse + spreadOffset + formContour;
        radius *= (0.8f + cutoffA * 0.2f + cutoffB * 0.2f + resonanceActivity * 0.1f);
        radius *= 1.f - coupleAmount * 0.055f;
        radius = std::max(radius, 0.f);

        float x = cx + cosf(angle) * radius;
        float y = cy + sinf(angle) * radius;

        // Routing changes the same particle field instead of adding a separate
        // diagram: serial modes shear it in signal-flow direction, while M/S
        // folds it into opposing, stereo-symmetric lobes.
        if (routing == Involution::ROUTING_SERIAL_A_B) {
            x += (y - cy) * (0.045f + coupleAmount * 0.075f);
        } else if (routing == Involution::ROUTING_SERIAL_B_A) {
            x -= (y - cy) * (0.045f + coupleAmount * 0.075f);
        } else if (routing == Involution::ROUTING_MID_SIDE) {
            float fold = sinf(angle * 2.f) * maxRadius * (0.035f + spreadAmount * 0.035f);
            x += fold;
            y -= fold * lanePolarity;
        }

        float distanceFromCenterX = std::fabs(x - cx);
        float distanceFromCenterY = std::fabs(y - cy);
        float diamondDistance = distanceFromCenterX / maxRadius + distanceFromCenterY / maxRadius;

        if (diamondDistance > kDiamondBoundary) {
            float scale = kDiamondBoundary / diamondDistance;
            x = cx + (x - cx) * scale;
            y = cy + (y - cy) * scale;
        }

        float baseSize = 0.35f + chaosAmount * 0.6f + orbitAmount * 0.2f;
        float sizeVar = sinf(time * (4.0f + tideAmount * 1.5f) + i * 0.32f + filterMorphPhase * 0.5f)
            * (0.18f + orbitAmount * 0.1f) * characterDynamics;
        float resonanceScale = 0.2f + 0.4f * resonanceActivity;
        float resonanceSize = (resonanceScale * 0.45f
            + sinf(time * 5.5f + i * 0.3f) * resonanceScale * 0.32f)
            * (0.35f + resonanceField * 0.65f);
        float dotRadius = clamp(baseSize + sizeVar + resonanceSize
            + spreadAmount * 0.08f, 0.18f, 1.5f);

        float hue = fmodf(time * (30.0f + tideAmount * 12.f) + i * (15.0f + particleForm * 6.f)
                          + particleForm * 140.0f + orbitAmount * 150.0f
                          + resonanceActivity * 120.0f + spreadAmount * 85.f
                          + coupleAmount * 70.f + routing * 24.f, 360.0f);

        float baseBrightness = 0.55f;
        float activityBrightness = chaosAmount * 0.85f;
        float filterBrightness = (cutoffA + cutoffB) * 0.12f;
        float resonanceBrightness = resonanceActivity * 0.22f;
        float orbitBrightness = orbitAmount * 0.45f;

        float brightness = baseBrightness + activityBrightness + filterBrightness
            + resonanceBrightness * resonanceField + orbitBrightness
            + spreadAmount * 0.08f + coupleAmount * 0.10f;
        brightness = clamp(brightness, 0.7f, 2.2f);
        float radiusNorm = radius / maxRadius;
        brightness *= (1.0f - radiusNorm * 0.18f);
        if (radiusNorm < 0.25f) {
            float boost = (0.25f - radiusNorm);
            brightness += boost * (2.7f + orbitAmount * 1.4f + particleForm * 0.9f + tideAmount * 0.9f);
            dotRadius += boost * 0.9f;
        }
        brightness = clamp(brightness, 0.75f, 2.3f);

        NVGcolor color;
        int rangeIdx;
        float hueT;
        if (hue < 120.0f) {
            rangeIdx = 0;
            hueT = hue / 120.0f;
        } else if (hue < 240.0f) {
            rangeIdx = 1;
            hueT = (hue - 120.0f) / 120.0f;
        } else {
            rangeIdx = 2;
            hueT = (hue - 240.0f) / 120.0f;
        }
        const ChaosParticleRange& r = ranges[rangeIdx];
        color = nvgRGBA(
            (int)((r.rBase + hueT * r.rT + particleForm * r.rAura) * brightness),
            (int)((r.gBase + hueT * r.gT + particleForm * r.gAura) * brightness),
            (int)((r.bBase + hueT * r.bT + particleForm * r.bAura) * brightness),
            (int)(brightness * 255));

        nvgBeginPath(vg);
        nvgCircle(vg, x, y, dotRadius);
        nvgFillColor(vg, color);
        nvgFill(vg);

        NVGcolor coreColor = color;
        coreColor.a = clamp(coreColor.a + 0.4f, 0.0f, 1.0f);
        nvgBeginPath(vg);
        nvgCircle(vg, x, y, dotRadius * 0.42f);
        nvgFillColor(vg, coreColor);
        nvgFill(vg);

        nvgSave(vg);
        nvgGlobalCompositeOperation(vg, NVG_LIGHTER);
        NVGcolor haloInner = color;
        haloInner.a = clamp(color.a * 0.65f + 0.25f, 0.25f, 0.95f);
        NVGcolor haloOuter = color;
        haloOuter.a = 0.0f;
        NVGpaint halo = nvgRadialGradient(vg, x, y, dotRadius * 0.3f, dotRadius * 2.4f, haloInner, haloOuter);
        nvgBeginPath(vg);
        nvgCircle(vg, x, y, dotRadius * 2.3f);
        nvgFillPaint(vg, halo);
        nvgFill(vg);
        nvgRestore(vg);
    }
}
