// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Atom/Atom.h"

#include <cmath>
#include <cstddef>

#include "hum/Euclid.h"
#include "hum/Pattern.h"
#include "hum/Scale.h"
#include "hum/Swing.h"
#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

constexpr long kShortestNote = 32;

double beatsPerStep(int rate, int steps, const Transport& transport) {
    if (rate == Atom::kFitBar) return transport.beatsPerBar() / (double) std::max(1, steps);
    return 1.0 / (double) (1 << std::clamp(rate, 0, 3));
}

}

void Atom::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
}

void Atom::reset() {
    held_.clear();
    turns_.fill(0);
    for (auto& n : now_) n.store(-1, std::memory_order_relaxed);
    stagedCount_ = 0;
    outCount_ = 0;
    offCount_ = 0;
}

Atom::Ring Atom::ringAt(int ring) const {
    const auto r = (size_t) ring;
    Ring shape;
    shape.steps = std::clamp((int) std::lround(stepsRef_[r].get(params, 16.0)), 1, kMaxSteps);
    shape.hits = std::clamp((int) std::lround(hitsRef_[r].get(params, 0.0)), 0, shape.steps);
    shape.rotate = (int) std::lround(rotateRef_[r].get(params, 0.0));
    shape.rate = std::clamp((int) std::lround(rateRef_[r].get(params, 2.0)), 0, (int) kFitBar);
    shape.fixed = fixedRef_[r].on(params);
    shape.note = std::clamp((int) std::lround(noteRef_[r].get(params, 60.0)), 0, kMidiMax);
    return shape;
}

int Atom::ringSteps(int ring) const { return ringAt(ring).steps; }

bool Atom::ringHits(int ring, int step) const {
    const auto shape = ringAt(ring);
    return euclid::hit(step, shape.steps, shape.hits, shape.rotate);
}

void Atom::emit(int offset, bool on, int note, int velocity) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, kMidiMax);
    e.data[2] = (unsigned char) (on ? std::clamp(velocity, 1, kMidiMax) : 0);
    e.size = 3;
    e.sampleOffset = offset;
    outEvents_[(size_t) outCount_++] = e;
}

void Atom::hear(bool latch) {
    if (wasLatched_ && !latch) held_.unlatch();
    wasLatched_ = latch;
    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(size_t) i];
        const int kind = e.data[0] & 0xF0;
        const bool on = kind == 0x90 && e.data[2] > 0;
        const bool off = kind == 0x80 || (kind == 0x90 && e.data[2] == 0);
        if (on) held_.press(e.data[1], latch);
        else if (off) held_.release(e.data[1], latch);
        else if (outCount_ < (int) outEvents_.size()) outEvents_[(size_t) outCount_++] = e;
    }
    stagedCount_ = 0;
}

void Atom::releaseDue(int numSamples) {
    for (int i = 0; i < offCount_;) {
        auto& due = offs_[(size_t) i];
        if (due.samplesLeft < numSamples) {
            emit((int) due.samplesLeft, false, due.note, 0);
            due = offs_[(size_t) --offCount_];
        } else {
            due.samplesLeft -= numSamples;
            ++i;
        }
    }
}

void Atom::hush() {
    for (int i = 0; i < offCount_; ++i) emit(0, false, offs_[(size_t) i].note, 0);
    offCount_ = 0;
    turns_.fill(0);
    for (auto& n : now_) n.store(-1, std::memory_order_relaxed);
}

void Atom::strike(int ring, const Ring& shape, const rings::Pool& pool, int at, long length) {
    const auto r = (size_t) ring;
    const int order = (int) std::lround(params.get("Order", 0.0));
    if (!shape.fixed && pool.count == 0) return;
    const int note = shape.fixed ? shape.note : pool.notes[(size_t) rings::pick(pool.count, order, turns_[r], random_)];
    ++turns_[r];
    for (int i = 0; i < offCount_; ++i) {
        if (offs_[(size_t) i].note != note) continue;
        emit(at, false, note, 0);
        offs_[(size_t) i] = offs_[(size_t) --offCount_];
        break;
    }
    emit(at, true, note, (int) std::lround(params.get("Velocity", 100.0)));
    if (offCount_ < (int) offs_.size()) offs_[(size_t) offCount_++] = {note, (long) at + std::max(kShortestNote, length)};
    fired_[r].fetch_add(1, std::memory_order_relaxed);
}

void Atom::turn(int ring, const Ring& shape, const rings::Pool& pool, int numSamples, const Transport& transport) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double stepBeats = beatsPerStep(shape.rate, shape.steps, transport);
    const bool fit = shape.rate == kFitBar;
    const double origin = fit ? transport.barStartBefore(transport.beats()) : 0.0;
    const double stepsPerSec = transport.tempo() / kSecondsPerMinute / stepBeats;
    const double ticksPerStep = (double) Pattern::kTicksPerBeat * stepBeats;
    const auto groove = swing::resolve(params, transport, (int) std::lround(ticksPerStep));
    const double lag = fit ? 0.0 : swing::cellFraction(groove.amount);
    const double step0 = (transport.beats() - origin) / stepBeats;
    const double stepEnd = step0 + (double) numSamples * stepsPerSec / sr;
    const double gate = std::clamp(params.get("Gate", 0.6), 0.05, 1.0);
    const long length = (long) (gate * sr / stepsPerSec);

    now_[(size_t) ring].store((int) ((long long) std::floor(step0) % shape.steps), std::memory_order_relaxed);
    for (long long k = (long long) std::floor(step0 - lag) - 1; k < (long long) std::ceil(stepEnd) + 1; ++k) {
        const double pos = (double) k + (fit ? 0.0 : swing::delaySteps((double) k, ticksPerStep, groove));
        if (pos < step0 || pos >= stepEnd || k < 0) continue;
        if (!euclid::hit(k, shape.steps, shape.hits, shape.rotate)) continue;
        strike(ring, shape, pool, std::min(numSamples - 1, (int) std::lround((pos - step0) / stepsPerSec * sr)), length);
    }
}

void Atom::process(const float* const*, int, float* const*, int, int numSamples, const Transport& transport) {
    hear(params.get("Latch", 0.0) >= 0.5);
    releaseDue(numSamples);
    const bool moved = transport.seekStamp() != seekSeen_;
    seekSeen_ = transport.seekStamp();
    if (!transport.playing() || moved) {
        if (wasPlaying_ || moved) hush();
        wasPlaying_ = transport.playing();
        if (!transport.playing()) return;
    }
    wasPlaying_ = true;

    const int key = std::clamp((int) std::lround(params.get("Key", 0.0)), 0, 11);
    const auto scale = Scale::byId((int) std::lround(params.get("Scale", 0.0)));
    const int order = (int) std::lround(params.get("Order", 0.0));
    const auto pool = rings::poolOf(held_, scale, transport.tuning(), key,
                                    (int) std::lround(params.get("Octaves", 1.0)), order == rings::kPlayed);
    for (int ring = 0; ring < kRings; ++ring) turn(ring, ringAt(ring), pool, numSamples, transport);
}

}
