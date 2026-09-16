// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/OscSerial.h"

#include <cstdint>

namespace hum {

OscSerialLink::~OscSerialLink() {
    stopThread(2000);
    const std::lock_guard<std::mutex> ll(linkLock_);
    link_.reset();
}

void OscSerialLink::configure(bool enabled, const std::string& port, int baud) {
    {
        const juce::SpinLock::ScopedLockType sl(wantLock_);
        wantPort_ = port;
        wantBaud_ = baud > 0 ? baud : 115200;
    }
    enabled_.store(enabled);
    if (enabled && !isThreadRunning()) startThread();
    if (!enabled) {
        const std::lock_guard<std::mutex> ll(linkLock_);
        link_.reset();
    }
}

bool OscSerialLink::isOpen() const {
    const std::lock_guard<std::mutex> ll(linkLock_);
    return link_ && link_->isOpen();
}

bool OscSerialLink::send(const std::string& address, float value) {
    std::shared_ptr<serial::Link> link;
    {
        const std::lock_guard<std::mutex> ll(linkLock_);
        link = link_;
    }
    if (!link || !link->isOpen()) return false;
    const auto bytes = osc::slipEncode(osc::encodeValue(address, value));
    return link->write(bytes.data(), (int) bytes.size());
}

std::shared_ptr<serial::Link> OscSerialLink::ensureOpen() {
    std::string port;
    int baud;
    {
        const juce::SpinLock::ScopedLockType sl(wantLock_);
        port = wantPort_;
        baud = wantBaud_;
    }
    const auto path = serial::pickDevice(port, 1);
    serial::Settings settings;
    settings.baud = baud;
    const std::lock_guard<std::mutex> ll(linkLock_);
    if (!link_ || link_->path() != path || link_->settings() != settings) {
        link_ = path.empty() ? nullptr : serial::acquire(path, settings);
        decoder_ = osc::SlipDecoder{};
    }
    return link_ && link_->isOpen() ? link_ : nullptr;
}

void OscSerialLink::run() {
    while (!threadShouldExit()) {
        if (!enabled_.load()) { wait(200); continue; }
        auto link = ensureOpen();
        if (!link) { wait(200); continue; }
        std::uint8_t buf[256];
        const int n = link->read(buf, sizeof(buf), 100);
        if (n <= 0) continue;
        decoder_.feed(buf, n, [this](const std::vector<std::uint8_t>& packet) {
            osc::decodePacket(packet.data(), packet.size(), [this](const osc::Message& m) {
                if (onMessage) onMessage(m);
            });
        });
    }
    const std::lock_guard<std::mutex> ll(linkLock_);
    link_.reset();
}

}
