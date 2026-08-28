#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "hum/LayoutSpec.h"
#include "hum/Registry.h"

namespace hum {
namespace {

inline LayoutSpec consoleLayout(const std::string& cls) {
    using CT = LayoutSpec::ControlType;
    LayoutSpec spec;
    const std::string suf = "Console";
    if (cls.size() <= suf.size() || cls.compare(cls.size() - suf.size(), suf.size(), suf) != 0)
        return spec;
    if (cls[0] != 'S') return spec;
    const std::string mid = cls.substr(1, cls.size() - suf.size() - 1);
    if (mid.empty() || mid.find_first_not_of("0123456789") != std::string::npos) return spec;
    const int n = std::stoi(mid);
    if (n < 1 || n > 8) return spec;

    spec.controls.push_back({CT::Combo, "", "", "Inputs", 8, 4, 92, 20, 2, false,
                             {"2", "3", "4", "5", "6", "7", "8"}, {{"reclass", "inputs"}}});
    spec.controls.push_back({CT::Combo, "Flavor", "", "Flavor", 104, 4, 124, 20, 2, false,
                             {"Clean", "British", "American", "Modern"}});
    spec.controls.push_back({CT::MiniToggle, "Direct", "", "Direct", 232, 4, 40, 20, 2, false});

    spec.width = kRackFullW;
    const int knobH = kKnobH, colW = kKnobPitch;
    const int chColW = std::clamp((spec.width - 16) / n, kKnobPitch, kKnobW + 40);
    const int charY = 46;
    spec.controls.push_back({CT::Label, "", "", "Console", 8, 30, 240, 14, 0, false});
    const char* charKnobs[][2] = {{"Output", "Out"}, {"Drive", "Drive"},
                                  {"Crosstalk", "X-Talk"}, {"Sag", "Sag"}};
    for (int i = 0; i < 4; ++i)
        spec.controls.push_back({CT::Knob, charKnobs[i][0], "", charKnobs[i][1],
                                 8 + i * colW, charY, kKnobW, knobH, 2, false});

    const int chLabelY = charY + knobH + 8, chY = chLabelY + 16;
    const int panY = chY + knobH + 2;
    const int msY = panY + knobH + 2;
    spec.controls.push_back({CT::Label, "", "", "Channels", 8, chLabelY, 240, 14, 0, false});
    for (int k = 0; k < n; ++k) {
        const std::string sfx = std::to_string(2 * k + 1) + "-" + std::to_string(2 * k + 2);
        const int x = 8 + k * chColW;

        spec.controls.push_back({CT::Knob, "Gain_" + sfx, "", sfx,
                                 x, chY, kKnobW, knobH, 2, false, {},
                                 {{"meter", std::to_string(k)}}});
        spec.controls.push_back({CT::Knob, "Pan_" + sfx, "", "Pan",
                                 x, panY, kKnobW, knobH, 2, false});
        spec.controls.push_back({CT::MiniToggle, "Mute_" + sfx, "", "M",
                                 x, msY, 20, 16, 2, false});
        spec.controls.push_back({CT::MiniToggle, "Solo_" + sfx, "", "S",
                                 x + 22, msY, 20, 16, 2, false});
    }
    spec.height = msY + 16 + 8;
    return spec;
}

inline void modeHeader(LayoutSpec& spec, int x, int y, int w) {
    spec.controls.push_back({LayoutSpec::ControlType::Combo, "", "", "", x, y, w, 16, 0,
                             false, {"Stereo", "Mono"}, {{"reclass", "mode"}}});
}

inline LayoutSpec dynamicsLayout(const std::string& cls) {
    using CT = LayoutSpec::ControlType;
    LayoutSpec spec;
    auto ends = [&](const char* s) {
        const std::string suf = s;
        return cls.size() >= suf.size() && cls.compare(cls.size() - suf.size(), suf.size(), suf) == 0;
    };

    struct Tog { std::string param, label; };
    struct Kn  { std::string param, label, dimWhen; };
    std::vector<Tog> togs;
    std::vector<Kn> knobs;
    bool rangeThresh = false;

    if (ends("Compressor")) {
        togs = {{"AutoMakeupGain", "Auto"}, {"Sync", "Sync"}};
        knobs = {{"Threshold", "Thresh"}, {"CompressionRatio", "Ratio"}, {"KneeWidth", "Knee"},
                 {"AttackTime", "Atk"}, {"HoldTime", "Hold"},
                 {"ReleaseTime", "Rel", "Sync"},
                 {"MakeupGain", "Makeup", "AutoMakeupGain"}, {"InputGain", "In"}};
    } else if (ends("Limiter")) {
        togs = {};
        knobs = {{"Threshold", "Thresh"}, {"Ceiling", "Ceil"}, {"HoldTime", "Hold"},
                 {"ReleaseTime", "Rel"}, {"InputGain", "In"}};
    } else if (ends("NoiseGate")) {
        togs = {{"Mode", "Duck"}};
        rangeThresh = true;
        knobs = {{"Range", "Range"}, {"AttackTime", "Atk"}, {"HoldTime", "Hold"},
                 {"ReleaseTime", "Rel"}, {"InputGain", "In"}};
    } else {
        return spec;
    }

    const int colW = kKnobPitch, knobH = kKnobH, top = 4, knobY = 24, pad = 8;
    int x = pad;
    modeHeader(spec, x, top, 76);
    x += 80;
    for (auto& t : togs) {
        spec.controls.push_back({CT::MiniToggle, t.param, "", t.label, x, top, 40, 16, 0, false});
        x += 44;
    }
    if (ends("Compressor"))
        spec.controls.push_back({CT::RhythmicUnit, "SyncMultiplier", "SyncUnit", "Rate",
                                 x, top, 210, 16, 0, false});
    x = pad;
    if (rangeThresh) {
        spec.controls.push_back({CT::RangeVSlider, "Threshold", "", "Thresh", x, knobY, colW - 6, knobH, 2, false});
        x += colW;
    }
    for (auto& k : knobs) {
        std::map<std::string, std::string> extra;
        if (!k.dimWhen.empty()) extra["dim-when"] = k.dimWhen;
        spec.controls.push_back({CT::Knob, k.param, "", k.label, x, knobY, kKnobW, knobH, 2,
                                 false, {}, std::move(extra)});
        x += colW;
    }
    spec.controls.push_back({CT::Label, "", "", "GR", x + 2, knobY - 16, 28, 14, 0, false});
    spec.controls.push_back({CT::GainReduction, "", "", "", x + 2, knobY, 28, knobH, 0, false});
    x += 36;
    spec.width = (x + pad <= kRackHalfW) ? kRackHalfW : kRackFullW;
    spec.height = knobY + knobH + 20;
    return spec;
}

inline LayoutSpec paraEqLayout(const std::string& cls) {
    using CT = LayoutSpec::ControlType;
    LayoutSpec spec;
    if (cls != "ParaEQ" && cls != "SParaEQ" && cls != "MParaEQ") return spec;
    struct Col { const char* label; std::vector<std::pair<std::string, std::string>> knobs; };
    const std::vector<Col> cols = {
        {"Low Shelf",  {{"LSCutoffFrequency", "Freq"}, {"LSGain", "Gain"}}},
        {"Band 1",     {{"BP1CenterFrequency", "Freq"}, {"BP1Bandwidth", "BW"}, {"BP1Gain", "Gain"}}},
        {"Band 2",     {{"BP2CenterFrequency", "Freq"}, {"BP2Bandwidth", "BW"}, {"BP2Gain", "Gain"}}},
        {"High Shelf", {{"HSCutoffFrequency", "Freq"}, {"HSGain", "Gain"}}},
    };
    const int colW = kKnobPitch + 2, pad = 6, ky = 36, kh = kKnobH, kw = kKnobW;
    modeHeader(spec, pad, 2, 76);
    int x = pad;
    for (auto& c : cols) {
        spec.controls.push_back({CT::Label, "", "", c.label, x, ky - 16, colW, 14, 0, false});
        int y = ky;
        for (auto& k : c.knobs) {
            spec.controls.push_back({CT::Knob, k.first, "", k.second, x, y, kw, kh, 2, false});
            y += kh + 6;
        }
        x += colW;
    }
    spec.width = kRackHalfW;
    spec.height = ky + 3 * (kh + 6) + 6;
    return spec;
}

}

void hum_register_layouts_humus(Registry& r) {
    r.registerLayoutProvider("humus", [](const std::string& genId,
                                         const std::string& cls) -> std::string {
        if (genId.empty() || genId == "console")
            if (auto s = consoleLayout(cls); !s.controls.empty()) return toJson(s);
        if (genId.empty() || genId == "dynamics")
            if (auto s = dynamicsLayout(cls); !s.controls.empty()) return toJson(s);
        if (genId.empty() || genId == "paraeq")
            if (auto s = paraEqLayout(cls); !s.controls.empty()) return toJson(s);
        return {};
    });
}

}
