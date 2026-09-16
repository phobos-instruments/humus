// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#include "hum/PatternMatrix.h"
#include "hum/dsp/PitchTrack.h"

#include "hum/dsp/DspMath.h"

namespace hum::melodytrace {

struct Frame {
    double midi = 0.0;
    double level = 0.0;
    bool voiced = false;
};

using Watch = std::function<bool(double)>;

inline std::vector<Frame> frames(const float* mono, int n, double sampleRate,
                                 const Watch& watch = {}) {
    std::vector<Frame> out;
    PitchTracker pt;
    pt.prepare(sampleRate);
    const int hop = PitchTracker::hopSamples();
    int sinceWatch = 0;
    for (int at = 0; at < n; at += hop) {
        if (watch && ++sinceWatch >= 64) {
            sinceWatch = 0;
            if (!watch(n > 0 ? (double) at / (double) n : 1.0)) return {};
        }
        const int take = std::min(hop, n - at);
        const int fresh = pt.push(mono + at, take);
        for (int f = 0; f < fresh; ++f) {
            Frame fr;
            fr.level = pt.level();
            if (pt.pitchHz() > 0.0 && pt.clarity() > 0.5) {
                fr.midi = hzToMidi(pt.pitchHz());
                fr.voiced = fr.midi > 12.0 && fr.midi < 120.0;
            }
            out.push_back(fr);
        }
    }
    return out;
}

inline std::vector<NoteEvent> notesFromFrames(const std::vector<Frame>& fr,
                                              double hopTicks) {
    double peak = 0.0;
    for (const auto& f : fr) peak = std::max(peak, f.level);
    const double gate = std::max(1.0e-4, peak * 0.04);

    std::vector<NoteEvent> out;
    const int minFrames = 3;
    int start = -1;
    double sumMidi = 0.0, sumLevel = 0.0;
    int count = 0, silent = 0;

    auto flush = [&](int endFrame) {
        if (start >= 0 && count >= minFrames) {
            NoteEvent n;
            n.tick = (int) std::lround(start * hopTicks);
            n.lengthTicks = std::max(6, (int) std::lround((endFrame - start) * hopTicks));
            n.pitch = std::clamp((int) std::lround(sumMidi / count), 0, kMidiMax);
            const double loud = std::sqrt(std::min(1.0, sumLevel / count / std::max(1.0e-9, peak)));
            n.velocity = std::clamp((int) std::lround(30.0 + 97.0 * loud), 1, kMidiMax);
            out.push_back(n);
        }
        start = -1;
        sumMidi = sumLevel = 0.0;
        count = 0;
        silent = 0;
    };

    for (int i = 0; i < (int) fr.size(); ++i) {
        const Frame& f = fr[(size_t) i];
        const bool on = f.voiced && f.level > gate;
        if (!on) {
            if (start >= 0 && ++silent > 1) flush(i - silent + 1);
            continue;
        }
        silent = 0;
        if (start >= 0 && count > 0
            && std::abs(f.midi - sumMidi / count) > 0.7) flush(i);
        if (start < 0) start = i;
        sumMidi += f.midi;
        sumLevel += f.level;
        ++count;
    }
    flush((int) fr.size());
    return out;
}

inline std::vector<NoteEvent> trace(const float* mono, int n, double sampleRate,
                                    double ticksPerSecond, const Watch& watch = {}) {
    const double hopTicks =
        ticksPerSecond * (double) PitchTracker::hopSamples() / sampleRate;
    return notesFromFrames(frames(mono, n, sampleRate, watch), hopTicks);
}

}
