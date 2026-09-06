#include "Repeater/Repeater.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void Repeater::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    ring_ = (long long) (32.0 * sampleRate);
    for (auto& b : buf_) b.assign((size_t) ring_, 0.0f);
    pos_ = 0;
    regionEnd_ = 0;
    xfN_ = std::max(32, (int) (0.005 * sampleRate));
    reset();
}

void Repeater::process(const float* const* in, int numIn, float* const* out, int numOut,
                       int numSamples, const Transport& transport) {
    auto tap = [&](int c, int n) -> float {
        return (c < numIn && in[c]) ? in[c][n] : 0.0f;
    };

    const bool engage = params.get("Repeat", 0.0) >= 0.5;
    const bool hold = params.get("Hold", 0.0) >= 0.5;
    const double mix = std::clamp(params.get("Mix", 1.0), 0.0, 1.0);
    static const double kBars[] = {1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 2.0, 4.0};
    const int li = std::clamp((int) params.get("Length", 4.0), 0, 6);
    const double loopBeats = kBars[li] * transport.beatsPerBar();
    const double spb = kSecondsPerMinute / std::max(1.0, transport.tempo()) * sampleRate_;
    const long long loopLen =
        std::clamp((long long) (loopBeats * spb), (long long) (2 * xfN_), ring_ - 2 * xfN_);

    if (engage && !engaged_) {
        engaged_ = true;
        double phase = 0.0;
        if (transport.playing()) phase = std::fmod(transport.beats(), loopBeats);
        play_ = std::llround(phase * spb);
        regionEnd_ = pos_ - play_;
    } else if (!engage && engaged_) {
        engaged_ = false;
    }

    if (engaged_ && hold) {
        const long long margin = ring_ - numSamples - 4;
        if (2 * loopLen + xfN_ < margin) {
            const long long start = regionEnd_ - loopLen - xfN_;
            if (pos_ - start >= margin)
                regionEnd_ += ((pos_ - start - margin) / loopLen + 1) * loopLen;
        }
    }
    if (engaged_ && !hold && lastLen_ > 0 && loopLen != lastLen_) {
        fadeFrom_ = lastLen_;
        fadeLeft_ = xfN_;
    }
    lastLen_ = engaged_ ? loopLen : 0;

    const double wTarget = engaged_ ? 1.0 : 0.0;
    const double wc = smoothCoeff(2.0, sampleRate_);

    auto rd = [&](int c, long long idx) -> float {
        long long r = idx % ring_;
        if (r < 0) r += ring_;
        return buf_[(size_t) c][(size_t) r];
    };

    for (int n = 0; n < numSamples; ++n) {
        {
            const long long w = pos_ % ring_;
            buf_[0][(size_t) w] = tap(0, n);
            buf_[1][(size_t) w] = tap(1, n);
            ++pos_;
        }
        w_ = wc * w_ + (1.0 - wc) * wTarget;
        const double g = mix * w_;
        const long long p = play_ % loopLen;
        for (int c = 0; c < numOut && c < 2; ++c) {
            double loop = 0.0;
            if (g > 1e-4) {
                if (hold) {
                    const long long idx = regionEnd_ - loopLen + p;
                    loop = rd(c, idx);
                    if (p >= loopLen - xfN_) {
                        const double t =
                            (double) (p - (loopLen - xfN_)) / (double) xfN_;
                        loop = loop * (1.0 - t) + rd(c, idx - loopLen) * t;
                    }
                } else {
                    loop = rd(c, pos_ - 1 - loopLen);
                    if (fadeLeft_ > 0) {
                        const double t = (double) fadeLeft_ / (double) xfN_;
                        loop = loop * (1.0 - t) + rd(c, pos_ - 1 - fadeFrom_) * t;
                    }
                }
            }
            out[c][n] = (float) (tap(c, n) * (1.0 - g) + loop * g);
        }
        if (fadeLeft_ > 0) --fadeLeft_;
        if (engaged_ || g > 1e-4) ++play_;
    }
}

}
