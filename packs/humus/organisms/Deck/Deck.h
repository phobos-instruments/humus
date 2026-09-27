// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/dsp/BeatGrid.h"
#include "common/ControlVinyl.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Files.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Params.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"
#include "common/TimeStretcher.h"

#include "hum/dsp/DspMath.h"
#include "hum/dsp/LiveWavWriter.h"
#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

class Deck : public Organism, public DeckControl, public LoadsOffThread, public LoadFault,
             public AbortsLoad, public DefersLoading, public MediaTracks, public ReloadOnParam,
             public ClipRecorder, public PlaysOwnPicture, public LoadedPicture,
             public MediaDuration, public InletMeter,
             public TimecodeStatus,
             public StereoFieldSource, public ControlSource, public VideoNode,
             public RollListener,
             public PinKinds {
public:
    Deck();
    ~Deck() override;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    int numVideoInputs() const override { return 1; }
    int numVideoOutputs() const override { return 1; }
    unsigned videoLaunchCount() const override { return videoLaunch_.load(); }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    bool controlOutlet(int) const override { return false; }
    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 3) return 0;
        const double len = (double) fileLen_.load();
        const double at = (double) playPos_.load();
        out[0] = {"position", (float) (len > 0.0 ? at / len : 0.0), 0.0f, 1.0f};
        out[1] = {"speed", tcOn_.load() ? tcSpeed_.load() : (float) playSpeed_.load(),
                  -4.0f, 4.0f};
        out[2] = {"seconds", (float) (at / std::max(1.0, fileSr_.load())), 0.0f, 36000.0f};
        return 3;
    }

    int64_t playbackPositionSamples() const override {
        const auto rolled = recLen_.load(std::memory_order_relaxed);
        return rolled > 0 ? rolled : playPos_.load();
    }
    int64_t fileLengthSamples() const override {
        const auto rolled = recLen_.load(std::memory_order_relaxed);
        return rolled > 0 ? rolled : fileLen_.load();
    }
    double  playbackSampleRate() const override {
        const double sr = fileSr_.load();
        return sr > 0.0 ? sr : sampleRate_;
    }
    double  effectiveBpm() const override { return effBpm_.load(); }
    double  playbackSpeed() const override { return playSpeed_.load(); }

    bool  timecodeOn() const override { return tcOn_.load(); }
    bool  timecodePresent() const override { return tcPresent_.load(); }
    int   timecodeFormat() const override { return tcFormat_.load(); }
    float timecodeCarrierHz() const override { return tcCarrier_.load(); }
    float timecodeLevel() const override { return tcLevel_.load(); }
    float timecodeSpeed() const override { return tcSpeed_.load(); }

    int fieldRead(float* lr, int maxPairs) const override {
        const unsigned w = fieldAt_.load(std::memory_order_relaxed);
        const int n = maxPairs < kFieldPairs ? maxPairs : kFieldPairs;
        for (int i = 0; i < n; ++i) {
            const unsigned idx = (w + (unsigned) kFieldPairs - (unsigned) n + (unsigned) i)
                                 % (unsigned) kFieldPairs;
            lr[2 * i] = field_[2 * idx];
            lr[2 * i + 1] = field_[2 * idx + 1];
        }
        return n;
    }
    unsigned fieldStamp() const override { return fieldStamp_.load(std::memory_order_relaxed); }
    float fieldCorrelation() const override { return fieldCorr_.load(std::memory_order_relaxed); }
    std::string playSwitch() const override { return "Active"; }
    void    requestSeekSamples(int64_t s) override { seekReq_.store(s < 0 ? 0 : s); }
    void    rolled() override;

    void    setBendPercent(double pct) override { bend_.store(pct); }

    void    setScrub(bool active, double targetSample) override {
        scrubTarget_.store(targetSample);
        scrubbing_.store(active);
    }

    void    beginLoopRoll() override { rollBeginReq_.store(true); }
    void    endLoopRoll() override { rollEndReq_.store(true); }

    void    loadFromFile(const std::string& uri) override;
    void    warmFromFile(const std::string& uri);
    std::string audioPathFor(const std::string& uri) const;

    static constexpr int kWaveBuckets = 8192;
    static constexpr int kBucketProbes = 256;
    std::vector<float> waveformPeaks() const override {
        const juce::ScopedLock sl(loadLock_);
        return peaks_;
    }

    std::vector<float> wavePeaksBetween(int64_t from, int64_t to, int buckets) const override;

    float loadProgress() const override { return loadProgress_.load(); }

    bool startTake(const std::string& wavPath, double sampleRate) override;
    void stopTake() override;
    int inletMeter(float* levels, int maxLevels) const override {
        const int n = std::min(maxLevels, kInletChans);
        for (int c = 0; c < n; ++c)
            levels[c] = inPeak_[(size_t) c].load(std::memory_order_relaxed);
        return n;
    }

    bool takeActive() const override { return takeActive_.load(std::memory_order_relaxed); }
    double takeStartBeat() const override { return takeStartBeat_.load(std::memory_order_relaxed); }
    std::int64_t takeLengthSamples() const override {
        return takeLen_.load(std::memory_order_relaxed);
    }
    int takeLaps(double*, std::int64_t*, int) const override { return 0; }
    void ensureClipsLoaded() override {}

    std::string loadFault() const override {
        const juce::ScopedLock sl(loadLock_);
        return fault_;
    }

    void abandonLoad() override { abandon_.store(true); }

    bool watchingVideoInput() const override {
        return takeActive_.load(std::memory_order_relaxed) || params.get("Record", 0.0) >= 0.5;
    }

    void deferLoading(bool defer) override { defer_ = defer; }

    std::string loadedPicture() const override {
        const juce::ScopedLock sl(loadLock_);
        return picture_;
    }

    std::vector<std::string> mediaTracks() const override {
        const juce::ScopedLock sl(loadLock_);
        return tracks_;
    }

    bool reloadsOn(const std::string& param) const override { return param == "AudioTrack"; }

    void noteMediaSeconds(double seconds) override {
        const int64_t frames = seconds > 0.0 ? (int64_t) (seconds * fileSr_.load()) : 0;
        silentLen_.store(frames);
        if (fileLen_.load() == 0 && frames > 0) fileLen_.store(frames);
        if (frames <= 0) return;
        const juce::ScopedLock sl(loadLock_);
        fault_.clear();
    }

private:
    void applyPending();
    void captureTake(const float* const* in, int numIn, int numSamples, const Transport& t);
    void publishPosition(int numSamples, double hostSr);
    void notePicture();
    void adoptStream(const SharedSound& sound, const SoundFill& filled, bool sameSound,
                     double wasSr);

    SharedAudio file_;
    std::string loadedUri_;
    std::string playingUri_;
    std::string picture_;
    std::string fault_;
    std::vector<std::string> tracks_;
    LiveWavWriter writer_;
    std::atomic<bool> takeActive_{false};
    std::atomic<double> takeStartBeat_{-1.0};
    std::atomic<std::int64_t> takeLen_{0};
    int loadedTrack_ = 0;
    std::atomic<bool> abandon_{false};
    std::atomic<bool> keepsPosition_{false};
    std::atomic<double> positionScale_{1.0};
    bool defer_ = false;
    std::vector<float> peaks_;
    double readPos_ = 0.0;
    double rate_ = 1.0;
    double velocity_ = 0.0;
    bool prevSync_ = false;
    bool prevKeylock_ = false;
    double sampleRate_ = kDefaultSampleRate;

    mutable juce::CriticalSection loadLock_;
    SoundFill fill_, pendingFill_;
    std::atomic<bool> streaming_{false};
    SharedAudio pending_;
    double pendingSr_ = kDefaultSampleRate;
    std::atomic<bool> hasPending_ {false};
    std::atomic<float> loadProgress_ {1.0f};
    std::atomic<int64_t> silentLen_ {0};

    std::atomic<int64_t> playPos_ {0};
    std::atomic<int64_t> recLen_ {0};
    static constexpr int kInletChans = 2;
    static constexpr float kInletDecay = 0.82f;
    std::array<std::atomic<float>, kInletChans> inPeak_ {};
    std::atomic<int64_t> fileLen_ {0};
    std::atomic<int64_t> seekReq_ {-1};
    std::atomic<double>  fileSr_  {kDefaultSampleRate};
    std::atomic<double>  effBpm_  {0.0};
    std::atomic<double>  bend_    {0.0};
    std::atomic<bool>    tcOn_ {false};
    std::atomic<bool>    tcPresent_ {false};
    std::atomic<float>   tcLevel_ {0.0f};
    std::atomic<float>   tcSpeed_ {0.0f};
    std::atomic<int>     tcFormat_ {-1};
    std::atomic<float>   tcCarrier_ {0.0f};
    float                field_[2 * StereoFieldSource::kFieldPairs] = {};
    std::atomic<unsigned> fieldAt_ {0};
    std::atomic<unsigned> fieldStamp_ {0};
    std::atomic<float>   fieldCorr_ {0.0f};
    std::atomic<bool>    scrubbing_ {false};
    std::atomic<double>  scrubTarget_ {0.0};
    std::atomic<bool>    rollBeginReq_ {false};
    std::atomic<bool>    rollEndReq_ {false};
    bool                 rollActive_ = false;
    double               rollPhantom_ = 0.0;

    TimeStretcher stretch_;
    std::vector<float> stInL_, stInR_, stOutL_, stOutR_;
    int maxBlock_ = 0;

    double tempo_ = 1.0;
    bool tempoPrimed_ = false;
    double stretchInFrac_ = 0.0;
    double declick_ = 1.0;
    double scrubGain_ = 0.0;

    std::atomic<unsigned> videoLaunch_ {0};
    std::atomic<double>   playSpeed_ {0.0};
    double                blockStartPos_ = 0.0;

    ControlVinyl vinyl_;
    std::vector<float> tcL_, tcR_;
    double vinylRate_ = kDefaultSampleRate;
};

}
