// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

namespace hum::about {

struct Line {
    const char* key;
    const char* text;
};

inline constexpr Line kTagline{
    "about.tagline",
    "An interactive, patchable music environment - a live modular instrument."};
inline constexpr Line kCopyright{"about.copyright", "Copyright (C) 2026 Gabriele Arcangelo Scalici (Phobos Instruments)"};
inline constexpr Line kAuthor{"about.author", "Written by Gabriele Arcangelo Scalici"};
inline constexpr Line kLicence{
    "about.licence",
    "Free software under the GNU Affero General Public License, version 3, official "
    "builds included. Anything built on it carries the same licence."};
inline constexpr Line kTrademark{
    "about.trademark",
    "The Humus name and the seedling mark are trademarks of the project and are not "
    "covered by that licence."};
inline constexpr Line kOwners{
    "about.owners",
    "The names and marks above belong to their owners. They are here to say what is "
    "inside Humus and under what terms, not to suggest that anyone endorses it."};
inline constexpr const char* kSite = "https://humus.phobos-instruments.com";
inline constexpr const char* kSource = "https://github.com/phobos-instruments/humus";

struct Credit {
    const char* name;
    const char* terms;
    const char* key;
    const char* what;
};

inline const std::vector<Credit>& credits() {
    static const std::vector<Credit> all = {
        {"Ross Bencina", "with thanks",
         "about.credit-ross-bencina",
         "for AudioMulch and the thinking behind it, one of the biggest inspirations "
         "this environment grew from"},
        {"The Metasurface, by Ross Bencina", "NIME 2005",
         "about.credit-the-metasurface",
         "\"Applying Natural Neighbour Interpolation to Two-to-Many Mapping\", the "
         "method the Metapad morphs a whole patch with"},
        {"JUCE", "AGPLv3",
         "about.credit-juce",
         "the application framework underneath: windows, audio devices, plugin hosting"},
        {"VST3 SDK", "GPLv3 arm of its dual licence",
         "about.credit-vst3-sdk",
         "the plugin format Humus hosts and ships as"},
        {"Steinberg ASIO SDK", "GPLv3 arm of its dual licence",
         "about.credit-steinberg-asio-sdk",
         "the low-latency audio driver interface on Windows"},
        {"Ableton Link", "GPLv2 or later",
         "about.credit-ableton-link",
         "tempo, beat and start shared with other apps on the network"},
        {"Asio", "Boost Software License 1.0",
         "about.credit-asio",
         "the networking library Ableton Link is built on, unrelated to the driver "
         "interface of the same name"},
        {"Monocypher", "CC0 or BSD-2",
         "about.credit-monocypher",
         "the signature checks behind licences, packs and updates"},
        {"LAME", "LGPL-2.0-or-later",
         "about.credit-lame",
         "writes the MP3 a bounce asks for"},
        {"music-synthesizer-for-android", "Apache-2.0",
         "about.credit-music-synthesizer-for-android",
         "one of the FM engines inside the pH synthesizer"},
        {"Nuked-OPN2, Nuked-OPM, Nuked-OPL3", "LGPL-2.1",
         "about.credit-nuked-opn2",
         "cycle-accurate emulation of the Yamaha FM chips inside pH"},
        {"reSID", "GPL-2.0 or later",
         "about.credit-resid",
         "the MOS SID emulation inside the Silt synthesizer"},
        {"NSFPlay, maintained by Brad Smith", "distributed freely, per its readme",
         "about.credit-nsfplay",
         "the Ricoh 2A03/2A07 and cartridge chips inside Grit; its embedded emu2413 "
         "and emu2149 cores are by Mitsutaka Okazaki"},
        {"Chip instrument banks by Vitaliy Novichkov", "MIT",
         "about.credit-chip-instrument-banks-by",
         "the FM voice banks pH ships with, from libOPNMIDI"},
        {"MediaPipe models", "Apache-2.0, per their model cards",
         "about.credit-mediapipe-models",
         "the hand and pose models behind Hands and Skeleton, converted and run by "
         "our own interpreter"},
        {"FFmpeg", "LGPL-2.1 or later",
         "about.credit-ffmpeg",
         "video decoding on Linux, decode only, no GPL or non-free components"},
        {"Signalsmith Stretch and Signalsmith Linear", "MIT",
         "about.credit-signalsmith-stretch-and-signalsmith",
         "the time stretch behind key-locked warping"},
        {"SoundTouch", "LGPL-2.1",
         "about.credit-soundtouch",
         "the time stretch on builds that are configured to use it instead"},
        {"Paulstretch, by Nasca Octavian Paul", "public domain",
         "about.credit-paulstretch",
         "the extreme stretch our Paulstretch organism implements"},
        {"Pink Trombone, by Neil Thapen", "MIT",
         "about.credit-pink-trombone",
         "the vocal tract our PinkTrombone organism is a port of"},
        {"Open303, by Robin Schmidt", "MIT",
         "about.credit-open303",
         "the diode ladder and output stages our Acid organism is based on"},
        {"Adventure Kid Reverb Impulse Responses, by Kristoffer Ekstrand", "CC BY 4.0",
         "about.credit-adventure-kid-reverb-impulse",
         "the room, cabinet and spring impulses that ship with the app"},
        {"sounds-tr808-fischer, by Michael Fischer", "CC0 1.0",
         "about.credit-sounds-tr808-fischer",
         "the analog drum one-shots that ship with the app"},
    };
    return all;
}

}
