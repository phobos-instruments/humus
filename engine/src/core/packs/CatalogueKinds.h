// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace hum::catalogue {

struct Kind {
    std::string id;
    std::string wildcard;
    std::string holds;
    std::string badge;
    std::vector<std::pair<std::string, std::string>> badges;
};

inline const std::vector<Kind>& kinds() {
    static const std::vector<Kind> all = {
        {"Impulses", "*.wav;*.aif;*.aiff;*.flac",
         "Impulse responses: a recording of a room, a plate or a spring.", "IR", {}},
        {"Samples", "*.wav;*.aif;*.aiff;*.flac",
         "One sound per file.", "ONE-SHOT", {}},
        {"Banks", "*.sf2;*.nsmp3;*.nsmp4;*.syx;*.opm;*.wopl;*.wopn;*.tfi;*.dmp",
         "Many instruments in one file.", "BANK",
         {{".sf2", "SF2"}, {".nsmp3", "NSMP"}, {".nsmp4", "NSMP"}, {".syx", "6-OP"}, {".opm", "4-OP"}, {".wopl", "2-OP"},
          {".wopn", "CHIP"}, {".tfi", "CHIP"}, {".dmp", "CHIP"}}},
        {"Riffs", "*.mid;*.midi;*.syx;*.seq",
         "Bassline patterns: a few steps of notes, gates and accents.", "RIFF",
         {{".mid", "MIDI"}, {".midi", "MIDI"}, {".syx", "DUMP"}, {".seq", "SEQ"}}},
        {"Scores", "*.mid;*.midi;*.kar;*.mus",
         "Scores: a song written out for instruments, not recorded.", "SCORE",
         {{".mid", "MIDI"}, {".midi", "MIDI"}, {".kar", "KARAOKE"}, {".mus", "MUS"}}},
        {"Scales", "*.scl",
         "Scala tuning files.", "SCL", {}},
        {"Shaders", "*.frag;*.fs;*.glsl;*.fsh;*.synScene",
         "Fragment shader scenes: a .frag or .fs file, or a .synScene folder.",
         "SCENE", {}},
    };
    return all;
}

}
