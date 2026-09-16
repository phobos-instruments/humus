// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SerialIn/SerialIn.h"

#include <cstring>

#include "hum/NamedValues.h"

namespace hum {

namespace {
const char* const kSlotNames[SerialIn::kValues] = {"a", "b", "c", "d", "e", "f", "g", "h"};
}

SerialIn::~SerialIn() {
    stopThread(2000);
    link_.reset();
}

void SerialIn::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    syncText("Port", params.getText("Port"));
    syncText("Parse", params.getText("Parse"));
    if (!isThreadRunning()) startThread();
}

int SerialIn::controlValues(ControlVal* out, int capacity) const {
    const int want = values_.load();
    const int n = capacity < want ? capacity : want;
    for (int i = 0; i < n; ++i) out[i] = {kSlotNames[i], latest_[(size_t) i].load()};
    if (hasParse_.load() && n == want && capacity > n) {
        out[n] = {"match", matchPulse_.load()};
        return n + 1;
    }
    return n;
}

int SerialIn::textLines(std::string* out, int capacity) const {
    const juce::SpinLock::ScopedLockType sl(textLock_);
    int n = 0;
    if (n < capacity) out[n++] = status_.empty() ? "port: looking for a device" : status_;
    if (n < capacity)
        out[n++] = lastGot_.empty() ? "got: nothing yet"
                                    : std::string(lastFit_ ? "got: " : "skipped: ") + serial::printable(lastGot_);
    return n;
}

void SerialIn::process(const float* const*, int, float* const*, int, int, const Transport&) {
    if (const auto* p = params.byName("Device")) deviceIndex_.store((int) p->value);
    const int custom = (int) params.get("Custom", 0.0);
    baud_.store(custom > 0 ? custom : serial::baudFromIndex((int) params.get("BaudRate", 8.0)));
    frame_.store(juce::jlimit(0, 3, (int) params.get("Frame", 0.0)));
    reset_.store(params.get("Reset", 1.0) >= 0.5);
    values_.store(juce::jlimit(1, kValues, (int) params.get("Values", 2.0)));
    const bool held = params.get("Reconnect", 0.0) >= 0.5;
    if (held && !reconnectHeld_) reconnects_.fetch_add(1);
    reconnectHeld_ = held;
    const unsigned m = matches_.load();
    matchPulse_.store(m != seenMatches_ ? 1.0f : 0.0f);
    seenMatches_ = m;
}

bool SerialIn::ensureOpen() {
    std::string want;
    {
        const juce::SpinLock::ScopedLockType sl(textLock_);
        want = portPath_;
    }
    want = serial::pickDevice(want, deviceIndex_.load());
    serial::Settings settings;
    settings.baud = baud_.load();
    settings.frame = frame_.load();
    settings.reset = reset_.load();
    if (!link_ || want != link_->path() || settings != link_->settings()) {
        link_ = serial::acquire(want, settings);
        lineLen_ = 0;
    }
    const bool open = link_ && link_->isOpen();
    {
        const juce::SpinLock::ScopedLockType sl(textLock_);
        status_ = want.empty() ? "port: no device found"
                : open ? "port: " + want + " at " + std::to_string(settings.baud) : "port: waiting for " + want;
    }
    return open;
}

std::string SerialIn::currentParse() {
    const juce::SpinLock::ScopedLockType sl(textLock_);
    return parse_;
}

void SerialIn::publish(const serial::Reading* readings, const bool* captured) {
    for (int v = 0; v < kValues; ++v) {
        if (!captured[v]) continue;
        latest_[(size_t) v].store(readings[v].value);
        NamedValues::instance().post(readings[v].name, readings[v].value);
    }
    matches_.fetch_add(1);
}

void SerialIn::takeLine() {
    line_[lineLen_] = '\0';
    const std::string pattern = currentParse();
    serial::Reading readings[kValues];
    bool captured[kValues];
    bool fit = true;
    if (!pattern.empty()) {
        fit = serial::matchLine(pattern, line_, readings, captured, kValues) >= 0;
    } else {
        const int got = serial::parseLine(line_, readings, kValues);
        for (int v = 0; v < kValues; ++v) captured[v] = v < got;
    }
    {
        const juce::SpinLock::ScopedLockType sl(textLock_);
        lastGot_.assign(line_, lineLen_);
        lastFit_ = fit;
    }
    if (fit) publish(readings, captured);
}

void SerialIn::takePackets() {
    const std::string pattern = currentParse();
    serial::Reading readings[kValues];
    bool captured[kValues];
    size_t start = 0;
    while (start < lineLen_) {
        size_t consumed = 0;
        const int got = serial::matchBuffer(pattern, line_ + start, lineLen_ - start, false,
                                            readings, captured, kValues, consumed);
        if (got == serial::kNeedMore) break;
        if (got < 0) { ++start; continue; }
        {
            const juce::SpinLock::ScopedLockType sl(textLock_);
            lastGot_.assign(line_ + start, consumed);
            lastFit_ = true;
        }
        publish(readings, captured);
        start += consumed > 0 ? consumed : 1;
    }
    if (start > 0) {
        std::memmove(line_, line_ + start, lineLen_ - start);
        lineLen_ -= start;
    }
}

void SerialIn::takeBytes(const char* buf, int n) {
    const bool binary = binary_.load();
    for (int i = 0; i < n; ++i) {
        const char c = buf[i];
        if (binary) {
            if (lineLen_ + 1 >= sizeof(line_)) lineLen_ = 0;
            line_[lineLen_++] = c;
            continue;
        }
        if (c == '\n' || c == '\r') {
            if (lineLen_ > 0) takeLine();
            lineLen_ = 0;
        } else if (lineLen_ + 1 < sizeof(line_)) {
            line_[lineLen_++] = c;
        } else {
            lineLen_ = 0;
        }
    }
    if (binary) takePackets();
}

void SerialIn::run() {
    while (!threadShouldExit()) {
        if (!ensureOpen()) { wait(200); continue; }
        if (const unsigned r = reconnects_.load(); r != seenReconnects_) {
            seenReconnects_ = r;
            link_->reconnect();
            wait(200);
            continue;
        }
        char buf[128];
        const int n = link_->read(buf, sizeof(buf), 200);
        if (n > 0) takeBytes(buf, n);
    }
    link_.reset();
}

}
