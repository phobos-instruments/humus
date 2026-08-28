#pragma once
#include <atomic>
#include <cstring>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class OscMonitor : public Organism, public OscLogSource {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double, int) override {}
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    void pushOsc(bool out, const char* address, const char* args) override {
        if (head_ - tail_ >= kSize) return;
        Logged& e = slots_[head_ % kSize];
        e.out = out;
        std::strncpy(e.address, address ? address : "", sizeof(e.address) - 1);
        e.address[sizeof(e.address) - 1] = '\0';
        std::strncpy(e.args, args ? args : "", sizeof(e.args) - 1);
        e.args[sizeof(e.args) - 1] = '\0';
        ++head_;
        ++generation_;
    }
    int consumeOscLog(Logged* dest, int maxEvents) override {
        int n = 0;
        while (tail_ != head_ && n < maxEvents) dest[n++] = slots_[tail_++ % kSize];
        return n;
    }
    unsigned oscLogGeneration() const override { return generation_; }

private:
    static constexpr unsigned kSize = 1024;
    Logged slots_[kSize];
    unsigned head_ = 0, tail_ = 0, generation_ = 0;
};

}
