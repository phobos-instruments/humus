// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/BankBrowser.h"
#include "gui/editor/BankSlotSpec.h"
#include "gui/editor/files/RiffImportPlan.h"
#include "gui/editor/juce/JuceFilePicker.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostPattern.h"
#include "io/PatchDocument.h"
#include "gui/common/Localisation.h"

namespace hum {

inline bool isRiffFile(const juce::String& path) { return files::isRiffPath(path.toStdString()); }

inline std::string riffClassOf(const ModelHost& host, const std::string& name) {
    const auto* cm = host.model().byName(name);
    return cm != nullptr ? cm->displayClass : std::string();
}

inline banks::Slot riffSlotFor(const std::string& cls) {
    for (const auto& c : layoutSpecFor(cls).controls)
        if (c.type == LayoutSpec::ControlType::BasslineImport)
            return {c.extraOr("kind", "Riffs"), c.extraOr("filter", files::kRiffFileWildcard),
                    c.extraOr("factory")};
    return {"Riffs", files::kRiffFileWildcard, {}};
}

inline void rememberRiffs(const ModelHost& host, const std::string& name,
                          const juce::StringArray& paths) {
    const auto cls = riffClassOf(host, name);
    if (cls.empty()) return;
    const auto slot = riffSlotFor(cls);
    for (const auto& path : paths)
        BankBrowser::remember(banks::referenceFor(juce::File(path), slot, cls), cls);
}

inline void importRiffFiles(BrickHost& host, const std::string& name, const juce::StringArray& paths) {
    std::vector<std::string> list;
    for (const auto& p : paths) list.push_back(p.toStdString());
    const auto all = files::readRiffPaths(list);
    if (all.empty()) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, tr("riff-import.nothing-title", "No pattern found"),
            tr("riff-import.nothing",
               "Nothing in that file reads as a bassline pattern. Riff takes standard MIDI files "
               "and the .syx pattern dumps and .seq pattern files of the common acid-box clones."));
        return;
    }
    host.pushUndo();
    const auto* cm = host.model().byName(name);
    const auto plan = files::planRiffImport(all, host.patterns().bank(name),
                                            cm != nullptr ? cm->pattern.matrixResolution : std::string());
    if (!plan.resolution.empty()) host.patterns().setResolution(name, plan.resolution);
    for (int i = 0; i < plan.placed; ++i)
        host.patterns().setBasslineSteps(name, plan.firstBank + i, all[(size_t) i].steps);
    rememberRiffs(host, name, paths);
    if (plan.overflow)
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, tr("riff-import.overflow-title", "Some patterns left out"),
            tr("riff-import.overflow",
               "Not every pattern in that file fitted: they fill the banks from the one showing "
               "up to H. Pick bank A first to take up to eight."));
}

inline void browseRiffs(BrickHost& host, const std::string& name, juce::Rectangle<int> anchor,
                        std::function<void()> onOpenFile, std::function<void()> done) {
    const auto cls = riffClassOf(host, name);
    const auto slot = riffSlotFor(cls);
    BankBrowser::show(anchor, cls, slot, {},
        [&host, name, cls, done](const std::string& ref) {
            juce::StringArray one;
            one.add(juce::String::fromUTF8(banks::resolve(ref, cls).c_str()));
            importRiffFiles(host, name, one);
            if (done) done();
        },
        std::move(onOpenFile), [cls](const std::string& ref) { BankBrowser::forget(ref, cls); });
}

inline void chooseRiffFiles(BrickHost& host, const std::string& name, JuceFilePicker& picker,
                            std::function<void()> done) {
    picker.pick(files::riffPick(host), [&host, name, done = std::move(done)](const std::vector<std::string>& paths) {
        if (paths.empty()) return;
        juce::StringArray list;
        for (const auto& p : paths) list.add(juce::String::fromUTF8(p.c_str()));
        importRiffFiles(host, name, list);
        if (done) done();
    });
}

}
