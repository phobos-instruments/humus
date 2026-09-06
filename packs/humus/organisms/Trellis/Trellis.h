#pragma once
#include <array>

#include "hum/Organism.h"
#include "hum/Scale.h"
#include "hum/dsp/PitchShifter.h"
#include "hum/dsp/PitchTrack.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Trellis : public Organism {
public:
    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 1; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    double snapMidi(double midi, const Tuning& tuning) const;

    double sampleRate_ = kDefaultSampleRate;
    PitchTracker tracker_;
    PitchShifter shifter_;

    int key_ = 0, scale_ = 0, window_ = 0;
    double targetCents_ = 0.0;
    double smoothCents_ = 0.0;
    double glideCoef_ = 0.3;
};

}
