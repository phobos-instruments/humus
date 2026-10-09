// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "common/RecordDecoder.h"

namespace hum {

class ControlVinyl {
public:
    struct Reading {
        double speed = 0.0;
        double carrierHz = 0.0;
        double restHz = 0.0;
        double needleSeconds = 0.0;
        bool toneArriving = false;
        bool located = false;
    };

    ControlVinyl();
    ~ControlVinyl();
    ControlVinyl(const ControlVinyl&) = delete;
    ControlVinyl& operator=(const ControlVinyl&) = delete;

    static const std::vector<std::string>& formats();

    void prepare(double sampleRate, const std::string& format);

    void chooseRecord(const std::string& format, bool rpm45);

    void reset();

    void push(const float* left, const float* right, int numSamples);

    Reading read() const;

    const std::string& format() const { return record_.format().empty() ? settled_ : record_.format(); }

    double restCarrierHz() const { return rest_.hz; }

    double toneHz() const { return toneHz_; }

    void pinRest(double hz);

    bool restPinned() const { return rest_.pinned; }

private:
    void latchProvisional();
    void learnRestTone(double hz, double seconds);
    void followTone(double hz, double seconds);

    struct RestTone {
        double hz = 0.0;
        double heldSeconds = 0.0;
        double challengerHz = 0.0;
        double challengerHeldSeconds = 0.0;
        double quietSeconds = 0.0;
        bool pinned = false;
    };

    void submitDetecting(int frames);

    struct Impl;
    std::unique_ptr<Impl> impl_;
    RecordDecoder record_;
    bool recordRpm45_ = false;
    std::string settled_;
    RestTone rest_;
    double toneHz_ = 0.0;
    double toneSeconds_ = 0.0;
};

}
