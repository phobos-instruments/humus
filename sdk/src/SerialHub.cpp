// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/SerialPort.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>

namespace hum::serial {

class Hub : public juce::Thread {
public:
    explicit Hub(std::string path) : juce::Thread("hum-serial-hub"), path_(std::move(path)) {}
    ~Hub() override {
        stopThread(2000);
        port_.close();
    }

    const std::string& path() const { return path_; }
    bool isOpen() const { return open_.load(); }
    unsigned generation() const { return generation_.load(); }

    void reopen() { reopen_.store(true); }

    void want(const Settings& settings) {
        const juce::SpinLock::ScopedLockType sl(settingsLock_);
        if (settings_ == settings) return;
        settings_ = settings;
        reopen_.store(true);
    }

    void subscribe(Link* link) {
        const juce::SpinLock::ScopedLockType sl(linksLock_);
        links_.push_back(link);
        if (!isThreadRunning()) startThread();
    }

    void unsubscribe(Link* link) {
        const juce::SpinLock::ScopedLockType sl(linksLock_);
        links_.erase(std::remove(links_.begin(), links_.end(), link), links_.end());
    }

    bool write(const void* buf, int n) {
        const std::lock_guard<std::mutex> wl(writeLock_);
        if (!open_.load()) return false;
        if (port_.write(buf, n)) return true;
        reopen_.store(true);
        return false;
    }

private:
    void run() override {
        while (!threadShouldExit()) {
            if (reopen_.exchange(false)) closePort();
            if (!port_.isOpen()) {
                if (!openPort()) { wait(1000); continue; }
            }
            std::uint8_t buf[256];
            const int n = port_.read(buf, sizeof(buf), 50);
            if (n < 0) { closePort(); continue; }
            if (n == 0) continue;
            const juce::SpinLock::ScopedLockType sl(linksLock_);
            for (auto* l : links_) l->deliver(buf, n);
        }
        closePort();
    }

    bool openPort() {
        const std::lock_guard<std::mutex> wl(writeLock_);
        Settings settings;
        {
            const juce::SpinLock::ScopedLockType sl(settingsLock_);
            settings = settings_;
        }
        if (!port_.open(path_, settings)) return false;
        ++generation_;
        open_.store(true);
        return true;
    }

    void closePort() {
        const std::lock_guard<std::mutex> wl(writeLock_);
        open_.store(false);
        port_.close();
    }

    std::string path_;
    Port port_;
    juce::SpinLock settingsLock_;
    Settings settings_;
    std::atomic<bool> open_{false};
    std::atomic<bool> reopen_{false};
    std::atomic<unsigned> generation_{0};
    std::mutex writeLock_;
    juce::SpinLock linksLock_;
    std::vector<Link*> links_;
};

namespace {
std::mutex& registryLock() {
    static std::mutex m;
    return m;
}
std::map<std::string, std::weak_ptr<Hub>>& registry() {
    static std::map<std::string, std::weak_ptr<Hub>> hubs;
    return hubs;
}
}

std::shared_ptr<Link> acquire(const std::string& path, const Settings& settings) {
    if (path.empty()) return nullptr;
    std::shared_ptr<Hub> hub;
    {
        const std::lock_guard<std::mutex> rl(registryLock());
        auto& hubs = registry();
        for (auto it = hubs.begin(); it != hubs.end();)
            it = it->second.expired() ? hubs.erase(it) : std::next(it);
        if (auto it = hubs.find(path); it != hubs.end()) hub = it->second.lock();
        if (!hub) {
            hub = std::make_shared<Hub>(path);
            hubs[path] = hub;
        }
    }
    hub->want(settings);
    std::shared_ptr<Link> link(new Link(hub, path, settings));
    hub->subscribe(link.get());
    return link;
}

Link::Link(std::shared_ptr<Hub> hub, std::string path, const Settings& settings)
    : hub_(std::move(hub)), path_(std::move(path)), settings_(settings) {}

Link::~Link() {
    if (hub_) hub_->unsubscribe(this);
}

bool Link::isOpen() const { return hub_ && hub_->isOpen(); }
unsigned Link::generation() const { return hub_ ? hub_->generation() : 0; }

void Link::deliver(const std::uint8_t* bytes, int n) {
    {
        const juce::SpinLock::ScopedLockType sl(queueLock_);
        for (int i = 0; i < n; ++i) {
            queue_[head_] = bytes[i];
            head_ = (head_ + 1) % kQueue;
            if (head_ == tail_) tail_ = (tail_ + 1) % kQueue;
        }
    }
    arrived_.signal();
}

int Link::read(void* buf, int max, int timeoutMs) {
    if (max <= 0) return 0;
    auto pop = [&]() -> int {
        const juce::SpinLock::ScopedLockType sl(queueLock_);
        int n = 0;
        auto* dst = (std::uint8_t*) buf;
        while (n < max && tail_ != head_) {
            dst[n++] = queue_[tail_];
            tail_ = (tail_ + 1) % kQueue;
        }
        return n;
    };
    if (const int n = pop(); n > 0) return n;
    if (!arrived_.wait(timeoutMs)) return 0;
    return pop();
}

bool Link::write(const void* buf, int n) { return hub_ && hub_->write(buf, n); }

void Link::reconnect() { if (hub_) hub_->reopen(); }

}
