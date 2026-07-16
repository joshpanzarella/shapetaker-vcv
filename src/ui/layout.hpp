#pragma once

#include <rack.hpp>
#include <string>
#include <fstream>
#include <sstream>

using namespace rack;

namespace shapetaker {
namespace ui {

/**
 * Layout and positioning utilities for consistent module design
 */
class LayoutHelper {
public:
    // Standard HP sizes and conversions
    static constexpr float HP_TO_MM = 5.08f;
    static constexpr float MM_TO_PX = 15.0f / 3.5f; // VCV Rack standard
    
    /**
     * Convert HP to pixels
     */
    static float hp2px(float hp) {
        return hp * HP_TO_MM * MM_TO_PX;
    }
    
    /**
     * Convert millimeters to pixels (wrapper for VCV's mm2px)
     */
    static Vec mm2px(Vec pos) {
        return rack::mm2px(pos);
    }
    
    /**
     * Standard module widths in HP
     */
    enum ModuleWidth {
        WIDTH_4HP = 4,
        WIDTH_6HP = 6,
        WIDTH_8HP = 8,
        WIDTH_10HP = 10,
        WIDTH_12HP = 12,
        WIDTH_14HP = 14,
        WIDTH_16HP = 16,
        WIDTH_18HP = 18,  // Torsion
        WIDTH_20HP = 20,
        WIDTH_26HP = 26,  // Transmutation
        WIDTH_28HP = 28,
        WIDTH_32HP = 32,
        WIDTH_42HP = 42
    };

    /**
     * Lightweight SVG panel parser to position controls by element id
     * Usage:
     *   PanelSVGParser p(asset::plugin(pluginInstance, "res/panels/YourPanel.svg"));
     *   Vec knobPos = p.centerPx("knob_id", 10.0f, 25.0f); // mm defaults
     *   auto r = p.rectMm("screen_id", 20.0f, 40.0f, 80.0f, 80.0f); // mm rect
     */
    class PanelSVGParser {
    private:
        std::string svg;

        static std::string readFile(const std::string& path) {
            std::ifstream f(path);
            if (!f) return {};
            std::stringstream ss; ss << f.rdbuf();
            return ss.str();
        }

        // Find the full tag string that contains id="..."
        size_t findPositionForIdInternal(const std::string& id) const {
            if (svg.empty()) return std::string::npos;
            std::string needle = std::string("id=\"") + id + "\"";
            return svg.find(needle);
        }

        std::string findTagForIdInternal(const std::string& id) const {
            size_t pos = findPositionForIdInternal(id);
            if (pos == std::string::npos) return {};
            size_t start = svg.rfind('<', pos);
            size_t end = svg.find('>', pos);
            if (start == std::string::npos || end == std::string::npos || end <= start) return {};
            return svg.substr(start, end - start + 1);
        }

        static float getAttrInternal(const std::string& tag, const std::string& key, float defVal) {
            if (tag.empty()) return defVal;
            std::string k = key + "=\"";
            size_t p = tag.find(k);
            if (p == std::string::npos) return defVal;
            p += k.size();
            size_t q = tag.find('"', p);
            if (q == std::string::npos) return defVal;
            try {
                return std::stof(tag.substr(p, q - p));
            } catch (...) { return defVal; }
        }

        static Vec getTransformOffsetInternal(const std::string& tag) {
            std::string transform = getAttrStr(tag, "transform", "");
            if (transform.empty())
                return Vec(0.f, 0.f);

            size_t p = transform.find("translate(");
            if (p != std::string::npos) {
                p += 10;
                size_t q = transform.find(')', p);
                if (q != std::string::npos) {
                    std::string body = transform.substr(p, q - p);
                    for (char& c : body) {
                        if (c == ',')
                            c = ' ';
                    }
                    std::stringstream ss(body);
                    float x = 0.f;
                    float y = 0.f;
                    ss >> x;
                    if (!(ss >> y))
                        y = 0.f;
                    return Vec(x, y);
                }
            }

            p = transform.find("matrix(");
            if (p != std::string::npos) {
                p += 7;
                size_t q = transform.find(')', p);
                if (q != std::string::npos) {
                    std::string body = transform.substr(p, q - p);
                    for (char& c : body) {
                        if (c == ',')
                            c = ' ';
                    }
                    std::stringstream ss(body);
                    float a = 1.f, b = 0.f, c = 0.f, d = 1.f, e = 0.f, f = 0.f;
                    ss >> a >> b >> c >> d >> e >> f;
                    return Vec(e, f);
                }
            }

            return Vec(0.f, 0.f);
        }

        Vec getAncestorTransformOffsetInternal(size_t idPos) const {
            if (idPos == std::string::npos)
                return Vec(0.f, 0.f);

            Vec offset(0.f, 0.f);
            size_t p = 0;
            while (true) {
                size_t gStart = svg.find("<g", p);
                if (gStart == std::string::npos || gStart >= idPos)
                    break;

                size_t tagEnd = svg.find('>', gStart);
                if (tagEnd == std::string::npos || tagEnd >= idPos)
                    break;

                size_t close = svg.find("</g>", tagEnd);
                if (close == std::string::npos || close > idPos) {
                    std::string tag = svg.substr(gStart, tagEnd - gStart + 1);
                    offset = offset.plus(getTransformOffsetInternal(tag));
                }

                p = tagEnd + 1;
            }
            return offset;
        }

    public:
        explicit PanelSVGParser(const std::string& svgPath) : svg(readFile(svgPath)) {}

        // Low-level helpers
        std::string findTagForId(const std::string& id) const { return findTagForIdInternal(id); }
        static float getAttr(const std::string& tag, const std::string& key, float defVal) { return getAttrInternal(tag, key, defVal); }
        static std::string getAttrStr(const std::string& tag, const std::string& key, const std::string& defVal = {}) {
            if (tag.empty()) return defVal;
            std::string needle = key + "=\"";
            size_t p = tag.find(needle);
            if (p == std::string::npos) return defVal;
            p += needle.size();
            size_t q = tag.find('"', p);
            if (q == std::string::npos) return defVal;
            return tag.substr(p, q - p);
        }

        // Get element center in millimeters (from circle cx/cy or rect x+width/2, y+height/2)
        Vec centerMm(const std::string& id, float defx, float defy) const {
            size_t idPos = findPositionForIdInternal(id);
            std::string tag = findTagForIdInternal(id);
            Vec offset = getAncestorTransformOffsetInternal(idPos).plus(getTransformOffsetInternal(tag));
            if (tag.find("<rect") != std::string::npos) {
                float rx = getAttrInternal(tag, "x", defx);
                float ry = getAttrInternal(tag, "y", defy);
                float rw = getAttrInternal(tag, "width", 0.0f);
                float rh = getAttrInternal(tag, "height", 0.0f);
                return Vec(rx + rw * 0.5f, ry + rh * 0.5f).plus(offset);
            }
            float cx = getAttrInternal(tag, "cx", defx);
            float cy = getAttrInternal(tag, "cy", defy);
            return Vec(cx, cy).plus(offset);
        }

        // Get element center in pixels (mm2px converted)
        Vec centerPx(const std::string& id, float defx, float defy) const {
            return mm2px(centerMm(id, defx, defy));
        }

        // Get element rect in millimeters
        Rect rectMm(const std::string& id, float defx, float defy, float defw, float defh) const {
            size_t idPos = findPositionForIdInternal(id);
            std::string tag = findTagForIdInternal(id);
            Vec offset = getAncestorTransformOffsetInternal(idPos).plus(getTransformOffsetInternal(tag));
            float x = getAttrInternal(tag, "x", defx);
            float y = getAttrInternal(tag, "y", defy);
            float w = getAttrInternal(tag, "width", defw);
            float h = getAttrInternal(tag, "height", defh);
            return Rect(Vec(x, y).plus(offset), Vec(w, h));
        }

        // Convenience static helpers (one-off)
        static Vec centerPxFromFile(const std::string& svgPath, const std::string& id, float defx, float defy) {
            PanelSVGParser p(svgPath);
            return p.centerPx(id, defx, defy);
        }
        static Rect rectMmFromFile(const std::string& svgPath, const std::string& id, float defx, float defy, float defw, float defh) {
            PanelSVGParser p(svgPath);
            return p.rectMm(id, defx, defy, defw, defh);
        }
    };

    /**
     * Create a parser and return a lambda for convenient widget positioning.
     * This encapsulates the common pattern used across all modules.
     *
     * Usage in ModuleWidget constructor:
     *   auto centerPx = LayoutHelper::createCenterPxHelper(
     *       asset::plugin(pluginInstance, "res/panels/YourPanel.svg")
     *   );
     *   addParam(createParamCentered<KnobType>(
     *       centerPx("knob_id", 10.0f, 25.0f),
     *       module,
     *       PARAM_ID
     *   ));
     */
    static std::function<Vec(const std::string&, float, float)>
    createCenterPxHelper(const std::string& svgPath) {
        // Create parser on the heap so it persists for the lambda's lifetime
        auto parser = std::make_shared<PanelSVGParser>(svgPath);
        return [parser](const std::string& id, float defx, float defy) -> Vec {
            return parser->centerPx(id, defx, defy);
        };
    }

    /**
     * Simplified version using a pre-constructed parser (avoids re-reading file).
     * Use this when you need the parser for other operations too.
     *
     * Usage:
     *   PanelSVGParser parser(asset::plugin(...));
     *   auto centerPx = LayoutHelper::createCenterPxHelper(parser);
     */
    static std::function<Vec(const std::string&, float, float)>
    createCenterPxHelper(const PanelSVGParser& parser) {
        return [&parser](const std::string& id, float defx, float defy) -> Vec {
            return parser.centerPx(id, defx, defy);
        };
    }

    /**
     * Legacy pattern support - creates parser and lambda in widget scope.
     * This is what modules currently use inline. Kept for reference.
     *
     * Preferred: Use createCenterPxHelper() instead.
     */
    struct WidgetPositioner {
        PanelSVGParser parser;

        explicit WidgetPositioner(const std::string& svgPath) : parser(svgPath) {}

        Vec centerPx(const std::string& id, float defx, float defy) const {
            return parser.centerPx(id, defx, defy);
        }

        // Create lambda for compatibility with existing code
        std::function<Vec(const std::string&, float, float)> getLambda() {
            return [this](const std::string& id, float defx, float defy) -> Vec {
                return this->centerPx(id, defx, defy);
            };
        }
    };
    
    /**
     * Get module width in pixels
     */
    static float getModuleWidth(ModuleWidth width) {
        return hp2px(static_cast<float>(width));
    }
    
    /**
     * Standard spacing measurements
     */
    struct Spacing {
        static constexpr float TIGHT = 2.0f;      // Tight spacing in mm
        static constexpr float NORMAL = 5.0f;     // Normal spacing in mm
        static constexpr float WIDE = 8.0f;       // Wide spacing in mm
        static constexpr float SECTION = 12.0f;   // Section separation in mm
    };
    
    /**
     * Standard screw positions for different module widths
     */
    class ScrewPositions {
    public:
        static constexpr float EDGE_INSET = 1.5f;
        static constexpr float DEFAULT_SCREW_SIZE = 17.f;

        static Vec topLeft() {
            return Vec(RACK_GRID_WIDTH, EDGE_INSET);
        }
        
        static Vec topRight(float moduleWidth) {
            return Vec(moduleWidth - RACK_GRID_WIDTH - DEFAULT_SCREW_SIZE, EDGE_INSET);
        }
        
        static Vec bottomLeft() {
            return Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - DEFAULT_SCREW_SIZE - EDGE_INSET);
        }
        
        static Vec bottomRight(float moduleWidth) {
            return Vec(moduleWidth - RACK_GRID_WIDTH - DEFAULT_SCREW_SIZE, RACK_GRID_HEIGHT - DEFAULT_SCREW_SIZE - EDGE_INSET);
        }
        
        // Add all standard screws to a module widget
        template <typename ScrewWidget = ScrewSilver>
        static void addStandardScrews(ModuleWidget* widget, float moduleWidth) {
            auto addScrew = [&](bool right, bool bottom) {
                auto* screw = createWidget<ScrewWidget>(Vec(0.f, 0.f));
                const float x = right ? (moduleWidth - RACK_GRID_WIDTH - screw->box.size.x) : RACK_GRID_WIDTH;
                const float y = bottom ? (RACK_GRID_HEIGHT - screw->box.size.y - EDGE_INSET) : EDGE_INSET;
                screw->box.pos = Vec(x, y);
                widget->addChild(screw);
            };

            addScrew(false, false);
            addScrew(true, false);
            addScrew(false, true);
            addScrew(true, true);
        }
    };
    
    /**
     * Grid-based layout helper for consistent positioning
     */
    class GridLayout {
    private:
        float moduleWidth;
        float startY;
        float columnWidth;

    public:
        GridLayout(float modWidth, float topMargin = 20.0f, int cols = 3) 
            : moduleWidth(modWidth), startY(topMargin) {
            columnWidth = (modWidth - 20.0f) / cols; // 10mm margins
        }
        
        /**
         * Get position for grid coordinate
         * @param col Column (0-based)
         * @param row Row (0-based) 
         * @param rowHeight Height per row in mm
         */
        Vec getPosition(int col, int row, float rowHeight = 15.0f) {
            float x = 10.0f + (col + 0.5f) * columnWidth; // Center in column
            float y = startY + row * rowHeight;
            return mm2px(Vec(x, y));
        }
        
        /**
         * Get centered position for single column layout
         */
        Vec getCenteredPosition(int row, float rowHeight = 15.0f) {
            float x = moduleWidth * 0.5f / MM_TO_PX; // Center of module in mm
            float y = startY + row * rowHeight;
            return mm2px(Vec(x, y));
        }
    };
    
    /**
     * Vertical column layout for parameters
     */
    class ColumnLayout {
    private:
        float x;
        float startY;
        float spacing;
        
    public:
        ColumnLayout(float columnX, float topMargin = 20.0f, float itemSpacing = Spacing::NORMAL) 
            : x(columnX), startY(topMargin), spacing(itemSpacing) {}
        
        Vec getPosition(int index) {
            return mm2px(Vec(x, startY + index * spacing));
        }
    };
    
    /**
     * Horizontal row layout for parameters
     */
    class RowLayout {
    private:
        float y;
        float startX;
        float spacing;
        
    public:
        RowLayout(float rowY, float leftMargin = 10.0f, float itemSpacing = Spacing::NORMAL)
            : y(rowY), startX(leftMargin), spacing(itemSpacing) {}
        
        Vec getPosition(int index) {
            return mm2px(Vec(startX + index * spacing, y));
        }
    };
    
    /**
     * I/O panel layout helper for audio/CV connections
     */
    class IOPanelLayout {
    public:
        /**
         * Standard I/O panel at bottom of module
         */
        static Vec getBottomIOPosition(float moduleWidth, int index, int totalCount, float bottomMargin = 15.0f) {
            float spacing = (moduleWidth - 20.0f) / (totalCount + 1); // 10mm margins
            float x = 10.0f + (index + 1) * spacing;
            float y = RACK_GRID_HEIGHT / MM_TO_PX - bottomMargin;
            return mm2px(Vec(x, y));
        }
        
        /**
         * Side panel I/O (left or right edge)
         */
        static Vec getSideIOPosition(bool isLeft, int index, float sideMargin = 5.0f, float topMargin = 30.0f, float spacing = Spacing::NORMAL) {
            float x = isLeft ? sideMargin : (RACK_GRID_WIDTH / MM_TO_PX - sideMargin);
            float y = topMargin + index * spacing;
            return mm2px(Vec(x, y));
        }
    };
    
    /**
     * Control grouping helpers
     */
    class ControlGroup {
    public:
        /**
         * Knob with CV input pair
         */
        struct KnobCVPair {
            Vec knobPos;
            Vec cvPos;
            
            KnobCVPair(Vec center, float separation = 8.0f) {
                knobPos = mm2px(Vec(center.x - separation * 0.5f, center.y));
                cvPos = mm2px(Vec(center.x + separation * 0.5f, center.y));
            }
        };
        
        /**
         * Parameter with attenuverter pair
         */
        struct ParamAttenuverterPair {
            Vec paramPos;
            Vec attenuPos;
            
            ParamAttenuverterPair(Vec center, float separation = 12.0f) {
                paramPos = mm2px(Vec(center.x - separation * 0.5f, center.y));
                attenuPos = mm2px(Vec(center.x + separation * 0.5f, center.y));
            }
        };
        
        /**
         * Input/Output pair (stereo)
         */
        struct StereoPair {
            Vec leftPos;
            Vec rightPos;
            
            StereoPair(Vec center, float separation = 10.0f) {
                leftPos = mm2px(Vec(center.x - separation * 0.5f, center.y));
                rightPos = mm2px(Vec(center.x + separation * 0.5f, center.y));
            }
        };
    };
};

/**
 * Common layout patterns for Shapetaker modules
 */
namespace Layouts {
    /**
     * Standard dual-channel layout (like Chiaroscuro)
     */
    class DualChannel {
    public:
        static constexpr float MODULE_WIDTH = 42.0f; // mm
        static constexpr float CHANNEL_SPACING = 18.0f; // mm between channels
        
        static Vec getChannelCenter(int channel) { // 0 = left, 1 = right
            float centerX = MODULE_WIDTH * 0.5f + (channel - 0.5f) * CHANNEL_SPACING;
            return Vec(centerX, 0); // Y will be added by specific layouts
        }
        
        static LayoutHelper::GridLayout createGrid() {
            return LayoutHelper::GridLayout(MODULE_WIDTH, 25.0f, 2); // 2 columns
        }
    };
    
    /**
     * Single channel layout (like Fatebinder)
     */
    class SingleChannel {
    public:
        static constexpr float MODULE_WIDTH = 20.0f; // mm
        
        static LayoutHelper::ColumnLayout createMainColumn() {
            return LayoutHelper::ColumnLayout(MODULE_WIDTH * 0.5f, 25.0f);
        }
    };
    
    /**
     * Sequencer layout (like Transmutation)
     */
    class SequencerLayout {
    public:
        static constexpr float MODULE_WIDTH = 131.318f; // mm (26HP)
        static constexpr float MATRIX_CENTER_X = MODULE_WIDTH * 0.5f;
        static constexpr float MATRIX_CENTER_Y = 65.0f; // mm from top
        
        static Vec getMatrixCenter() {
            return Vec(MATRIX_CENTER_X, MATRIX_CENTER_Y);
        }
        
        static LayoutHelper::RowLayout createTopRow() {
            return LayoutHelper::RowLayout(15.0f, 15.0f, 15.0f);
        }
        
        static LayoutHelper::RowLayout createBottomRow() {
            return LayoutHelper::RowLayout(115.0f, 15.0f, 15.0f);
        }
    };
}

} // namespace ui
} // namespace shapetaker
