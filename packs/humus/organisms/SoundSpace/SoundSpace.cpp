// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SoundSpace/SoundSpace.h"

#include <algorithm>
#include <utility>
#include <cmath>
#include <cstdint>
#include <cstring>

#include <juce_events/juce_events.h>

#include "hum/dsp/DspMath.h"

namespace hum {

double SoundSpace::scatteredGap(double interval, double scatter, double roll) {
    const double swing = kMaxScatter * std::clamp(scatter, 0.0, 1.0);
    return interval * (1.0 + swing * (2.0 * std::clamp(roll, 0.0, 1.0) - 1.0));
}

class SoundSpace::Lifecycle : public juce::Timer {
public:
    explicit Lifecycle(SoundSpace& o) : owner_(o) { startTimer(250); }
    ~Lifecycle() override { stopTimer(); }
    void timerCallback() override {
        owner_.captureTick();
        owner_.liveTick();
        const int wanted = owner_.liveWanted() ? 50 : 250;
        if (getTimerInterval() != wanted) startTimer(wanted);
    }
private:
    SoundSpace& owner_;
};

SoundSpace::SoundSpace() = default;

SoundSpace::~SoundSpace() {
    alive_->store(false);
    lifecycle_.reset();
    if (writer_.active()) {
        writerLive_.store(false);
        for (int i = 0; i < 400 && inWrite_.load(); ++i) juce::Thread::sleep(1);
        writer_.stop();
    }
}

void SoundSpace::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    inletMeter_.prepare(sampleRate);
    const auto ring = (size_t) (kLiveRingSeconds * sampleRate);
    if (liveLeft_.size() != ring) {
        liveLeft_.assign(ring, 0.0f);
        liveRight_.assign(ring, 0.0f);
        forgetLive();
    }
    if (refreshUris()) publishCorpus(analyzeCorpus(uris_, sampleRate_));
    applyPending();
    if (!lifecycle_ && juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        lifecycle_ = std::make_unique<Lifecycle>(*this);
    reset();
}

void SoundSpace::captureTick() {
    const bool want = params.get("Record", 0.0) >= 0.5;
    if (!want) armLatch_ = false;
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const bool capped = writer_.active()
        && capturedSamples_.load() >= (std::int64_t) (kCaptureCapSeconds * sr);

    if (writer_.active() && (!want || capped)) {
        writerLive_.store(false);
        for (int i = 0; i < 400 && inWrite_.load(); ++i) juce::Thread::sleep(1);
        writer_.stop();
        if (capped) armLatch_ = true;
        if (capturedPeak_.load() < 1.0e-4f) {
            juce::File(juce::String(capturePath_)).deleteFile();
            return;
        }
        int slot = kFiles;
        for (int f = 0; f < kFiles; ++f)
            if (params.getText("File" + std::to_string(f + 1)).empty()) { slot = f + 1; break; }
        const juce::ScopedLock sl(captureLock_);
        completed_ = {slot, capturePath_};
        hasCompleted_ = true;
        return;
    }

    if (want && !writer_.active() && !armLatch_) {
        auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("Humus").getChildFile("Recordings")
                       .getChildFile("Captures");
        dir.createDirectory();
        const auto leaf = juce::String(name().empty() ? "SoundSpace" : name())
                              .replaceCharacter('/', '-')
                          + juce::Time::getCurrentTime().formatted("-%H%M%S") + ".wav";
        capturePath_ = dir.getChildFile(leaf).getFullPathName().toStdString();
        capturedSamples_.store(0);
        capturedPeak_.store(0.0f);
        if (writer_.start(capturePath_, 2, sr))
            writerLive_.store(true, std::memory_order_release);
    }
}

bool SoundSpace::fetchCompletedTake(Take& out) {
    const juce::ScopedLock sl(captureLock_);
    if (!hasCompleted_) return false;
    out = completed_;
    hasCompleted_ = false;
    return true;
}

void SoundSpace::reset() {
    for (auto& v : voices_) v = Voice{};
    spawnCountdown_ = 0.0;
}

bool SoundSpace::refreshUris() {
    bool changed = false;
    for (int f = 0; f < kFiles; ++f) {
        const std::string uri = params.getText("File" + std::to_string(f + 1));
        if (uri != uris_[(size_t) f]) { uris_[(size_t) f] = uri; changed = true; }
    }
    return changed;
}

void SoundSpace::publishCorpus(std::shared_ptr<Corpus> corpus) {
    corpusPoints_.clear();
    corpusPoints_.reserve(corpus->grains.size());
    for (const auto& g : corpus->grains) corpusPoints_.push_back({g.x, g.y, g.file});
    projection_ = corpus->projection;
    for (size_t i = 0; i < liveGrains_.size(); ++i) {
        if (liveGrains_[i].start.load() < 0) continue;
        float x = 0.5f, y = 0.5f;
        projection_.place(liveFeatures_[i], x, y);
        liveGrains_[i].x.store(x);
        liveGrains_[i].y.store(y);
    }
    rebuildDisplay(liveWritten_.load());
    {
        const juce::ScopedLock sl(loadLock_);
        pending_ = std::move(corpus);
    }
    hasPending_.store(true);
}

void SoundSpace::loadFromFile(const std::string&) {
    if (!refreshUris()) return;
    const unsigned seq = ++loadSeq_;
    if (juce::MessageManager::getInstanceWithoutCreating() == nullptr) {
        publishCorpus(analyzeCorpus(uris_, sampleRate_));
        return;
    }
    juce::Thread::launch([self = this, alive = alive_, uris = uris_, sr = sampleRate_, seq] {
        auto corpus = analyzeCorpus(uris, sr);
        juce::MessageManager::callAsync([self, alive, corpus = std::move(corpus), seq] {
            if (!alive->load() || seq != self->loadSeq_) return;
            self->publishCorpus(corpus);
        });
    });
}

void SoundSpace::applyPending() {
    const juce::ScopedTryLock sl(loadLock_);
    if (!sl.isLocked()) return;
    std::swap(corpus_, pending_);
    for (auto& v : voices_) v = Voice{};
    hasPending_.store(false);
}

void SoundSpace::process(const float* const* in, int numIn, float* const* out, int numOut,
                         int numSamples, const Transport& transport) {
    const float* inlet[2] = {numIn > 0 && in ? in[0] : nullptr,
                             numIn > 0 && in ? in[numIn > 1 ? 1 : 0] : nullptr};
    inletMeter_.measure(inlet, 2, numSamples);
    if (writerLive_.load(std::memory_order_acquire) && numIn > 0 && in != nullptr) {
        struct InFlight {
            std::atomic<bool>& f;
            explicit InFlight(std::atomic<bool>& a) : f(a) { f.store(true); }
            ~InFlight() { f.store(false); }
        } guard(inWrite_);
        if (writerLive_.load(std::memory_order_acquire)) {
            const float* chans[2] = {in[0], numIn > 1 ? in[1] : in[0]};
            writer_.write(chans, numSamples);
            capturedSamples_.fetch_add(numSamples);
            float pk = capturedPeak_.load(std::memory_order_relaxed);
            for (int n = 0; n < numSamples; n += 8)
                pk = std::max({pk, std::abs(chans[0][n]), std::abs(chans[1][n])});
            capturedPeak_.store(pk, std::memory_order_relaxed);
        }
    }

    for (int c = 0; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    if (numOut == 0) return;
    if (hasPending_.load()) applyPending();
    const bool live = liveWanted() && !liveLeft_.empty();
    const bool recording = live && params.get("Freeze", 0.0) < 0.5 && inlet[0] != nullptr;
    if (recording) writeLive(inlet[0], inlet[1], numSamples, (float) params.get("Input", 1.0));
    mute_ = params.get("Mute", 0.0) >= 0.5;

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double density = std::max(0.1, params.get("Density", 8.0));
    const double interval = sr / density;

    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];
    int cursor = 0;
    while (cursor < numSamples) {
        if (spawnCountdown_ <= 0.0) {
            const double pull = std::clamp(params.get("Quantize", 1.0), 0.0, 1.0);
            const double wait = onGrid_ ? 0.0 : samplesToGrid(transport, cursor) * pull;
            if (wait >= 1.0) {
                spawnCountdown_ = wait;
                onGrid_ = true;
            } else {
                spawnGrain();
                spawnCountdown_ += scatteredGap(interval, params.get("Scatter", 0.0), between(0.0, 1.0));
                onGrid_ = false;
            }
        }
        const int run = std::min(numSamples - cursor,
                                 std::max(1, (int) std::ceil(spawnCountdown_)));
        renderAdd(L + cursor, R + cursor, run);
        cursor += run;
        spawnCountdown_ -= run;
    }

    if (const auto feedback = (float) params.get("Feedback", 0.0); recording && feedback > 0.0f)
        feedLiveBack(L, R, numSamples, feedback);
    if (inlet[0] == nullptr) return;
    const auto wet = (float) params.get("Mix", 1.0);
    const float dry = mute_ ? 0.0f : 1.0f - wet;
    for (int n = 0; n < numSamples; ++n) {
        L[n] = wet * L[n] + dry * inlet[0][n];
        if (R != L) R[n] = wet * R[n] + dry * inlet[1][n];
    }
}

}
