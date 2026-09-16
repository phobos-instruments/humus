// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "hum/SerialParse.h"

namespace hum::serial {

inline const std::vector<int>& standardBauds() {
    static const std::vector<int> rates = {300,    1200,   2400,   4800,   9600,   19200,  38400, 57600,
                                           115200, 230400, 250000, 460800, 500000, 921600, 1000000};
    return rates;
}

inline int baudFromIndex(int index) {
    const auto& r = standardBauds();
    return r[(size_t) juce::jlimit(0, (int) r.size() - 1, index)];
}

enum Frame : int { kFrame8N1 = 0, kFrame8E1 = 1, kFrame8O1 = 2, kFrame8N2 = 3 };

struct Settings {
    int baud = 9600;
    int frame = kFrame8N1;
    bool reset = true;

    bool operator==(const Settings& o) const {
        return baud == o.baud && frame == o.frame && reset == o.reset;
    }
    bool operator!=(const Settings& o) const { return !(*this == o); }
};

std::vector<std::string> listDevices();

inline std::string pickDevice(const std::string& explicitPath, int deviceIndex) {
    if (!explicitPath.empty()) return explicitPath;
    const auto devices = listDevices();
    if (deviceIndex >= 2 && (size_t) (deviceIndex - 2) < devices.size())
        return devices[(size_t) (deviceIndex - 2)];
    return devices.empty() ? std::string() : devices.front();
}

class Port {
public:
    Port() = default;
    ~Port() { close(); }
    Port(const Port&) = delete;
    Port& operator=(const Port&) = delete;

    bool open(const std::string& path, const Settings& settings);
    void close();
    bool isOpen() const { return handle_ != -1; }
    const std::string& path() const { return path_; }
    int baud() const { return settings_.baud; }
    const Settings& settings() const { return settings_; }

    int read(void* buf, int max, int timeoutMs);
    bool write(const void* buf, int n);

private:
    std::intptr_t handle_ = -1;
    std::string path_;
    Settings settings_;
};

class Hub;

class Link {
public:
    ~Link();
    bool isOpen() const;
    unsigned generation() const;
    int read(void* buf, int max, int timeoutMs);
    bool write(const void* buf, int n);
    void reconnect();
    const std::string& path() const { return path_; }
    int baud() const { return settings_.baud; }
    int frame() const { return settings_.frame; }
    const Settings& settings() const { return settings_; }

private:
    friend class Hub;
    friend std::shared_ptr<Link> acquire(const std::string& path, const Settings& settings);
    Link(std::shared_ptr<Hub> hub, std::string path, const Settings& settings);
    void deliver(const std::uint8_t* bytes, int n);

    std::shared_ptr<Hub> hub_;
    std::string path_;
    Settings settings_;
    static constexpr int kQueue = 4096;
    std::uint8_t queue_[kQueue];
    int head_ = 0;
    int tail_ = 0;
    juce::SpinLock queueLock_;
    juce::WaitableEvent arrived_;
};

std::shared_ptr<Link> acquire(const std::string& path, const Settings& settings);

inline std::shared_ptr<Link> acquire(const std::string& path, int baud, int frame = kFrame8N1) {
    Settings s;
    s.baud = baud;
    s.frame = frame;
    return acquire(path, s);
}

}
