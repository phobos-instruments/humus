// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "hum/caps/Midi.h"
#include "hum/Tuning.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class BendRetuner {
public:
    static constexpr int kChannels = 16;

    void reset() {
        for (auto& v : ch_) v = {};
        next_ = 0;
    }

    int rewrite(const MidiEvent* in, int count, const Tuning& t,
                MidiEvent* out, int outCap) {
        int n = 0;
        auto put = [&](int offset, unsigned char a, unsigned char b, unsigned char c) {
            if (n >= outCap) return;
            out[n].sampleOffset = offset;
            out[n].data[0] = a;
            out[n].data[1] = b;
            out[n].data[2] = c;
            out[n].size = 3;
            ++n;
        };

        for (int i = 0; i < count; ++i) {
            const auto& e = in[i];
            const int st = e.data[0] & 0xF0;
            if (st == 0x90 && e.data[2] > 0) {
                const double hz = t.hz((double) e.data[1]);
                double m = hz > 0.0 ? hzToMidi(hz) : (double) e.data[1];
                if (m < 0.0) m = 0.0;
                if (m > kMidiMaxD) m = kMidiMaxD;
                const int outNote = (int) std::lround(m);
                const double residue = m - (double) outNote;
                int bend = 8192 + (int) std::lround(residue / 2.0 * 8192.0);
                if (bend < 0) bend = 0;
                if (bend > 16383) bend = 16383;

                const int c = allocate(e.data[1], outNote);
                put(e.sampleOffset, (unsigned char) (0xE0 | c),
                    (unsigned char) (bend & 0x7F), (unsigned char) ((bend >> 7) & 0x7F));
                put(e.sampleOffset, (unsigned char) (0x90 | c),
                    (unsigned char) outNote, e.data[2]);
            } else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) {
                const int c = release(e.data[1]);
                if (c >= 0)
                    put(e.sampleOffset, (unsigned char) (0x80 | c),
                        (unsigned char) ch_[(std::size_t) c].outNote, 0);
            } else if (st == 0xB0) {
                for (int c = 0; c < kChannels; ++c)
                    put(e.sampleOffset, (unsigned char) (0xB0 | c), e.data[1], e.data[2]);
            } else {
                if (n < outCap) out[n++] = e;
            }
        }
        return n;
    }

private:
    struct Voice {
        int srcNote = -1;
        int outNote = 0;
        std::uint32_t age = 0;
    };

    int allocate(int srcNote, int outNote) {
        for (int c = 0; c < kChannels; ++c)
            if (ch_[(std::size_t) c].srcNote == srcNote) {
                ch_[(std::size_t) c].outNote = outNote;
                ch_[(std::size_t) c].age = ++stamp_;
                return c;
            }
        for (int c = 0; c < kChannels; ++c)
            if (ch_[(std::size_t) c].srcNote < 0) {
                ch_[(std::size_t) c] = {srcNote, outNote, ++stamp_};
                return c;
            }
        int oldest = 0;
        for (int c = 1; c < kChannels; ++c)
            if (ch_[(std::size_t) c].age < ch_[(std::size_t) oldest].age) oldest = c;
        ch_[(std::size_t) oldest] = {srcNote, outNote, ++stamp_};
        return oldest;
    }

    int release(int srcNote) {
        for (int c = 0; c < kChannels; ++c)
            if (ch_[(std::size_t) c].srcNote == srcNote) {
                ch_[(std::size_t) c].srcNote = -1;
                return c;
            }
        return -1;
    }

    Voice ch_[kChannels];
    int next_ = 0;
    std::uint32_t stamp_ = 0;
};

}
