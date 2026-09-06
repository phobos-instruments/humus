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

    if (const auto* p = params.byName("Text")) {
        if (p->text != cachedText_) {
            cachedText_ = p->text;
            pattern_ = morse::encode(cachedText_);
            closeKey();
            reset();
        }
    }

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    if (!transport.playing() || pattern_.empty()) {
        std::fill(o, o + numSamples, 0.0f);
        closeKey();
        reset();
        return;
    }

    const bool sync = params.get("Sync", 0.0) >= 0.5;
    const double wpm = std::clamp(params.get("WPM", 15.0), 5.0, 40.0);
    const int note = (int) params.get("Note", 74.0);
    const bool loop = params.get("Loop", 1.0) >= 0.5;
    const float level = (float) params.get("Level", 0.9);

    const double tempo = transport.tempo() > 0.0 ? transport.tempo() : 120.0;
    const double unit = sync ? sr * kSecondsPerMinute / (tempo * 4.0) : sr * 1.2 / wpm;
    const double f0 = transport.tuning().hz(note);
    const double dt = f0 / sr;
    const float ramp = 1.0f - std::exp((float) (-1.0 / (0.003 * sr)));

    for (int i = 0; i < numSamples; ++i) {
        bool key = false;
        if (!done_) {
            if (seg_ >= pattern_.size()) {
                if (!loop) done_ = true;
                else if (++segPos_ >= (long) (7.0 * unit)) { seg_ = 0; segPos_ = 0; }
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
}

}
