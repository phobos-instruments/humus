// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>

#include "hum/caps/Audio.h"
#include "hum/Organism.h"
#include "hum/dsp/Oversampler.h"
#include "hum/dsp/SvfTpt.h"

namespace hum {

class ValveFilter : public Organism, public LatencyReporting {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        for (auto& o : os_) o.prepare(2);
        reset();
    }
    int latencySamples() const override { return os_[0].latency(); }
    void reset() override {
        for (auto& s : svf_) s.reset();
        for (auto& o : os_) o.reset();
        for (auto& d : dry_) d.fill(0.0f);
        dryPos_ = 0;
        for (auto& d : dcx_) d = 0.0;
        for (auto& d : dcy_) d = 0.0;
        env_ = 0.0;
        lfoPhase_ = 0.0;
        on_ = hpG_ = bpG_ = lpG_ = 0.0;
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    SvfTpt svf_[2];
    double dcx_[2] = {}, dcy_[2] = {};
    double env_ = 0.0;
    double lfoPhase_ = 0.0;
    double on_ = 0.0, hpG_ = 0.0, bpG_ = 0.0, lpG_ = 0.0;
    double g_ = 0.1, k_ = 1.4, vg_ = 1.0, norm_ = 1.0, hard_ = 0.0, asym_ = 0.1;
    Oversampler os_[2];
    static constexpr int kDryRing = 64;
    std::array<std::array<float, kDryRing>, 2> dry_ {};
    int dryPos_ = 0;
};

}
