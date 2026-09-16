// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include "core/packs/Categories.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "hum/CameraCapture.h"
#include "hum/SerialPort.h"

#include "core/graph/GraphIo.h"
#include "core/net/LinkChase.h"
#include "core/packs/Roles.h"
#include "gui/app/AppSettings.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"

#include "hum/dsp/DspMath.h"

namespace hum {

void EngineHost::audioDeviceIOCallbackWithContext(const float* const* in, int numIn,
                                                  float* const* out, int numOut, int numSamples,
                                                  const juce::AudioIODeviceCallbackContext&) {
    const juce::ScopedNoDenormals noDenormals;
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    for (int c = 0; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    if (numSamples > block_) {
        refusedBlocks_.fetch_add(1, std::memory_order_relaxed);
        if (numSamples > refusedLargest_.load(std::memory_order_relaxed))
            refusedLargest_.store(numSamples, std::memory_order_relaxed);
        return;
    }
    {
        const juce::ScopedTryLock sl(lock_);
        if (sl.isLocked() && graph_ && numSamples > graph_->preparedBlock()) {
            refusedBlocks_.fetch_add(1, std::memory_order_relaxed);
            if (numSamples > refusedLargest_.load(std::memory_order_relaxed))
                refusedLargest_.store(numSamples, std::memory_order_relaxed);
        }
        if (sl.isLocked() && graph_ && numSamples <= graph_->preparedBlock()) {
            takeTransportRequests();
            if (const double punch = punchInBeat_.load(std::memory_order_relaxed);
                punch >= 0.0 && graph_->transport().playing() && graph_->transport().beats() >= punch) {
                punchInBeat_.store(-1.0, std::memory_order_relaxed);
                punchFire_.store(true, std::memory_order_release);
            }
            if (link_ != nullptr && linkEnabled_.load(std::memory_order_relaxed)) {
                auto& t = graph_->transport();
                const auto pulse = link_->capture(numSamples, sampleRate_,
                                                  outputLatency_, t.beatsPerBar());
                if (pulse.bpm > 0.0) {
                    const bool armed = t.playing()
                        && linkAlign_.exchange(false, std::memory_order_relaxed);
                    const auto v = linkchase::decide(t.beats(), pulse.bpm, pulse.phase,
                                                     t.beatsPerBar(), t.playing(), armed);
                    t.setTempo(v.tempo);
                    if (v.jump) t.setBeatPosition(v.jumpToBeats);
                }
            }
            const bool dcOk = !dcBuf_.empty()
                              && numSamples <= (int) dcBuf_[0].size();
            if (dcOk)
                for (int c = 0; c < numIn && c < (int) dcBuf_.size()
                                && c < kMaxDeviceChannels; ++c)
                    dcBlock_[c].process(in[c], dcBuf_[(size_t) c].data(), numSamples);
            for (int p = 0; p < physInCount_; ++p) {
                const int idx = inMap_[p];
                physIn_[p] = idx >= 0 && idx < numIn
                                 ? (dcOk && idx < (int) dcBuf_.size()
                                        ? dcBuf_[(size_t) idx].data() : in[idx])
                                 : nullptr;
            }
            graph_->transport().setLiveInput(physIn_, physInCount_, numSamples);
            graph_->processBlock(numSamples);
            graph_->transport().setLiveInput(nullptr, 0, 0);
            publishClock();
            for (auto& [tap, node] : masterTaps_) {
                if (graph_->nodeBypass(node)) continue;
                const int first = tap->firstChannel();
                const bool fold = monoFold_ && first == 0 && tap->channels() > 1;
                const float g = fold ? 0.5f : 1.0f;
                for (int c = 0; c < tap->channels(); ++c) {
                    const int phys = first + c;
                    const int dst = fold ? (masterFallback_ ? 0 : outMap_[0])
                                  : masterFallback_ && first == 0 ? c
                                  : phys >= 0 && phys < kMaxDeviceChannels ? outMap_[phys] : -1;
                    if (dst < 0 || dst >= numOut) continue;
                    const float* src = tap->channelData(c);
                    for (int i = 0; i < numSamples; ++i) out[dst][i] += g * src[i];
                }
            }
            for (auto& [a, node] : auxOuts_) {
                if (graph_->nodeBypass(node)) continue;
                const int ch = a->channel();
                const int dst = ch >= 0 && ch < kMaxDeviceChannels ? outMap_[ch] : -1;
                if (dst < 0 || dst >= numOut) continue;
                const float* src = a->channelData();
                const int n = std::min(a->blockLength(), numSamples);
                for (int i = 0; i < n; ++i) out[dst][i] += src[i];
            }

            if (std::int64_t left = countInLeft_.load(std::memory_order_acquire);
                left > 0 && numOut > 0) {
                auto& t = graph_->transport();
                const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
                const double spb = countInTick_ > 0.0 ? countInTick_ : std::max(1.0, t.samplesPerBeat());
                const double bar = countInBar_ > 0.0 ? countInBar_ : spb * 4.0;
                const float gain = metroGain_.load(std::memory_order_relaxed);
                const int dL = masterFallback_ ? 0 : outMap_[0];
                const int dR = masterFallback_ ? 1 : outMap_[1];
                const int cL = dL >= 0 && dL < numOut ? dL : 0;
                const int cR = dR >= 0 && dR < numOut ? dR : cL;
                for (int n = 0; n < numSamples && left > 0; ++n, --left) {
                    const double elapsed = (double) (countInTotal_ - left);
                    if (elapsed >= countInNextBeat_) {
                        const bool down = std::fmod(countInNextBeat_, bar) < spb * 0.5;
                        clickLen_ = (int) (sr * (down ? 0.060 : 0.040));
                        clickRemain_ = clickLen_;
                        clickPhase_ = 0.0;
                        clickStep_ = 2.0 * juce::MathConstants<double>::pi
                                     * (down ? 1568.0 : 1046.5) / sr;
                        clickAmp_ = down ? 0.5f : 0.32f;
                        countInNextBeat_ += spb;
                    }
                    if (clickRemain_ > 0) {
                        const float env = (float) clickRemain_ / (float) clickLen_;
                        const float s = gain * clickAmp_ * env * env
                                        * (float) std::sin(clickPhase_);
                        clickPhase_ += clickStep_;
                        --clickRemain_;
                        out[cL][n] += s;
                        if (cR != cL) out[cR][n] += s;
                    }
                }
                if (left == 0) {
                    t.setPlaying(true);
                    countInFire_.store(true, std::memory_order_release);
                    clickRemain_ = 0;
                }
                countInLeft_.store(left, std::memory_order_release);
            }

            if (metronome_.load(std::memory_order_relaxed) && graph_->transport().playing()
                && numOut > 0) {
                auto& t = graph_->transport();
                const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
                const double dBeat = t.tempo() / kSecondsPerMinute / sr;
                double beat = t.beats() - dBeat * numSamples;
                const double tick = t.meterAt(std::max(0.0, beat)).quarterNotesPerBeat();
                if (metroNextBeat_ < beat || metroNextBeat_ > beat + 1.5 * tick) {
                    const double bs = t.barStartBefore(std::max(0.0, beat));
                    metroNextBeat_ = bs + std::ceil((beat - bs) / tick - 1.0e-9) * tick;
                }
                const int dL = masterFallback_ ? 0 : outMap_[0];
                const int dR = masterFallback_ ? 1 : outMap_[1];
                const int cL = dL >= 0 && dL < numOut ? dL : 0;
                const int cR = dR >= 0 && dR < numOut ? dR : cL;
                for (int n = 0; n < numSamples; ++n) {
                    if (beat >= metroNextBeat_) {
                        const double bs = t.barStartBefore(metroNextBeat_ + 1.0e-6);
                        const double nb = t.nextBarStart(metroNextBeat_ + 1.0e-6);
                        const bool downbeat = metroNextBeat_ - bs < 0.25 * tick
                                              || nb - metroNextBeat_ < 0.25 * tick;
                        clickLen_ = (int) (sr * (downbeat ? 0.060 : 0.040));
                        clickRemain_ = clickLen_;
                        clickPhase_ = 0.0;
                        clickStep_ = 2.0 * juce::MathConstants<double>::pi
                                     * (downbeat ? 1568.0 : 1046.5) / sr;
                        clickAmp_ = downbeat ? 0.5f : 0.32f;
                        const double nextTick = metroNextBeat_ + tick;
                        metroNextBeat_ = nextTick > nb - 1.0e-6 ? nb : nextTick;
                    }
                    beat += dBeat;
                    if (clickRemain_ > 0) {
                        const float env = (float) clickRemain_ / (float) clickLen_;
                        const float s = metroGain_.load(std::memory_order_relaxed)
                                        * clickAmp_ * env * env
                                        * (float) std::sin(clickPhase_);
                        clickPhase_ += clickStep_;
                        --clickRemain_;
                        out[cL][n] += s;
                        if (cR != cL) out[cR][n] += s;
                    }
                }
            } else if (countInLeft_.load(std::memory_order_relaxed) <= 0) {
                clickRemain_ = 0;
            }
        } else if (!sl.isLocked() && fadeGain_ > 0.0005f) {
            dropouts_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    if (fadeRestart_.exchange(false)) fadeGain_ = 0.0f;
    const float target = fadeTarget_.load();
    const float inc = (float) (1.0 / (0.012 * (sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate)));
    const float master = outputGain_.load();
    const bool lim = limiterOn_.load(std::memory_order_relaxed);
    if (lim)
        limiter_.set(0.966, 80.0, 10.0,
                     sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate);
    float limMinGain = 1.0f;
    for (int n = 0; n < numSamples; ++n) {
        if (fadeGain_ < target)      fadeGain_ = std::min(target, fadeGain_ + inc);
        else if (fadeGain_ > target) fadeGain_ = std::max(target, fadeGain_ - inc);
        const float g = fadeGain_ * master;
        if (!lim) {
            for (int c = 0; c < numOut; ++c) out[c][n] *= g;
            continue;
        }
        float peak = 0.0f;
        for (int c = 0; c < numOut; ++c) {
            out[c][n] *= g;
            peak = std::max(peak, std::abs(out[c][n]));
        }
        const float lg = (float) limiter_.process(peak);
        limMinGain = std::min(limMinGain, lg);
        for (int c = 0; c < numOut; ++c)
            out[c][n] = juce::jlimit(-1.0f, 1.0f, out[c][n] * lg);
    }
    limiterGr_.store(1.0f - limMinGain, std::memory_order_relaxed);
    fadeGainPub_.store(fadeGain_);

    const int mL = masterFallback_ ? 0 : outMap_[0];
    const int mR = masterFallback_ ? 1 : outMap_[1];
    const int rL = mL >= 0 && mL < numOut ? mL : 0;
    const int rR = mR >= 0 && mR < numOut ? mR : rL;

    if ((mixRecording_.load(std::memory_order_relaxed) || playing_) && numOut > 0) {
        const juce::ScopedTryLock sl(lock_);
        if (sl.isLocked()) {
            if (mixRecording_.load(std::memory_order_relaxed) && mixWriter_.active()) {
                const float* chans[2] = { out[rL], out[rR] };
                mixWriter_.write(chans, numSamples);
            }
            if (playing_ && !perfAudioL_.empty()) {
                const int rs = (int) perfAudioL_.size();
                std::int64_t w = perfAudioWrite_.load(std::memory_order_relaxed);
                for (int n = 0; n < numSamples; ++n) {
                    const int idx = (int) ((w + n) % rs);
                    perfAudioL_[(size_t) idx] = out[rL][n];
                    perfAudioR_[(size_t) idx] = out[rR][n];
                }
                perfAudioWrite_.store(w + numSamples, std::memory_order_relaxed);
            }
        }
    }

    for (int c = 0; c < 2 && c < numOut; ++c) {
        const float* src = out[c == 0 ? rL : rR];
        float peak = 0.0f;
        for (int n = 0; n < numSamples; ++n) peak = std::max(peak, std::abs(src[n]));
        level_[c].store(peak);
    }

    const double budgetMs =
        1000.0 * numSamples / (sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate);
    audioLoad_.store((float) ((juce::Time::getMillisecondCounterHiRes() - t0) / budgetMs),
                     std::memory_order_relaxed);
    lastCallbackMs_.store(juce::Time::getMillisecondCounterHiRes(),
                          std::memory_order_relaxed);
}

}
