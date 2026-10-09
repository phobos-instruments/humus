// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/RecordDecoder.h"

#include <cstddef>

#include "common/TimecodeTables.h"
#include "xwax/timecoder.h"

namespace hum {

namespace {
constexpr double kPositionsPerSecond = 1000.0;
}

struct RecordDecoder::Impl {
    timecoder tc{};
    bool live = false;
};

RecordDecoder::RecordDecoder() : impl_(std::make_unique<Impl>()) {}

RecordDecoder::~RecordDecoder() { release(); }

void RecordDecoder::release() {
    if (!impl_->live) return;
    timecoder_clear(&impl_->tc);
    impl_->live = false;
}

void RecordDecoder::choose(const std::string& format, double rpmFactor, double sampleRate) {
    if (format == format_ && rpmFactor == rpm_ && sampleRate == sampleRate_) return;
    release();
    format_ = format;
    rpm_ = rpmFactor;
    sampleRate_ = sampleRate;
    if (!format_.empty()) TimecodeTables::instance().ready(format_);
}

bool RecordDecoder::wake() {
    if (impl_->live || format_.empty() || sampleRate_ <= 0.0) return impl_->live;
    auto* def = TimecodeTables::instance().ready(format_);
    if (def == nullptr) return false;
    timecoder_init(&impl_->tc, def, rpm_, (unsigned int) sampleRate_, false, false);
    impl_->live = true;
    return true;
}

void RecordDecoder::submit(signed short* interleaved, int frames) {
    if (frames > 0 && wake()) timecoder_submit(&impl_->tc, interleaved, (std::size_t) frames);
}

RecordDecoder::Reading RecordDecoder::read() const {
    Reading r;
    if (!impl_->live) return r;
    auto& tc = impl_->tc;
    r.live = true;
    r.speed = timecoder_get_pitch(&tc);
    r.nominalHz = timecoder_get_resolution(&tc);
    r.carrierHz = r.speed * r.nominalHz;
    double when = 0.0;
    const signed int position = timecoder_get_position(&tc, &when);
    if (position >= 0) {
        r.located = true;
        r.staleSeconds = when;
        r.seconds = (double) position / kPositionsPerSecond + when * r.speed;
    }
    return r;
}

}
