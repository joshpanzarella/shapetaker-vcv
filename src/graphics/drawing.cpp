// Graphics drawing utilities implementation
#include "drawing.hpp"
#include <sstream>
#include <vector>
#include <string>
// Legacy includes removed - functionality now implemented directly

namespace shapetaker {
namespace graphics {

void drawVoiceCountDots(const widget::Widget::DrawArgs& args, Vec center, int count,
                        float radius, float dotRadius, NVGcolor color) {
    if (count <= 0) return;
    
    nvgSave(args.vg);
    float angleStep = 2.0f * M_PI / count;
    for (int i = 0; i < count; i++) {
        float angle = i * angleStep - M_PI / 2; // Start at top
        float x = center.x + radius * cosf(angle);
        float y = center.y + radius * sinf(angle);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, x, y, dotRadius);
        nvgFillColor(args.vg, color);
        nvgFill(args.vg);
    }
    nvgRestore(args.vg);
}

void drawAlchemicalSymbol(const widget::Widget::DrawArgs& args, Vec pos, int symbolId,
                         NVGcolor color, float size, float strokeWidth) {
    if (!isValidSymbolId(symbolId)) return;
    nvgSave(args.vg);
    nvgTranslate(args.vg, pos.x, pos.y);
    nvgStrokeColor(args.vg, color);
    nvgFillColor(args.vg, color);
    nvgStrokeWidth(args.vg, strokeWidth);

    switch (symbolId) {
        case 0: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size * 0.3f); nvgFill(args.vg); break; // Sol
        case 1: nvgBeginPath(args.vg); nvgArc(args.vg, 0, 0, size, 0.3f * M_PI, 1.7f * M_PI, NVG_CW); nvgStroke(args.vg); break; // Luna
        case 2: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size * 0.3f, size * 0.4f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.6f, -size * 0.8f); nvgLineTo(args.vg, 0, -size * 0.6f); nvgLineTo(args.vg, size * 0.6f, -size * 0.8f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size * 0.2f); nvgLineTo(args.vg, 0, size * 0.8f); nvgMoveTo(args.vg, -size * 0.3f, size * 0.5f); nvgLineTo(args.vg, size * 0.3f, size * 0.5f); nvgStroke(args.vg); break; // Mercury
        case 3: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size * 0.3f, size * 0.5f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size * 0.2f); nvgLineTo(args.vg, 0, size * 0.8f); nvgMoveTo(args.vg, -size * 0.3f, size * 0.5f); nvgLineTo(args.vg, size * 0.3f, size * 0.5f); nvgStroke(args.vg); break; // Venus
        case 4: nvgBeginPath(args.vg); nvgCircle(args.vg, -size * 0.2f, size * 0.2f, size * 0.4f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, size * 0.2f, -size * 0.2f); nvgLineTo(args.vg, size * 0.7f, -size * 0.7f); nvgLineTo(args.vg, size * 0.4f, -size * 0.7f); nvgMoveTo(args.vg, size * 0.7f, -size * 0.7f); nvgLineTo(args.vg, size * 0.7f, -size * 0.4f); nvgStroke(args.vg); break; // Mars
        case 5: nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.6f, 0); nvgLineTo(args.vg, size * 0.2f, 0); nvgMoveTo(args.vg, 0, -size * 0.6f); nvgLineTo(args.vg, 0, size * 0.6f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgArc(args.vg, size * 0.4f, -size * 0.3f, size * 0.3f, M_PI * 0.5f, M_PI * 1.5f, NVG_CCW); nvgStroke(args.vg); break; // Jupiter
        case 6: nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.2f, 0); nvgLineTo(args.vg, size * 0.6f, 0); nvgMoveTo(args.vg, 0, -size * 0.6f); nvgLineTo(args.vg, 0, size * 0.6f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgArc(args.vg, -size * 0.4f, -size * 0.3f, size * 0.3f, M_PI * 1.5f, M_PI * 0.5f, NVG_CCW); nvgStroke(args.vg); break; // Saturn
        case 7: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, -size * 0.8f, size * 0.6f); nvgLineTo(args.vg, size * 0.8f, size * 0.6f); nvgClosePath(args.vg); nvgStroke(args.vg); break; // Fire
        case 8: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size); nvgLineTo(args.vg, -size * 0.8f, -size * 0.6f); nvgLineTo(args.vg, size * 0.8f, -size * 0.6f); nvgClosePath(args.vg); nvgStroke(args.vg); break; // Water
        case 9: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, -size * 0.8f, size * 0.6f); nvgLineTo(args.vg, size * 0.8f, size * 0.6f); nvgClosePath(args.vg); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.4f, 0); nvgLineTo(args.vg, size * 0.4f, 0); nvgStroke(args.vg); break; // Air
        case 10: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size); nvgLineTo(args.vg, -size * 0.8f, -size * 0.6f); nvgLineTo(args.vg, size * 0.8f, -size * 0.6f); nvgClosePath(args.vg); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.4f, 0); nvgLineTo(args.vg, size * 0.4f, 0); nvgStroke(args.vg); break; // Earth
        case 11: nvgBeginPath(args.vg); nvgCircle(args.vg, -size * 0.3f, 0, size * 0.4f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, size * 0.3f, 0, size * 0.4f); nvgStroke(args.vg); break; // Quintessence
        case 12: nvgBeginPath(args.vg); for (int i = 0; i < 5; i++){ int idx=(i*2)%5; float ang=idx*2*M_PI/5 - M_PI/2; float x=cosf(ang)*size, y=sinf(ang)*size; if(i==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgClosePath(args.vg); nvgStroke(args.vg); break; // Pentagram
        case 13: { // Amalgam (circle with horizontal bar and cross)
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size*0.2f, size*0.4f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.6f, -size*0.2f); nvgLineTo(args.vg, size*0.6f, -size*0.2f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size*0.2f); nvgLineTo(args.vg, 0, size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.3f, size*0.6f); nvgLineTo(args.vg, size*0.3f, size*0.6f); nvgStroke(args.vg);
            break;
        }
        case 14: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.2f); nvgLineTo(args.vg, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.5f, size * 0.2f); nvgLineTo(args.vg, size * 0.5f, size * 0.2f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgArc(args.vg, 0, -size * 0.4f, size * 0.3f, 0, M_PI, NVG_CW); nvgStroke(args.vg); break; // Ankh
        case 15: nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.8f, 0); nvgBezierTo(args.vg, -size * 0.8f, -size * 0.5f, size * 0.8f, -size * 0.5f, size * 0.8f, 0); nvgBezierTo(args.vg, size * 0.8f, size * 0.5f, -size * 0.8f, size * 0.5f, -size * 0.8f, 0); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size * 0.2f); nvgFill(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.3f, size * 0.2f); nvgLineTo(args.vg, -size * 0.3f, size * 0.8f); nvgStroke(args.vg); break; // Eye of Horus
        case 16: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size * 0.8f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, size * 0.8f, 0, size * 0.15f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, size * 0.65f, 0); nvgLineTo(args.vg, size * 0.5f, 0); nvgStroke(args.vg); break; // Ouroboros
        case 17: nvgBeginPath(args.vg); for (int i = 0; i < 3; i++) { float ang0 = i * 2.0f * M_PI / 3.0f; nvgMoveTo(args.vg, 0, 0); for (int j = 1; j <= 8; j++) { float t = j / 8.0f; float ang = ang0 + t * M_PI; float rr = t * size; nvgLineTo(args.vg, cosf(ang) * rr, sinf(ang) * rr); } } nvgStroke(args.vg); break; // Triskele
        case 18: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.6f); nvgBezierTo(args.vg, -size * 0.4f, -size * 0.2f, -size * 0.4f, size * 0.2f, 0, size * 0.6f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.6f); nvgBezierTo(args.vg, size * 0.4f, -size * 0.2f, size * 0.4f, size * 0.2f, 0, size * 0.6f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.3f, -size * 0.8f); nvgLineTo(args.vg, 0, -size * 0.6f); nvgLineTo(args.vg, size * 0.3f, -size * 0.8f); nvgStroke(args.vg); break; // Caduceus
        case 19: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgArc(args.vg, 0, -size * 0.5f, size * 0.5f, 0, M_PI, NVG_CW); nvgArc(args.vg, 0, size * 0.5f, size * 0.5f, M_PI, 2 * M_PI, NVG_CCW); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size * 0.5f, size * 0.15f); nvgFill(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, 0, size * 0.5f, size * 0.15f); nvgStroke(args.vg); break; // Yin Yang
        case 20: { // Cinnabar (circle with X and cross on top)
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, size*0.2f, size*0.5f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.35f, -size*0.15f); nvgLineTo(args.vg, size*0.35f, size*0.55f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.35f, size*0.55f); nvgLineTo(args.vg, size*0.35f, -size*0.15f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.3f); nvgLineTo(args.vg, 0, -size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.3f, -size*0.6f); nvgLineTo(args.vg, size*0.3f, -size*0.6f); nvgStroke(args.vg);
            break;
        }
        case 21: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.5f); nvgLineTo(args.vg, -size * 0.6f, size * 0.1f); nvgLineTo(args.vg, size * 0.6f, size * 0.1f); nvgClosePath(args.vg); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size * 0.1f); nvgLineTo(args.vg, 0, size * 0.8f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.3f, size * 0.45f); nvgLineTo(args.vg, size * 0.3f, size * 0.45f); nvgStroke(args.vg); break; // Sulfur (occult)
        case 22: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size * 0.6f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.8f, 0); nvgLineTo(args.vg, size * 0.8f, 0); nvgStroke(args.vg); break; // Salt
        case 23: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size * 0.4f, size * 0.3f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.1f); nvgLineTo(args.vg, 0, size); nvgStroke(args.vg); break; // Antimony
        case 24: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, 0, size); nvgMoveTo(args.vg, -size * 0.6f, 0); nvgLineTo(args.vg, size * 0.6f, 0); nvgStroke(args.vg); break; // Phosphorus
        case 25: nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.6f, size * 0.6f); nvgLineTo(args.vg, size * 0.6f, -size * 0.6f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.6f, -size * 0.6f); nvgLineTo(args.vg, size * 0.6f, size * 0.6f); nvgStroke(args.vg); break; // Arsenic
        case 26: { // Platinum (crescent moon combined with circle)
            nvgBeginPath(args.vg); nvgCircle(args.vg, size*0.2f, 0, size*0.5f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, -size*0.2f, 0, size*0.6f, 0.2f*M_PI, 1.8f*M_PI, NVG_CW); nvgStroke(args.vg);
            break;
        }
        case 27: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.8f); nvgLineTo(args.vg, -size * 0.6f, size * 0.4f); nvgLineTo(args.vg, size * 0.6f, size * 0.4f); nvgClosePath(args.vg); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.7f, 0); nvgLineTo(args.vg, size * 0.7f, 0); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgArc(args.vg, 0, size * 0.2f, size * 0.9f, M_PI, 2*M_PI, NVG_CCW); nvgStroke(args.vg); break; // Vinegar
        case 28: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.8f, 0); nvgLineTo(args.vg, size * 0.8f, 0); nvgStroke(args.vg); break; // Saltpeter
        case 29: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size * 0.22f); nvgFill(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, 0, size); nvgMoveTo(args.vg, -size, 0); nvgLineTo(args.vg, size, 0); nvgStroke(args.vg); break; // Vitriol
        case 30: { float rr = size * 0.75f; nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -rr); nvgLineTo(args.vg, rr, rr); nvgLineTo(args.vg, -rr, rr); nvgClosePath(args.vg); nvgStroke(args.vg); break; } // Nitre
        case 31: nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, -size); nvgLineTo(args.vg, size, size); nvgMoveTo(args.vg, size, -size); nvgLineTo(args.vg, -size, size); nvgStroke(args.vg); break; // Alum
        case 32: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.6f); nvgLineTo(args.vg, 0, size * 0.6f); nvgMoveTo(args.vg, -size * 0.6f, 0); nvgLineTo(args.vg, size * 0.6f, 0); nvgStroke(args.vg); break; // Sulfuric acid
        case 33: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.4f, -size); nvgLineTo(args.vg, -size * 0.4f, size); nvgMoveTo(args.vg, size * 0.4f, -size); nvgLineTo(args.vg, size * 0.4f, size); nvgStroke(args.vg); break; // Sal Ammoniac
        case 34: nvgBeginPath(args.vg); { float turns = 2.0f, steps = 40.0f; for (int i=0;i<=steps;i++){ float t=i/steps; float ang=-M_PI/2 + t*turns*2*M_PI; float r2=t*size; float x=r2*cosf(ang), y=r2*sinf(ang); if(i==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} } nvgStroke(args.vg); break; // Spirit
        case 35: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgBezierTo(args.vg, size * 0.8f, -size * 0.3f, size * 0.8f, size * 0.6f, 0, size * 0.9f); nvgBezierTo(args.vg, -size * 0.8f, size * 0.6f, -size * 0.8f, -size * 0.3f, 0, -size); nvgClosePath(args.vg); nvgStroke(args.vg); break; // Oil
        case 36: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); { float r3=size*0.65f; nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -r3); nvgLineTo(args.vg, r3, r3); nvgLineTo(args.vg, -r3, r3); nvgClosePath(args.vg); nvgStroke(args.vg);} break; // Aqua Vitae
        case 37: nvgBeginPath(args.vg); nvgRect(args.vg, -size, -size, size*2, size*2); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size); nvgLineTo(args.vg, -size * 0.8f, -size * 0.6f); nvgLineTo(args.vg, size * 0.8f, -size * 0.6f); nvgClosePath(args.vg); nvgStroke(args.vg); break; // Earth of Fire
        case 38: nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, -size * 0.2f); nvgLineTo(args.vg, -size * 0.5f, size * 0.5f); nvgLineTo(args.vg, 0, -size * 0.2f); nvgLineTo(args.vg, size * 0.5f, size * 0.5f); nvgLineTo(args.vg, size, -size * 0.2f); nvgStroke(args.vg); break; // Tartar
        case 39: nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size * 0.8f); nvgLineTo(args.vg, 0, size * 0.4f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.6f, -size * 0.2f); nvgLineTo(args.vg, size * 0.6f, -size * 0.2f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size * 0.4f, -size * 0.5f); nvgLineTo(args.vg, size * 0.4f, -size * 0.5f); nvgStroke(args.vg); nvgBeginPath(args.vg); for (float t = 0; t < 2 * M_PI; t += 0.1f) { float x = size * 0.4f * sinf(t) / (1 + cosf(t) * cosf(t)); float y = size * 0.6f + size * 0.2f * sinf(t) * cosf(t) / (1 + cosf(t) * cosf(t)); if (t == 0) nvgMoveTo(args.vg, x, y); else nvgLineTo(args.vg, x, y);} nvgStroke(args.vg); break; // Leviathan Cross
        case 40: { float r2 = size*0.6f; for (int i=0;i<3;i++){ float ang = -M_PI/2 + i*2*M_PI/3; nvgBeginPath(args.vg); nvgArc(args.vg, r2*cosf(ang), r2*sinf(ang), r2, ang+M_PI/6, ang+M_PI*5/6, NVG_CW); nvgStroke(args.vg);} break; } // Triquetra
        case 41: { nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.5f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgArc(args.vg, -size*0.8f, 0, size*0.7f, -M_PI/2, M_PI/2, NVG_CW); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgArc(args.vg, size*0.8f, 0, size*0.7f, M_PI/2, -M_PI/2, NVG_CCW); nvgStroke(args.vg); break; } // Triple Moon
        case 42: { nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); for (int i=0;i<5;i++){ float ang=-M_PI/2 + i*2*M_PI/5; if(i==0) nvgMoveTo(args.vg, size*cosf(ang), size*sinf(ang)); else nvgLineTo(args.vg, size*cosf(ang), size*sinf(ang)); float ang2=ang+2*M_PI/5; nvgLineTo(args.vg, size*0.38f*cosf(ang2), size*0.38f*sinf(ang2)); } nvgClosePath(args.vg); nvgStroke(args.vg); break; } // Pentacle
        case 43: { nvgBeginPath(args.vg); for(int i=0;i<7;i++){ int idx=(i*3)%7; float ang=-M_PI/2 + idx*2*M_PI/7; float x=size*cosf(ang), y=size*sinf(ang); if(i==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgClosePath(args.vg); nvgStroke(args.vg); break; } // Heptagram
        case 44: { nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.7f, size*0.6f); nvgLineTo(args.vg, size*0.7f, -size*0.6f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.5f, -size*0.8f); nvgLineTo(args.vg, size*0.8f, -size*0.5f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.8f, size*0.5f); nvgLineTo(args.vg, -size*0.5f, size*0.8f); nvgStroke(args.vg); break; } // Crossed Keys
        case 45: { nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size); nvgLineTo(args.vg, 0, -size); nvgStroke(args.vg); for(int i=-1;i<=1;i++) { nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, i*size*0.5f, -size*0.3f); nvgStroke(args.vg);} break; } // Trident
        case 46: { nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size); nvgLineTo(args.vg, 0, -size*0.4f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.5f, 0); nvgLineTo(args.vg, 0, -size*0.4f); nvgLineTo(args.vg, size*0.5f, 0); nvgStroke(args.vg); break; } // Algiz
        case 47: { nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size); nvgLineTo(args.vg, 0, -size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.5f, -size*0.5f); nvgLineTo(args.vg, 0, -size); nvgLineTo(args.vg, size*0.5f, -size*0.5f); nvgStroke(args.vg); break; } // Tiwaz
        case 48: { nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.9f); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, size*0.9f, 0, size*0.12f); nvgFill(args.vg); break; } // Ouroboros small
        case 49: { nvgBeginPath(args.vg); for(int i=0;i<40;i++){ float t=i/39.f; float r2=t*size; float a=-M_PI/2 + t*4*M_PI; float x=r2*cosf(a), y=r2*sinf(a); if(i==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgStroke(args.vg); break; } // Double spiral
        case 50: { nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, size*0.866f, size*0.5f); nvgLineTo(args.vg, -size*0.866f, size*0.5f); nvgClosePath(args.vg); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.35f); nvgStroke(args.vg); break; } // Triangle+circle
        case 51: { nvgBeginPath(args.vg); nvgRect(args.vg, -size, -size, size*2, size*2); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, -size); nvgLineTo(args.vg, size, size); nvgMoveTo(args.vg, size, -size); nvgLineTo(args.vg, -size, size); nvgStroke(args.vg); break; } // Square+X
        case 52: { nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); for(int i=0;i<3;i++){ float a = -M_PI/2 + i*2*M_PI/3; nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0,0); nvgLineTo(args.vg, size*cosf(a), size*sinf(a)); nvgStroke(args.vg);} break; } // 3-spoke
        case 53: { nvgBeginPath(args.vg); nvgArc(args.vg, 0, 0, size, 0.2f*M_PI, 1.8f*M_PI, NVG_CW); nvgStroke(args.vg); nvgBeginPath(args.vg); for(int i=0;i<5;i++){ float ang=-M_PI/2 + i*2*M_PI/5; float r2=size*0.45f; float x=r2*cosf(ang), y=r2*sinf(ang); if(i==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgClosePath(args.vg); nvgStroke(args.vg); break; } // Crescent+star
        case 54: { nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); nvgBeginPath(args.vg); for(float t=0;t<2*M_PI;t+=0.1f){ float x=size*0.5f*sinf(t)/(1+cosf(t)*cosf(t)); float y=size*0.3f*sinf(t)*cosf(t)/(1+cosf(t)*cosf(t)); if(t==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgStroke(args.vg); break; } // Infinity circle
        case 55: { nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.5f); nvgStroke(args.vg); for(int i=0;i<12;i++){ float a=i*2*M_PI/12; nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0,0); nvgLineTo(args.vg, size*cosf(a), size*sinf(a)); nvgStroke(args.vg);} break; } // Sun rays
        case 56: { nvgBeginPath(args.vg); for(int i=0;i<6;i++){ float a=-M_PI/2 + i*2*M_PI/6; float x=size*cosf(a), y=size*sinf(a); if(i==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgClosePath(args.vg); nvgStroke(args.vg); break; } // Hexagon
        case 57: { nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, -size*0.2f); nvgLineTo(args.vg, size, -size*0.2f); nvgLineTo(args.vg, size*0.4f, size*0.6f); nvgLineTo(args.vg, -size*0.4f, size*0.6f); nvgClosePath(args.vg); nvgStroke(args.vg); break; } // Anvil
        case 58: { nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.7f, -size); nvgLineTo(args.vg, size*0.7f, -size); nvgLineTo(args.vg, -size*0.7f, size); nvgLineTo(args.vg, size*0.7f, size); nvgStroke(args.vg); nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.7f, -size); nvgLineTo(args.vg, size*0.7f, size); nvgMoveTo(args.vg, size*0.7f, -size); nvgLineTo(args.vg, -size*0.7f, size); nvgStroke(args.vg); break; } // Hourglass
        case 59: { for(int i=1;i<=4;i++){ nvgBeginPath(args.vg); nvgCircle(args.vg, 0,0, size*(i/4.0f)); nvgStroke(args.vg);} break; } // Labyrinth
        // --- New glyphs 60..79 ---
        case 60: { // Triple Moon (full with two crescents)
            float r = size * 0.55f;
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, r); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, -size*1.05f, 0, r, -M_PI/2, M_PI/2, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg,  size*1.05f, 0, r,  M_PI/2, -M_PI/2, NVG_CW); nvgStroke(args.vg);
            break;
        }
        case 61: { // Heptagram inside circle
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg);
            nvgBeginPath(args.vg);
            for (int i = 0; i < 7; ++i) {
                int j = (i * 3) % 7; // skip-3 star
                float ai = -M_PI/2 + i * 2*M_PI/7; float aj = -M_PI/2 + j * 2*M_PI/7;
                float xi = size * cosf(ai), yi = size * sinf(ai);
                float xj = size * cosf(aj), yj = size * sinf(aj);
                if (i == 0) nvgMoveTo(args.vg, xi, yi); nvgLineTo(args.vg, xj, yj);
            }
            nvgStroke(args.vg);
            break;
        }
        case 62: { // Triquetra (three interlaced arcs)
            float r = size * 0.9f;
            for (int k = 0; k < 3; ++k) {
                float a = k * 2.f * M_PI / 3.f - M_PI/2;
                nvgBeginPath(args.vg);
                nvgArc(args.vg, 0, 0, r*0.6f, a, a + M_PI, NVG_CW);
                nvgStroke(args.vg);
            }
            break;
        }
        case 63: { // Ankh
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size*0.45f, size*0.35f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.1f); nvgLineTo(args.vg, 0, size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.5f, 0); nvgLineTo(args.vg, size*0.5f, 0); nvgStroke(args.vg);
            break;
        }
        case 64: { // Crucible (Triangle with crossed top line)
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.2f); nvgLineTo(args.vg, -size*0.6f, size*0.7f); nvgLineTo(args.vg, size*0.6f, size*0.7f); nvgClosePath(args.vg); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.2f); nvgLineTo(args.vg, 0, -size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.4f, -size*0.6f); nvgLineTo(args.vg, size*0.4f, -size*0.6f); nvgStroke(args.vg);
            break;
        }
        case 65: { // Crescent over cross
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, -size*0.6f, size*0.5f, M_PI*0.1f, M_PI*0.9f, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.2f); nvgLineTo(args.vg, 0, size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.5f, 0); nvgLineTo(args.vg, size*0.5f, 0); nvgStroke(args.vg);
            break;
        }
        case 66: { // Triple cross on staff
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, 0, size); nvgStroke(args.vg);
            for (int i = -1; i <= 1; ++i) { float y = i * size*0.3f; nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.6f, y); nvgLineTo(args.vg, size*0.6f, y); nvgStroke(args.vg);} break;
        }
        case 67: { // Arrow in circle
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.7f, size*0.7f); nvgLineTo(args.vg, size*0.7f, -size*0.7f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.5f, -size*0.7f); nvgLineTo(args.vg, size*0.8f, -size*0.7f); nvgLineTo(args.vg, size*0.8f, -size*0.4f); nvgStroke(args.vg);
            break;
        }
        case 68: { // Eye
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, 0);
            // Use cubic with identical control points to emulate a quadratic arch
            nvgBezierTo(args.vg, 0, -size*0.7f, 0, -size*0.7f, size, 0); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, 0);
            nvgBezierTo(args.vg, 0,  size*0.7f, 0,  size*0.7f, size, 0); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.3f); nvgStroke(args.vg);
            break;
        }
        case 69: { // Crescent with dot
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, 0, size, M_PI*0.2f, M_PI*1.8f, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, size*0.4f, 0, size*0.12f); nvgFill(args.vg);
            break;
        }
        case 70: { // Crossed arrows
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.9f, size*0.9f); nvgLineTo(args.vg, size*0.9f, -size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.65f, -size*0.9f); nvgLineTo(args.vg, size*0.9f, -size*0.65f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.9f, -size*0.9f); nvgLineTo(args.vg, size*0.9f, size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.65f, size*0.9f); nvgLineTo(args.vg, size*0.9f, size*0.65f); nvgStroke(args.vg);
            break;
        }
        case 71: { // S-curve in circle
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg);
            for (int i=0;i<10;++i){ float t=i/9.f; float x=-size + 2*size*t; float y = sinf(t*M_PI*2) * size*0.4f; if(i==0) nvgBeginPath(args.vg), nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgStroke(args.vg);
            break;
        }
        case 72: { // Single spiral
            nvgBeginPath(args.vg); for (int i=0;i<64;++i){ float t=i/63.f; float r=t*size; float a=t*3.5f*M_PI - M_PI/2; float x=r*cosf(a), y=r*sinf(a); if(i==0) nvgMoveTo(args.vg,x,y); else nvgLineTo(args.vg,x,y);} nvgStroke(args.vg); break;
        }
        case 73: { // Labrys (double axe)
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.7f); nvgLineTo(args.vg, 0, size*0.7f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, -size*0.3f, 0, size*0.4f, -M_PI/2, M_PI/2, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg,  size*0.3f, 0, size*0.4f,  M_PI/2, -M_PI/2, NVG_CW); nvgStroke(args.vg);
            break;
        }
        case 74: { // Node (circle with two bars)
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.6f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, -size*0.5f); nvgLineTo(args.vg, size, -size*0.5f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size,  size*0.5f); nvgLineTo(args.vg, size,  size*0.5f); nvgStroke(args.vg);
            break;
        }
        case 75: { // Dagaz-like rune
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, -size); nvgLineTo(args.vg, 0, 0); nvgLineTo(args.vg, size, -size); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size, size); nvgLineTo(args.vg, 0, 0); nvgLineTo(args.vg, size, size); nvgStroke(args.vg);
            break;
        }
        case 76: { // Ehwaz/M shape
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.8f, size*0.8f); nvgLineTo(args.vg, -size*0.2f, -size*0.8f); nvgLineTo(args.vg, size*0.2f, size*0.8f); nvgLineTo(args.vg, size*0.8f, -size*0.8f); nvgStroke(args.vg);
            break;
        }
        case 77: { // Inguz (double diamonds)
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.8f, 0); nvgLineTo(args.vg, -size*0.4f, -size*0.6f); nvgLineTo(args.vg, 0, 0); nvgLineTo(args.vg, -size*0.4f, size*0.6f); nvgClosePath(args.vg); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg,  size*0.8f, 0); nvgLineTo(args.vg,  size*0.4f, -size*0.6f); nvgLineTo(args.vg, 0, 0); nvgLineTo(args.vg,  size*0.4f, size*0.6f); nvgClosePath(args.vg); nvgStroke(args.vg);
            break;
        }
        case 78: { // Triquetral knot (three small circles)
            float r = size*0.45f; float d = size*0.5f;
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -d, r); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, -d*0.866f, d*0.5f, r); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg,  d*0.866f,  d*0.5f, r); nvgStroke(args.vg);
            break;
        }
        case 79: { // Yin-Yang simplified
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, 0, size, -M_PI/2, M_PI/2, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, -size*0.5f, size*0.5f, 0, 2*M_PI, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, 0,  size*0.5f, size*0.5f, 0, 2*M_PI, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size*0.5f, size*0.12f); nvgFill(args.vg);
            break;
        }
        case 80: { // Bismuth
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, -size*0.3f, size*0.4f, M_PI*0.1f, M_PI*0.9f, NVG_CCW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, 0); nvgLineTo(args.vg, 0, size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.4f, size*0.6f); nvgLineTo(args.vg, size*0.4f, size*0.6f); nvgStroke(args.vg);
            break;
        }
        case 81: { // Zinc
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.6f, -size*0.6f); nvgLineTo(args.vg, size*0.6f, -size*0.6f); nvgLineTo(args.vg, -size*0.6f, size*0.6f); nvgLineTo(args.vg, size*0.6f, size*0.6f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size*0.2f); nvgLineTo(args.vg, 0, size*0.9f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.3f, size*0.7f); nvgLineTo(args.vg, size*0.3f, size*0.7f); nvgStroke(args.vg);
            break;
        }
        case 82: { // Glass (Vesica Piscis cross)
            nvgBeginPath(args.vg); nvgCircle(args.vg, -size*0.3f, 0, size*0.5f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, size*0.3f, 0, size*0.5f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.8f); nvgLineTo(args.vg, 0, size*0.8f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.8f, 0); nvgLineTo(args.vg, size*0.8f, 0); nvgStroke(args.vg);
            break;
        }
        case 83: { // Sublimation
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.8f, size*0.8f); nvgLineTo(args.vg, size*0.8f, size*0.8f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, 0, size*0.6f, M_PI*0.8f, M_PI*0.2f, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.48f, size*0.35f); nvgLineTo(args.vg, -size*0.48f, size*0.8f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.48f, size*0.35f); nvgLineTo(args.vg, size*0.48f, size*0.8f); nvgStroke(args.vg);
            break;
        }
        case 84: { // Precipitation
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.8f, -size*0.8f); nvgLineTo(args.vg, size*0.8f, -size*0.8f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, 0, size*0.6f, M_PI*1.2f, M_PI*1.8f, NVG_CCW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.48f, -size*0.35f); nvgLineTo(args.vg, -size*0.48f, -size*0.8f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.48f, -size*0.35f); nvgLineTo(args.vg, size*0.48f, -size*0.8f); nvgStroke(args.vg);
            break;
        }
        case 85: { // Putrefaction
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.6f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.9f, 0); nvgLineTo(args.vg, size*0.9f, 0); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size*0.8f, size*0.12f); nvgFill(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, size*0.8f, size*0.12f); nvgFill(args.vg);
            break;
        }
        case 86: { // Calcination
            nvgBeginPath(args.vg); nvgRect(args.vg, -size*0.4f, 0, size*0.8f, size*0.8f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, -size*0.2f, size*0.5f, M_PI*0.1f, M_PI*0.9f, NVG_CW); nvgStroke(args.vg);
            break;
        }
        case 87: { // Aqua Fortis
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.8f); nvgLineTo(args.vg, -size*0.7f, size*0.6f); nvgLineTo(args.vg, size*0.7f, size*0.6f); nvgClosePath(args.vg); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, 0); nvgLineTo(args.vg, 0, size*0.4f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.2f, size*0.2f); nvgLineTo(args.vg, size*0.2f, size*0.2f); nvgStroke(args.vg);
            break;
        }
        case 88: { // Wax
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.6f, -size*0.4f); nvgLineTo(args.vg, -size*0.3f, size*0.4f); nvgLineTo(args.vg, 0, -size*0.2f); nvgLineTo(args.vg, size*0.3f, size*0.4f); nvgLineTo(args.vg, size*0.6f, -size*0.4f); nvgStroke(args.vg);
            break;
        }
        case 89: { // Quicklime
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.8f); nvgLineTo(args.vg, -size*0.6f, 0); nvgLineTo(args.vg, 0, size*0.8f); nvgLineTo(args.vg, size*0.6f, 0); nvgClosePath(args.vg); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.4f, size*0.8f); nvgLineTo(args.vg, size*0.4f, size*0.8f); nvgStroke(args.vg);
            break;
        }
        case 90: { // Day
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.7f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size); nvgLineTo(args.vg, 0, size); nvgStroke(args.vg);
            break;
        }
        case 91: { // Night
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, 0, size*0.5f, M_PI*0.2f, M_PI*1.8f, NVG_CW); nvgStroke(args.vg);
            break;
        }
        case 92: { // Hour
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.5f, -size*0.7f); nvgLineTo(args.vg, -size*0.7f, 0); nvgLineTo(args.vg, size*0.5f, size*0.7f); nvgClosePath(args.vg); nvgStroke(args.vg);
            break;
        }
        case 93: { // Retort
            nvgBeginPath(args.vg); nvgCircle(args.vg, -size*0.2f, -size*0.2f, size*0.5f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.2f, -size*0.5f); nvgBezierTo(args.vg, size*0.8f, -size*0.5f, size*0.8f, size*0.6f, size*0.3f, size*0.8f); nvgStroke(args.vg);
            break;
        }
        case 94: { // Crucible variant
            nvgBeginPath(args.vg); nvgRect(args.vg, -size*0.6f, -size*0.6f, size*1.2f, size*1.2f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, -size*0.3f, -size*0.2f, size*0.1f); nvgFill(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, size*0.3f, -size*0.2f, size*0.1f); nvgFill(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, size*0.2f, size*0.1f); nvgFill(args.vg);
            break;
        }
        case 95: { // Ashes
            nvgBeginPath(args.vg); nvgRect(args.vg, -size*0.6f, -size*0.6f, size*1.2f, size*1.2f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.6f, -size*0.6f); nvgLineTo(args.vg, size*0.6f, size*0.6f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.6f, size*0.6f); nvgLineTo(args.vg, size*0.6f, -size*0.6f); nvgStroke(args.vg);
            break;
        }
        case 96: { // Purify
            nvgBeginPath(args.vg); nvgRect(args.vg, -size*0.8f, -size*0.8f, size*1.6f, size*1.6f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size*0.5f); nvgStroke(args.vg);
            break;
        }
        case 97: { // Boil
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, size*0.2f, size*0.6f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.4f, -size*0.5f); nvgBezierTo(args.vg, -size*0.2f, -size*0.8f, -size*0.2f, -size*0.2f, 0, -size*0.5f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, -size*0.5f); nvgBezierTo(args.vg, size*0.2f, -size*0.8f, size*0.2f, -size*0.2f, size*0.4f, -size*0.5f); nvgStroke(args.vg);
            break;
        }
        case 98: { // Filter
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.8f, -size*0.6f); nvgLineTo(args.vg, 0, size*0.6f); nvgLineTo(args.vg, size*0.8f, -size*0.6f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgCircle(args.vg, 0, -size*0.6f, size*0.3f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, 0, size*0.6f); nvgLineTo(args.vg, 0, size*0.9f); nvgStroke(args.vg);
            break;
        }
        case 99: { // Magnet
            nvgBeginPath(args.vg); nvgArc(args.vg, 0, -size*0.2f, size*0.6f, M_PI, 2*M_PI, NVG_CW); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, -size*0.6f, -size*0.2f); nvgLineTo(args.vg, -size*0.6f, size*0.4f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgMoveTo(args.vg, size*0.6f, -size*0.2f); nvgLineTo(args.vg, size*0.6f, size*0.4f); nvgStroke(args.vg);
            nvgBeginPath(args.vg); nvgRect(args.vg, -size*0.8f, size*0.6f, size*1.6f, size*0.3f); nvgStroke(args.vg);
            break;
        }
        default: nvgBeginPath(args.vg); nvgCircle(args.vg, 0, 0, size); nvgStroke(args.vg); break;
    }
    nvgRestore(args.vg);
}

bool isValidSymbolId(int symbolId) {
    return symbolId >= 0 && symbolId < 100;
}

void drawVignettePatinaScratches(const widget::Widget::DrawArgs& args,
                                float x, float y, float w, float h,
                                float cornerRadius, int scratchCount,
                                NVGcolor vignette1, NVGcolor vignette2,
                                int patinaLayers, float scratchAlpha,
                                int scratchVariations, unsigned int seed) {
    nvgSave(args.vg);
    
    // Draw vignette effect (darkening at edges)
    NVGpaint vignette = nvgRadialGradient(args.vg, x + w/2, y + h/2, 
                                         std::min(w, h) * 0.3f, std::min(w, h) * 0.7f,
                                         vignette1, vignette2);
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, x, y, w, h, cornerRadius);
    nvgFillPaint(args.vg, vignette);
    nvgFill(args.vg);
    
    // Add some simple scratch effects
    if (scratchCount > 0) {
        nvgStrokeColor(args.vg, nvgRGBAf(0.3f, 0.3f, 0.25f, scratchAlpha));
        nvgStrokeWidth(args.vg, 0.5f);
        
        // Simple random scratches - using seed for reproducibility
        srand(seed);
        for (int i = 0; i < scratchCount; i++) {
            float x1 = x + (rand() % (int)w);
            float y1 = y + (rand() % (int)h);
            float x2 = x1 + (rand() % 20 - 10);
            float y2 = y1 + (rand() % 20 - 10);
            
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x1, y1);
            nvgLineTo(args.vg, x2, y2);
            nvgStroke(args.vg);
        }
    }
    
    nvgRestore(args.vg);
}

void drawMicroScratches(const widget::Widget::DrawArgs& args,
                        float x, float y, float w, float h,
                        int scratchCount, unsigned int seed,
                        float intensity) {
    if (scratchCount <= 0) return;
    float strength = clamp(intensity, 0.0f, 2.0f);
    if (strength <= 0.0f) return;

    auto rnd = [&]() {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return (seed & 0xFFFF) / 65535.f;
    };

    auto signedRnd = [&]() {
        return rnd() * 2.0f - 1.0f;
    };

    auto drawScratch = [&](float x1, float y1, float x2, float y2, float bow, float width, float alpha) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) return;
        float ox = -dy / len;
        float oy = dx / len;
        float offset = 0.35f;
        float mx = (x1 + x2) * 0.5f + ox * bow;
        float my = (y1 + y2) * 0.5f + oy * bow;

        int shadowA = static_cast<int>(rack::math::clamp(alpha * 0.86f, 0.0f, 34.0f));
        int lightA = static_cast<int>(rack::math::clamp(alpha * 0.72f, 0.0f, 28.0f));
        int warmA = static_cast<int>(rack::math::clamp(alpha * 0.48f, 0.0f, 18.0f));

        nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, shadowA));
        nvgStrokeWidth(args.vg, width);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, x1 + ox * offset, y1 + oy * offset);
        nvgQuadTo(args.vg, mx + ox * offset, my + oy * offset, x2 + ox * offset, y2 + oy * offset);
        nvgStroke(args.vg);

        nvgStrokeColor(args.vg, nvgRGBA(214, 202, 178, lightA));
        nvgStrokeWidth(args.vg, width * 0.7f);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, x1 - ox * offset, y1 - oy * offset);
        nvgQuadTo(args.vg, mx - ox * offset, my - oy * offset, x2 - ox * offset, y2 - oy * offset);
        nvgStroke(args.vg);

        if (rnd() > 0.68f) {
            nvgStrokeColor(args.vg, nvgRGBA(92, 72, 48, warmA));
            nvgStrokeWidth(args.vg, width * 0.45f);
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x1, y1);
            nvgQuadTo(args.vg, mx, my, x2, y2);
            nvgStroke(args.vg);
        }
    };

    nvgSave(args.vg);
    nvgIntersectScissor(args.vg, x, y, w, h);

    float minDim = std::min(w, h);
    auto distributedPoint = [&](int index, int total, float marginX, float marginY) -> Vec {
        total = std::max(1, total);
        float usableW = w * (1.0f - marginX * 2.0f);
        float usableH = h * (1.0f - marginY * 2.0f);
        float aspect = usableW / std::max(1.0f, usableH);
        int cols = std::max(1, static_cast<int>(std::ceil(std::sqrt(total * aspect))));
        int rows = std::max(1, static_cast<int>(std::ceil(total / static_cast<float>(cols))));
        int slotCount = std::max(1, cols * rows);
        int slot = index % slotCount;
        int cellX = slot % cols;
        int cellY = slot / cols;
        float jx = 0.18f + rnd() * 0.64f;
        float jy = 0.18f + rnd() * 0.64f;
        return Vec(
            x + w * marginX + ((cellX + jx) / cols) * usableW,
            y + h * marginY + ((cellY + jy) / rows) * usableH
        );
    };

    int scuffCount = std::max(1, scratchCount / 5);
    for (int i = 0; i < scuffCount; ++i) {
        Vec p = distributedPoint(i, scuffCount, 0.12f, 0.10f);
        float cx = p.x;
        float cy = p.y;
        float rw = (0.028f + rnd() * 0.045f) * minDim * strength;
        float rh = rw * (0.28f + rnd() * 0.32f);
        float rot = signedRnd() * 0.55f;
        int scuffAlpha = static_cast<int>(rack::math::clamp((1.5f + rnd() * 2.0f) * strength, 0.0f, 7.0f));

        nvgSave(args.vg);
        nvgTranslate(args.vg, cx, cy);
        nvgRotate(args.vg, rot);
        NVGpaint haze = nvgRadialGradient(args.vg, 0.0f, 0.0f, rh * 0.15f, rw,
            nvgRGBA(150, 132, 104, scuffAlpha), nvgRGBA(150, 132, 104, 0));
        nvgBeginPath(args.vg);
        nvgEllipse(args.vg, 0.0f, 0.0f, rw, rh);
        nvgFillPaint(args.vg, haze);
        nvgFill(args.vg);

        int rubs = 2 + static_cast<int>(rnd() * 3.0f);
        for (int r = 0; r < rubs; ++r) {
            float yy = signedRnd() * rh * 0.55f;
            float half = rw * (0.34f + rnd() * 0.32f);
            nvgStrokeColor(args.vg, nvgRGBA(186, 170, 142, static_cast<int>(scuffAlpha * 0.75f)));
            nvgStrokeWidth(args.vg, 0.28f + rnd() * 0.28f);
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, -half, yy);
            nvgQuadTo(args.vg, 0.0f, yy + signedRnd() * rh * 0.25f, half, yy + signedRnd() * rh * 0.18f);
            nvgStroke(args.vg);
        }
        nvgRestore(args.vg);
    }

    for (int i = 0; i < scratchCount; ++i) {
        bool longHairline = rnd() > 0.72f;
        float length = (longHairline ? (0.10f + rnd() * 0.15f) : (0.025f + rnd() * 0.075f)) * minDim;
        float angle = signedRnd() * 0.42f;
        if (rnd() > 0.82f)
            angle += (rnd() > 0.5f ? 1.0f : -1.0f) * (0.55f + rnd() * 0.65f);
        Vec p = distributedPoint(i, scratchCount, 0.04f, 0.05f);
        float cx = p.x;
        float cy = p.y;
        float dx = std::cos(angle) * length * 0.5f;
        float dy = std::sin(angle) * length * 0.5f;
        float width = (0.22f + rnd() * 0.44f) * (0.72f + 0.24f * strength);
        float alpha = (6.0f + rnd() * 9.0f) * strength;
        float bow = signedRnd() * length * 0.045f;
        drawScratch(cx - dx, cy - dy, cx + dx, cy + dy, bow, width, alpha);

        if (rnd() > 0.70f) {
            float offset = (1.0f + rnd() * 2.8f) * (rnd() > 0.5f ? 1.0f : -1.0f);
            drawScratch(cx - dx * 0.55f, cy - dy * 0.55f + offset, cx + dx * 0.55f, cy + dy * 0.55f + offset,
                bow * 0.7f, width * 0.65f, alpha * 0.58f);
        }
    }

    int nickCount = std::max(2, scratchCount / 4);
    for (int i = 0; i < nickCount; ++i) {
        Vec p = distributedPoint(i, nickCount, 0.03f, 0.04f);
        float cx = p.x;
        float cy = p.y;
        float r = (0.35f + rnd() * 0.9f) * strength;
        int a = static_cast<int>(rack::math::clamp((2.0f + rnd() * 4.0f) * strength, 0.0f, 10.0f));
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx + 0.25f, cy + 0.35f, r * 0.95f);
        nvgFillColor(args.vg, nvgRGBA(0, 0, 0, static_cast<int>(a * 0.35f)));
        nvgFill(args.vg);
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, r * 0.62f);
        nvgFillColor(args.vg, nvgRGBA(170, 154, 126, a));
        nvgFill(args.vg);
    }

    nvgRestore(args.vg);
}

void drawVintageText(const widget::Widget::DrawArgs& args, Vec pos, 
                    const std::string& text, NVGcolor color, float fontSize,
                    bool addGlow, float glowRadius) {
    nvgSave(args.vg);
    nvgFontSize(args.vg, fontSize);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    
    if (addGlow) {
        // Draw glow effect
        nvgFontBlur(args.vg, glowRadius);
        nvgFillColor(args.vg, nvgRGBAf(color.r, color.g, color.b, 0.3f));
        nvgText(args.vg, pos.x, pos.y, text.c_str(), nullptr);
        nvgFontBlur(args.vg, 0);
    }
    
    // Draw main text
    nvgFillColor(args.vg, color);
    nvgText(args.vg, pos.x, pos.y, text.c_str(), nullptr);
    nvgRestore(args.vg);
}

void drawScanlines(const widget::Widget::DrawArgs& args, float x, float y, 
                  float w, float h, float lineSpacing, float alpha) {
    nvgSave(args.vg);
    nvgStrokeColor(args.vg, nvgRGBAf(0, 0, 0, alpha));
    nvgStrokeWidth(args.vg, 1.0f);
    
    for (float yPos = y; yPos < y + h; yPos += lineSpacing) {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, x, yPos);
        nvgLineTo(args.vg, x + w, yPos);
        nvgStroke(args.vg);
    }
    nvgRestore(args.vg);
}

void drawPhosphorGlow(const widget::Widget::DrawArgs& args, Vec center, 
                     float radius, NVGcolor color, float intensity) {
    nvgSave(args.vg);
    
    // Create radial gradient for phosphor glow
    NVGpaint glow = nvgRadialGradient(args.vg, center.x, center.y, 
                                     radius * 0.3f, radius,
                                     nvgRGBAf(color.r, color.g, color.b, intensity),
                                     nvgRGBAf(color.r, color.g, color.b, 0));
    
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, center.x, center.y, radius);
    nvgFillPaint(args.vg, glow);
    nvgFill(args.vg);
    nvgRestore(args.vg);
}

void drawShadowMask(const widget::Widget::DrawArgs& args, float x, float y,
                   float w, float h, float triadWidth, float alpha) {
    if (triadWidth <= 0.0f || alpha <= 0.0f) return;
    nvgSave(args.vg);
    nvgScissor(args.vg, x, y, w, h);
    nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
    float bandW = triadWidth / 3.0f;
    for (float cx = x; cx < x + w; cx += triadWidth) {
        // Red
        nvgBeginPath(args.vg);
        nvgRect(args.vg, cx, y, bandW, h);
        nvgFillColor(args.vg, nvgRGBAf(1.f, 0.f, 0.f, alpha * 0.20f));
        nvgFill(args.vg);
        // Green
        nvgBeginPath(args.vg);
        nvgRect(args.vg, cx + bandW, y, bandW, h);
        nvgFillColor(args.vg, nvgRGBAf(0.f, 1.f, 0.f, alpha * 0.20f));
        nvgFill(args.vg);
        // Blue
        nvgBeginPath(args.vg);
        nvgRect(args.vg, cx + 2.f * bandW, y, bandW, h);
        nvgFillColor(args.vg, nvgRGBAf(0.f, 0.f, 1.f, alpha * 0.20f));
        nvgFill(args.vg);
    }
    nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
    nvgResetScissor(args.vg);
    nvgRestore(args.vg);
}

void drawGlassReflections(const widget::Widget::DrawArgs& args, float x, float y,
                         float w, float h, float intensity) {
    if (intensity <= 0.f) return;
    nvgSave(args.vg);
    // Diagonal sweep from top-left to center
    NVGpaint diag = nvgLinearGradient(args.vg,
        x + w * 0.05f, y + h * 0.05f,
        x + w * 0.55f, y + h * 0.45f,
        nvgRGBA(255, 255, 255, (int)(intensity * 255 * 0.65f)), nvgRGBA(255, 255, 255, 0));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, x, y, w, h);
    nvgFillPaint(args.vg, diag);
    nvgFill(args.vg);

    // Bottom-right curved crescent highlight
    NVGpaint bottom = nvgRadialGradient(args.vg,
        x + w * 0.82f, y + h * 0.85f,
        h * 0.02f, h * 0.38f,
        nvgRGBA(255, 255, 255, (int)(intensity * 255 * 0.35f)), nvgRGBA(255, 255, 255, 0));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, x, y, w, h);
    nvgFillPaint(args.vg, bottom);
    nvgFill(args.vg);

    // Small specular dot near top-left
    NVGpaint dot = nvgRadialGradient(args.vg,
        x + w * 0.20f, y + h * 0.18f,
        h * 0.0f, h * 0.10f,
        nvgRGBA(255, 255, 255, (int)(intensity * 255 * 0.50f)), nvgRGBA(255, 255, 255, 0));
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, x + w * 0.20f, y + h * 0.18f, h * 0.12f);
    nvgFillPaint(args.vg, dot);
    nvgFill(args.vg);

    nvgRestore(args.vg);
}

std::vector<std::string> wrapText(const std::string& text, float maxWidth, NVGcontext* vg) {
    std::vector<std::string> lines;
    if (text.empty()) return lines;
    
    // Check if text fits in single line
    float textWidth = nvgTextBounds(vg, 0, 0, text.c_str(), NULL, NULL);
    if (textWidth <= maxWidth) {
        lines.push_back(text);
        return lines;
    }
    
    // Split into words
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;
    while (std::getline(ss, word, ' ')) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    
    if (words.empty()) return lines;
    
    // Build lines by adding words until maxWidth is exceeded
    std::string currentLine = words[0];
    
    for (size_t i = 1; i < words.size(); ++i) {
        std::string testLine = currentLine + " " + words[i];
        float testWidth = nvgTextBounds(vg, 0, 0, testLine.c_str(), NULL, NULL);
        
        if (testWidth <= maxWidth) {
            currentLine = testLine;
        } else {
            // Line is full, start new line
            lines.push_back(currentLine);
            currentLine = words[i];
        }
    }
    
    // Add final line
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines;
}

}} // namespace shapetaker::graphics
