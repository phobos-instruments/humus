// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <memory>
#include <string>

#include "hum/caps/Graph.h"
#include "hum/dsp/Prepared.h"
#include "hum/dsp/WaveTable.h"

namespace hum {

class LfoTable {
public:
    static constexpr int kMaxFrames = 16;

    void sync(const std::string& text);
    void publish(const std::string& text);
    void adopt();
    void prepare(double sampleRate);
    void glideTo(double position) { target_ = position; }
    double at(double phase);
    void publishCycle();
    int readCycle(MotionTrace::Point* out, int capacity) const;

private:
    struct Frames {
        float data[kMaxFrames][kWaveTableLen] = {};
        int count = 1;
    };
    static std::unique_ptr<Frames> decode(const std::string& text);

    std::unique_ptr<Frames> frames_;
    Prepared<std::unique_ptr<Frames>> pending_;
    std::string applied_;
    double position_ = 0.0, target_ = 0.0, glide_ = 1.0;
    std::array<std::atomic<float>, MotionTrace::kPoints> cycle_{};
};

}
