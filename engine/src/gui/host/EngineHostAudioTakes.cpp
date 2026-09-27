// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

#include "core/app/AppPaths.h"
#include "core/packs/Roles.h"
#include "hum/caps/Files.h"

namespace hum {

namespace {

double secondsOf(const std::string& path) {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(fileAt(path)));
    if (!reader || reader->sampleRate <= 0.0) return 0.0;
    return (double) reader->lengthInSamples / reader->sampleRate;
}

juce::String fileStem(const std::string& node, const std::string& take) {
    return (juce::String(node) + "-" + juce::String(take)).replaceCharacter(' ', '-').toLowerCase();
}

}

std::vector<AudioTakeInfo> EngineHost::audioTakesOf(const std::string& node) {
    std::vector<AudioTakeInfo> out;
    auto* takes = dynamic_cast<AudioTakes*>(liveOrganism(node));
    if (takes == nullptr) return out;
    for (int i = 0; i < takes->audioTakeCount(); ++i)
        out.push_back({i, takes->audioTakeName(i), takes->audioTakeReady(i)});
    return out;
}

std::string EngineHost::exportAudioTake(const std::string& node, int take, const std::string& folder) {
    auto* takes = dynamic_cast<AudioTakes*>(liveOrganism(node));
    if (takes == nullptr || !takes->audioTakeReady(take)) return {};
    const auto dir = folder.empty() ? record_.recordingsDir() : fileAt(folder);
    if (!dir.createDirectory()) return {};
    const auto file = dir.getNonexistentChildFile(fileStem(node, takes->audioTakeName(take)), ".wav", false);
    const auto path = pathOf(file);
    return takes->writeAudioTake(take, path) ? path : std::string();
}

int EngineHost::audioToTimeline(const std::string& node, int take, std::string& error, int atTick,
                                const std::string& folder) {
    const auto takes = audioTakesOf(node);
    if (takes.empty()) { error = node + " has no audio to send"; return 0; }
    const int bar = 4 * Pattern::kTicksPerBeat;
    if (atTick < 0) atTick = (int) (std::llround(positionBeats() * Pattern::kTicksPerBeat) / bar) * bar;
    const double bpm = tempo() > 0.0 ? tempo() : 120.0;
    beginTransaction();
    pushUndo();
    int placed = 0;
    for (const auto& t : takes) {
        if (!t.ready || (take >= 0 && t.index != take)) continue;
        const auto path = exportAudioTake(node, t.index, folder);
        const double seconds = path.empty() ? 0.0 : secondsOf(path);
        if (seconds <= 0.0) continue;
        const int ticks = std::max(1, (int) std::llround(seconds * bpm / kSecondsPerMinute * Pattern::kTicksPerBeat));
        const auto track = addOrganism(classWithRole(role::kAudioTrack), spotBelowPatch());
        const int clip = clips().addAudio(track, atTick, ticks, path);
        if (clip < 0) continue;
        clips().rename(track, clip, node + " " + t.name);
        ++placed;
    }
    endTransaction();
    if (placed == 0) error = "nothing recorded on " + node + " yet";
    else if (onArrangementChanged) scheduler_.post([cb = onArrangementChanged] { cb(); });
    return placed;
}

}
