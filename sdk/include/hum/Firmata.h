// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>

namespace hum::firmata {

constexpr std::uint8_t kDigitalMessage = 0x90;
constexpr std::uint8_t kAnalogMessage = 0xE0;
constexpr std::uint8_t kReportAnalog = 0xC0;
constexpr std::uint8_t kReportDigital = 0xD0;
constexpr std::uint8_t kSetPinMode = 0xF4;
constexpr std::uint8_t kSetDigitalPinValue = 0xF5;
constexpr std::uint8_t kProtocolVersion = 0xF9;
constexpr std::uint8_t kSystemReset = 0xFF;
constexpr std::uint8_t kSysexStart = 0xF0;
constexpr std::uint8_t kSysexEnd = 0xF7;

constexpr std::uint8_t kSysexAnalogMappingQuery = 0x69;
constexpr std::uint8_t kSysexAnalogMappingResponse = 0x6A;
constexpr std::uint8_t kSysexCapabilityQuery = 0x6B;
constexpr std::uint8_t kSysexCapabilityResponse = 0x6C;
constexpr std::uint8_t kSysexExtendedAnalog = 0x6F;
constexpr std::uint8_t kSysexReportFirmware = 0x79;
constexpr std::uint8_t kSysexSamplingInterval = 0x7A;

enum PinMode : std::uint8_t { kModeInput = 0, kModeOutput = 1, kModeAnalog = 2, kModePwm = 3 };

constexpr int kMaxPins = 128;
constexpr int kMaxChannels = 16;
constexpr int kDefaultAnalogBits = 10;

struct Event {
    enum Kind { kNone, kAnalog, kDigitalPort, kVersion, kFirmware, kCapabilities, kAnalogMapping };
    Kind kind = kNone;
    int index = 0;
    int value = 0;
};

class Parser {
public:
    Parser() { clear(); }

    void clear() {
        state_ = kIdle;
        sysexLen_ = 0;
        sysexOverflow_ = false;
        for (auto& b : bitsByPin_) b = 0;
        for (auto& b : bitsByChannel_) b = kDefaultAnalogBits;
        for (auto& p : pinByChannel_) p = -1;
    }

    bool feed(std::uint8_t b, Event& ev) {
        ev = Event{};
        if (b == kSysexStart) { state_ = kSysex; sysexLen_ = 0; sysexOverflow_ = false; return false; }
        if (b == kSysexEnd) {
            const bool inSysex = state_ == kSysex;
            state_ = kIdle;
            return inSysex && !sysexOverflow_ && finishSysex(ev);
        }
        if (b & 0x80) return startMessage(b);
        if (state_ == kSysex) {
            if (sysexLen_ < (int) sizeof(sysex_)) sysex_[sysexLen_++] = b;
            else sysexOverflow_ = true;
            return false;
        }
        if (state_ != kData) return false;
        data_[got_++] = b;
        if (got_ < need_) return false;
        state_ = kIdle;
        return finishMessage(ev);
    }

    int analogBits(int channel) const {
        return channel >= 0 && channel < kMaxChannels ? bitsByChannel_[channel] : kDefaultAnalogBits;
    }
    int pinForChannel(int channel) const {
        return channel >= 0 && channel < kMaxChannels ? pinByChannel_[channel] : -1;
    }

private:
    enum State { kIdle, kData, kSysex };

    bool startMessage(std::uint8_t b) {
        const std::uint8_t cmd = b < 0xF0 ? (std::uint8_t) (b & 0xF0) : b;
        channel_ = b < 0xF0 ? (b & 0x0F) : 0;
        command_ = cmd;
        got_ = 0;
        switch (cmd) {
            case kDigitalMessage: case kAnalogMessage: case kSetPinMode:
            case kSetDigitalPinValue: case kProtocolVersion:
                need_ = 2; break;
            case kReportAnalog: case kReportDigital:
                need_ = 1; break;
            default:
                state_ = kIdle;
                return false;
        }
        state_ = kData;
        return false;
    }

    bool finishMessage(Event& ev) {
        switch (command_) {
            case kAnalogMessage:
                ev.kind = Event::kAnalog;
                ev.index = channel_;
                ev.value = data_[0] | (data_[1] << 7);
                return true;
            case kDigitalMessage:
                ev.kind = Event::kDigitalPort;
                ev.index = channel_;
                ev.value = data_[0] | (data_[1] << 7);
                return true;
            case kProtocolVersion:
                ev.kind = Event::kVersion;
                ev.index = data_[0];
                ev.value = data_[1];
                return true;
            default:
                return false;
        }
    }

    bool finishSysex(Event& ev) {
        if (sysexLen_ < 1) return false;
        switch (sysex_[0]) {
            case kSysexReportFirmware:
                ev.kind = Event::kFirmware;
                if (sysexLen_ >= 3) { ev.index = sysex_[1]; ev.value = sysex_[2]; }
                return true;
            case kSysexCapabilityResponse: {
                int pin = 0;
                int i = 1;
                while (i < sysexLen_ && pin < kMaxPins) {
                    if (sysex_[i] == 0x7F) { ++pin; ++i; continue; }
                    if (i + 1 >= sysexLen_) break;
                    if (sysex_[i] == kModeAnalog) bitsByPin_[pin] = sysex_[i + 1];
                    i += 2;
                }
                mapChannels();
                ev.kind = Event::kCapabilities;
                ev.value = pin;
                return true;
            }
            case kSysexAnalogMappingResponse: {
                for (auto& p : pinByChannel_) p = -1;
                for (int pin = 0; pin + 1 < sysexLen_ && pin < kMaxPins; ++pin) {
                    const int ch = sysex_[pin + 1];
                    if (ch < kMaxChannels && pinByChannel_[ch] < 0) pinByChannel_[ch] = pin;
                }
                mapChannels();
                ev.kind = Event::kAnalogMapping;
                ev.value = sysexLen_ - 1;
                return true;
            }
            case kSysexExtendedAnalog:
                if (sysexLen_ < 3) return false;
                ev.kind = Event::kAnalog;
                ev.index = sysex_[1];
                ev.value = sysex_[2] | (sysexLen_ > 3 ? sysex_[3] << 7 : 0)
                         | (sysexLen_ > 4 ? sysex_[4] << 14 : 0);
                return true;
            default:
                return false;
        }
    }

    void mapChannels() {
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            const int pin = pinByChannel_[ch];
            bitsByChannel_[ch] = pin >= 0 && bitsByPin_[pin] > 0 ? bitsByPin_[pin] : kDefaultAnalogBits;
        }
    }

    State state_ = kIdle;
    std::uint8_t command_ = 0;
    std::uint8_t channel_ = 0;
    int need_ = 0;
    int got_ = 0;
    std::uint8_t data_[2]{};
    std::uint8_t sysex_[512]{};
    int sysexLen_ = 0;
    bool sysexOverflow_ = false;
    std::uint8_t bitsByPin_[kMaxPins]{};
    std::uint8_t bitsByChannel_[kMaxChannels]{};
    int pinByChannel_[kMaxChannels]{};
};

inline float normalise(int value, int bits) {
    const int top = (1 << (bits < 1 ? 1 : (bits > 24 ? 24 : bits))) - 1;
    const float v = (float) value / (float) top;
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline int setPinMode(std::uint8_t* out, int pin, PinMode mode) {
    out[0] = kSetPinMode;
    out[1] = (std::uint8_t) (pin & 0x7F);
    out[2] = (std::uint8_t) mode;
    return 3;
}

inline int reportAnalog(std::uint8_t* out, int channel, bool on) {
    out[0] = (std::uint8_t) (kReportAnalog | (channel & 0x0F));
    out[1] = on ? 1 : 0;
    return 2;
}

inline int reportDigital(std::uint8_t* out, int port, bool on) {
    out[0] = (std::uint8_t) (kReportDigital | (port & 0x0F));
    out[1] = on ? 1 : 0;
    return 2;
}

inline int digitalWrite(std::uint8_t* out, int pin, bool high) {
    out[0] = kSetDigitalPinValue;
    out[1] = (std::uint8_t) (pin & 0x7F);
    out[2] = high ? 1 : 0;
    return 3;
}

inline int analogWrite(std::uint8_t* out, int pin, int value) {
    if (pin < kMaxChannels) {
        out[0] = (std::uint8_t) (kAnalogMessage | (pin & 0x0F));
        out[1] = (std::uint8_t) (value & 0x7F);
        out[2] = (std::uint8_t) ((value >> 7) & 0x7F);
        return 3;
    }
    out[0] = kSysexStart;
    out[1] = kSysexExtendedAnalog;
    out[2] = (std::uint8_t) (pin & 0x7F);
    out[3] = (std::uint8_t) (value & 0x7F);
    out[4] = (std::uint8_t) ((value >> 7) & 0x7F);
    out[5] = kSysexEnd;
    return 6;
}

inline int samplingInterval(std::uint8_t* out, int ms) {
    out[0] = kSysexStart;
    out[1] = kSysexSamplingInterval;
    out[2] = (std::uint8_t) (ms & 0x7F);
    out[3] = (std::uint8_t) ((ms >> 7) & 0x7F);
    out[4] = kSysexEnd;
    return 5;
}

inline int capabilityQuery(std::uint8_t* out) {
    out[0] = kSysexStart;
    out[1] = kSysexCapabilityQuery;
    out[2] = kSysexEnd;
    return 3;
}

inline int analogMappingQuery(std::uint8_t* out) {
    out[0] = kSysexStart;
    out[1] = kSysexAnalogMappingQuery;
    out[2] = kSysexEnd;
    return 3;
}

}
