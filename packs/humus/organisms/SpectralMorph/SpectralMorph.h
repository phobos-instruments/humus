#pragma once
#include <vector>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

#include "common/Stft.h"

namespace hum {

class SpectralMorph : public Organism, public LatencyReporting {
public:
    explicit SpectralMorph(int channels = 2) : ch_(channels) {}

    int numAudioInputs() const override { return 2 * ch_; }
    int numAudioOutputs() const override { return ch_; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int latencySamples() const override {
        return chans_.empty() ? 0 : chans_.front().a.latencySamples();
    }

private:
    struct Chan {
        Stft a, b;
        std::vector<float> aFifo;
        int fHead = 0, fTail = 0, fCount = 0;
        std::vector<float> dry;
        int dryPos = 0;
    };

    void pushA(Chan& c, const float* reim);
    void blendB(Chan& c, float* reim);

    int ch_ = 2;
    std::vector<Chan> chans_;
    std::vector<float> discard_, silence_;
    int bins_ = 0, entryLen_ = 0, fCap_ = 0;
    float morph_ = 0.0f;
};

}
