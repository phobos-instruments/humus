// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "hum/Extensions.h"
#include "hum/Parameter.h"
#include "hum/Pattern.h"
#include "hum/Transport.h"

#include "hum/dsp/DspMath.h"

namespace hum {

struct OrganismState {
    const std::vector<Parameter>& properties;
    const Pattern& pattern;
    const OrganismStateExt* ext = nullptr;
};

class Organism {
public:
    virtual ~Organism() = default;

    virtual const void* extension(const char*) const { return nullptr; }

    void setName(std::string n) { name_ = std::move(n); }
    const std::string& name() const { return name_; }

    virtual int numAudioInputs() const = 0;
    virtual int numAudioOutputs() const = 0;

    virtual void configureChannels(int, int) {}

    virtual void prepare(double sampleRate, int maxBlock) = 0;
    virtual void reset() {}

    virtual void process(const float* const* in, int numIn,
                         float* const* out, int numOut,
                         int numSamples, const Transport& transport) = 0;

    virtual void loadFrom(const OrganismState& state);

    virtual void setPattern(const Pattern&) {}

    virtual void onTextChanged(const std::string&, const std::string&) {}

    virtual std::string matchToken() const { return {}; }

    virtual double masterTempo() const { return 0.0; }

    ParameterSet params;

protected:
    std::string name_;
    double sampleRate_ = kDefaultSampleRate;
};

using OrganismPtr = std::unique_ptr<Organism>;

}
