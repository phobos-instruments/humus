// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <memory>

namespace hum {

class TimeStretcher {
public:
    TimeStretcher();
    explicit TimeStretcher(long seed);
    ~TimeStretcher();

    bool available() const;
    void prepare(double sampleRate, int channels, int maxBlock);
    void reset();

    int inputLatency() const;
    int outputLatency() const;
    int prerollSamples() const;
    void setTranspose(double factor);
    void preroll(const float* const* in, int inN, double tempoFactor);

    void process(const float* const* in, int inN,
                 float* const* out, int outN, double tempoFactor);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
