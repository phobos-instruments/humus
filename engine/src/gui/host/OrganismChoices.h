// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "gui/common/Localisation.h"
#include "gui/editor/Words.h"
#include "gui/host/ModelHost.h"
#include "hum/caps/Files.h"
#include "hum/caps/Params.h"

namespace hum {

using ChoiceItems = std::vector<std::pair<int, std::string>>;

inline std::optional<ChoiceItems> organismChoiceItems(ModelHost& host, const std::string& source,
                                                      const std::string& organism) {
    constexpr Words kOneTrack{"choices.sound-track", "Sound track"};
    ChoiceItems items;
    if (source == "patch-bank") {
        if (auto* pb = dynamic_cast<PatchBank*>(host.liveOrganism(organism)))
            for (int i = 0, n = pb->patchCount(); i < n; ++i) {
                auto name = pb->patchNameAt(i);
                items.push_back({i + 1, name.empty() ? std::to_string(i + 1) : std::to_string(i + 1) + " " + name});
            }
        if (items.empty()) items.push_back({1, "1"});
        return items;
    }
    if (source == "media-tracks") {
        if (auto* tracks = dynamic_cast<MediaTracks*>(host.liveOrganism(organism))) {
            const auto names = tracks->mediaTracks();
            for (size_t i = 0; i < names.size(); ++i) items.push_back({(int) i, names[i]});
        }
        if (items.empty()) items.push_back({0, host.translated(kOneTrack.key, kOneTrack.written)});
        return items;
    }
    return std::nullopt;
}

inline ChoiceItems channelChoiceItems(const std::string& source) {
    ChoiceItems items;
    if (source == "audio-in-pairs" || source == "audio-out-pairs")
        for (int i = 1; i <= 7; i += 2)
            items.push_back({i, tr("choices.channels-pair", "Channels {first}/{second}",
                                   {{"first", juce::String(i)}, {"second", juce::String(i + 1)}})
                                    .toStdString()});
    if (source == "audio-in-channels" || source == "audio-out-channels")
        for (int i = 1; i <= 8; ++i)
            items.push_back({i, tr("choices.channel-n", "Channel {n}", {{"n", juce::String(i)}}).toStdString()});
    return items;
}

}
