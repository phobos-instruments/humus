#pragma once
#include <atomic>
#include <vector>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/DelayLine.h"
#include "hum/dsp/SvfTpt.h"

namespace hum {

class Fern : public Organism, public SigilSource {
public:
    explicit Fern(int channels) : ch_(channels) {}

    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int sigil(Prim* out, int capacity, double timeSeconds) override;

private:
    struct Chan {
        DelayLine line;
        SvfTpt lp, hp;
        float pos = -1.0f;
        float tap[2] = {0.0f, 0.0f};
        int cur = 0;
        float xf = 1.0f;
        float pending = -1.0f;
        float last = 0.0f;
    };

    float glideTape(Chan& c, float target) const;
    float readFade(Chan& c, float target);

    int ch_;
    std::vector<Chan> chans_;
    int maxDelay_ = 0;
    int fadeLen_ = 1;

    std::atomic<float> sigFeedback_{0.35f}, sigDamp_{0.35f}, sigMix_{0.35f};
};

}
