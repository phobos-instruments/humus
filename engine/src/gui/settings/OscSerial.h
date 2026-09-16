// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <juce_core/juce_core.h>

#include "core/net/OscSlip.h"
#include "hum/SerialPort.h"

namespace hum {

class OscSerialLink : private juce::Thread {
public:
    std::function<void(const osc::Message&)> onMessage;

    OscSerialLink() : juce::Thread("hum-osc-serial") {}
    ~OscSerialLink() override;

    void configure(bool enabled, const std::string& port, int baud);
    bool enabled() const { return enabled_.load(); }
    bool isOpen() const;
    bool send(const std::string& address, float value);

private:
    void run() override;
    std::shared_ptr<serial::Link> ensureOpen();

    std::atomic<bool> enabled_{false};
    juce::SpinLock wantLock_;
    std::string wantPort_;
    int wantBaud_ = 115200;

    mutable std::mutex linkLock_;
    std::shared_ptr<serial::Link> link_;
    osc::SlipDecoder decoder_;
};

}
