#include <string>

#include "hum/LayoutSpec.h"
#include "hum/Registry.h"

namespace hum {
namespace {

inline void addSizeHeader(LayoutSpec& spec, bool panOption) {
    using CT = LayoutSpec::ControlType;
    std::vector<std::string> modes = {"Stereo", "Mono"};
    if (panOption) modes.push_back("Pan");
    spec.controls.push_back({CT::Combo, "", "", "Inputs", 8, 4, 108, 20, 2, false,
                             {"2", "3", "4", "5", "6", "7", "8"}, {{"reclass", "inputs"}}});
    spec.controls.push_back({CT::Combo, "", "", "Mode", 126, 4, 130, 20, 2, false,
                             std::move(modes), {{"reclass", "mode"}}});
}

inline LayoutSpec fileRecorderLayout(const std::string& cls) {
    using CT = LayoutSpec::ControlType;
    LayoutSpec spec;
    const std::string suf = "FileRecorder";
    if (cls.size() < suf.size() || cls.compare(cls.size() - suf.size(), suf.size(), suf) != 0)
        return spec;
    const std::string head = cls.substr(0, cls.size() - suf.size());
    int n = 2;
    if (!head.empty()) {
        if (head.find_first_not_of("0123456789") != std::string::npos) return spec;
        n = std::stoi(head);
        if (n < 1 || n > 32) return spec;
    }

    const int pad = 10, rowH = 22, pitch = 26, chW = 168;
    int y = 8;
    if (!head.empty()) {
        std::vector<std::string> sizes;
        for (int i = 1; i <= 32; ++i) sizes.push_back(std::to_string(i));
        spec.controls.push_back({CT::Combo, "", "", "Tracks", pad, y, 120, 20, 2, false,
                                 std::move(sizes), {{"reclass", "inputs"}}});
        y += 28;
    }
    for (int i = 1; i <= n; ++i) {
        const std::string s = std::to_string(i);
        spec.controls.push_back({CT::SoundFile, "File_" + s, "", "File " + s,
                                 pad, y, kRackFullW - 2 * pad - chW - 8, rowH, 0, false});
        spec.controls.push_back({CT::IntSpinner, "RequestedChannelCounts_" + s, "", "Ch",
                                 kRackFullW - pad - chW, y, chW, rowH, 0, false});
        y += pitch;
    }
    y += 6;
    spec.controls.push_back({CT::Combo, "PunchMode", "", "Punch", pad, y, 210, rowH, 0, false,
                             {"Manual", "SoundIn Sync", "Timed"}});
    spec.controls.push_back({CT::DoubleSpinner, "RecordDuration", "", "Time ms",
                             pad + 220, y, 200, rowH, 0, false});
    y += 28;
    spec.controls.push_back({CT::Combo, "FileMode", "", "Mode", pad, y, 210, rowH, 0, false,
                             {"Overwrite", "Append"}});
    spec.controls.push_back({CT::Toggle, "Record", "", "Record", pad + 220, y, 200, 24, 0, false});
    spec.width = kRackFullW;
    spec.height = y + 34;
    return spec;
}

inline LayoutSpec mixerLayout(const std::string& cls) {
    LayoutSpec spec;
    if (cls.size() < 7 || cls.compare(cls.size() - 5, 5, "Mixer") != 0) return spec;
    const char p = cls[0];
    if (p != 'S' && p != 'M' && p != 'P') return spec;
    const std::string mid = cls.substr(1, cls.size() - 6);
    if (mid.empty() || mid.find_first_not_of("0123456789") != std::string::npos) return spec;
    const int n = std::stoi(mid);
    if (n < 1 || n > 8) return spec;
    const bool stereo = (p == 'S');
    const bool pan = (p == 'P');

    addSizeHeader(spec, true);
    spec.width = (n <= 3) ? kRackHalfW : kRackFullW;
    const int kMasterX = 8, kChannelsX = 72, pad = 8;
    const int colW = std::max(kKnobPitch, (spec.width - kChannelsX - pad) / n);
    const int knobY = 50, knobH = kKnobH, top = 32;
    auto column = [&](int x, const std::string& gain, const std::string& mute,
                      const std::string& solo, const std::string& label, int meterCh) {
        spec.controls.push_back({LayoutSpec::ControlType::MiniToggle, mute, "", "M", x, top, 24, 16, 2, false});
        if (!solo.empty())
            spec.controls.push_back({LayoutSpec::ControlType::MiniToggle, solo, "", "S", x + 26, top, 24, 16, 2, false});
        std::map<std::string, std::string> extra;
        if (meterCh >= 0) extra["meter"] = std::to_string(meterCh);
        spec.controls.push_back({LayoutSpec::ControlType::Knob, gain, "", label, x, knobY,
                                 kKnobW, knobH, 2, false, {}, extra});
    };

    column(kMasterX, "MasterGain", "MasterMute", "", "Master", -1);
    const int panY = knobY + knobH + 4;
    for (int k = 0; k < n; ++k) {
        const std::string sfx = stereo ? std::to_string(2 * k + 1) + "-" + std::to_string(2 * k + 2)
                                       : std::to_string(k + 1);
        const int x = kChannelsX + k * colW;
        column(x, "Gain_" + sfx, "Mute_" + sfx, "Solo_" + sfx, sfx, k);
        if (pan)
            spec.controls.push_back({LayoutSpec::ControlType::Knob, "Pan_" + sfx, "", "Pan",
                                     x, panY, kKnobW, knobH, 2, false});
    }
    spec.height = (pan ? panY + knobH : knobY + knobH) + 18;
    return spec;
}

inline LayoutSpec busLayout(const std::string& cls) {
    LayoutSpec spec;
    if (cls.size() < 5 || cls.compare(cls.size() - 3, 3, "Bus") != 0) return spec;
    const char p = cls[0];
    if (p != 'S' && p != 'M') return spec;
    const std::string mid = cls.substr(1, cls.size() - 4);
    if (mid.empty() || mid.find_first_not_of("0123456789") != std::string::npos) return spec;

    addSizeHeader(spec, false);
    spec.width = kRackHalfW;
    spec.height = 30;
    return spec;
}

inline LayoutSpec midiBusLayout(const std::string& cls) {
    LayoutSpec spec;
    if (cls.rfind("Midi", 0) != 0 || cls.size() < 8
        || cls.compare(cls.size() - 3, 3, "Bus") != 0)
        return spec;
    const std::string mid = cls.substr(4, cls.size() - 7);
    if (mid.empty() || mid.find_first_not_of("0123456789") != std::string::npos) return spec;

    spec.controls.push_back({LayoutSpec::ControlType::Combo, "", "", "Inputs", 8, 4, 108, 20, 2,
                             false, {"2", "3", "4", "6", "8"}, {{"reclass", "inputs"}}});
    spec.width = kRackHalfW;
    spec.height = 30;
    return spec;
}

inline LayoutSpec gainLayout(const std::string& cls) {
    using CT = LayoutSpec::ControlType;
    LayoutSpec spec;
    if (cls != "SGain" && cls != "MGain") return spec;
    spec.controls.push_back({CT::Combo, "", "", "Mode", 8, 4, 180, 20, 2, false,
                             {"Stereo", "Mono"}, {{"reclass", "mode"}}});
    spec.controls.push_back({CT::MiniToggle, "Mute", "", "Mute", 196, 4, 89, 20, 2, false});
    spec.controls.push_back({CT::Knob, "Gain", "", "Gain", 8, 30, kKnobW, kKnobH, 2, false});
    spec.controls.push_back({CT::LevelBars, "", "", "", 84, 52, 201, 24, 0, false});
    spec.width = kRackHalfW;
    spec.height = 30 + kKnobH + 8;
    return spec;
}

}

void hum_register_layouts_core(Registry& r) {
    r.registerLayoutProvider("core", [](const std::string& genId,
                                          const std::string& cls) -> std::string {
        if (genId.empty() || genId == "mixer")
            if (auto s = mixerLayout(cls); !s.controls.empty()) return toJson(s);
        if (genId.empty() || genId == "bus")
            if (auto s = busLayout(cls); !s.controls.empty()) return toJson(s);
        if (genId.empty() || genId == "midibus")
            if (auto s = midiBusLayout(cls); !s.controls.empty()) return toJson(s);
        if (genId.empty() || genId == "gain")
            if (auto s = gainLayout(cls); !s.controls.empty()) return toJson(s);
        if (genId.empty() || genId == "filerecorder")
            if (auto s = fileRecorderLayout(cls); !s.controls.empty()) return toJson(s);
        return {};
    });
}

}
