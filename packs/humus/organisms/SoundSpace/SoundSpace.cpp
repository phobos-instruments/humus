#include "SoundSpace/SoundSpace.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#include <juce_events/juce_events.h>

namespace hum {

namespace {
inline float frand(std::uint32_t& s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (float) (s & 0xFFFFFF) / (float) 0xFFFFFF;
}
}

class SoundSpace::Lifecycle : public juce::Timer {
public:
    explicit Lifecycle(SoundSpace& o) : owner_(o) { startTimer(250); }
    ~Lifecycle() override { stopTimer(); }
    void timerCallback() override { owner_.captureTick(); }
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
    if (refreshUris()) publishCorpus(analyzeCorpus(uris_, sampleRate_));
    applyPending();
    if (!lifecycle_ && juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        lifecycle_ = std::make_unique<Lifecycle>(*this);
    reset();
}

void SoundSpace::captureTick() {
    const bool want = params.get("Record", 0.0) >= 0.5;
    if (!want) armLatch_ = false;
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
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
    displayPoints_.clear();
    displayPoints_.reserve(corpus->grains.size());
    for (const auto& g : corpus->grains) displayPoints_.push_back({g.x, g.y, g.file});
    ++generation_;
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
    const juce::ScopedLock sl(loadLock_);
    corpus_ = std::move(pending_);
    for (auto& v : voices_) v = Voice{};
    hasPending_.store(false);
}

void SoundSpace::spawnGrain() {
    if (!corpus_ || corpus_->grains.empty()) return;
    Voice* slot = nullptr;
    for (auto& v : voices_)
        if (v.file < 0) { slot = &v; break; }
    if (!slot) return;

    const float spray = (float) params.get("Spray", 0.12);
    const float tx = (float) params.get("X", 0.5) + spray * (frand(rng_) * 2.0f - 1.0f);
    const float ty = (float) params.get("Y", 0.5) + spray * (frand(rng_) * 2.0f - 1.0f);
    const Grain* best = nullptr;
    float bestD = 1.0e9f;
    for (const auto& g : corpus_->grains) {
        const float dx = g.x - tx, dy = g.y - ty;
        const float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; best = &g; }
    }
    if (!best) return;

    const auto& file = corpus_->files[(size_t) best->file];
    if (file.getNumSamples() == 0) return;
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    const double semis = params.get("Pitch", 0.0);
    const double rate = std::pow(2.0, semis / 12.0)
                        * (corpus_->rates[(size_t) best->file] / sr);
    int wanted = (int) (params.get("GrainSize", 120.0) * 0.001 * sr);
    if (const double sizeRand = params.get("SizeRand", 0.0); sizeRand > 0.0)
        wanted = (int) (wanted * (1.0 + sizeRand * (frand(rng_) * 2.0f - 1.0f) * 0.6));
    const int availOut = (int) ((file.getNumSamples() - best->start) / std::max(0.01, rate)) - 2;
    const int total = std::clamp(wanted, 32, std::max(32, availOut));

    const float spread = (float) params.get("Spread", 0.4);
    const float pan = 0.5f + spread * (frand(rng_) - 0.5f);
    double vrate = rate;
    if (const double jitter = params.get("Jitter", 0.0); jitter > 0.0)
        vrate *= std::pow(2.0, jitter * (frand(rng_) * 2.0f - 1.0f) * 7.0 / 12.0);
    slot->file = best->file;
    slot->pos = best->start;
    slot->rate = vrate;
    slot->total = slot->remaining = total;
    slot->ampL = 1.0f - pan;
    slot->ampR = pan;
}

void SoundSpace::renderAdd(float* left, float* right, int numSamples) {
    if (!corpus_) return;
    const float master = mute_ ? 0.0f : (float) params.get("Gain", 1.0);
    for (auto& v : voices_) {
        if (v.file < 0) continue;
        const auto& buf = corpus_->files[(size_t) v.file];
        const int len = buf.getNumSamples();
        const int chans = buf.getNumChannels();
        const float* srcL = buf.getReadPointer(0);
        const float* srcR = buf.getReadPointer(chans > 1 ? 1 : 0);
        for (int n = 0; n < numSamples && v.remaining > 0; ++n, --v.remaining) {
            const int i0 = (int) v.pos;
            if (i0 >= len - 1) { v.remaining = 0; break; }
            const float frac = (float) (v.pos - i0);
            const float t = 1.0f - (float) v.remaining / (float) v.total;
            const float w = 0.5f - 0.5f * std::cos(6.2831853f * t);
            const float sL = srcL[i0] + frac * (srcL[i0 + 1] - srcL[i0]);
            const float sR = srcR[i0] + frac * (srcR[i0 + 1] - srcR[i0]);
            const float g = w * master;
            left[n]  += g * v.ampL * sL * 2.0f;
            right[n] += g * v.ampR * sR * 2.0f;
            v.pos += v.rate;
        }
        if (v.remaining <= 0) v = Voice{};
    }
}

void SoundSpace::process(const float* const* in, int numIn, float* const* out, int numOut,
                         int numSamples, const Transport&) {
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
    if (!corpus_ || corpus_->grains.empty()) return;
    mute_ = params.get("Mute", 0.0) >= 0.5;

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    const double density = std::max(0.1, params.get("Density", 8.0));
    const double interval = sr / density;

    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];
    int cursor = 0;
    while (cursor < numSamples) {
        if (spawnCountdown_ <= 0.0) { spawnGrain(); spawnCountdown_ += interval; }
        const int run = std::min(numSamples - cursor,
                                 std::max(1, (int) std::ceil(spawnCountdown_)));
        renderAdd(L + cursor, R + cursor, run);
        cursor += run;
        spawnCountdown_ -= run;
    }
}

}
