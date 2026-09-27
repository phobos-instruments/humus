// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/readouts/ScopeReadings.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace hum::readout {

bool StereoField::poll(ModelHost& host, const std::string& organism) {
    auto* src = live::source<StereoFieldSource>(host, organism);
    if (src == nullptr) return false;
    const unsigned stamp = src->fieldStamp();
    if (stamp != lastStamp_) {
        lastStamp_ = stamp;
        pairs_ = src->fieldRead(lr_, kMaxPairs);
        corr_ = src->fieldCorrelation();
        fade_ = 1.0f;
        cloud_ = pairs_ > 0;
        return true;
    }
    if (!cloud_) return false;
    fade_ *= 0.82f;
    if (fade_ < 0.03f) {
        cloud_ = false;
        fade_ = 0.0f;
    }
    return true;
}

bool LfoCycle::poll(ModelHost& host, const std::string& organism, int width) {
    float ph = phase_, held = held_;
    const live::Controls controls(host, organism);
    ph = controls.valueOr("phase", ph);
    held = controls.valueOr("wave", held);
    const int w = std::max(1, width);
    if ((int) (ph * w) == (int) (phase_ * w) && std::abs(held - held_) < 0.004f) return false;
    phase_ = ph;
    held_ = held;
    return true;
}

LfoCycle::Shape LfoCycle::shape(ModelHost& host, const std::string& organism) const {
    return {(int) std::clamp(host.liveParamValue(organism, waveform_), 0.0, 5.0),
            std::clamp(host.liveParamValue(organism, amplitude_), 0.0, 1.0),
            std::clamp(host.liveParamValue(organism, offset_), -1.0, 1.0)};
}

double LfoCycle::valueAt(const Shape& s, double phase) const {
    const double v = s.wave == 5 ? (double) held_ * 2.0 - 1.0 : shapeAt(s.wave, phase);
    return s.offset + s.amplitude * v;
}

double LfoCycle::shapeAt(int wave, double p) {
    return wave == 1 ? Lfo::triangle(p)
         : wave == 2 ? Lfo::square(p)
         : wave == 3 ? Lfo::sawUp(p)
         : wave == 4 ? Lfo::sawDown(p)
                     : Lfo::sine(p);
}

std::vector<double> Harmonics::read(ModelHost& host, const std::string& organism, double& peak) const {
    std::vector<double> a(amps_.size());
    peak = 0.0;
    for (size_t k = 0; k < amps_.size(); ++k) {
        a[k] = host.liveParamValue(organism, amps_[k]);
        peak += std::abs(a[k]);
    }
    if (peak <= 1e-9) peak = 1.0;
    return a;
}

double Harmonics::valueAt(const std::vector<double>& a, double peak, double t) {
    double y = 0.0;
    for (size_t k = 0; k < a.size(); ++k) y += a[k] * std::sin(2.0 * kPi * (double) (k + 1) * t);
    return y / peak;
}

bool Pitch::poll(ModelHost& host, const std::string& organism) {
    auto* src = live::source<PitchDetectSource>(host, organism);
    const float hz = src ? src->detectedHz() : 0.0f;
    const float lv = src ? src->detectLevel() : 0.0f;
    const float cl = src ? src->detectClarity() : 0.0f;
    const int nt = src ? src->detectedNote() : -1;
    std::vector<int> chord;
    if (auto* poly = live::source<ChordDetectSource>(host, organism)) {
        int notes[ChordDetectSource::kMaxChordNotes] = {};
        const int found = poly->chordNotes(notes, ChordDetectSource::kMaxChordNotes);
        chord.assign(notes, notes + std::max(0, found));
    }
    if (std::abs(hz - hz_) <= 0.2f && std::abs(lv - level_) <= 0.01f && std::abs(cl - clarity_) <= 0.01f
        && nt == note_ && chord == chord_)
        return false;
    hz_ = hz;
    level_ = lv;
    clarity_ = cl;
    note_ = nt;
    chord_ = std::move(chord);
    return true;
}

bool Timecode::poll(ModelHost& host, const std::string& organism) {
    auto* src = live::source<TimecodeStatus>(host, organism);
    const bool on = src != nullptr && src->timecodeOn();
    const bool there = src != nullptr && src->timecodePresent();
    const float lv = src != nullptr ? src->timecodeLevel() : 0.0f;
    const float sp = src != nullptr ? src->timecodeSpeed() : 0.0f;
    const float hz = src != nullptr ? src->timecodeCarrierHz() : 0.0f;
    if (on == on_ && there == present_ && std::abs(lv - level_) <= 0.002f
        && std::abs(sp - speed_) <= 0.002f && std::abs(hz - carrier_) <= 0.5f)
        return false;
    on_ = on;
    present_ = there;
    level_ = lv;
    speed_ = sp;
    carrier_ = hz;
    return true;
}

double Pitch::cents() const {
    const double midi = hzToMidi((double) hz_);
    return (midi - std::round(midi)) * 100.0;
}

Pitch::Tuning Pitch::tuning() const {
    const double ac = std::abs(cents());
    return ac < 5.0 ? Tuning::InTune : ac < 20.0 ? Tuning::Close : Tuning::Off;
}

int Sigil::draw(ModelHost& host, const std::string& organism, double seconds) {
    auto* src = live::source<SigilSource>(host, organism);
    return src == nullptr ? -1 : src->sigil(prims_, kMaxPrims, seconds);
}

}
