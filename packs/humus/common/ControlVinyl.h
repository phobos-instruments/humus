// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hum {

class ControlVinyl {
public:
    struct Reading {
        double speed = 0.0;
        double carrierHz = 0.0;
        double restHz = 0.0;
        std::int64_t positionSamples = -1;
        bool toneArriving = false;
        bool located = false;
    };

    ControlVinyl();
    ~ControlVinyl();
    ControlVinyl(const ControlVinyl&) = delete;
    ControlVinyl& operator=(const ControlVinyl&) = delete;

    static std::vector<std::string> formats();

    void prepare(double sampleRate, const std::string& format);

    void reset();

    void push(const float* left, const float* right, int numSamples);

    Reading read() const;

    const std::string& format() const { return settled_; }

    double restCarrierHz() const { return rest_.hz; }

private:
    void latchProvisional();
    void learnRestTone(double hz, double seconds);

    struct RestTone {
        double hz = 0.0;
        double heldSeconds = 0.0;
        double challengerHz = 0.0;
        double challengerHeldSeconds = 0.0;
        double quietSeconds = 0.0;
    };

    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string settled_;
    RestTone rest_;
};

}
