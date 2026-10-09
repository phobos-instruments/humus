// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct timecode_def;

namespace hum {

struct TimecodeRecord {
    const char* label;
    const char* format;
    double leadInSeconds;
};

const std::vector<TimecodeRecord>& timecodeRecords();

class TimecodeTables {
public:
    static constexpr std::size_t kMaxFormats = 16;

    static TimecodeTables& instance();
    ~TimecodeTables();

    timecode_def* ready(const std::string& format);
    timecode_def* waitFor(const std::string& format, std::chrono::milliseconds limit);
    bool failed(const std::string& format) const;

    static std::string& cacheFolderForTesting();
    static std::string cacheFolder();

private:
    TimecodeTables();
    int slotOf(const std::string& format) const;
    void work();

    std::array<std::atomic<timecode_def*>, kMaxFormats> built_{};
    std::array<std::atomic<bool>, kMaxFormats> asked_{};
    std::array<std::atomic<bool>, kMaxFormats> broken_{};
    std::mutex lock_;
    std::condition_variable wake_;
    bool stopping_ = false;
    std::thread worker_;
};

}
