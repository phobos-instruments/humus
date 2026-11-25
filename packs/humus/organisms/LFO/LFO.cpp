// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "LFO/LFO.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

constexpr double kTracedCycles = 4.0;
constexpr double kTracedSteppedSeconds = 8.0;

}

void LfoGen::reset() {
    lfo_.reset();
    prevPhase_ = 1.0;
    seconds_ = beats_ = 0.0;
    traced_ = false;
    formula_.restart(name_);
    trace_.reset();
}

LfoFormula::Moment LfoGen::momentFor(double rate, bool sync, double syncBeats, const Transport& transport) {
    if (formula_.adopt() || !traced_) trace_.reset();
    traced_ = true;
    LfoFormula::Moment m;
    m.freq = sync ? transport.tempo() / kSecondsPerMinute / syncBeats : rate;
    m.bpm = transport.tempo();
    m.sampleRate = sampleRate_;
    m.x = (float) params.get("X", 0.5);
    m.y = (float) params.get("Y", 0.5);
    m.z = (float) params.get("Z", 0.5);
    m.w = (float) params.get("W", 0.5);
    m.seconds = transport.playing() ? (double) transport.samplePosition() / sampleRate_ : seconds_;
    m.beats = transport.playing() ? transport.beats() : beats_;
    const double window = (formula_.steps() ? kTracedSteppedSeconds : kTracedCycles) / std::max(m.freq, 1e-3);
    trace_.setSlice((int) (std::clamp(window, 0.25, 60.0) * sampleRate_ / LfoTrace::kPoints));
    return m;
}

void LfoGen::process(const float* const* in, int numIn, float* const* out, int numOut,
                     int numSamples, const Transport& transport) {
    const double rate = std::clamp(params.get("Rate", 1.0), 0.01, 1000.0);
    const int wave = std::clamp((int) params.get("Waveform", 0.0), 0, kLfoFunction);
    const double amp = std::clamp(params.get("Amplitude", 1.0), 0.0, 1.0);
    const double off = std::clamp(params.get("Offset", 0.0), -1.0, 1.0);
    const bool sync = params.get("Sync", 0.0) >= 0.5;
    const double beats = std::max(0.0625, params.get("SyncBeats", 1.0));

    if (sync) lfo_.setPeriodSamples(std::max(1.0, transport.samplesPerBeat() * beats));
    else      lfo_.setRate(rate, sampleRate_);
    if (sync && transport.playing()) lfo_.reset(Lfo::wrap(transport.beats() / beats));

    const bool table = wave == kLfoFunction && params.get("FunctionKind", 0.0) >= 0.5;
    const bool function = wave == kLfoFunction && !table;
    kind_.store(table ? Kind::Cycle : function ? Kind::Trace : Kind::Params, std::memory_order_relaxed);
    LfoFormula::Moment m;
    if (function) m = momentFor(rate, sync, beats, transport);
    else traced_ = false;
    if (table) {
        table_.adopt();
        table_.glideTo(params.get("Position", 0.0));
    }
    const double dt = 1.0 / sampleRate_;
    const double beatsPerSample = transport.tempo() / kSecondsPerMinute / sampleRate_;

    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    float* dst = numOut > 0 ? out[0] : nullptr;
    double lastV = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        const double p = lfo_.tick();
        if (wave == 5 && p < prevPhase_) {
            rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
            held_ = (double) (rng_ & 0xFFFFFF) / (double) 0x7FFFFF - 1.0;
        }
        prevPhase_ = p;
        double v;
        if (table) {
            v = table_.at(p);
        } else if (function) {
            m.phase = p;
            m.input = src ? src[i] : 0.0;
            v = formula_.at(m);
            m.seconds += dt;
            m.beats += beatsPerSample;
        } else {
            v = wave == 1 ? Lfo::triangle(p)
              : wave == 2 ? Lfo::square(p)
              : wave == 3 ? Lfo::sawUp(p)
              : wave == 4 ? Lfo::sawDown(p)
              : wave == 5 ? held_
                          : Lfo::sine(p);
        }
        if (dst) dst[i] = (src ? src[i] : 0.0f) + (float) (off + amp * v);
        if (function) trace_.add(off + amp * v);
        lastV = v;
    }
    if (function) {
        seconds_ = m.seconds;
        beats_ = m.beats;
    }
    if (table) table_.publishCycle();
    for (int c = 1; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    if (numSamples > 0) {
        ctl_.store((float) std::clamp((lastV + 1.0) * 0.5, 0.0, 1.0), std::memory_order_relaxed);
        phase_.store((float) lfo_.phase(), std::memory_order_relaxed);
    }
}

}
