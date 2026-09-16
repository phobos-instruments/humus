// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace hum {

namespace role {
inline constexpr std::string_view kAudioIn = "audio-in";
inline constexpr std::string_view kAudioOut = "audio-out";
inline constexpr std::string_view kMasterIn = "master-in";
inline constexpr std::string_view kMasterOut = "master-out";
inline constexpr std::string_view kMidiIn = "midi-in";
inline constexpr std::string_view kMidiOut = "midi-out";
inline constexpr std::string_view kAudioTrack = "audio-track";
inline constexpr std::string_view kVideoTrack = "video-track";
inline constexpr std::string_view kMidiTrack = "midi-track";
inline constexpr std::string_view kTuning = "tuning";
inline constexpr std::string_view kSampleKit = "sample-kit";
inline constexpr std::string_view kDefaultInstrument = "default-instrument";
inline constexpr std::string_view kMixAnchor = "mix-anchor";
inline constexpr std::string_view kDeck = "deck";
inline constexpr std::string_view kNoteLanes = "note-lanes";
inline constexpr std::string_view kClipPads = "clip-pads";
}

struct RoleContract {
    std::string_view role;
    bool single;
    std::vector<std::string_view> params;
};

const std::vector<RoleContract>& roleContracts();

bool classHasRole(const std::string& classString, std::string_view role);
std::string classWithRole(std::string_view role);

}
