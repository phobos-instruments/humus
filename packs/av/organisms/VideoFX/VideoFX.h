#pragma once
#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class VideoFX : public Organism, public VideoNode, public VideoFxNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double, int) override {}
    void process(const float* const*, int, float* const*, int, int,
                 const Transport&) override {}

    int numVideoInputs() const override { return 1; }
    int numVideoOutputs() const override { return 1; }
};

}
