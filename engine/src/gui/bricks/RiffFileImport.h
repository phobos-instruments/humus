// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/RiffImport.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostPattern.h"
#include "io/PatchDocument.h"
#include "gui/common/Localisation.h"

namespace hum {

inline constexpr const char* kRiffFileWildcard = "*.mid;*.midi;*.syx;*.seq";
inline constexpr const char* kTripletResolution = "1/12";
inline constexpr const char* kStraightResolution = "1/16";

inline bool isRiffFile(const juce::String& path) {
    return juce::File(path).hasFileExtension("mid;midi;syx;seq");
}

inline std::vector<riff::Imported> readRiffFiles(const juce::StringArray& paths) {
    std::vector<riff::Imported> all;
    for (const auto& path : paths) {
        juce::MemoryBlock bytes;
        if (!juce::File(path).loadFileAsData(bytes)) continue;
        auto got = riff::importBytes(static_cast<const std::uint8_t*>(bytes.getData()), bytes.getSize());
        for (auto& one : got) all.push_back(std::move(one));
    }
    return all;
}

inline void importRiffFiles(BrickHost& host, const std::string& name, const juce::StringArray& paths) {
    const auto all = readRiffFiles(paths);
    if (all.empty()) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, tr("riff-import.nothing-title", "No pattern found"),
            tr("riff-import.nothing",
               "Nothing in that file reads as a bassline pattern. Riff takes standard MIDI files "
               "and the .syx pattern dumps and .seq pattern files of the common acid-box clones."));
        return;
    }
    const int first = host.patterns().bank(name);
    host.pushUndo();
    const auto* cm = host.model().byName(name);
    const std::string now = cm != nullptr ? cm->pattern.matrixResolution : std::string();
    if (all.front().triplet) host.patterns().setResolution(name, kTripletResolution);
    else if (now == kTripletResolution) host.patterns().setResolution(name, kStraightResolution);

    int placed = 0;
    for (; placed < (int) all.size() && first + placed < kPatternBanks; ++placed)
        host.patterns().setBasslineSteps(name, first + placed, all[(size_t) placed].steps);
    if (placed < (int) all.size())
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, tr("riff-import.overflow-title", "Some patterns left out"),
            tr("riff-import.overflow",
               "Not every pattern in that file fitted: they fill the banks from the one showing "
               "up to H. Pick bank A first to take up to eight."));
}

inline void chooseRiffFiles(BrickHost& host, const std::string& name,
                            std::unique_ptr<juce::FileChooser>& chooser, std::function<void()> done) {
    chooser = std::make_unique<juce::FileChooser>(
        tr("pattern-step-grid.import-title", "Import bassline patterns"), juce::File(), kRiffFileWildcard);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                             | juce::FileBrowserComponent::canSelectMultipleItems,
                         [&host, name, done = std::move(done)](const juce::FileChooser& fc) {
        juce::StringArray paths;
        for (const auto& f : fc.getResults()) paths.add(f.getFullPathName());
        if (paths.isEmpty()) return;
        importRiffFiles(host, name, paths);
        if (done) done();
    });
}

}
