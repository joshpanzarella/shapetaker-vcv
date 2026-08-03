#pragma once
#include "../plugin.hpp"

// Forward declaration - Involution struct is defined in involution.cpp before this header is included
struct Involution;

struct ChaosVisualizer : Widget {
    Involution* module;
    float time = 0.0f;
    float chaosPhase = 0.0f;
    float filterMorphPhase = 0.0f;
    float cutoffPhase = 0.0f;
    float resonancePhase = 0.0f;
    float inputRotationPhase = 0.0f;
    float visualInputActivity = 0.0f;
    shapetaker::FastSmoother visualChaosRateSmoother;
    shapetaker::FastSmoother visualCutoffASmoother, visualCutoffBSmoother;
    shapetaker::FastSmoother visualResonanceASmoother, visualResonanceBSmoother;
    shapetaker::FastSmoother visualFilterMorphSmoother, visualChaosAmountSmoother;
    shapetaker::FastSmoother visualOrbitSmoother, visualTideSmoother;

    ChaosVisualizer(Involution* module) : module(module) {
        box.size = Vec(173, 138);
    }

    void step() override;
    void onButton(const event::Button& e) override;
    void drawLayer(const DrawArgs& args, int layer) override;

private:
    void drawSquareChaos(NVGcontext* vg, float cx, float cy, float maxRadius,
                        float chaosAmount, float chaosPhase, float filterMorph,
                        float orbitAmount, float tideAmount,
                        float cutoffA, float cutoffB, float resonanceA, float resonanceB,
                        float filterMorphPhase, float cutoffPhase, float resonancePhase);
};

// Note: Implementations are in involution.cpp after the Involution struct definition
// This avoids circular dependency issues while keeping the widget code organized
