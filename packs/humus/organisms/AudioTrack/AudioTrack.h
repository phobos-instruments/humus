#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/Pattern.h"
#include "hum/dsp/LevelMeter.h"
#include "hum/dsp/LiveWavWriter.h"
#include "hum/dsp/PitchShifter.h"

namespace hum {

class AudioTrack : public Organism, public ClipArrangement, public ClipRecorder,
                   public LevelMeterSource {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void loadFrom(const OrganismState& state) override;
    void setPattern(const Pattern& pattern) override {
        pattern_ = pattern;
        clipsDirty_ = true;
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    bool startTake(const std::string& wavPath, double sampleRate) override;
    void stopTake() override;
    bool takeActive() const override { return takeActive_.load(std::memory_order_relaxed); }
    double takeStartBeat() const override { return takeStartBeat_.load(std::memory_order_relaxed); }
    std::int64_t takeLengthSamples() const override { return takeLen_.load(std::memory_order_relaxed); }
    int takeLaps(double* beats, std::int64_t* samples, int maxLaps) const override;
    void ensureClipsLoaded() override;

    int meterChannels() const override { return 2; }
    float meterLevel(int ch) const override { return meter_.level(ch); }

private:
    struct ClipPlay {
        double startBeat = 0.0, lenBeats = 0.0;
        bool loop = false;
        std::int64_t offset = 0;
        double clipStartBeat = 0.0, periodBeats = 0.0;
        double gain = 1.0;
        std::shared_ptr<const juce::AudioBuffer<float>> pcm;
        double readPos = -1.0;
        double rate = 1.0;
        double sourceBpm = 0.0;
        int warpMode = 0;
        double fadeInBeats = 0.0, fadeOutBeats = 0.0;
        bool reverse = false;
        double pitchRatio = 1.0;
        std::array<PitchShifter, 2> shift;
        int shiftLatency = 0;
    };
    struct CachedFile {
        std::shared_ptr<const juce::AudioBuffer<float>> pcm;
        double fileSampleRate = 0.0;
    };

    void applyPending();
    void renderClips(float* const* out, int numOut, int numSamples, const Transport& t);
    const CachedFile* fileFor(const std::string& path);

    Pattern pattern_;
    bool clipsDirty_ = false;
    std::map<std::string, CachedFile> cache_;
    std::string takePath_;

    juce::CriticalSection loadLock_;
    std::vector<ClipPlay> pendingClips_;
    std::atomic<bool> hasPending_{false};

    std::vector<ClipPlay> clips_;
    double lastBlockEndBeat_ = -1.0;

    LiveWavWriter writer_;
    std::atomic<bool> takeActive_{false};
    std::atomic<double> takeStartBeat_{-1.0};
    std::atomic<std::int64_t> takeLen_{0};
    std::atomic<int> lapCount_{0};
    std::array<double, kMaxLaps> lapBeats_{};
    std::array<std::int64_t, kMaxLaps> lapSamples_{};
    double lastCaptureBeat_ = -1.0;
    LevelMeter meter_;
};

}
