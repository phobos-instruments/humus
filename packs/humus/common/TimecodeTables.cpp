// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/TimecodeTables.h"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <system_error>

#include "hum/FileBytes.h"
#include "xwax/timecoder.h"

namespace hum {

namespace {

const char* const kFormats[] = {
    "serato_2a",     "serato_2b",     "serato_cd",      "traktor_a",   "traktor_b",
    "traktor_mk2_a", "traktor_mk2_b", "traktor_mk2_cd", "mixvibes_v2", "mixvibes_7inch",
    "pioneer_a",     "pioneer_b",
};

constexpr int kFormatCount = (int) (sizeof(kFormats) / sizeof(kFormats[0]));
static_assert(kFormatCount <= (int) TimecodeTables::kMaxFormats);

constexpr auto kIdleWait = std::chrono::milliseconds(250);
constexpr auto kPollStep = std::chrono::milliseconds(2);

}

const std::vector<TimecodeRecord>& timecodeRecords() {
    static const std::vector<TimecodeRecord> records = {
        {"Learn from the record", "", 0.0},
        {"Traktor MK2 side A", "traktor_mk2_a", 10.0},
        {"Traktor MK2 side B", "traktor_mk2_b", 10.0},
        {"Traktor MK2 CD", "traktor_mk2_cd", 10.0},
        {"Traktor MK1 side A", "traktor_a", 10.0},
        {"Traktor MK1 side B", "traktor_b", 10.0},
        {"Serato side A", "serato_2a", 28.0},
        {"Serato side B", "serato_2b", 28.0},
        {"Serato CD", "serato_cd", 0.0},
        {"Mixvibes V2", "mixvibes_v2", 0.0},
        {"Mixvibes 7-inch", "mixvibes_7inch", 0.0},
        {"Pioneer side A", "pioneer_a", 10.0},
        {"Pioneer side B", "pioneer_b", 10.0},
    };
    return records;
}

TimecodeTables& TimecodeTables::instance() {
    static TimecodeTables tables;
    return tables;
}

TimecodeTables::TimecodeTables() : worker_([this] { work(); }) {}

TimecodeTables::~TimecodeTables() {
    {
        const std::lock_guard<std::mutex> held(lock_);
        stopping_ = true;
    }
    wake_.notify_all();
    if (worker_.joinable()) worker_.join();
}

int TimecodeTables::slotOf(const std::string& format) const {
    for (int i = 0; i < kFormatCount; ++i)
        if (format == kFormats[i]) return i;
    return -1;
}

timecode_def* TimecodeTables::ready(const std::string& format) {
    const int slot = slotOf(format);
    if (slot < 0) return nullptr;
    if (auto* def = built_[(std::size_t) slot].load(std::memory_order_acquire)) return def;
    if (!asked_[(std::size_t) slot].exchange(true)) wake_.notify_one();
    return nullptr;
}

timecode_def* TimecodeTables::waitFor(const std::string& format, std::chrono::milliseconds limit) {
    const auto until = std::chrono::steady_clock::now() + limit;
    for (;;) {
        if (auto* def = ready(format)) return def;
        if (failed(format) || std::chrono::steady_clock::now() >= until) return nullptr;
        std::this_thread::sleep_for(kPollStep);
    }
}

bool TimecodeTables::failed(const std::string& format) const {
    const int slot = slotOf(format);
    return slot < 0 || broken_[(std::size_t) slot].load();
}

std::string& TimecodeTables::cacheFolderForTesting() {
    static std::string folder;
    return folder;
}

std::string TimecodeTables::cacheFolder() {
    if (!cacheFolderForTesting().empty()) return cacheFolderForTesting();
#if defined(_WIN32)
    if (const wchar_t* local = _wgetenv(L"LOCALAPPDATA"); local != nullptr && *local != 0)
        return utf8FromWide(local) + "\\Humus\\Timecode";
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != 0)
        return std::string(home) + "/Library/Caches/Humus/Timecode";
#else
    if (const char* cache = std::getenv("XDG_CACHE_HOME"); cache != nullptr && *cache != 0)
        return std::string(cache) + "/humus/timecode";
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != 0)
        return std::string(home) + "/.cache/humus/timecode";
#endif
    return {};
}

void TimecodeTables::work() {
    std::unique_lock<std::mutex> held(lock_);
    while (!stopping_) {
        int slot = -1;
        for (int i = 0; i < kFormatCount && slot < 0; ++i) {
            const auto at = (std::size_t) i;
            if (asked_[at].load() && built_[at].load() == nullptr && !broken_[at].load()) slot = i;
        }
        if (slot < 0) {
            wake_.wait_for(held, kIdleWait);
            continue;
        }
        held.unlock();
        const std::string folder = cacheFolder();
        std::error_code ignored;
        if (!folder.empty()) std::filesystem::create_directories(utf8Path(folder), ignored);
        auto* def = timecoder_find_definition(kFormats[slot], folder.empty() ? nullptr : folder.c_str());
        held.lock();
        if (def != nullptr) built_[(std::size_t) slot].store(def, std::memory_order_release);
        else broken_[(std::size_t) slot].store(true);
    }
}

}
