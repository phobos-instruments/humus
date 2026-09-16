// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SerialOut/SerialOut.h"

#include <cmath>

namespace hum {

namespace {
const char* const kDefaultFormat = "%1 %2\\n";
}

SerialOut::~SerialOut() {
    stopThread(2000);
    link_.reset();
}

void SerialOut::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    syncText("Port", params.getText("Port"));
    syncText("Format", params.getText("Format"));
    if (!isThreadRunning()) startThread();
}

bool SerialOut::controlInlet(const std::string& param) const {
    if (param == "Trigger") return (int) params.get("Send", 0.0) == kOnTrigger;
    if (param == "Message") return lines_.load() > 1;
    if (param.size() == 6 && param.compare(0, 5, "Value") == 0)
        return param[5] - '0' <= juce::jlimit(1, kValues, (int) params.get("Values", 2.0));
    return true;
}

void SerialOut::setSocketSource(const std::string& param, const std::string& sourceName) {
    if (param.size() != 6 || param.compare(0, 5, "Value") != 0) return;
    const int slot = param[5] - '1';
    if (slot < 0 || slot >= kValues) return;
    const juce::SpinLock::ScopedLockType sl(textLock_);
    sourceNames_[slot] = sourceName;
}

int SerialOut::textLines(std::string* out, int capacity) const {
    const juce::SpinLock::ScopedLockType sl(textLock_);
    int n = 0;
    if (n < capacity) out[n++] = status_.empty() ? "port: looking for a device" : status_;
    if (n < capacity) out[n++] = lastSent_.empty() ? "sent: nothing yet" : "sent: " + serial::printable(lastSent_);
    return n;
}

std::string SerialOut::socketSourceForTest(int slot) const {
    const juce::SpinLock::ScopedLockType sl(textLock_);
    return slot >= 0 && slot < kValues ? sourceNames_[slot] : std::string();
}

void SerialOut::readSlots() {
    char valueName[8] = "Value1", scaleName[8] = "Scale1", intName[8] = "Int1";
    for (int c = 0; c < kValues; ++c) {
        const char digit = (char) ('1' + c);
        valueName[5] = digit;
        scaleName[5] = digit;
        intName[3] = digit;
        latest_[(size_t) c].store((float) params.get(valueName, 0.0));
        scale_[(size_t) c].store((float) params.get(scaleName, 1.0));
        integer_[(size_t) c].store(params.get(intName, 0.0) >= 0.5);
    }
}

void SerialOut::readTrigger() {
    const bool high = params.get("Trigger", 0.0) >= 0.5;
    if (primed_ && high && !lastTrigger_) triggers_.fetch_add(1);
    lastTrigger_ = high;
    primed_ = true;
}

void SerialOut::process(const float* const*, int, float* const*, int, int, const Transport&) {
    readSlots();
    readTrigger();
    const bool held = params.get("Reconnect", 0.0) >= 0.5;
    if (held && !reconnectHeld_) reconnects_.fetch_add(1);
    reconnectHeld_ = held;
    if (const auto* p = params.byName("Rate")) rateHz_.store((float) p->value);
    if (const auto* p = params.byName("Device")) deviceIndex_.store((int) p->value);
    const int custom = (int) params.get("Custom", 0.0);
    baud_.store(custom > 0 ? custom : serial::baudFromIndex((int) params.get("BaudRate", 8.0)));
    frame_.store(juce::jlimit(0, 3, (int) params.get("Frame", 0.0)));
    reset_.store(params.get("Reset", 1.0) >= 0.5);
    sendMode_.store(juce::jlimit(0, 2, (int) params.get("Send", 0.0)));
    message_.store(juce::jlimit(0, kValues - 1, (int) std::lround(params.get("Message", 1.0)) - 1));
    values_.store(juce::jlimit(1, kValues, (int) params.get("Values", 2.0)));
}

bool SerialOut::ensureOpen() {
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
        sentAny_ = false;
    }
    const bool open = link_ && link_->isOpen();
    note(want.empty() ? "port: no device found" : (open ? "port: " + want + " at " + std::to_string(settings.baud)
                                                        : "port: waiting for " + want), nullptr);
    return open;
}

std::string SerialOut::currentFormat() {
    const juce::SpinLock::ScopedLockType sl(textLock_);
    if (format_.empty()) return kDefaultFormat;
    const int want = juce::jmin(message_.load(), lines_.load() - 1);
    size_t start = 0;
    for (int line = 0; line < want; ++line) {
        const size_t nl = format_.find('\n', start);
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    const size_t end = format_.find('\n', start);
    return format_.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

bool SerialOut::shouldSend(const serial::Slot* slots, int count) {
    switch (sendMode_.load()) {
        case kOnTrigger: {
            const unsigned t = triggers_.load();
            if (t == sentTriggers_) return false;
            sentTriggers_ = t;
            return true;
        }
        case kOnChange: {
            bool changed = !sentAny_ || message_.load() != sentMessage_;
            for (int i = 0; i < count && !changed; ++i)
                changed = std::abs(slots[i].value - sent_[i]) > 1e-4f;
            return changed;
        }
        default:
            return true;
    }
}

void SerialOut::run() {
    while (!threadShouldExit()) {
        const float r = juce::jlimit(1.0f, 200.0f, rateHz_.load());
        const int intervalMs = juce::jmax(5, (int) (1000.0f / r));
        if (!ensureOpen()) { wait(intervalMs); continue; }
        if (const unsigned asked = reconnects_.load(); asked != seenReconnects_) {
            seenReconnects_ = asked;
            link_->reconnect();
            wait(intervalMs);
            continue;
        }
        serial::Slot slots[kValues];
        for (int c = 0; c < kValues; ++c) {
            slots[c].value = latest_[(size_t) c].load();
            slots[c].scale = scale_[(size_t) c].load();
            slots[c].integer = integer_[(size_t) c].load();
        }
        {
            const juce::SpinLock::ScopedLockType sl(textLock_);
            for (int c = 0; c < kValues; ++c) slots[c].name = sourceNames_[c];
        }
        if (shouldSend(slots, values_.load())) {
            const std::string msg = serial::formatLine(currentFormat(), slots, kValues);
            if (!msg.empty() && link_->write(msg.data(), (int) msg.size())) {
                for (int c = 0; c < kValues; ++c) sent_[c] = slots[c].value;
                sentAny_ = true;
                sentMessage_ = message_.load();
                const juce::SpinLock::ScopedLockType sl(textLock_);
                lastSent_ = msg;
            }
        }
        wait(intervalMs);
    }
    link_.reset();
}

}
