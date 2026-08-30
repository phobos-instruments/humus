#include "AudioTrack/AudioTrack.h"

#include "hum/ClipStack.h"

#include <cmath>

#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

namespace {
juce::AudioBuffer<float> resampleLinear(const juce::AudioBuffer<float>& src,
                                        double fromRate, double toRate) {
    const double ratio = fromRate / toRate;
    const int outLen = (int) std::floor((double) src.getNumSamples() / ratio);
    juce::AudioBuffer<float> out(src.getNumChannels(), std::max(1, outLen));
    for (int c = 0; c < src.getNumChannels(); ++c) {
        const float* s = src.getReadPointer(c);
        float* d = out.getWritePointer(c);
        for (int n = 0; n < out.getNumSamples(); ++n) {
            const double pos = n * ratio;
            const int i = (int) pos;
            const double f = pos - i;
            const float a = s[std::min(i, src.getNumSamples() - 1)];
            const float b = s[std::min(i + 1, src.getNumSamples() - 1)];
            d[n] = (float) (a + (b - a) * f);
        }
    }
    return out;
}
}

const AudioTrack::CachedFile* AudioTrack::fileFor(const std::string& path) {
    auto it = cache_.find(path);
    if (it != cache_.end()) return &it->second;

    juce::AudioBuffer<float> buf;
    double fileSr = sampleRate_;
    if (!loadSoundFile(path, buf, fileSr)) return nullptr;
    if (std::abs(fileSr - sampleRate_) > 0.5)
        buf = resampleLinear(buf, fileSr, sampleRate_);

    CachedFile cf;
    cf.pcm = std::make_shared<const juce::AudioBuffer<float>>(std::move(buf));
    cf.fileSampleRate = fileSr;
    return &cache_.emplace(path, std::move(cf)).first->second;
}

void AudioTrack::ensureClipsLoaded() {
    if (!clipsDirty_) return;
    clipsDirty_ = false;

    std::vector<ClipPlay> next;
    for (int i = 0; i < (int) pattern_.channels.size(); ++i) {
        const auto& ch = pattern_.channels[(size_t) i];
        if (ch.type != "audio-clip" || ch.startTick < 0 || ch.audioFile.empty()) continue;
        const auto* cf = fileFor(ch.audioFile);
        if (!cf || !cf->pcm) continue;
        for (const auto& span : clipstack::soundingSpans(pattern_, i)) {
            ClipPlay cp;
            cp.startBeat = span.startTick / (double) Pattern::kTicksPerBeat;
            cp.lenBeats = std::max(1, span.lengthTicks) / (double) Pattern::kTicksPerBeat;
            cp.clipStartBeat = ch.startTick / (double) Pattern::kTicksPerBeat;
            cp.periodBeats = std::max(1, ch.lengthTicks) / (double) Pattern::kTicksPerBeat;
            cp.loop = ch.loopClip;
            cp.offset = ch.audioOffset;
            cp.gain = ch.audioGain;
            cp.sourceBpm = ch.sourceBpm;
            cp.warpMode = ch.warpMode;
            cp.fadeInBeats = ch.fadeInTicks / (double) Pattern::kTicksPerBeat;
            cp.fadeOutBeats = ch.fadeOutTicks / (double) Pattern::kTicksPerBeat;
            cp.fadeInCurve = (float) ch.fadeInCurve;
            cp.fadeOutCurve = (float) ch.fadeOutCurve;
            cp.reverse = ch.audioReverse;
            cp.pitchRatio = std::pow(2.0, ch.audioPitch / 12.0);
            for (auto& s : cp.shift) s.prepare(sampleRate_);
            cp.shiftLatency = (int) (sampleRate_ * 0.03);
            cp.pcm = cf->pcm;
            next.push_back(std::move(cp));
        }
    }
    {
        const juce::ScopedLock sl(loadLock_);
        pendingClips_ = std::move(next);
    }
    hasPending_.store(true, std::memory_order_release);
}

}
