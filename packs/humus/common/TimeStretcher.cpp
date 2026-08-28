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
    int channels = 2;
#if defined(HUM_STRETCH_SIGNALSMITH)
    signalsmith::stretch::SignalsmithStretch<float> st;
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    soundtouch::SoundTouch st;
    std::vector<float> inI, outI;
#endif
};

TimeStretcher::TimeStretcher() : impl_(std::make_unique<Impl>()) {}
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
    std::vector<float> z((size_t) n, 0.0f), oL((size_t) n), oR((size_t) n);
    const float* in[2] = { z.data(), z.data() };
    float* out[2] = { oL.data(), oR.data() };
    impl_->st.process(in, n, out, n);
    impl_->st.reset();
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    impl_->st.setChannels((unsigned) channels);
    impl_->st.setSampleRate((unsigned) std::lround(sampleRate));
    impl_->st.setPitch(1.0);
    impl_->inI.reserve((size_t) (maxBlock * 4 + 16) * channels);
    impl_->outI.reserve((size_t) (maxBlock + 16) * channels);
#else
    (void) sampleRate; (void) channels; (void) maxBlock;
#endif
}

void TimeStretcher::reset() {
#if defined(HUM_STRETCH_SIGNALSMITH)
    impl_->st.reset();
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    impl_->st.clear();
#endif
}

void TimeStretcher::process(const float* const* in, int inN,
                            float* const* out, int outN, double tempoFactor) {
#if defined(HUM_STRETCH_SIGNALSMITH)
    (void) tempoFactor;
    impl_->st.setTransposeFactor(1.0f);
    impl_->st.process(in, inN, out, outN);
#elif defined(HUM_STRETCH_SOUNDTOUCH)
    const int ch = impl_->channels;
    auto& inI = impl_->inI; auto& outI = impl_->outI;
    inI.resize((size_t) inN * ch);
    for (int i = 0; i < inN; ++i)
        for (int c = 0; c < ch; ++c) inI[(size_t) (i * ch + c)] = in[c][i];
    impl_->st.setTempo(std::max(0.05, tempoFactor));
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
