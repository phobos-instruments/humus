#include "SerialIn/SerialIn.h"

#include <cmath>

#if !JUCE_WINDOWS
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#endif

#include "hum/SerialPort.h"

namespace hum {

SerialIn::~SerialIn() {
    stopThread(2000);
    closePort();
}

void SerialIn::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
    if (!isThreadRunning()) startThread();
}

void SerialIn::reset() {
    smoothed_[0] = latest_[0].load();
    smoothed_[1] = latest_[1].load();
}

void SerialIn::process(const float* const*, int,
                       float* const* out, int numOut,
                       int numSamples, const Transport&) {
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
    const double ms = params.get("Smooth", 20.0);
    smoothK_ = ms <= 0.0 ? 1.0f
                         : (float) (1.0 - std::exp(-1.0 / (ms * 0.001 * sampleRate_)));
    for (int c = 0; c < 2 && c < numOut; ++c) {
        const float target = latest_[(size_t) c].load();
        float y = smoothed_[c];
        float* dst = out[c];
        for (int i = 0; i < numSamples; ++i) {
            y += (target - y) * smoothK_;
            dst[i] = y;
        }
        smoothed_[c] = y;
    }
}

void SerialIn::run() {
#if !JUCE_WINDOWS
    char line[128];
    size_t lineLen = 0;
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
            lineLen = 0;
            if (want.empty() || (fd_ = serial::openPort(want, baud, false)) < 0) {
                wait(1000);
                continue;
            }
            openedPath_ = want;
            openedBaud_ = baud;
        }
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd_, &set);
        timeval tv{0, 200 * 1000};
        const int ready = ::select(fd_ + 1, &set, nullptr, nullptr, &tv);
        if (ready < 0 && errno != EINTR) { closePort(); continue; }
        if (ready <= 0) continue;
        char buf[64];
        const ssize_t n = ::read(fd_, buf, sizeof(buf));
        if (n <= 0) {
            if (n == 0 || (errno != EAGAIN && errno != EINTR))
                closePort();
            continue;
        }
        for (ssize_t i = 0; i < n; ++i) {
            if (!ascii_.load()) {
                latest_[0].store((float) (unsigned char) buf[i] / 255.0f);
                continue;
            }
            const char c = buf[i];
            if (c == '\n' || c == '\r') {
                if (lineLen > 0) {
                    line[lineLen] = '\0';
                    float vals[2];
                    const int got = serial::parseFloats(line, vals, 2);
                    for (int v = 0; v < got; ++v) latest_[(size_t) v].store(vals[v]);
                }
                lineLen = 0;
            } else if (lineLen + 1 < sizeof(line)) {
                line[lineLen++] = c;
            } else {
                lineLen = 0;
            }
        }
    }
#endif
    closePort();
}

void SerialIn::closePort() {
#if !JUCE_WINDOWS
    if (fd_ >= 0) ::close(fd_);
#endif
    fd_ = -1;
    openedPath_.clear();
    openedBaud_ = 0;
}

}
