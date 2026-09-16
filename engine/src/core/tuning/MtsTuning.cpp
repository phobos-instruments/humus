// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/tuning/MtsTuning.h"

#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void mtsNoteTriple(double hz, unsigned char out[3]) {
    if (!(hz > 0.0) || !std::isfinite(hz)) {
        out[0] = out[1] = out[2] = 0x7F;
        return;
    }
    double m = hzToMidi(hz);
    if (m < 0.0) m = 0.0;
    if (m > kMidiMaxD + 16382.0 / 16384.0) m = kMidiMaxD + 16382.0 / 16384.0;
    int semi = (int) std::floor(m);
    int frac = (int) std::lround((m - (double) semi) * 16384.0);
    if (frac >= 16384) { ++semi, frac = 0; }
    if (semi > kMidiMax) { semi = kMidiMax; frac = 16382; }
    if (semi == 0x7F && frac == 16383) frac = 16382;
    out[0] = (unsigned char) semi;
    out[1] = (unsigned char) ((frac >> 7) & 0x7F);
    out[2] = (unsigned char) (frac & 0x7F);
}

std::vector<juce::MidiMessage> mtsRetuneMessages(const Tuning& t) {
    std::vector<juce::MidiMessage> out;
    out.reserve(2);
    for (int half = 0; half < 2; ++half) {
        const int first = half * 64;
        unsigned char buf[8 + 64 * 4 + 1];
        int n = 0;
        buf[n++] = 0xF0;
        buf[n++] = 0x7F;
        buf[n++] = 0x7F;
        buf[n++] = 0x08;
        buf[n++] = 0x02;
        buf[n++] = 0x00;
        buf[n++] = 64;
        for (int k = first; k < first + 64; ++k) {
            buf[n++] = (unsigned char) k;
            mtsNoteTriple(t.hz((double) k), buf + n);
            n += 3;
        }
        buf[n++] = 0xF7;
        out.push_back(juce::MidiMessage(buf, n, 0.0));
    }
    return out;
}

}
