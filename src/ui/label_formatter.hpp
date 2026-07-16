#pragma once

#include <rack.hpp>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

namespace shapetaker {
namespace ui {

class LabelFormatter {
private:
    static std::string stripParenthetical(const std::string& value) {
        std::string out;
        int depth = 0;
        for (char c : value) {
            if (c == '(') {
                depth++;
                continue;
            }
            if (c == ')') {
                if (depth > 0)
                    depth--;
                continue;
            }
            if (depth == 0)
                out.push_back(c);
        }
        return out;
    }

    static std::string collapseWhitespace(const std::string& value) {
        std::string out;
        bool inSpace = false;
        for (char c : value) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!inSpace) {
                    out.push_back(' ');
                    inSpace = true;
                }
            } else {
                out.push_back(c);
                inSpace = false;
            }
        }
        // Trim
        while (!out.empty() && out.front() == ' ')
            out.erase(out.begin());
        while (!out.empty() && out.back() == ' ')
            out.pop_back();
        return out;
    }

    static std::string baseClean(const std::string& label) {
        if (label.empty())
            return "";
        std::string s = stripParenthetical(label);
        // Convert hyphen/underscore to space for easier tokenization
        for (char& c : s) {
            if (c == '-' || c == '_')
                c = ' ';
        }
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
            return std::tolower(ch);
        });
        // Replace colon with space so we preserve the prefix
        std::replace(s.begin(), s.end(), ':', ' ');
        s = collapseWhitespace(s);
        return s;
    }

    static std::string replaceVPerOct(const std::string& label) {
        std::string s = label;
        auto contains = [&](const std::string& needle) {
            return s.find(needle) != std::string::npos;
        };
        if (contains("v/oct") || contains("voct") || contains("volt per octave") || contains("volts per octave") ||
            contains("voltage per octave") || contains("pitch cv") || contains("v per oct")) {
            if (contains("v oscillator"))
                return "v/oct v";
            if (contains("z oscillator"))
                return "v/oct z";
            return "v/oct";
        }
        return s;
    }

    static std::string applyWordMap(const std::string& label) {
        static const std::unordered_map<std::string, std::string> wordMap = {
            {"envelope", "env"}, {"frequency", "freq"}, {"resonance", "res"}, {"response", "mode"},
            {"sensitivity", "sens"}, {"modulation", "mod"}, {"mod", "mod"}, {"control", "ctrl"},
            {"voltage", "v"}, {"amount", "amt"}, {"trigger", "trig"}, {"probability", "prob"},
            {"gate", "gate"}, {"delay", "delay"}, {"feedback", "feedback"}, {"cross", "cross"},
            {"link", "link"}, {"channels", ""}, {"channel", ""}, {"polyphonic", ""}, {"poly", "poly"},
            {"output", ""}, {"input", ""}, {"rate", "rate"}, {"shape", "shape"}, {"drift", "drift"},
            {"jitter", "jitter"}, {"alternate", "alt"}, {"alternating", "alt"}, {"interval", "interval"},
            {"complexity", "complexity"}, {"slew", "slew"}
        };

        std::vector<std::string> tokens;
        std::string current;
        auto flushToken = [&]() {
            if (!current.empty()) {
                auto it = wordMap.find(current);
                if (it != wordMap.end())
                    current = it->second;
                if (!current.empty())
                    tokens.push_back(current);
                current.clear();
            }
        };

        for (char c : label) {
            if (c == ' ') {
                flushToken();
            } else {
                current.push_back(c);
            }
        }
        flushToken();

        std::string result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i)
                result.push_back(' ');
            result += tokens[i];
        }
        return result.empty() ? label : result;
    }

    static const std::unordered_map<std::string, std::string>& inputMap() {
        static const std::unordered_map<std::string, std::string> map = {
            {"audio left/mono", "audio l"},
            {"audio left", "audio l"},
            {"audio right", "audio r"},
            {"audio b", "audio b"},
            {"left audio", "audio l"},
            {"right audio", "audio r"},
            {"left/mono", "audio l"},
            {"gate input", "gate"},
            {"crossfade cv", "crossfade cv"},
            {"dcw gate", "dcw gate"},
            {"dcw trigger", "dcw trigger"},
            {"delay 1 time cv", "delay 1 time"},
            {"delay 2 time cv", "delay 2 time"},
            {"delay 3 time cv", "delay 3 time"},
            {"distortion amount cv", "dist amount"},
            {"distortion type cv", "dist type"},
            {"drive amount cv", "drive amount"},
            {"filter a resonance cv", "filter a res"},
            {"filter b resonance cv", "filter b res"},
            {"filter a cutoff cv", "filter a cutoff"},
            {"filter b cutoff cv", "filter b cutoff"},
            {"length a cv", "length a"},
            {"length b cv", "length b"},
            {"lfo 1 rate cv", "lfo 1 rate"},
            {"lfo 2 rate cv", "lfo 2 rate"},
            {"lfo 3 rate cv", "lfo 3 rate"},
            {"lfo/sweep cv", "lfo sweep"},
            {"mix control cv", "mix control"},
            {"mix cv", "mix cv"},
            {"mod depth cv", "mod depth"},
            {"probability cv", "probability"},
            {"rate cv", "rate"},
            {"repeats cv", "repeats"},
            {"reset b", "reset b"},
            {"speed 1 cv", "speed 1"},
            {"speed 2 cv", "speed 2"},
            {"speed 3 cv", "speed 3"},
            {"speed 4 cv", "speed 4"},
            {"start a trigger", "start a"},
            {"start b trigger", "start b"},
            {"stop a trigger", "stop a"},
            {"stop b trigger", "stop b"},
            {"tap/step", "tap"},
            {"symmetry cv", "symmetry"},
            {"torsion cv", "torsion"},
            {"v fine tune cv", "v fine"},
            {"v fine tune cv amount", "v fine amt"},
            {"z fine tune cv", "z fine"},
            {"z fine tune cv amount", "z fine amt"},
            {"v shape cv", "v shape cv"},
            {"v shape cv amount", "v shape cv amt"},
            {"z shape cv", "z shape cv"},
            {"z shape cv amount", "z shape cv amt"},
            {"v oscillator v/oct", "v/oct v"},
            {"z oscillator v/oct", "v/oct z"},
            {"vca control voltage", "vca cv"},
            {"sidechain detector", "sidechain"}
        };
        return map;
    }

    static const std::unordered_map<std::string, std::string>& outputMap() {
        static const std::unordered_map<std::string, std::string> map = {
            {"audio left", "audio l"},
            {"audio right", "audio r"},
            {"audio b", "audio b"},
            {"left", "out l"},
            {"left output", "out l"},
            {"right", "out r"},
            {"right output", "out r"},
            {"l", "out l"},
            {"r", "out r"},
            {"left/mono", "out l"},
            {"delay 1 tap output", "delay 1 tap"},
            {"delay 2 tap output", "delay 2 tap"},
            {"delay 3 tap output", "delay 3 tap"},
            {"edge difference", "edge diff"},
            {"composite gate", "gate composite"},
            {"envelope 2", "env 2"},
            {"envelope 2 gate", "env 2 gate"},
            {"envelope 2 eoc", "env 2 eoc"},
            {"envelope 4", "env 4"},
            {"envelope 4 gate", "env 4 gate"},
            {"envelope 4 eoc", "env 4 eoc"},
            {"ring 1 cv", "ring 1"},
            {"ring 2 cv", "ring 2"},
            {"ring 3 cv", "ring 3"},
            {"gate a", "gate a"},
            {"gate a (polyphonic)", "gate a"},
            {"gate b", "gate b"},
            {"gate b (polyphonic)", "gate b"},
            {"main mix cv", "mix cv"}
        };
        return map;
    }

    static std::string normalizeInputLabel(const std::string& label) {
        std::string base = replaceVPerOct(baseClean(label));
        if (base.empty())
            return base;
        auto it = inputMap().find(base);
        if (it != inputMap().end())
            return it->second;
        return applyWordMap(base);
    }

    static std::string normalizeOutputLabel(const std::string& label) {
        std::string base = baseClean(label);
        if (base.empty())
            return base;
        auto it = outputMap().find(base);
        if (it != outputMap().end())
            return it->second;
        return applyWordMap(base);
    }

    static std::string normalizeParamLabel(const std::string& label) {
        std::string base = replaceVPerOct(baseClean(label));
        return applyWordMap(base);
    }

public:
    static void normalizeModuleControls(rack::engine::Module* module) {
        if (!module)
            return;

        for (size_t i = 0; i < module->paramQuantities.size(); ++i) {
            rack::engine::ParamQuantity* pq = module->paramQuantities[i];
            if (!pq)
                continue;
            pq->name = normalizeParamLabel(pq->name);
        }

        // Note: Input/Output port names are managed through configInput/configOutput
        // and stored in the module's inputInfos/outputInfos, not directly on the ports
        for (size_t i = 0; i < module->inputInfos.size(); ++i) {
            if (module->inputInfos[i]) {
                module->inputInfos[i]->name = normalizeInputLabel(module->inputInfos[i]->name);
            }
        }

        for (size_t i = 0; i < module->outputInfos.size(); ++i) {
            if (module->outputInfos[i]) {
                module->outputInfos[i]->name = normalizeOutputLabel(module->outputInfos[i]->name);
            }
        }
    }
};

} // namespace ui
} // namespace shapetaker
