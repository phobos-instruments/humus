// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "core/browser/FileIndex.h"

namespace hum::browser {

struct IndexConfig {
    std::string indexFile;
    std::function<std::vector<std::string>()> places;
    std::function<bool()> busy;
    std::function<std::int64_t()> clock;
    std::function<Facts(const std::string& path)> soundFacts;
};

class BrowserIndex {
public:
    static constexpr int kWalkBudget = 64;
    static constexpr int kBusyWalkBudget = 16;
    static constexpr int kProbeBatch = 8;
    static constexpr int kIdleMs = 60000;
    static constexpr int kBusyMs = 100;
    static constexpr int kSaveEveryMs = 5000;

    explicit BrowserIndex(IndexConfig config);
    ~BrowserIndex();
    BrowserIndex(const BrowserIndex&) = delete;
    BrowserIndex& operator=(const BrowserIndex&) = delete;

    void startScanning();
    void stopScanning();
    void rescan();
    bool scanning() const { return running_.load(); }

    void scanNow();
    int probePending(int most);
    bool saveNow();
    int pruneOutsidePlaces();
    int pruneGone();
    static bool gone(const std::string& path);

    template <class Fn> auto read(Fn&& fn) const {
        const std::lock_guard<std::mutex> hold(lock_);
        return fn(index_);
    }
    template <class Fn> void edit(Fn&& fn) {
        {
            const std::lock_guard<std::mutex> hold(lock_);
            fn(index_);
        }
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    unsigned version() const { return version_.load(std::memory_order_relaxed); }
    std::string scanningPlace() const;
    bool placePending(const std::string& path) const;
    bool anyPlacePending() const;
    std::int64_t now() const;
    std::vector<std::string> places() const;

private:
    void run();
    bool walkPlace(const std::string& place);
    bool paused() const { return config_.busy && config_.busy(); }
    bool kickPending();
    bool walked(const std::string& root) const;
    std::vector<std::string> unreadFirst() const;
    bool stopping() const { return stop_.load(); }
    void rest(int ms);
    void pause(int ms);
    Facts probe(const std::string& path, Kind kind) const;

    IndexConfig config_;
    mutable std::mutex lock_;
    FileIndex index_;
    std::atomic<unsigned> version_{0};
    mutable std::mutex scanningLock_;
    std::string scanning_;
    std::set<std::string> walked_;
    std::chrono::steady_clock::time_point lastSave_{};

    std::thread worker_;
    std::atomic<bool> running_{false}, stop_{false};
    std::mutex wakeLock_;
    std::condition_variable wake_;
    bool kicked_ = false;
};

}
