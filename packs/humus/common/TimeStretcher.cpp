// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/TimeStretcher.h"

#include <algorithm>
#include <cmath>
#include <vector>

#if defined(HUM_STRETCH_SIGNALSMITH)
#include <signalsmith-stretch.h>
#elif defined(HUM_STRETCH_SOUNDTOUCH)
#include <SoundTouch.h>
#endif

namespace hum {

struct TimeStretcher::Impl {
    Impl() = default;
#if defined(HUM_STRETCH_SIGNALSMITH)
    explicit Impl(long seed) : st(seed) {}
#else
    explicit Impl(long) {}
#endif
    int channels = 2;
    double transpose = 1.0;
#if defined(HUM_STRETCH_SIGNALSMITH)
    signalsmith::stretch::SignalsmithStretch<float> st;
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    soundtouch::SoundTouch st;
    std::vector<float> inI, outI;
#endif
};

TimeStretcher::TimeStretcher() : impl_(std::make_unique<Impl>()) {}
TimeStretcher::TimeStretcher(long seed) : impl_(std::make_unique<Impl>(seed)) {}
TimeStretcher::~TimeStretcher() = default;

bool TimeStretcher::available() const {
#if defined(HUM_HAS_KEYLOCK)
    return true;
#else
    return false;
#endif
}

void TimeStretcher::prepare(double sampleRate, int channels, int maxBlock) {
    impl_->channels = channels;
#if defined(HUM_STRETCH_SIGNALSMITH)
    impl_->st.presetDefault(channels, (float) sampleRate);
    impl_->st.setTransposeFactor(1.0f);
    const int n = std::max(256, maxBlock);
    std::vector<std::vector<float>> z((size_t) channels, std::vector<float>((size_t) n, 0.0f));
    std::vector<std::vector<float>> o((size_t) channels, std::vector<float>((size_t) n, 0.0f));
    std::vector<const float*> in((size_t) channels);
    std::vector<float*> out((size_t) channels);
    for (int c = 0; c < channels; ++c) {
        in[(size_t) c] = z[(size_t) c].data();
        out[(size_t) c] = o[(size_t) c].data();
    }
    impl_->st.process(in.data(), n, out.data(), n);
    impl_->st.reset();
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    impl_->st.setChannels((unsigned) channels);
    impl_->st.setSampleRate((unsigned) std::lround(sampleRate));
    impl_->st.setPitch(1.0);
    impl_->inI.reserve((size_t) (maxBlock * 4 + 16) * channels);
    impl_->outI.reserve((size_t) (maxBlock + 16) * channels);
#else
    (void) sampleRate; (void) maxBlock;
#endif
}

void TimeStretcher::reset() {
#if defined(HUM_STRETCH_SIGNALSMITH)
    impl_->st.reset();
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    impl_->st.clear();
#endif
}

int TimeStretcher::inputLatency() const {
#if defined(HUM_STRETCH_SIGNALSMITH)
    return impl_->st.inputLatency();
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    return impl_->st.getSetting(SETTING_INITIAL_LATENCY);
#else
    return 0;
#endif
}

int TimeStretcher::outputLatency() const {
#if defined(HUM_STRETCH_SIGNALSMITH)
    return impl_->st.outputLatency();
#else
    return 0;
#endif
}

int TimeStretcher::prerollSamples() const {
#if defined(HUM_STRETCH_SIGNALSMITH)
    return impl_->st.blockSamples() + impl_->st.intervalSamples();
#else
    return 0;
#endif
}

void TimeStretcher::setTranspose(double factor) {
    impl_->transpose = std::clamp(factor, 0.125, 8.0);
}

void TimeStretcher::preroll(const float* const* in, int inN, double tempoFactor) {
#if defined(HUM_STRETCH_SIGNALSMITH)
    impl_->st.seek(in, inN, tempoFactor);
#else
    (void) in; (void) inN; (void) tempoFactor;
#endif
}

void TimeStretcher::process(const float* const* in, int inN,
                            float* const* out, int outN, double tempoFactor) {
#if defined(HUM_STRETCH_SIGNALSMITH)
    (void) tempoFactor;
    impl_->st.setTransposeFactor((float) impl_->transpose);
    impl_->st.process(in, inN, out, outN);
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    const int ch = impl_->channels;
    auto& inI = impl_->inI; auto& outI = impl_->outI;
    inI.resize((size_t) inN * ch);
    for (int i = 0; i < inN; ++i)
        for (int c = 0; c < ch; ++c) inI[(size_t) (i * ch + c)] = in[c][i];
    impl_->st.setTempo(std::max(0.05, tempoFactor));
    impl_->st.setPitch(impl_->transpose);
    impl_->st.putSamples(inI.data(), (unsigned) inN);
    outI.resize((size_t) outN * ch);
    const unsigned recv = impl_->st.receiveSamples(outI.data(), (unsigned) outN);
    for (int i = 0; i < outN; ++i)
        for (int c = 0; c < ch; ++c)
            out[c][i] = i < (int) recv ? outI[(size_t) (i * ch + c)] : 0.0f;
#else
    (void) in; (void) inN; (void) tempoFactor;
    for (int c = 0; c < impl_->channels; ++c)
        if (out[c]) for (int i = 0; i < outN; ++i) out[c][i] = 0.0f;
#endif
}

}
