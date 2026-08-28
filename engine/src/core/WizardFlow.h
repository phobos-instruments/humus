#pragma once
#include <string>
#include <vector>

namespace hum {
namespace wizard {

enum Page { kAudio = 0, kMidi, kAppearance, kMenuStyle, kTelemetry, kFinish, kNumPages };

inline const char* pageTitle(Page p) {
    switch (p) {
        case kAudio:      return "Audio";
        case kMidi:       return "MIDI";
        case kAppearance: return "Appearance";
        case kMenuStyle:  return "Menus";
        case kTelemetry:  return "Privacy";
        case kFinish:     return "Ready";
        default:          return "";
    }
}

inline Page next(Page p) { return p >= kFinish ? kFinish : (Page) (p + 1); }
inline Page back(Page p) { return p <= kAudio ? kAudio : (Page) (p - 1); }
inline bool isFirst(Page p) { return p == kAudio; }
inline bool isLast(Page p) { return p == kFinish; }

inline std::vector<std::string> summaryLines(const std::string& audioDevice,
                                             int midiInsEnabled,
                                             const std::string& themeName,
                                             bool modernMenus,
                                             bool telemetryOn,
                                             bool updatesOn) {
    std::vector<std::string> out;
    out.push_back("Audio: "
                  + (audioDevice.empty() ? std::string("not configured") : audioDevice));
    out.push_back("MIDI: " + std::to_string(midiInsEnabled)
                  + (midiInsEnabled == 1 ? " input" : " inputs") + " enabled");
    out.push_back("Theme: " + (themeName.empty() ? std::string("Humus") : themeName));
    out.push_back("Creation menus: " + std::string(modernMenus ? "Modern" : "Classic"));
    out.push_back(std::string("Usage data: ")
                  + (telemetryOn ? "shared anonymously" : "not shared"));
    out.push_back(std::string("Update checks: ")
                  + (updatesOn ? "at startup" : "only when you ask"));
    return out;
}

}
}
