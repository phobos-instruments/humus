#pragma once
#include <cmath>
#include <vector>

#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {

struct TimedMidi {
    double beat = 0.0;
    unsigned char status = 0, d1 = 0, d2 = 0;
};

inline std::vector<NoteEvent> assembleRecordedNotes(
    const std::vector<TimedMidi>& events, int durationTicks, int quantizeTicks,
    bool finalizePending, int defaultLenTicks, std::vector<TimedMidi>& remaining,
    double offsetBeats = 0.0, bool wrap = true) {
    remaining.clear();
    std::vector<NoteEvent> out;
    if (durationTicks <= 0) return out;

    struct Pending { double beat; int pitch; int vel; };
    std::vector<Pending> open;

    auto startTick = [&](double beat) {
        long t = std::lround((beat - offsetBeats) * Pattern::kTicksPerBeat);
        if (quantizeTicks > 0)
            t = std::lround(t / (double) quantizeTicks) * quantizeTicks;
        if (wrap) {
            t %= durationTicks;
            if (t < 0) t += durationTicks;
        } else if (t < 0 || t >= durationTicks) {
            return -1L;
        }
        return t;
    };

    for (const auto& e : events) {
        const bool on = (e.status & 0xF0) == 0x90 && e.d2 > 0;
        const bool off = (e.status & 0xF0) == 0x80 || ((e.status & 0xF0) == 0x90 && e.d2 == 0);
        if (on) {
            open.push_back({e.beat, (int) e.d1, (int) e.d2});
        } else if (off) {
            for (int i = (int) open.size() - 1; i >= 0; --i) {
                if (open[(size_t) i].pitch != (int) e.d1) continue;
                const long t = startTick(open[(size_t) i].beat);
                if (t < 0) { open.erase(open.begin() + i); break; }
                NoteEvent n;
                n.tick = (int) t;
                n.pitch = open[(size_t) i].pitch;
                n.velocity = open[(size_t) i].vel;
                const long len = std::lround((e.beat - open[(size_t) i].beat)
                                             * Pattern::kTicksPerBeat);
                n.lengthTicks = (int) (len < 1 ? 1 : len);
                if (wrap) {
                    if (n.lengthTicks >= durationTicks) n.lengthTicks = durationTicks - 1;
                } else if (n.tick + n.lengthTicks > durationTicks) {
                    n.lengthTicks = durationTicks - n.tick;
                }
                out.push_back(n);
                open.erase(open.begin() + i);
                break;
            }
        }
    }

    for (const auto& p : open) {
        if (finalizePending) {
            const long t = startTick(p.beat);
            if (t < 0) continue;
            NoteEvent n;
            n.tick = (int) t;
            n.pitch = p.pitch;
            n.velocity = p.vel;
            n.lengthTicks = defaultLenTicks < 1 ? 1 : defaultLenTicks;
            if (!wrap && n.tick + n.lengthTicks > durationTicks)
                n.lengthTicks = durationTicks - n.tick;
            out.push_back(n);
        } else {
            TimedMidi t;
            t.beat = p.beat;
            t.status = 0x90;
            t.d1 = (unsigned char) p.pitch;
            t.d2 = (unsigned char) p.vel;
            remaining.push_back(t);
        }
    }
    return out;
}

}
