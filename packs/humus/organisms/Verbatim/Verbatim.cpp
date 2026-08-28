#include "Verbatim/Verbatim.h"

#include <algorithm>
#include <cmath>

#include <juce_audio_formats/juce_audio_formats.h>

#include "hum/dsp/DspMath.h"

namespace hum {

void Verbatim::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    maxBlock_ = std::max(16, maxBlock);
    conv_.prepare({sampleRate, (juce::uint32) maxBlock_, 2});
    scratch_.setSize(2, maxBlock_);
    const int preMax = (int) (kMaxPredelayMs * 0.001 * sampleRate) + maxBlock_ + 4;
    for (auto& d : pre_) d.prepare(preMax);
    reset();
    loadFromFile(lastPath_);
}

void Verbatim::reset() {
    conv_.reset();
    for (auto& d : pre_) d.clear();
    for (auto& v : lp_) v = 0.0;
    for (auto& v : hp_) v = 0.0;
    smSnap_ = true;
}

void Verbatim::loadFromFile(const std::string& uri) {
    const std::string path = !uri.empty() ? uri
                           : params.byName("File") != nullptr ? params.byName("File")->text
                                                              : std::string();
    lastPath_ = path;
    const juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    if (!f.existsAsFile()) { irSet_.store(false, std::memory_order_relaxed); return; }

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0) {
        irSet_.store(false, std::memory_order_relaxed);
        return;
    }
    const int chans = std::clamp((int) reader->numChannels, 1, 2);
    const int len = (int) std::min<juce::int64>(reader->lengthInSamples,
                                                (juce::int64) (kMaxIrSeconds * reader->sampleRate));
    juce::AudioBuffer<float> ir(chans, len);
    reader->read(&ir, 0, len, 0, true, chans > 1);
    if (params.get("Reverse", 0.0) >= 0.5) ir.reverse(0, len);
    conv_.loadImpulseResponse(std::move(ir), reader->sampleRate,
                              juce::dsp::Convolution::Stereo::yes,
                              juce::dsp::Convolution::Trim::yes,
                              juce::dsp::Convolution::Normalise::yes);
    irSet_.store(true, std::memory_order_relaxed);
}

void Verbatim::process(const float* const* in, int numIn, float* const* out, int numOut,
                     int numSamples, const Transport&) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    const double mix = std::clamp(params.get("Mix", 0.35), 0.0, 1.0);
    const float preSamps =
        (float) (std::clamp(params.get("Predelay", 0.0), 0.0, kMaxPredelayMs) * 0.001 * sr);
    const double lowCut = std::clamp(params.get("LowCut", 20.0), 20.0, 2000.0);
    const double damp = std::clamp(params.get("Damp", 20000.0), 500.0, 20000.0);
    const bool wet = irSet_.load(std::memory_order_relaxed);

    auto tap = [&](int c, int n) -> float {
        return (c < numIn && in[c]) ? in[c][n] : 0.0f;
    };
    for (int n = 0; n < numSamples; ++n) {
        scratch_.setSample(0, n, tap(0, n));
        scratch_.setSample(1, n, tap(1, n));
    }
    if (wet) {
        juce::dsp::AudioBlock<float> block(scratch_.getArrayOfWritePointers(), 2, 0,
                                           (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        conv_.process(ctx);
    } else {
        scratch_.clear(0, numSamples);
    }

    const double aLp = 1.0 - std::exp(-2.0 * M_PI * damp / sr);
    const double aHp = 1.0 - std::exp(-2.0 * M_PI * lowCut / sr);
    const double aMix = smoothCoeff(10.0, sr);
    const double aPre = smoothCoeff(50.0, sr);
    if (smSnap_) { mixSm_ = mix; preSm_ = preSamps; smSnap_ = false; }
    for (int n = 0; n < numSamples; ++n) {
        mixSm_ = aMix * mixSm_ + (1.0 - aMix) * mix;
        preSm_ = aPre * preSm_ + (1.0 - aPre) * preSamps;
        const float dryG = (float) std::cos(mixSm_ * juce::MathConstants<double>::halfPi);
        const float wetG = (float) std::sin(mixSm_ * juce::MathConstants<double>::halfPi);
        for (int c = 0; c < 2; ++c) {
            pre_[c].write(scratch_.getSample(c, n));
            double w = pre_[c].read((float) preSm_ + 1.0f);
            lp_[c] += aLp * (w - lp_[c]);
            w = lp_[c];
            hp_[c] += aHp * (w - hp_[c]);
            w -= hp_[c];
            if (c < numOut)
                out[c][n] = (float) (tap(c, n) * dryG + w * wetG);
        }
    }
}

}
