// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <string_view>

#include "hum/caps/Osc.h"
#include "hum/Organism.h"

namespace hum {

class OscMonitor : public Organism, public OscLogSource {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double, int) override {}
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    static void copyBounded(char* dst, std::size_t capacity, const char* src) {
        const std::string_view text(src ? src : "");
        const std::size_t n = std::min(text.size(), capacity - 1);
        std::memcpy(dst, text.data(), n);
        dst[n] = '\0';
    }

    void pushOsc(bool out, const char* address, const char* args) override {
        if (head_ - tail_ >= kSize) return;
        Logged& e = slots_[head_ % kSize];
        e.out = out;
        copyBounded(e.address, sizeof(e.address), address);
        copyBounded(e.args, sizeof(e.args), args);
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
