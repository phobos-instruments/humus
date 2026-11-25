// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/OscUdpListener.h"

namespace hum {

namespace {
constexpr int kMaxDatagram = 65536;
constexpr int kWaitMs = 100;
}

bool OscUdpListener::open(int port) {
    close();
    auto socket = std::make_shared<juce::DatagramSocket>(false);
    socket->setEnablePortReuse(false);
    if (!socket->bindToPort(port)) return false;
    {
        const std::lock_guard<std::mutex> l(lock_);
        socket_ = std::move(socket);
    }
    startThread();
    return true;
}

void OscUdpListener::close() {
    signalThreadShouldExit();
    std::shared_ptr<juce::DatagramSocket> socket;
    {
        const std::lock_guard<std::mutex> l(lock_);
        socket = std::move(socket_);
    }
    if (socket) socket->shutdown();
    stopThread(2000);
}

bool OscUdpListener::isOpen() const {
    const std::lock_guard<std::mutex> l(lock_);
    return socket_ != nullptr;
}

void OscUdpListener::run() {
    std::shared_ptr<juce::DatagramSocket> socket;
    {
        const std::lock_guard<std::mutex> l(lock_);
        socket = socket_;
    }
    if (!socket) return;
    Packet buffer((size_t) kMaxDatagram);
    while (!threadShouldExit()) {
        const int ready = socket->waitUntilReady(true, kWaitMs);
        if (ready < 0) return;
        if (ready == 0) continue;
        juce::String senderIp;
        int senderPort = 0;
        const int n = socket->read(buffer.data(), kMaxDatagram, false, senderIp, senderPort);
        if (n <= 0 || threadShouldExit()) continue;
        if (onPacket) onPacket(Packet(buffer.begin(), buffer.begin() + n), senderIp.toStdString());
    }
}

}
