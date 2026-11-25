// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/app/WizardFlow.h"
#include "gui/common/Localisation.h"

namespace hum::wizard {

inline juce::String pageName(Page p) {
    switch (p) {
        case kAudio:      return tr("setup-wizard.page-audio", "Audio");
        case kMidi:       return tr("setup-wizard.page-midi", "MIDI");
        case kAppearance: return tr("setup-wizard.page-appearance", "Appearance");
        case kMenuStyle:  return tr("setup-wizard.page-menus", "Menus");
        case kTelemetry:  return tr("setup-wizard.page-privacy", "Privacy");
        case kFinish:     return tr("setup-wizard.page-ready", "Ready");
        default:          return {};
    }
}

inline std::vector<juce::String> summaryText(const std::string& audioDevice, int midiInsEnabled,
                                             const std::string& themeName, bool modernMenus, bool telemetryOn,
                                             bool updatesOn) {
    std::vector<juce::String> out;
    out.push_back(audioDevice.empty()
                      ? tr("setup-wizard.summary-audio-none", "Audio: not configured")
                      : tr("setup-wizard.summary-audio", "Audio: {device}", {{"device", juce::String(audioDevice)}}));
    out.push_back(trn("setup-wizard.summary-midi", "MIDI: {n} input enabled", "MIDI: {n} inputs enabled", midiInsEnabled));
    out.push_back(tr("setup-wizard.summary-theme", "Theme: {theme}",
                     {{"theme", juce::String(themeName.empty() ? std::string("Humus") : themeName)}}));
    out.push_back(modernMenus ? tr("setup-wizard.summary-menus-modern", "Creation menus: Modern")
                              : tr("setup-wizard.summary-menus-classic", "Creation menus: Classic"));
    out.push_back(updatesOn ? tr("setup-wizard.summary-updates-on", "Update checks: at startup")
                            : tr("setup-wizard.summary-updates-off", "Update checks: only when you ask"));
    out.push_back(telemetryOn ? tr("setup-wizard.summary-usage-on", "Usage data: shared anonymously")
                              : tr("setup-wizard.summary-usage-off", "Usage data: not shared"));
    return out;
}

}
