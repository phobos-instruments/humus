// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/BrowserIndex.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <utility>

#include "core/browser/FolderScan.h"
#include "core/browser/ProjectFacts.h"
#include "core/browser/ThreadPriority.h"
#include "hum/FileBytes.h"

namespace hum::browser {

BrowserIndex::BrowserIndex(IndexConfig config) : config_(std::move(config)) {
    if (!config_.indexFile.empty()) loadIndex(config_.indexFile, index_);
}

BrowserIndex::~BrowserIndex() {
    stopScanning();
    saveNow();
}

void BrowserIndex::startScanning() {
    if (running_.exchange(true)) return;
    stop_ = false;
    worker_ = std::thread([this] {
        lowerCurrentThreadPriority();
        run();
    });
}

void BrowserIndex::stopScanning() {
    if (!running_.load()) return;
    stop_ = true;
    rescan();
    if (worker_.joinable()) worker_.join();
    running_ = false;
}

void BrowserIndex::rescan() {
    {
        const std::lock_guard<std::mutex> hold(wakeLock_);
        kicked_ = true;
    }
    wake_.notify_all();
}

void BrowserIndex::pause(int ms) {
    std::unique_lock<std::mutex> hold(wakeLock_);
    wake_.wait_for(hold, std::chrono::milliseconds(ms), [this] { return stopping(); });
}

void BrowserIndex::rest(int ms) {
    std::unique_lock<std::mutex> hold(wakeLock_);
    wake_.wait_for(hold, std::chrono::milliseconds(ms), [this] { return kicked_ || stopping(); });
    kicked_ = false;
}

std::int64_t BrowserIndex::now() const {
    if (config_.clock) return config_.clock();
    using namespace std::chrono;
    return (std::int64_t) duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

std::vector<std::string> BrowserIndex::places() const {
    auto out = config_.places ? config_.places() : std::vector<std::string>{};
    for (const auto& w : read([](const FileIndex& i) { return i.watched(); })) out.push_back(w);
    return out;
}

std::string BrowserIndex::scanningPlace() const {
    const std::lock_guard<std::mutex> hold(scanningLock_);
    return scanning_;
}

bool BrowserIndex::walked(const std::string& root) const {
    const std::lock_guard<std::mutex> hold(scanningLock_);
    return walked_.count(root) > 0;
}

bool BrowserIndex::placePending(const std::string& path) const {
    bool covered = false;
    for (const auto& root : places()) {
        if (path != root && !isUnder(path, root)) continue;
        if (walked(root)) return false;
        covered = true;
    }
    return covered;
}

bool BrowserIndex::anyPlacePending() const {
    const auto roots = places();
    return std::any_of(roots.begin(), roots.end(), [this](const std::string& root) { return !walked(root); });
}

std::vector<std::string> BrowserIndex::unreadFirst() const {
    auto order = places();
    std::stable_partition(order.begin(), order.end(), [this](const std::string& root) { return !walked(root); });
    return order;
}

bool BrowserIndex::kickPending() {
    const std::lock_guard<std::mutex> hold(wakeLock_);
    return kicked_;
}

bool BrowserIndex::walkPlace(const std::string& place) {
    {
        const std::lock_guard<std::mutex> hold(scanningLock_);
        scanning_ = place;
    }
    FolderScan walk(place);
    std::vector<Seen> batch;
    ScanPass pass(index_, place, now());
    while (!walk.done()) {
        if (stopping()) return false;
        const bool slow = running_.load() && paused();
        if (slow) pause(kBusyMs);
        batch.clear();
        walk.step(slow ? kBusyWalkBudget : kWalkBudget, [&](const Seen& s) { batch.push_back(s); });
        edit([&](FileIndex&) { for (const auto& s : batch) pass.take(s); });
    }
    {
        const std::lock_guard<std::mutex> hold(scanningLock_);
        walked_.insert(place);
        scanning_.clear();
    }
    edit([&](FileIndex&) { pass.finish(); });
    return true;
}

Facts BrowserIndex::probe(const std::string& path, Kind kind) const {
    if (kind == Kind::Project || kind == Kind::Patch) return probeProject(path);
    if (config_.soundFacts) return config_.soundFacts(path);
    Facts none;
    none.probed = true;
    return none;
}

int BrowserIndex::probePending(int most) {
    const auto todo = read([most](const FileIndex& i) {
        std::vector<std::pair<std::string, Kind>> out;
        for (const auto& [path, e] : i.entries()) {
            if ((int) out.size() >= most) break;
            const bool probes = e.kind == Kind::Sound || e.kind == Kind::Impulse || e.kind == Kind::Project || e.kind == Kind::Patch;
            if (probes && !e.facts.probed) out.emplace_back(path, e.kind);
        }
        return out;
    });
    for (const auto& item : todo) {
        if (stopping()) break;
        const auto& path = item.first;
        const auto facts = probe(path, item.second);
        edit([&](FileIndex& i) { i.setFacts(path, facts); });
    }
    return (int) todo.size();
}

bool BrowserIndex::gone(const std::string& path) {
    std::error_code ec;
    return !std::filesystem::exists(utf8Path(path), ec) && !onMissingDrive(path);
}

int BrowserIndex::pruneGone() {
    const auto paths = read([](const FileIndex& i) {
        std::vector<std::string> out;
        for (const auto& [path, e] : i.entries()) out.push_back(path);
        return out;
    });
    std::vector<std::string> dead;
    for (const auto& p : paths)
        if (gone(p)) dead.push_back(p);
    if (dead.empty()) return 0;
    edit([&](FileIndex& i) { for (const auto& p : dead) i.forget(p); });
    return (int) dead.size();
}

int BrowserIndex::pruneOutsidePlaces() {
    const auto roots = places();
    int n = 0;
    edit([&](FileIndex& i) { n = i.forgetOutside(roots); });
    return n;
}

void BrowserIndex::scanNow() {
    pruneGone();
    pruneOutsidePlaces();
    for (const auto& place : places())
        if (!walkPlace(place)) return;
    while (probePending(kProbeBatch) > 0)
        if (stopping()) return;
}

bool BrowserIndex::saveNow() {
    if (config_.indexFile.empty()) return false;
    const std::lock_guard<std::mutex> hold(lock_);
    if (!index_.dirty()) return true;
    std::error_code ec;
    std::filesystem::create_directories(utf8Path(config_.indexFile).parent_path(), ec);
    lastSave_ = std::chrono::steady_clock::now();
    return saveIndex(config_.indexFile, index_);
}

void BrowserIndex::run() {
    while (!stopping()) {
        pruneGone();
        pruneOutsidePlaces();
        for (const auto& place : unreadFirst()) {
            if (!walkPlace(place)) return;
            saveNow();
        }
        while (!stopping() && !kickPending()) {
            const bool slow = paused();
            if (slow) pause(kBusyMs);
            if (probePending(slow ? 1 : kProbeBatch) == 0) break;
            if (std::chrono::steady_clock::now() - lastSave_ > std::chrono::milliseconds(kSaveEveryMs)) saveNow();
        }
        saveNow();
        rest(kIdleMs);
    }
}

}
