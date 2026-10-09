// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Deck/Deck.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include "common/TimecodeTables.h"

namespace hum {

namespace {

constexpr double kAgreeSeconds = 0.05;
constexpr double kTrustAfterSeconds = 0.2;
constexpr double kJumpSeconds = 0.3;
constexpr double kSettleSeconds = 0.25;
constexpr double kMostNudge = 0.05;

double leadInOf(const std::string& format) {
    for (const auto& record : timecodeRecords())
        if (!format.empty() && format == record.format) return record.leadInSeconds;
    return 0.0;
}

}

void Deck::captureField(int numSamples) {
    unsigned w = fieldAt_.load(std::memory_order_relaxed);
    double sll = 0.0, srr = 0.0, slr = 0.0;
    for (int n = 0; n < numSamples; ++n) {
        const float l = tcL_[(size_t) n], r = tcR_[(size_t) n];
        field_[2 * w] = l;
        field_[2 * w + 1] = r;
        w = (w + 1) % (unsigned) StereoFieldSource::kFieldPairs;
        sll += (double) l * l;
        srr += (double) r * r;
        slr += (double) l * r;
    }
    fieldAt_.store(w, std::memory_order_relaxed);
    const double denom = std::sqrt(sll * srr);
    if (denom > 1e-12) fieldCorr_.store((float) (slr / denom), std::memory_order_relaxed);
    fieldStamp_.fetch_add(1, std::memory_order_relaxed);
}

void Deck::chooseTimecodeRecord() {
    const auto& records = timecodeRecords();
    const int record = std::clamp((int) params.get("TimecodeRecord", 0.0), 0, (int) records.size() - 1);
    const bool rpm45 = params.get("Timecode45", 0.0) >= 0.5;
    if (record == tcRecord_ && rpm45 == tc45_) return;
    tcRecord_ = record;
    tc45_ = rpm45;
    forgetNeedle();
    vinyl_.chooseRecord(records[(std::size_t) record].format, rpm45);
}

void Deck::forgetNeedle() {
    needleLast_ = -1.0e9;
    needleSince_ = needleClock_;
}

bool Deck::trustNeedle(const ControlVinyl::Reading& reading) {
    const double expected = needleLast_ + (needleClock_ - needleSeenAt_) * reading.speed;
    if (std::abs(reading.needleSeconds - expected) >= kAgreeSeconds) needleSince_ = needleClock_;
    needleLast_ = reading.needleSeconds;
    needleSeenAt_ = needleClock_;
    return needleClock_ - needleSince_ >= kTrustAfterSeconds;
}

double Deck::followNeedle(const ControlVinyl::Reading& reading, double fileSr, int64_t len,
                          double hostSr, double blockSeconds) {
    needleClock_ += blockSeconds;
    if (params.get("TimecodeAbsolute", 0.0) < 0.5 || !reading.toneArriving) {
        forgetNeedle();
        return 0.0;
    }
    if (!reading.located || !trustNeedle(reading)) return 0.0;
    const double target = (reading.needleSeconds - leadInOf(vinyl_.format())) * fileSr;
    const double error = target - (readPos_ + beyond_);
    if (std::abs(error) > kJumpSeconds * fileSr) {
        readPos_ = std::clamp(target, 0.0, (double) len);
        beyond_ = target - readPos_;
        declick_ = 0.0;
        return 0.0;
    }
    return std::clamp(error / (kSettleSeconds * hostSr), -kMostNudge, kMostNudge);
}

Deck::TimecodeDrive Deck::readTimecode(const float* const* in, int numIn, int numSamples,
                                       double hostSr, double baseRate, double fileSr, int64_t len) {
    TimecodeDrive drive;
    const bool tcOn = params.get("Timecode", 0.0) >= 0.5;
    tcOn_.store(tcOn);
    if (tcOn && numIn >= 2 && in != nullptr && in[0] != nullptr && in[1] != nullptr) {
        if (std::abs(hostSr - vinylRate_) > 1.0) {
            vinylRate_ = hostSr;
            vinyl_.prepare(vinylRate_, {});
        }
        chooseTimecodeRecord();
        const bool flip = params.get("TimecodeFlip", 0.0) >= 0.5;
        tcL_.resize((size_t) numSamples);
        tcR_.resize((size_t) numSamples);
        double power = 0.0;
        for (int n = 0; n < numSamples; ++n) {
            const float l = in[flip ? 1 : 0][n], r = in[flip ? 0 : 1][n];
            tcL_[(size_t) n] = l;
            tcR_[(size_t) n] = r;
            power += (double) l * l + (double) r * r;
        }
        captureField(numSamples);
        if (const double rest = params.get("TimecodeRest", 0.0); rest != tcRestApplied_) {
            tcRestApplied_ = rest;
            vinyl_.pinRest(rest);
        }
        vinyl_.push(tcL_.data(), tcR_.data(), numSamples);
        tcTone_.store((float) vinyl_.toneHz());
        const auto reading = vinyl_.read();
        const double nudge = followNeedle(reading, fileSr, len, hostSr, (double) numSamples / hostSr);
        drive.active = true;
        drive.target = reading.toneArriving ? reading.speed * baseRate + nudge : 0.0;
        tcLevel_.store((float) std::sqrt(power / std::max(1, 2 * numSamples)));
        tcPresent_.store(reading.toneArriving);
        tcSpeed_.store((float) reading.speed);
        tcCarrier_.store((float) reading.restHz);
        tcLocated_.store(reading.located && needleClock_ - needleSince_ >= kTrustAfterSeconds);
        const auto& all = ControlVinyl::formats();
        const auto& settled = vinyl_.format();
        int index = -1;
        for (size_t i = 0; i < all.size(); ++i)
            if (all[i] == settled) index = (int) i;
        tcFormat_.store(index);
    }
    if (!drive.active) {
        tcTone_.store(0.0f);
        tcPresent_.store(false);
        tcLevel_.store(0.0f);
        tcSpeed_.store(0.0f);
        tcLocated_.store(false);
    }
    return drive;
}

}
