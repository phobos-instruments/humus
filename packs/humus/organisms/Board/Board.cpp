// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Board/Board.h"

#include <cmath>
#include <cstdint>

namespace hum {

namespace {
constexpr std::uint32_t kBootWaitMs = 3000;
constexpr int kReadTimeoutMs = 20;
const char* const kOutletNames[Board::kOutlets] = {"out1", "out2", "out3", "out4", "out5", "out6"};
const int kDefaultPins[Board::kInlets] = {13, 11, 10, 9, 6, 5};
}

Board::~Board() {
    stopThread(2000);
    link_.reset();
}

void Board::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    syncPort(params.getText("Port"));
    if (!isThreadRunning()) startThread();
}

bool Board::controlInlet(const std::string& param) const {
    if (param.size() == 3 && param.compare(0, 2, "In") == 0)
        return param[2] - '0' <= juce::jlimit(1, kInlets, (int) params.get("Inlets", 2.0));
    return true;
}

int Board::controlValues(ControlVal* out, int capacity) const {
    const int want = outlets_.load();
    const int n = capacity < want ? capacity : want;
    for (int i = 0; i < n; ++i) out[i] = {kOutletNames[i], outletValue(i)};
    return n;
}

int Board::textLines(std::string* out, int capacity) const {
    if (capacity < 1) return 0;
    const juce::SpinLock::ScopedLockType sl(portLock_);
    out[0] = status_.empty() ? "port: looking for a device" : status_;
    return 1;
}

void Board::readParams() {
    if (const auto* p = params.byName("Device")) deviceIndex_.store((int) p->value);
    const int custom = (int) params.get("Custom", 0.0);
    baud_.store(custom > 0 ? custom : serial::baudFromIndex((int) params.get("BaudRate", 7.0)));
    reset_.store(params.get("Reset", 1.0) >= 0.5);
    sampleMs_.store(juce::jlimit(1, 100, (int) params.get("Rate", 19.0)));
    outlets_.store(juce::jlimit(1, kOutlets, (int) params.get("Outlets", 2.0)));
    inlets_.store(juce::jlimit(1, kInlets, (int) params.get("Inlets", 2.0)));
    const bool held = params.get("Reconnect", 0.0) >= 0.5;
    if (held && !reconnectHeld_) reconnects_.fetch_add(1);
    reconnectHeld_ = held;
    char analogName[8] = "Analog1", pinName[8] = "Pin1", modeName[8] = "Mode1", inName[4] = "In1";
    for (int i = 0; i < kOutlets; ++i) {
        analogName[6] = (char) ('1' + i);
        channel_[(size_t) i].store(juce::jlimit(0, kChannels - 1, (int) params.get(analogName, (double) i)));
    }
    for (int i = 0; i < kInlets; ++i) {
        pinName[3] = (char) ('1' + i);
        modeName[4] = (char) ('1' + i);
        inName[2] = (char) ('1' + i);
        pin_[(size_t) i].store((int) params.get(pinName, (double) kDefaultPins[i]));
        pwm_[(size_t) i].store(params.get(modeName, 0.0) >= 0.5 ? 1 : 0);
        inlet_[(size_t) i].store((float) params.get(inName, 0.0));
    }
}

void Board::process(const float* const*, int, float* const*, int, int, const Transport&) {
    readParams();
}

bool Board::ensureOpen() {
    std::string want;
    {
        const juce::SpinLock::ScopedLockType sl(portLock_);
        want = portPath_;
    }
    want = serial::pickDevice(want, deviceIndex_.load());
    serial::Settings settings;
    settings.baud = baud_.load();
    settings.reset = reset_.load();
    if (!link_ || want != link_->path() || settings != link_->settings()) {
        link_ = serial::acquire(want, settings);
        configured_.store(false);
        seenGeneration_ = 0;
    }
    if (!link_ || !link_->isOpen()) {
        configured_.store(false);
        note(want.empty() ? "port: no device found" : "port: waiting for " + want);
        return false;
    }
    if (const unsigned g = link_->generation(); g != seenGeneration_) {
        seenGeneration_ = g;
        onOpened();
    }
    note(configured_.load() ? "board: talking on " + want + (mappingKnown_ ? ", pins mapped" : "")
                            : "board: booting on " + want);
    return true;
}

void Board::onOpened() {
    parser_.clear();
    configured_.store(false);
    openedAt_ = juce::Time::getMillisecondCounter();
    versionSeen_ = false;
    mappingKnown_ = false;
    sentSampleMs_ = -1;
    for (int c = 0; c < kChannels; ++c) {
        reported_[c] = false;
        analogModeSent_[c] = false;
    }
    for (int i = 0; i < kInlets; ++i) {
        sentPin_[i] = -1;
        sentPwm_[i] = -1;
        sentValue_[i] = -1;
    }
}

bool Board::send(const std::uint8_t* bytes, int n) {
    if (link_ && link_->write(bytes, n)) return true;
    configured_.store(false);
    return false;
}

void Board::takeEvent(const firmata::Event& ev) {
    switch (ev.kind) {
        case firmata::Event::kAnalog:
            if (ev.index < kChannels)
                analog_[(size_t) ev.index].store(
                    firmata::normalise(ev.value, parser_.analogBits(ev.index)));
            break;
        case firmata::Event::kVersion:
        case firmata::Event::kFirmware:
            versionSeen_ = true;
            break;
        case firmata::Event::kAnalogMapping:
            mappingKnown_ = true;
            for (int c = 0; c < kChannels; ++c) analogModeSent_[c] = false;
            break;
        default:
            break;
    }
}

void Board::configure() {
    std::uint8_t msg[8];
    if (!send(msg, firmata::capabilityQuery(msg))) return;
    if (!send(msg, firmata::analogMappingQuery(msg))) return;
    configured_.store(true);
}

void Board::pumpReports() {
    bool wanted[kChannels]{};
    const int outlets = outlets_.load();
    for (int o = 0; o < outlets; ++o) wanted[channel_[(size_t) o].load()] = true;
    std::uint8_t msg[4];
    for (int c = 0; c < kChannels; ++c) {
        if (wanted[c] && mappingKnown_ && !analogModeSent_[c]) {
            if (const int pin = parser_.pinForChannel(c); pin >= 0)
                if (!send(msg, firmata::setPinMode(msg, pin, firmata::kModeAnalog))) return;
            analogModeSent_[c] = true;
        }
        if (wanted[c] == reported_[c]) continue;
        if (!send(msg, firmata::reportAnalog(msg, c, wanted[c]))) return;
        reported_[c] = wanted[c];
    }
}

void Board::pumpOutputs() {
    std::uint8_t msg[8];
    if (const int ms = sampleMs_.load(); ms != sentSampleMs_) {
        if (!send(msg, firmata::samplingInterval(msg, ms))) return;
        sentSampleMs_ = ms;
    }
    const int inlets = inlets_.load();
    for (int i = 0; i < inlets; ++i) {
        const int pin = pin_[(size_t) i].load();
        const int pwm = pwm_[(size_t) i].load();
        if (pin != sentPin_[i] || pwm != sentPwm_[i]) {
            if (!send(msg, firmata::setPinMode(msg, pin, pwm ? firmata::kModePwm
                                                            : firmata::kModeOutput)))
                return;
            sentPin_[i] = pin;
            sentPwm_[i] = pwm;
            sentValue_[i] = -1;
        }
        const float x = inlet_[(size_t) i].load();
        const int value = pwm ? juce::jlimit(0, 255, (int) std::lround(x * 255.0f))
                              : (x > 0.5f ? 1 : 0);
        if (value == sentValue_[i]) continue;
        const int n = pwm ? firmata::analogWrite(msg, pin, value)
                          : firmata::digitalWrite(msg, pin, value != 0);
        if (!send(msg, n)) return;
        sentValue_[i] = value;
    }
}

void Board::run() {
    while (!threadShouldExit()) {
        if (!ensureOpen()) { wait(200); continue; }
        if (const unsigned r = reconnects_.load(); r != seenReconnects_) {
            seenReconnects_ = r;
            link_->reconnect();
            wait(200);
            continue;
        }
        std::uint8_t buf[64];
        const int n = link_->read(buf, sizeof(buf), kReadTimeoutMs);
        firmata::Event ev;
        for (int i = 0; i < n; ++i)
            if (parser_.feed(buf[i], ev)) takeEvent(ev);
        if (!link_->isOpen()) { configured_.store(false); continue; }
        if (!configured_.load()) {
            const bool booted = versionSeen_
                || juce::Time::getMillisecondCounter() - openedAt_ > kBootWaitMs;
            if (booted) configure();
            continue;
        }
        pumpReports();
        if (configured_.load()) pumpOutputs();
    }
    link_.reset();
}

}
