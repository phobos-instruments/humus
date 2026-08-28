#include "SerialOut/SerialOut.h"

#include <cstdio>

#if !JUCE_WINDOWS
#include <errno.h>
#include <unistd.h>
#endif

#include "hum/Number.h"
#include "hum/SerialPort.h"

namespace hum {

SerialOut::~SerialOut() {
    stopThread(2000);
    closePort();
}

void SerialOut::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    if (!isThreadRunning()) startThread();
}

void SerialOut::process(const float* const* in, int numIn,
                        float* const* out, int numOut,
                        int numSamples, const Transport&) {
    juce::ignoreUnused(out, numOut);
    for (int c = 0; c < 2; ++c)
        latest_[(size_t) c].store(c < numIn && in && in[c] && numSamples > 0
                                      ? in[c][numSamples - 1] : 0.0f);
    if (const auto* p = params.byName("Rate")) rateHz_.store((float) p->value);
    if (const auto* p = params.byName("Device")) deviceIndex_.store((int) p->value);
    baud_.store(serial::baudFromIndex((int) params.get("BaudRate", 8.0)));
    if (const auto* p = params.byName("Baud")) baud_.store((int) p->value);
    if (const auto* p = params.byName("ASCII")) ascii_.store(p->value >= 0.5);
    if (const auto* p = params.byName("Port")) {
        if (p->text != cachedText_) {
            cachedText_ = p->text;
            const juce::SpinLock::ScopedLockType sl(portLock_);
            portPath_ = cachedText_;
        }
    }
}

void SerialOut::run() {
#if !JUCE_WINDOWS
    while (!threadShouldExit()) {
        std::string want;
        {
            const juce::SpinLock::ScopedLockType sl(portLock_);
            want = portPath_;
        }
        if (want.empty()) {
            const auto devices = serial::listDevices();
            const int idx = deviceIndex_.load();
            if (idx >= 2 && (size_t) (idx - 2) < devices.size())
                want = devices[(size_t) (idx - 2)];
            else if (!devices.empty())
                want = devices.front();
        }
        const int baud = baud_.load();
        if (fd_ < 0 || want != openedPath_ || baud != openedBaud_) {
            closePort();
            if (want.empty() || (fd_ = serial::openPort(want, baud, true)) < 0) {
                wait(1000);
                continue;
            }
            openedPath_ = want;
            openedBaud_ = baud;
        }
        char buf[64];
        int n;
        if (ascii_.load()) {
            n = std::snprintf(buf, sizeof(buf), "%.4f %.4f\n",
                              (double) latest_[0].load(), (double) latest_[1].load());
            fixDecimalPoint(buf);
        } else {
            auto byteOf = [](float v) {
                return (char) (unsigned char) juce::jlimit(0, 255, (int) (v * 255.0f + 0.5f));
            };
            buf[0] = byteOf(latest_[0].load());
            buf[1] = byteOf(latest_[1].load());
            n = 2;
        }
        if (n > 0 && ::write(fd_, buf, (size_t) n) < 0 && errno != EAGAIN)
            closePort();
        const float r = juce::jlimit(1.0f, 200.0f, rateHz_.load());
        wait(juce::jmax(5, (int) (1000.0f / r)));
    }
#endif
    closePort();
}

void SerialOut::closePort() {
#if !JUCE_WINDOWS
    if (fd_ >= 0) ::close(fd_);
#endif
    fd_ = -1;
    openedPath_.clear();
    openedBaud_ = 0;
}

}
