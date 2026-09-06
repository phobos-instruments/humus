#include "Halogen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum {
namespace {

constexpr float kSizeF = (float) Halogen::kSize;

constexpr float kPartial = 0.05f;
constexpr float kOlaScale = kPartial * kSizeF / 4.0f;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

}

void Halogen::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
    fft_ = std::make_unique<juce::dsp::FFT>(kOrder);
    window_.resize((size_t) kSize);
    for (int i = 0; i < kSize; ++i)
        window_[(size_t) i] = 0.5f - 0.5f * std::cos((float) (kTwoPi * i / kSize));
    fftBuf_.assign((size_t) (2 * kSize), 0.0f);
    mag_.assign((size_t) (kSize / 2), 0.0f);
    std::uint32_t rng = 0xA10CE4u;
    for (int c = 0; c < 2; ++c) {
        accum_[c].assign((size_t) kSize, 0.0f);
        phase_[c].assign((size_t) (kSize / 2), 0.0f);
        for (auto& p : phase_[c]) {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            p = (float) (kTwoPi * (double) (rng & 0xFFFF) / 65536.0);
        }
    }
    reset();
}

void Halogen::reset() {
    for (auto& a : accum_) std::fill(a.begin(), a.end(), 0.0f);
    std::fill(mag_.begin(), mag_.end(), 0.0f);
    outRead_ = kHop;
}

void Halogen::loadFromFile(const std::string& uri) {
    PixelField next;
    if (!uri.empty()) loadPixelField(uri, next);
    PixelField retired;
    {
        const juce::SpinLock::ScopedLockType sl(swap_);
        retired = std::move(pending_);
        pending_ = std::move(next);
        ready_.store(true, std::memory_order_release);
    }
    loaded_ = uri;
}

void Halogen::adoptPending() {
    if (!ready_.load(std::memory_order_acquire)) return;
    const juce::SpinLock::ScopedTryLockType sl(swap_);
    if (!sl.isLocked()) return;
    std::swap(field_.v, pending_.v);
    std::swap(field_.width, pending_.width);
    std::swap(field_.height, pending_.height);
    ready_.store(false, std::memory_order_relaxed);
    std::fill(mag_.begin(), mag_.end(), 0.0f);
}

void Halogen::frame(double column) {
    adoptPending();
    const int bins = kSize / 2;
    const float gate = (float) std::clamp(params.get("Gate", 0.1), 0.0, 1.0);
    const float blur = (float) std::clamp(params.get("Blur", 0.2), 0.0, 1.0);
    const float tilt = (float) std::clamp(params.get("Tilt", 0.0), -1.0, 1.0);
    const double lowHz = std::clamp(params.get("Low", 55.0), 20.0, 2000.0);
    const double highHz = std::max(lowHz * 1.25,
                                   std::clamp(params.get("High", 8000.0), 200.0, 16000.0));
    const double y = std::clamp(params.get("Y", 0.0), 0.0, 1.0);
    const double height = std::clamp(params.get("Height", 1.0), 0.02, 1.0);
    const float smooth = blur * 0.985f;

    if (field_.empty()) {
        for (auto& m : mag_) m *= smooth;
    } else {
        const double colF = std::clamp(column, 0.0, 1.0) * (double) (field_.width - 1);
        const int c0 = std::clamp((int) colF, 0, field_.width - 1);
        const int c1 = std::min(c0 + 1, field_.width - 1);
        const float cx = (float) (colF - c0);
        const double rowTop = (1.0 - y - height) * (double) (field_.height - 1);
        const double rowSpan = height * (double) (field_.height - 1);
        const double logLow = std::log(lowHz), logSpan = std::log(highHz) - logLow;
        const double binHz = sampleRate_ / kSize;
        for (int k = 1; k < bins; ++k) {
            const double hz = k * binHz;
            float target = 0.0f;
            if (hz >= lowHz && hz <= highHz) {
                const double up = (std::log(hz) - logLow) / logSpan;
                const int row = std::clamp((int) std::lround(rowTop + (1.0 - up) * rowSpan),
                                           0, field_.height - 1);
                float px = lerp(field_.at(c0, row), field_.at(c1, row), cx);
                px = px <= gate ? 0.0f : (px - gate) / std::max(1e-4f, 1.0f - gate);
                target = px * px;
                if (tilt != 0.0f) target *= std::pow((float) (hz / lowHz), tilt * 0.5f);
            }
            mag_[(size_t) k] = lerp(target, mag_[(size_t) k], smooth);
        }
    }

    float sum = 0.0f;
    for (int k = 1; k < bins; ++k) sum += mag_[(size_t) k];
    levelOut_.store(std::min(1.0f, sum * 0.5f), std::memory_order_relaxed);

    for (int c = 0; c < 2; ++c) {
        std::fill(fftBuf_.begin(), fftBuf_.end(), 0.0f);
        for (int k = 1; k < bins; ++k) {
            const float m = mag_[(size_t) k];
            if (m <= 1e-6f) continue;
            float& ph = phase_[c][(size_t) k];
            ph = (float) std::fmod(ph + kTwoPi * 0.25 * k, kTwoPi);
            const float re = m * std::cos(ph), im = m * std::sin(ph);
            fftBuf_[(size_t) (2 * k)] = re;
            fftBuf_[(size_t) (2 * k + 1)] = im;
            fftBuf_[(size_t) (2 * (kSize - k))] = re;
            fftBuf_[(size_t) (2 * (kSize - k) + 1)] = -im;
        }
        fft_->performRealOnlyInverseTransform(fftBuf_.data());
        auto& acc = accum_[c];
        for (int i = 0; i < kSize; ++i)
            acc[(size_t) i] += fftBuf_[(size_t) i] * window_[(size_t) i] * kOlaScale;
    }
}

void Halogen::process(const float* const*, int, float* const* out, int numOut,
                      int numSamples, const Transport&) {
    if (fft_ == nullptr) return;
    const int mode = (int) std::lround(params.get("Scan", 0.0));
    const double rate = std::clamp(params.get("Rate", 0.25), 0.01, 8.0);
    const double x = std::clamp(params.get("X", 0.0), 0.0, 1.0);
    const double span = std::clamp(params.get("Span", 1.0), 0.01, 1.0);
    const bool reverse = params.get("Reverse", 0.0) >= 0.5;
    const float level = (float) std::clamp(params.get("Level", 1.0), 0.0, 2.0);
    const double step = rate * kHop / sampleRate_;

    for (int n = 0; n < numSamples; ++n) {
        if (outRead_ >= kHop) {
            if (mode == 0) {
                scan_ += reverse ? -step : step;
                scan_ -= std::floor(scan_);
            }
            const double head = mode == 0 ? x + scan_ * span : x;
            const double pos = std::clamp(head - std::floor(head), 0.0, 1.0);
            scanOut_.store((float) pos, std::memory_order_relaxed);
            frame(pos);
            outRead_ = 0;
        }
        for (int c = 0; c < numOut; ++c)
            out[c][n] = level * accum_[(size_t) std::min(c, 1)][(size_t) outRead_];
        ++outRead_;
        if (outRead_ >= kHop)
            for (auto& acc : accum_) {
                std::copy(acc.begin() + kHop, acc.end(), acc.begin());
                std::fill(acc.end() - kHop, acc.end(), 0.0f);
            }
    }
}

}
