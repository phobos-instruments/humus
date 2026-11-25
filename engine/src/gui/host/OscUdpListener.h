// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class OscUdpListener : private juce::Thread {
public:
    using Packet = std::vector<std::uint8_t>;
    std::function<void(Packet packet, std::string from)> onPacket;

    OscUdpListener() : juce::Thread("hum-osc-udp") {}
    ~OscUdpListener() override { close(); }

    bool open(int port);
    void close();
    bool isOpen() const;

private:
    void run() override;

    mutable std::mutex lock_;
    std::shared_ptr<juce::DatagramSocket> socket_;
};

}
