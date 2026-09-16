// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "hum/Meter.h"

namespace hum::trackslayout {

enum class Kind { Track, AutoLane, PodHeader, BoxRow };

struct Slot {
    int track = -1;
    Kind kind = Kind::Track;
    std::string param;
    std::string laneKind;
    int y = 0, h = 0;
};

struct AutoLaneInfo { std::string param, kind; };

inline std::vector<Slot> build(int nTracks,
        const std::function<bool(int)>& expanded,
        const std::function<std::vector<AutoLaneInfo>(int)>& lanesOf,
        int top, int trackH, int laneH,
        const std::function<std::string(int)>& podOf = {},
        const std::function<bool(const std::string&)>& podCollapsed = {},
        int podH = 0,
        const std::function<bool(int)>& boxRow = {},
        int boxRowH = 0) {
    std::vector<Slot> out;
    int y = top;
    std::string openPod;
    for (int t = 0; t < nTracks; ++t) {
        const std::string pod = podOf ? podOf(t) : std::string();
        if (pod != openPod) {
            openPod = pod;
            if (!pod.empty()) {
                out.push_back({-1, Kind::PodHeader, pod, "", y, podH > 0 ? podH : trackH});
                y += podH > 0 ? podH : trackH;
            }
        }
        if (!pod.empty() && podCollapsed && podCollapsed(pod)) continue;
        if (trackH > 0) {
            out.push_back({t, Kind::Track, "", "", y, trackH});
            y += trackH;
        }
        if (boxRow && boxRowH > 0 && boxRow(t)) {
            out.push_back({t, Kind::BoxRow, "", "", y, boxRowH});
            y += boxRowH;
        }
        if (expanded(t))
            for (const auto& l : lanesOf(t)) {
                out.push_back({t, Kind::AutoLane, l.param, l.kind, y, laneH});
                y += laneH;
            }
    }
    return out;
}

inline int podRowCount(int nTracks, const std::function<std::string(int)>& podOf,
                       const std::string& pod) {
    int n = 0;
    for (int t = 0; t < nTracks; ++t)
        if (podOf && podOf(t) == pod) ++n;
    return n;
}

inline int totalHeight(const std::vector<Slot>& slots, int top) {
    return slots.empty() ? top : slots.back().y + slots.back().h;
}

inline int slotAt(const std::vector<Slot>& slots, int y) {
    for (int i = 0; i < (int) slots.size(); ++i)
        if (y >= slots[(size_t) i].y && y < slots[(size_t) i].y + slots[(size_t) i].h) return i;
    return -1;
}

inline int trackY(const std::vector<Slot>& slots, int track) {
    for (const auto& s : slots)
        if (s.kind == Kind::Track && s.track == track) return s.y;
    return -1;
}

inline double gridBeats(double pixelsPerBeat, Meter meter) {
    const double bar = meter.quarterNotesPerBar();
    const double unit = meter.quarterNotesPerBeat();
    for (const double div : {unit * 0.25, unit * 0.5, unit})
        if (div * pixelsPerBeat >= 10.0) return std::min(div, bar);
    return bar;
}

inline double gridBeats(double pixelsPerBeat, int beatsPerBar) {
    return gridBeats(pixelsPerBeat, Meter{beatsPerBar > 0 ? beatsPerBar : 4, 4});
}

inline double snapBeats(double beat, double pixelsPerBeat, Meter meter, double barStart, bool bypass) {
    if (bypass) return std::max(0.0, beat);
    const double res = gridBeats(pixelsPerBeat, meter);
    return std::max(0.0, barStart + std::round((beat - barStart) / res) * res);
}

inline double snapBeats(double beat, double pixelsPerBeat, int beatsPerBar, bool bypass) {
    return snapBeats(beat, pixelsPerBeat, Meter{beatsPerBar > 0 ? beatsPerBar : 4, 4}, 0.0, bypass);
}

inline std::string gridLabel(double beats) {
    if (beats <= 0.07) return "1/64";
    if (beats <= 0.13) return "1/32";
    if (beats <= 0.26) return "1/16";
    if (beats <= 0.51) return "1/8";
    if (beats <= 1.01) return "1/4";
    if (beats <= 2.01) return "1/2";
    return "Bar";
}

}
