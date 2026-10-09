// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <memory>
#include <string>

namespace hum {

class RecordDecoder {
public:
    struct Reading {
        bool live = false;
        double speed = 0.0;
        double carrierHz = 0.0;
        double nominalHz = 0.0;
        bool located = false;
        double seconds = 0.0;
        double staleSeconds = 0.0;
    };

    static constexpr double kRpm45 = 45.0 / (100.0 / 3.0);

    RecordDecoder();
    ~RecordDecoder();
    RecordDecoder(const RecordDecoder&) = delete;
    RecordDecoder& operator=(const RecordDecoder&) = delete;

    void choose(const std::string& format, double rpmFactor, double sampleRate);
    const std::string& format() const { return format_; }

    void submit(signed short* interleaved, int frames);
    Reading read() const;

private:
    bool wake();
    void release();

    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string format_;
    double rpm_ = 1.0;
    double sampleRate_ = 0.0;
};

}
