// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Morse/Morse.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void Morse::emit(int offset, bool on, int note) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, kMidiMax);
    e.data[2] = (unsigned char) (on ? 100 : 0);
    e.size = 3;
    e.sampleOffset = offset;
    outEvents_[(size_t) outCount_++] = e;
}

void Morse::process(const float* const*, int, float* const* out, int numOut,
                    int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* o = out[0];
    for (int c = 1; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    auto closeKey = [&] {
        if (keyWasOn_) { emit(0, false, midiNote_); keyWasOn_ = false; }
    };

    if (pendingPattern_.adopt(pattern_)) {
        closeKey();
        reset();
    }

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    if (!transport.playing() || pattern_.empty()) {
        std::fill(o, o + numSamples, 0.0f);
        closeKey();
        reset();
        runNow_.store(-1, std::memory_order_relaxed);
        return;
    }

    const bool sync = params.get("Sync", 0.0) >= 0.5;
    const double wpm = std::clamp(params.get("WPM", 15.0), 5.0, 40.0);
    const int note = (int) params.get("Note", 74.0);
    const bool loop = params.get("Loop", 1.0) >= 0.5;
    const float level = (float) params.get("Level", 0.9);

    const double tempo = transport.tempo() > 0.0 ? transport.tempo() : 120.0;
    const double unit = sync ? sr * kSecondsPerMinute / (tempo * kUnitsPerBeat) : sr * 1.2 / wpm;
    const double f0 = transport.tuning().hz(note);
    const double dt = f0 / sr;
    const float ramp = 1.0f - std::exp((float) (-1.0 / (0.003 * sr)));
    if (sync) placeAt(transport.beats() * kUnitsPerBeat, unit, loop);

    for (int i = 0; i < numSamples; ++i) {
        bool key = false;
        if (!done_) {
            if (seg_ >= pattern_.size()) {
                if (!loop) done_ = true;
                else if (++segPos_ >= (long) (kLoopGapUnits * unit)) { seg_ = 0; segPos_ = 0; }
            }
            if (seg_ < pattern_.size()) {
                const auto& s = pattern_[seg_];
                key = s.on;
                if (++segPos_ >= (long) ((double) s.units * unit)) { ++seg_; segPos_ = 0; }
            }
        }
        if (key != keyWasOn_) {
            if (key) { midiNote_ = note; emit(i, true, midiNote_); }
            else emit(i, false, midiNote_);
            keyWasOn_ = key;
        }
        amp_ += ((key ? 1.0f : 0.0f) - amp_) * ramp;
        phase_ += dt;
        if (phase_ >= 1.0) phase_ -= 1.0;
        o[i] = (float) std::sin(phase_ * kTwoPi) * amp_ * level;
    }
    runNow_.store(done_ || seg_ >= pattern_.size() ? -1 : (int) seg_, std::memory_order_relaxed);
}

void Morse::placeAt(double units, double unitSamples, bool loop) {
    long total = 0;
    for (const auto& s : pattern_) total += s.units;
    const long cycle = total + (loop ? kLoopGapUnits : 0);
    if (cycle <= 0) return;
    if (!loop && units >= (double) total) { done_ = true; seg_ = pattern_.size(); return; }
    double at = loop ? units - std::floor(units / (double) cycle) * (double) cycle : units;
    done_ = false;
    seg_ = 0;
    while (seg_ < pattern_.size() && at >= (double) pattern_[seg_].units) at -= pattern_[seg_++].units;
    segPos_ = (long) (at * unitSamples);
}

}
