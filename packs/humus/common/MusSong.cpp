// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/MidiSong.h"

#include <algorithm>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum::midisong {

namespace {

constexpr double kTicksPerSecond = 140.0;
constexpr double kMusBpm = 120.0;
constexpr double kTicksPerBeat = kTicksPerSecond * kSecondsPerMinute / kMusBpm;

constexpr int kMusPercussion = 15;
constexpr int kMidiPercussion = 9;

enum MusKind { Release = 0, Play = 1, Bend = 2, System = 3, Control = 4, ScoreEnd = 6 };

uint8_t midiChannelFor(int musChannel) {
    if (musChannel == kMusPercussion) return (uint8_t) kMidiPercussion;
    if (musChannel == kMidiPercussion) return (uint8_t) kMusPercussion;
    return (uint8_t) musChannel;
}

constexpr int kCcModulation = 1;
constexpr int kCcVolume = 7;
constexpr int kCcPan = 10;
constexpr int kCcExpression = 11;
constexpr int kCcSustain = 64;
constexpr int kCcSoftPedal = 67;
constexpr int kCcReverb = 91;
constexpr int kCcChorus = 93;
constexpr int kCcAllSoundOff = 120;
constexpr int kCcResetControllers = 121;
constexpr int kCcAllNotesOff = 123;
constexpr int kCcMonoModeOn = 126;
constexpr int kCcPolyModeOn = kSevenBitMax;

int controllerFor(int musController) {
    switch (musController) {
        case 2: return kCcModulation;
        case 3: return kCcVolume;
        case 4: return kCcPan;
        case 5: return kCcExpression;
        case 6: return kCcReverb;
        case 7: return kCcChorus;
        case 8: return kCcSustain;
        case 9: return kCcSoftPedal;
        default: break;
    }
    return -1;
}

int systemControllerFor(int musController) {
    switch (musController) {
        case 10: return kCcAllSoundOff;
        case 11: return kCcAllNotesOff;
        case 12: return kCcMonoModeOn;
        case 13: return kCcPolyModeOn;
        case 14: return kCcResetControllers;
        default: break;
    }
    return -1;
}

struct Reader {
    const uint8_t* d;
    size_t n;
    size_t at = 0;
    bool bad = false;

    bool has(size_t count) const { return at + count <= n; }
    uint8_t byte() {
        if (!has(1)) { bad = true; return 0; }
        return d[at++];
    }
    uint16_t word(size_t off) const {
        return (uint16_t) (d[off] | (d[off + 1] << 8));
    }
};

}

bool parseMus(const void* data, size_t n, Song& out) {
    if (!looksLikeMus(data, n) || n < 16) return false;
    const auto* d = static_cast<const uint8_t*>(data);
    Reader r{d, n};
    const size_t scoreStart = r.word(6);
    if (scoreStart >= n) return false;

    out.events.clear();
    out.lyrics.clear();
    out.tempo.assign(1, {0.0, kMusBpm});
    out.program.assign(kChannels, 0);

    r.at = scoreStart;
    double ticks = 0.0;
    uint8_t volume[kChannels];
    std::fill(std::begin(volume), std::end(volume), (uint8_t) 100);
    bool programSeen[kChannels] = {};
    bool ended = false;

    while (!ended && !r.bad && r.has(1)) {
        const uint8_t desc = r.byte();
        const bool delayFollows = (desc & 0x80) != 0;
        const int kind = (desc >> 4) & 0x07;
        const int musChannel = desc & 0x0f;
        const uint8_t channel = midiChannelFor(musChannel);
        const double beat = ticks / kTicksPerBeat;

        switch (kind) {
            case Release: {
                const uint8_t note = (uint8_t) (r.byte() & 0x7f);
                out.events.push_back({beat, Kind::NoteOff, channel, note, 0});
                break;
            }
            case Play: {
                const uint8_t first = r.byte();
                const uint8_t note = (uint8_t) (first & 0x7f);
                if ((first & 0x80) != 0) volume[channel] = (uint8_t) (r.byte() & 0x7f);
                out.events.push_back({beat, Kind::NoteOn, channel, note, volume[channel]});
                break;
            }
            case Bend: {
                const int raw = r.byte();
                const int wheel = juce::jlimit(0, 16383, raw * 64);
                out.events.push_back({beat, Kind::PitchBend, channel,
                                      (uint8_t) (wheel & 0x7f), (uint8_t) ((wheel >> 7) & 0x7f)});
                break;
            }
            case System: {
                const int cc = systemControllerFor(r.byte() & 0x7f);
                if (cc == kCcAllNotesOff) out.events.push_back({beat, Kind::AllNotesOff, channel, 0, 0});
                else if (cc >= 0)
                    out.events.push_back({beat, Kind::Controller, channel, (uint8_t) cc, 0});
                break;
            }
            case Control: {
                const int which = r.byte() & 0x7f;
                const uint8_t value = (uint8_t) (r.byte() & 0x7f);
                if (which == 0) {
                    out.events.push_back({beat, Kind::Program, channel, value, 0});
                    if (!programSeen[channel]) {
                        out.program[channel] = value;
                        programSeen[channel] = true;
                    }
                } else if (const int cc = controllerFor(which); cc >= 0)
                    out.events.push_back({beat, Kind::Controller, channel, (uint8_t) cc, value});
                break;
            }
            case ScoreEnd: ended = true; break;
            default: break;
        }

        if (delayFollows && !ended) {
            double delay = 0.0;
            for (int guard = 0; guard < 5; ++guard) {
                const uint8_t b = r.byte();
                delay = delay * 128.0 + (double) (b & 0x7f);
                if ((b & 0x80) == 0) break;
            }
            ticks += delay;
        }
    }

    if (r.bad || out.events.empty()) return false;
    std::stable_sort(out.events.begin(), out.events.end(),
                     [](const Event& a, const Event& b) { return a.beat < b.beat; });
    out.lengthBeats = out.events.back().beat;
    return true;
}

}
