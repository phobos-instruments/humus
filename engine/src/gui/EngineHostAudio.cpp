#include "gui/EngineHost.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "hum/CameraCapture.h"
#include "hum/SerialPort.h"

#include "core/GraphIo.h"
#include "core/LinkChase.h"
#include "gui/AppSettings.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"
#include "hum/Capabilities.h"

namespace hum {

namespace {
constexpr const char* kAudioStateKey = "audio.state";
constexpr int kMaxChannels = 32;
}

void EngineHost::requestFadeIn() {
    fadeTarget_.store(1.0f);
    fadeRestart_.store(true);
}

void EngineHost::persistAudioState() {
    if (auto state = devices_.createStateXml())
        AppSettings::instance().set(kAudioStateKey, state->toString());
}

void EngineHost::seedDeviceFormatFromSettings() {
    if (ignoreSavedFormat_) return;
    if (auto saved = juce::parseXML(AppSettings::instance().getString(kAudioStateKey))) {
        const double r = saved->getDoubleAttribute("audioDeviceRate", 0.0);
        const int b = saved->getIntAttribute("audioDeviceBufferSize", 0);
        if (r > 0.0) sampleRate_ = r;
        if (b > 0) block_ = b;
    }
}

void EngineHost::startAudioAsync(std::function<void(bool, std::string)> done) {
    if (audioRunning_ || audioStarting_) { if (done) done(true, {}); return; }
    audioStarting_ = true;
    fadeGainPub_.store(0.0f);
    fadeTarget_.store(0.0f);

    auto saved = std::shared_ptr<juce::XmlElement>(
        juce::parseXML(AppSettings::instance().getString(kAudioStateKey)).release());
    if (saved) saved->removeAttribute("audioDeviceRate");

    juce::MessageManager::callAsync([this, alive = hostAlive_, saved, done = std::move(done)] {
        if (!*alive) return;
        juce::String e = devices_.initialise(2, 2, saved.get(), true);
        if (e.isNotEmpty()) e = devices_.initialise(0, 2, saved.get(), true);
        if (e.isNotEmpty()) e = devices_.initialise(0, 2, nullptr, true);
        audioStarting_ = false;
        if (cancelStart_) {
            cancelStart_ = false;
            devices_.closeAudioDevice();
            if (done) done(false, "cancelled");
            return;
        }
        if (e.isNotEmpty()) { if (done) done(false, e.toStdString()); return; }
        devices_.addAudioCallback(this);
        audioRunning_ = true;
        requestFadeIn();
        if (done) done(true, {});
    });
}

bool EngineHost::ensureAudio() {
    if (audioRunning_ || audioStarting_) return true;
    startAudioAsync();
    return true;
}

void EngineHost::stopAudio() {
    if (audioStarting_) cancelStart_ = true;
    if (!audioRunning_) return;
    persistAudioState();
    fadeTarget_.store(0.0f);
    for (int i = 0; i < 60 && fadeGainPub_.load() > 0.0005f; ++i)
        juce::Thread::sleep(1);
    devices_.removeAudioCallback(this);
    devices_.closeAudioDevice();
    audioRunning_ = false;
    fadeGainPub_.store(0.0f);
    level_[0].store(0.0f);
    level_[1].store(0.0f);
}

void EngineHost::scheduleAuxChannelCheck() {
    if (auxCheckPending_) return;
    auxCheckPending_ = true;
    juce::MessageManager::callAsync([this, alive = hostAlive_] {
        if (!*alive) return;
        auxCheckPending_ = false;
        ensureAuxChannels();
    });
}

void EngineHost::ensureAuxChannels() {
    if (!audioRunning_) return;
    auto* dev = devices_.getCurrentAudioDevice();
    if (dev == nullptr || graph_ == nullptr) return;

    juce::BigInteger needIn, needOut;
    for (int i = 0; i < graph_->nodeCount(); ++i) {
        auto* c = graph_->organism(i);
        if (auto* hin = dynamic_cast<HardwareIn*>(c)) {
            const int ch = hin->channel();
            const int lo = hin->channelCount() > 1 ? 2 : 0;
            for (int k = 0; k < hin->channelCount(); ++k)
                if (ch + k >= lo && ch + k < kMaxDeviceChannels) needIn.setBit(ch + k);
        }
        if (auto* hout = dynamic_cast<HardwareOut*>(c)) {
            const int ch = hout->channel();
            if (ch >= 0 && ch < kMaxDeviceChannels) needOut.setBit(ch);
        }
        if (auto* tap = dynamic_cast<MasterTap*>(c)) {
            const int first = tap->firstChannel();
            for (int k = 0; k < tap->channels(); ++k)
                if (first + k >= 2 && first + k < kMaxDeviceChannels) needOut.setBit(first + k);
        }
    }
    if (needIn.isZero() && needOut.isZero()) return;

    const juce::BigInteger curIn = dev->getActiveInputChannels();
    const juce::BigInteger curOut = dev->getActiveOutputChannels();
    juce::BigInteger wantIn = curIn, wantOut = curOut;
    const int devIns = dev->getInputChannelNames().size();
    const int devOuts = dev->getOutputChannelNames().size();
    for (int b = 0; b < devIns && b < kMaxDeviceChannels; ++b)
        if (needIn[b]) wantIn.setBit(b);
    for (int b = 0; b < devOuts && b < kMaxDeviceChannels; ++b)
        if (needOut[b]) wantOut.setBit(b);
    if (wantIn == curIn && wantOut == curOut) return;

    if (dev->getName() != auxTriedDevice_) {
        auxTriedDevice_ = dev->getName();
        auxTriedIn_.clear();
        auxTriedOut_.clear();
    }
    if (wantIn == auxTriedIn_ && wantOut == auxTriedOut_) return;
    auxTriedIn_ = wantIn;
    auxTriedOut_ = wantOut;

    auto setup = devices_.getAudioDeviceSetup();
    setup.inputChannels = wantIn;
    setup.outputChannels = wantOut;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = false;
    deviceRestarts_.fetch_add(1, std::memory_order_relaxed);
    devices_.setAudioDeviceSetup(setup, true);
    persistAudioState();
}

void EngineHost::play() {
    if (capturing_) beginCapturePass();
    if (linkEnabled_.load(std::memory_order_relaxed)) {
        linkAlign_.store(true, std::memory_order_relaxed);
        if (link_ && linkStartStop_.load(std::memory_order_relaxed)) {
            link_->proposePlaying(true);
            linkLastSessionPlaying_ = true;
        }
    }
    if (countInEnabled_.load(std::memory_order_relaxed) && !playing_ && audioRunning_) {
        const juce::ScopedLock sl(lock_);
        if (graph_) {
            countInTotal_ = (std::int64_t) (4.0 * graph_->transport().samplesPerBeat());
            countInNextBeat_ = 0.0;
            countInLeft_.store(countInTotal_, std::memory_order_release);
            return;
        }
    }
    playing_ = true;
    clearTouches();
    {
        const juce::ScopedLock sl(lock_);
        if (graph_) graph_->transport().setPlaying(true);
    }
    record_.onPlay();
    syncTransportEvent(true);
}

void EngineHost::pumpOscOut() {
    for (auto& cm : model_.organisms) {
        auto* src = dynamic_cast<OscValueSource*>(liveOrganism(cm.name));
        if (src == nullptr || !src->oscEnabled()) continue;
        OscValueSource::OscVal vals[64];
        const int n = src->oscValues(vals, 64);
        auto& last = oscOutLast_[cm.name];
        if ((int) last.size() != n) last.assign((size_t) n, -1.0e9f);
        juce::String seg;
        for (const char ch : cm.name)
            seg += (juce::CharacterFunctions::isLetterOrDigit((juce::juce_wchar) ch)
                    || ch == '_' || ch == '-') ? juce::String::charToString((juce::juce_wchar) ch)
                                               : juce::String("_");
        for (int i = 0; i < n; ++i) {
            if (std::abs(vals[i].value - last[(size_t) i]) < 1.0e-4f) continue;
            last[(size_t) i] = vals[i].value;
            osc().sendValue("/humus/" + seg + "/" + vals[i].suffix, vals[i].value);
        }
    }
}

void EngineHost::serviceCountIn() {
    if (!countInFire_.exchange(false)) return;
    playing_ = true;
    clearTouches();
    record_.onPlay();
    syncTransportEvent(true);
}

void EngineHost::stop() {
    countInLeft_.store(0);
    record_.onStop();
    finishOnDemandClips();
    playing_ = false;
    clearTouches();
    {
        const juce::ScopedLock sl(lock_);
        if (graph_) graph_->transport().setPlaying(false);
    }
    syncTransportEvent(false);
    if (link_ && linkEnabled_.load(std::memory_order_relaxed)
        && linkStartStop_.load(std::memory_order_relaxed)) {
        link_->proposePlaying(false);
        linkLastSessionPlaying_ = false;
    }
}

void EngineHost::goToStart() { setPositionBeats(0.0); }

void EngineHost::playFromStart() { goToStart(); play(); }

void EngineHost::setPositionBeats(double beat) {
    const double target = beat < 0.0 ? 0.0 : beat;
    seekBeatsReq_.store(target, std::memory_order_relaxed);
    if (!audioRunning_) {
        const juce::ScopedLock sl(lock_);
        const double b = seekBeatsReq_.exchange(-1.0, std::memory_order_relaxed);
        if (graph_ && b >= 0.0) graph_->transport().setBeatPosition(b);
        publishClock();
    }
    if (!playing_) applyStateAt(target);
}

void EngineHost::setTempo(double bpm) {
    if (bpm <= 0.0) return;
    model_.clock.tempo = bpm;
    if (link_ && linkEnabled_.load(std::memory_order_relaxed)) link_->proposeTempo(bpm);
    tempoReq_.store(bpm, std::memory_order_relaxed);
    if (!audioRunning_) {
        const juce::ScopedLock sl(lock_);
        const double t = tempoReq_.exchange(0.0, std::memory_order_relaxed);
        if (graph_ && t > 0.0) graph_->transport().setTempo(t);
        publishClock();
    }
}

void EngineHost::publishClock() {
    if (graph_ == nullptr) {
        liveBar_.store(1, std::memory_order_relaxed);
        liveBeatInBar_.store(1.0, std::memory_order_relaxed);
        liveBeats_.store(0.0, std::memory_order_relaxed);
        liveSeconds_.store(0.0, std::memory_order_relaxed);
        return;
    }
    auto& t = graph_->transport();
    liveBar_.store(t.bar(), std::memory_order_relaxed);
    liveBeatInBar_.store(t.beatInBar(), std::memory_order_relaxed);
    liveBeats_.store(t.beats(), std::memory_order_relaxed);
    liveSeconds_.store(sampleRate_ > 0.0
                           ? (double) t.samplePosition() / sampleRate_ : 0.0,
                       std::memory_order_relaxed);
}

int EngineHost::positionBar() { return liveBar_.load(std::memory_order_relaxed); }
double EngineHost::positionBeat() { return liveBeatInBar_.load(std::memory_order_relaxed); }
double EngineHost::positionBeats() { return liveBeats_.load(std::memory_order_relaxed); }
double EngineHost::positionSeconds() { return liveSeconds_.load(std::memory_order_relaxed); }

float EngineHost::outputLevel(int channel) const {
    return (channel >= 0 && channel < 2) ? level_[channel].load() : 0.0f;
}

void EngineHost::audioDeviceAboutToStart(juce::AudioIODevice* d) {
    const double rate = d->getCurrentSampleRate();
    const int ringLen = (int) (kRingSeconds * rate);
    const int nIn = d->getActiveInputChannels().countNumberOfSetBits();
    {
        const juce::ScopedLock sl(lock_);
        sampleRate_ = rate;
        block_ = d->getCurrentBufferSizeSamples();
        outputLatency_ = d->getOutputLatencyInSamples();
        if ((int) perfAudioL_.size() != ringLen) {
            perfAudioL_.assign((size_t) ringLen, 0.0f);
            perfAudioR_.assign((size_t) ringLen, 0.0f);
            perfAudioWrite_.store(0, std::memory_order_relaxed);
        }
        buildChannelMap(d->getActiveInputChannels(), inMap_, kMaxDeviceChannels);
        buildChannelMap(d->getActiveOutputChannels(), outMap_, kMaxDeviceChannels);
        physInCount_ = 0;
        for (int p = 0; p < kMaxDeviceChannels; ++p)
            if (inMap_[p] >= 0) physInCount_ = p + 1;
        dcBuf_.assign((size_t) nIn, std::vector<float>((size_t) juce::jmax(block_, 64)));
        for (auto& b : dcBlock_) b.prepare(sampleRate_);
        masterFallback_ = outMap_[0] < 0 && outMap_[1] < 0;
        monoFold_ = d->getActiveOutputChannels().countNumberOfSetBits() == 1;
    }
    rebuild();
    if (std::getenv("HUMUS_AUDIO_DEBUG"))
        std::fprintf(stderr,
                     "[audio] start rate=%.0f block=%d ins=%d outs=%d"
                     " (previous device refused %d blocks, largest %d)\n",
                     sampleRate_, block_, nIn,
                     d->getActiveOutputChannels().countNumberOfSetBits(),
                     refusedBlocks_.exchange(0, std::memory_order_relaxed),
                     refusedLargest_.exchange(0, std::memory_order_relaxed));
}

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
            if (const double bpm = tempoReq_.exchange(0.0, std::memory_order_relaxed); bpm > 0.0)
                graph_->transport().setTempo(bpm);
            if (const double b = seekBeatsReq_.exchange(-1.0, std::memory_order_relaxed); b >= 0.0)
                graph_->transport().setBeatPosition(b);
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
                const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
                const double spb = std::max(1.0, t.samplesPerBeat());
                const float gain = metroGain_.load(std::memory_order_relaxed);
                const int dL = masterFallback_ ? 0 : outMap_[0];
                const int dR = masterFallback_ ? 1 : outMap_[1];
                const int cL = dL >= 0 && dL < numOut ? dL : 0;
                const int cR = dR >= 0 && dR < numOut ? dR : cL;
                for (int n = 0; n < numSamples && left > 0; ++n, --left) {
                    const double elapsed = (double) (countInTotal_ - left);
                    if (elapsed >= countInNextBeat_) {
                        const bool down = countInNextBeat_ < 0.5;
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
                const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
                const double dBeat = t.tempo() / 60.0 / sr;
                const double perBar = t.beatsPerBar();
                double beat = t.beats() - dBeat * numSamples;
                if (metroNextBeat_ < beat || metroNextBeat_ > beat + 1.5)
                    metroNextBeat_ = std::ceil(beat - 1.0e-9);
                const int dL = masterFallback_ ? 0 : outMap_[0];
                const int dR = masterFallback_ ? 1 : outMap_[1];
                const int cL = dL >= 0 && dL < numOut ? dL : 0;
                const int cR = dR >= 0 && dR < numOut ? dR : cL;
                for (int n = 0; n < numSamples; ++n) {
                    if (beat >= metroNextBeat_) {
                        const double inBar = std::fmod(metroNextBeat_, std::max(1.0, perBar));
                        const bool downbeat = inBar < 0.25 || inBar > perBar - 0.25;
                        clickLen_ = (int) (sr * (downbeat ? 0.060 : 0.040));
                        clickRemain_ = clickLen_;
                        clickPhase_ = 0.0;
                        clickStep_ = 2.0 * juce::MathConstants<double>::pi
                                     * (downbeat ? 1568.0 : 1046.5) / sr;
                        clickAmp_ = downbeat ? 0.5f : 0.32f;
                        metroNextBeat_ += 1.0;
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
    const float inc = (float) (1.0 / (0.012 * (sampleRate_ > 0.0 ? sampleRate_ : 44100.0)));
    const float master = outputGain_.load();
    const bool lim = limiterOn_.load(std::memory_order_relaxed);
    if (lim)
        limiter_.set(0.966, 80.0, 10.0,
                     sampleRate_ > 0.0 ? sampleRate_ : 44100.0);
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
        1000.0 * numSamples / (sampleRate_ > 0.0 ? sampleRate_ : 44100.0);
    audioLoad_.store((float) ((juce::Time::getMillisecondCounterHiRes() - t0) / budgetMs),
                     std::memory_order_relaxed);
    lastCallbackMs_.store(juce::Time::getMillisecondCounterHiRes(),
                          std::memory_order_relaxed);
}

bool EngineHost::renderToFile(const std::string& path, double seconds, std::string& error) {
    AudioGraph g;
    if (!buildGraph(model_, g, error)) return false;
    g.prepare(sampleRate_, block_, model_.clock.tempo);
    MasterTap* so = nullptr;
    for (int i = 0; i < g.nodeCount(); ++i)
        if (auto* s = dynamic_cast<MasterTap*>(g.organism(i))) { so = s; break; }
    if (!so) { error = "patch has no SoundOut"; return false; }

    const int64_t total = (int64_t) (seconds * sampleRate_);
    std::vector<std::vector<float>> buf((size_t) so->channels());
    int64_t done = 0;
    while (done < total) {
        int n = (int) std::min<int64_t>(block_, total - done);
        g.processBlock(n);
        for (int c = 0; c < so->channels(); ++c)
            buf[(size_t) c].insert(buf[(size_t) c].end(), so->channelData(c), so->channelData(c) + so->lastBlockLength());
        done += n;
    }
    if (!writeWav(path, buf, sampleRate_)) { error = "could not write " + path; return false; }
    return true;
}

bool EngineHost::startMixRecording(const std::string& path, std::string& error) {
    const juce::ScopedLock sl(lock_);
    if (mixWriter_.active()) mixWriter_.stop();
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    if (!mixWriter_.start(path, 2, sr, false)) {
        error = "could not open " + path + " for recording";
        return false;
    }
    mixRecording_.store(true);
    return true;
}

void EngineHost::stopMixRecording() {
    const juce::ScopedLock sl(lock_);
    mixRecording_.store(false);
    mixWriter_.stop();
}

std::vector<std::pair<int, std::string>> EngineHost::choiceItems(const std::string& source,
                                                                 const std::string& organism) {
    std::vector<std::pair<int, std::string>> items;
    if (source == "patch-bank") {
        if (auto* pb = dynamic_cast<PatchBank*>(graph_ ? graph_->find(organism) : nullptr))
            for (int i = 0, n = pb->patchCount(); i < n; ++i) {
                auto name = pb->patchNameAt(i);
                items.push_back({i + 1, name.empty() ? std::to_string(i + 1)
                                                     : std::to_string(i + 1) + " " + name});
            }
        if (items.empty()) items.push_back({1, "1"});
        return items;
    }
    if (source == "video-inputs") {
        const auto cams = CameraCapture::availableDevices();
        for (size_t i = 0; i < cams.size(); ++i)
            items.push_back({(int) i + 1, cams[i]});   // utf8-ok: data
        if (items.empty()) items.push_back({1, "Default camera"});
        return items;
    }
    if (source == "serial-ports") {
        items.push_back({1, "Auto - first USB serial"});
        const auto devs = serial::listDevices();
        for (size_t i = 0; i < devs.size(); ++i) {
            const auto slash = devs[i].rfind('/');
            items.push_back({(int) i + 2,
                             slash == std::string::npos ? devs[i]
                                                        : devs[i].substr(slash + 1)});
        }
        return items;
    }
    const bool midiIn = source == "midi-in-ports", midiOut = source == "midi-out-ports";
    if (midiIn || midiOut) {
        auto& s = AppSettings::instance();
        for (int p = 1; p <= kMidiPorts; ++p) {
            const auto key = juce::String(midiIn ? "midi.in." : "midi.out.") + juce::String(p);
            auto name = s.getString(key + ".name");
            if (name.isEmpty()) name = s.getString(key).isEmpty() ? "(none)" : "(not connected)";
            if (name.length() > 16) name = name.substring(0, 15).trim() + "...";
            items.push_back({p, "Port " + std::to_string(p) + " - "   // utf8-ok: data
                            + name.toStdString()});
        }
        return items;
    }
    const bool inPairs = source == "audio-in-pairs";
    if (inPairs || source == "audio-out-pairs") {
        auto* dev = devices_.getCurrentAudioDevice();
        const auto names = dev ? (inPairs ? dev->getInputChannelNames()
                                          : dev->getOutputChannelNames())
                               : juce::StringArray();
        for (int i = 0; i < names.size(); i += 2) {
            std::string label = std::to_string(i + 1) + "/" + std::to_string(i + 2)
                              + " - " + names[i].toStdString();   // utf8-ok: data
            if (i + 1 < names.size()) label += " / " + names[i + 1].toStdString();
            items.push_back({i + 1, label});
        }
        if (items.empty())
            for (int i = 1; i <= 7; i += 2)
                items.push_back({i, "Channels " + std::to_string(i) + "/" + std::to_string(i + 1)});
        return items;
    }
    const bool audioIn = source == "audio-in-channels";
    if (audioIn || source == "audio-out-channels") {
        auto* dev = devices_.getCurrentAudioDevice();
        const auto names = dev ? (audioIn ? dev->getInputChannelNames()
                                          : dev->getOutputChannelNames())
                               : juce::StringArray();
        for (int i = 0; i < names.size(); ++i)
            items.push_back({i + 1, std::to_string(i + 1) + " - "   // utf8-ok: data
                                + names[i].toStdString()});
        if (items.empty())
            for (int i = 1; i <= 8; ++i) items.push_back({i, "Channel " + std::to_string(i)});
    }
    return items;
}

void EngineHost::primeOffline(int blocks) {
    const juce::ScopedLock sl(lock_);
    if (!graph_ || blocks <= 0) return;
    const std::int64_t pos = graph_->transport().samplePosition();
    const double beats = graph_->transport().beats();
    for (int i = 0; i < blocks; ++i) graph_->processBlock(block_);
    graph_->transport().restorePosition(pos, beats);
    publishClock();
}

}
