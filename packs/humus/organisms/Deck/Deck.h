#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/dsp/BeatGrid.h"
#include "hum/dsp/Timecode.h"
#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "common/TimeStretcher.h"

namespace hum {

class Deck : public Organism, public DeckControl {
public:
    Deck();
    ~Deck() override;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    double masterTempo() const override;

    int64_t playbackPositionSamples() const override { return playPos_.load(); }
    int64_t fileLengthSamples() const override { return fileLen_.load(); }
    double  playbackSampleRate() const override { return fileSr_.load(); }
    double  effectiveBpm() const override { return effBpm_.load(); }
    void    requestSeekSamples(int64_t s) override { seekReq_.store(s < 0 ? 0 : s); }

    void    setBendPercent(double pct) override { bend_.store(pct); }

    void    setScrub(bool active, double targetSample) override {
        scrubTarget_.store(targetSample);
        scrubbing_.store(active);
    }

    void    beginLoopRoll() override { rollBeginReq_.store(true); }
    void    endLoopRoll() override { rollEndReq_.store(true); }

    void    loadFromFile(const std::string& uri) override;

    static constexpr int kWaveBuckets = 8192;
    const std::vector<float>& waveformPeaks() const override { return peaks_; }

private:
    void applyPending();

    juce::AudioBuffer<float> file_;
    std::string loadedUri_;
    std::vector<float> peaks_;
    double readPos_ = 0.0;
    double rate_ = 1.0;
    double velocity_ = 0.0;
    bool prevSync_ = false;
    bool prevKeylock_ = false;
    double sampleRate_ = 44100.0;

    juce::CriticalSection loadLock_;
    juce::AudioBuffer<float> pending_;
    double pendingSr_ = 44100.0;
    std::atomic<bool> hasPending_ {false};

    std::atomic<int64_t> playPos_ {0};
    std::atomic<int64_t> fileLen_ {0};
    std::atomic<int64_t> seekReq_ {-1};
    std::atomic<double>  fileSr_  {44100.0};
    std::atomic<double>  effBpm_  {0.0};
    std::atomic<double>  bend_    {0.0};
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

    TimecodeTracker tc_;
    double prevTcHz_ = 0.0;
};

}
