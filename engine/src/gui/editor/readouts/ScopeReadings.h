// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/midi/MidiFormat.h"
#include "gui/editor/LiveControls.h"
#include "gui/editor/readouts/ControlReadings.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Video.h"
#include "hum/dsp/DspMath.h"
#include "hum/dsp/Lfo.h"

namespace hum::readout {

class StereoField {
public:
    static constexpr int kMaxPairs = StereoFieldSource::kFieldPairs;

    bool poll(ModelHost& host, const std::string& organism);

    bool hasCloud() const { return cloud_; }
    int pairs() const { return pairs_; }
    float left(int i) const { return lr_[2 * i]; }
    float right(int i) const { return lr_[2 * i + 1]; }
    float side(int i) const { return (left(i) - right(i)) * kSqrtHalfF; }
    float mid(int i) const { return (left(i) + right(i)) * kSqrtHalfF; }
    float correlation() const { return corr_; }
    float fade() const { return fade_; }

private:
    float lr_[2 * kMaxPairs] = {};
    int pairs_ = 0;
    float corr_ = 0.0f;
    float fade_ = 0.0f;
    bool cloud_ = false;
    unsigned lastStamp_ = 0;
};

class LfoCycle {
public:
    LfoCycle(std::string waveform, std::string amplitude, std::string offset)
        : waveform_(std::move(waveform)), amplitude_(std::move(amplitude)), offset_(std::move(offset)) {}

    bool poll(ModelHost& host, const std::string& organism, int width);

    struct Shape {
        int wave = 0;
        double amplitude = 0.0, offset = 0.0;
    };

    Shape shape(ModelHost& host, const std::string& organism) const;
    double valueAt(const Shape& s, double phase) const;
    static double shapeAt(int wave, double p);

    float phase() const { return phase_; }

private:
    std::string waveform_, amplitude_, offset_;
    float phase_ = 0.0f, held_ = 0.5f;
};

class Harmonics {
public:
    explicit Harmonics(std::vector<std::string> amplitudes) : amps_(std::move(amplitudes)) {}

    bool empty() const { return amps_.empty(); }

    std::vector<double> read(ModelHost& host, const std::string& organism, double& peak) const;
    static double valueAt(const std::vector<double>& a, double peak, double t);

private:
    std::vector<std::string> amps_;
};

class Pitch {
public:
    enum class Tuning { InTune, Close, Off };

    bool poll(ModelHost& host, const std::string& organism);

    float hz() const { return hz_; }
    float level() const { return level_; }
    float clarity() const { return clarity_; }
    int note() const { return note_; }

    double cents() const;
    Tuning tuning() const;

    const std::vector<int>& chord() const { return chord_; }

private:
    float hz_ = 0.0f, level_ = 0.0f, clarity_ = 0.0f;
    int note_ = -1;
    std::vector<int> chord_;
};

class Timecode {
public:
    bool poll(ModelHost& host, const std::string& organism);

    bool on() const { return on_; }
    bool present() const { return present_; }
    float level() const { return level_; }
    float speed() const { return speed_; }
    float restCarrierHz() const { return carrier_; }

private:
    bool on_ = false, present_ = false;
    float level_ = 0.0f, speed_ = 0.0f, carrier_ = 0.0f;
};

class Sigil {
public:
    static constexpr int kMaxPrims = 48;

    static bool present(ModelHost& host, const std::string& organism) {
        return live::source<SigilSource>(host, organism) != nullptr;
    }

    int draw(ModelHost& host, const std::string& organism, double seconds);

    const SigilSource::Prim& prim(int i) const { return prims_[i]; }

private:
    SigilSource::Prim prims_[kMaxPrims];
};

}
