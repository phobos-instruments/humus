#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "hum/ClipStack.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"
#include "hum/Swing.h"

namespace hum::noteschedule {

struct Voice {
    int start = 0, len = 0;
    bool loop = false;
    int gateFrom = 0, gateTo = 0x7fffffff;
    double swing = -1.0;
    std::vector<NoteEvent> notes;
    std::vector<CCEvent> ccs;
};

struct Edge { int offset = 0; bool on = false; int pitch = 0; int vel = 0; int cc = -1; };

inline std::vector<Voice> prepare(const Pattern& p, int defaultDurationTicks) {
    std::vector<Voice> out;
    for (int i = 0; i < (int) p.channels.size(); ++i) {
        const auto& ch = p.channels[(size_t) i];
        if (ch.type != "note-events") continue;
        Voice base;
        if (ch.startTick < 0) {
            base.start = 0; base.len = defaultDurationTicks; base.loop = true;
        } else {
            base.start = ch.startTick;
            base.len = ch.lengthTicks > 0 ? ch.lengthTicks : defaultDurationTicks;
            base.loop = ch.loopClip;
        }
        base.swing = ch.swing;
        base.notes = decodeNoteEvents(ch.matrix);
        base.ccs = decodeCCEvents(ch.matrix);
        for (const auto& span : clipstack::soundingSpans(p, i)) {
            Voice v = base;
            v.gateFrom = span.startTick;
            v.gateTo = span.startTick + std::max(1, span.lengthTicks);
            out.push_back(v);
        }
    }
    return out;
}

inline int window(const std::vector<Voice>& voices, double fromBeat, double toBeat,
                  int sampleBase, int numSamples, double samplesPerBeat,
                  double velScale, Edge* out, int cap, int n,
                  bool closeNoteOffs = false, swing::Groove groove = {}) {
    const double samplesPerTick = samplesPerBeat / Pattern::kTicksPerBeat;
    const double tickStart = fromBeat * Pattern::kTicksPerBeat;
    const double tickEnd = toBeat * Pattern::kTicksPerBeat;
    if (tickEnd <= tickStart) return n;
    auto push = [&](double fire, bool on, int pitch, int vel, int cc) {
        if (n >= cap) return;
        const int off = std::clamp(
            sampleBase + (int) std::lround((fire - tickStart) * samplesPerTick),
            0, numSamples - 1);
        out[n++] = {off, on, pitch, vel, cc};
    };
    auto fits = [&](double fire, bool on, int cc) {
        if (fire < tickStart) return false;
        if (fire < tickEnd) return true;
        return closeNoteOffs && !on && cc < 0 && fire <= tickEnd + 1.0e-6;
    };
    const double maxDelay = (double) groove.gridTicks / 3.0;
    swing::Groove active = groove;
    auto swung = [&](double fire, int anchorBack) {
        return fire + swing::delayTicks(fire - anchorBack, active);
    };
    auto scheduleLoop = [&](int tick, int period, int minTick, int maxTick, bool on,
                            int pitch, int vel, int cc, int anchorBack = 0) {
        if (period <= 0) return;
        const double k = std::ceil((tickStart - maxDelay - tick) / (double) period);
        for (double fire = tick + k * period; fire <= tickEnd; fire += period) {
            if (fire < minTick || fire >= maxTick) continue;
            const double at = swung(fire, anchorBack);
            if (fits(at, on, cc)) push(at, on, pitch, vel, cc);
        }
    };
    auto scheduleOnce = [&](int tick, bool on, int pitch, int vel, int cc,
                            int anchorBack = 0) {
        const double at = swung((double) tick, anchorBack);
        if (fits(at, on, cc)) push(at, on, pitch, vel, cc);
    };
    for (const auto& c : voices) {
        active = c.swing >= 0.0 ? swing::Groove{c.swing, groove.gridTicks} : groove;
        for (const auto& nt : c.notes) {
            if (nt.tick >= c.len) continue;
            const int vel = std::clamp((int) std::lround(nt.velocity * velScale), 1, 127);
            if (c.loop) {
                scheduleLoop(c.start + nt.tick, c.len, c.gateFrom, c.gateTo,
                             true, nt.pitch, vel, -1);
                scheduleLoop(c.start + nt.tick + nt.lengthTicks, c.len, c.gateFrom,
                             c.gateTo + c.len, false, nt.pitch, 0, -1, nt.lengthTicks);
            } else {
                const int at = c.start + nt.tick;
                if (at < c.gateFrom || at >= c.gateTo) continue;
                scheduleOnce(at, true, nt.pitch, vel, -1);
                const int offAt = std::min({at + nt.lengthTicks, c.start + c.len, c.gateTo});
                scheduleOnce(offAt, false, nt.pitch, 0, -1, offAt - at);
            }
        }
        for (const auto& cc : c.ccs) {
            if (cc.tick >= c.len) continue;
            const int at = c.start + cc.tick;
            if (c.loop) scheduleLoop(at, c.len, c.gateFrom, c.gateTo,
                                     false, cc.controller, cc.value, cc.controller);
            else if (at >= c.gateFrom && at < c.gateTo)
                scheduleOnce(at, false, cc.controller, cc.value, cc.controller);
        }
    }
    return n;
}

inline int collect(const std::vector<Voice>& voices, double beat0, int numSamples,
                   double samplesPerBeat, double velScale, Edge* out, int cap,
                   double loopStart = 0.0, double loopEnd = 0.0,
                   swing::Groove groove = {}) {
    if (out == nullptr || cap <= 0 || numSamples <= 0 || samplesPerBeat <= 0.0) return 0;
    const double beatEnd = beat0 + numSamples / samplesPerBeat;
    if (!(loopEnd > loopStart && beat0 < loopEnd && beatEnd > loopEnd))
        return window(voices, beat0, beatEnd, 0, numSamples, samplesPerBeat,
                      velScale, out, cap, 0, false, groove);
    const int split = (int) std::lround((loopEnd - beat0) * samplesPerBeat);
    int n = window(voices, beat0, loopEnd, 0, numSamples, samplesPerBeat,
                   velScale, out, cap, 0, true, groove);
    return window(voices, loopStart, loopStart + (beatEnd - loopEnd), split,
                  numSamples, samplesPerBeat, velScale, out, cap, n, false, groove);
}

inline void order(Edge* e, int n) {
    auto rank = [](const Edge& x) { return x.cc >= 0 ? 1 : (x.on ? 2 : 0); };
    std::stable_sort(e, e + n, [&](const Edge& a, const Edge& b) {
        return a.offset != b.offset ? a.offset < b.offset : rank(a) < rank(b);
    });
}

}
