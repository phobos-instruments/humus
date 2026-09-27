// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/caps/Audio.h"
#include "hum/caps/Files.h"
#include "hum/caps/Samples.h"
#include "hum/Organism.h"
#include "hum/dsp/LevelMeter.h"
#include "hum/dsp/LiveWavWriter.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class SoundSpace : public Organism, public FileLoader, public SoundMapSource,
                   public LiveCaptureSource, public LevelMeterSource, public GrainFlashSource {
public:
    static constexpr int kFiles = 4;
    static constexpr int kMaxGrainVoices = 200;
    static constexpr int kGrainsPerFile = 800;
    static constexpr double kCaptureCapSeconds = 120.0;
    static constexpr int kLiveGrains = 512;
    static constexpr int kFeatures = 6;
    static constexpr int kWindow = 4096;
    static constexpr double kLiveRingSeconds = 10.0;
    static constexpr double kLiveWindowSeconds = 8.0;
    static constexpr double kLiveFreshSeconds = 0.35;
    static constexpr float kLiveFloor = -3.5f;
    static constexpr double kMaxScatter = 0.95;
    static double scatteredGap(double interval, double scatter, double roll);
    using Feature = std::array<float, kFeatures>;

    SoundSpace();
    ~SoundSpace() override;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string&) override;

    unsigned mapGeneration() const override { return generation_; }
    const std::vector<MapPoint>& mapPoints() const override { return displayPoints_; }
    int recentGrains(Flash* out, int capacity) const override;

    bool fetchCompletedTake(Take& out) override;

    int meterChannels() const override { return 2; }
    float meterLevel(int channel) const override { return inletMeter_.level(channel); }

    void captureTick();
    void liveTick();
    bool liveWanted() const { return params.get("Live", 0.0) >= 0.5; }

private:
    struct Grain { int file = 0; int start = 0; int len = 0; float x = 0.5f, y = 0.5f; };
    struct Projection {
        bool fitted = false;
        std::array<double, kFeatures> mean{}, sd{}, first{}, second{};
        float loX = 0.0f, loY = 0.0f, scaleX = 0.0f, scaleY = 0.0f;
        void place(const Feature& feature, float& x, float& y) const;
    };
    struct LiveGrain {
        std::atomic<std::int64_t> start{-1};
        std::atomic<float> x{0.5f}, y{0.5f};
    };
    struct Picked { int file = -1; double start = 0.0; double sourceRate = 0.0; int room = 0; float x = 0.0f, y = 0.0f; };
    struct FlashSlot {
        std::atomic<float> x{0.0f}, y{0.0f}, seconds{0.0f};
        std::atomic<int> file{0};
        std::atomic<std::uint32_t> serial{0};
    };
    static constexpr int kFlashRing = 96;
    class Analyser;
    struct AnalyserDeleter { void operator()(Analyser*) const; };
    static Analyser* makeAnalyser();
    static Feature measureLive(Analyser&, const float* mono, double sampleRate);
    static void forgetPrevious(Analyser&);
    struct Corpus {
        Projection projection;
        std::array<juce::AudioBuffer<float>, kFiles> files;
        std::array<double, kFiles> rates {kDefaultSampleRate, kDefaultSampleRate, kDefaultSampleRate, kDefaultSampleRate};
        std::vector<Grain> grains;
    };
    struct Voice {
        int file = -1;
        double pos = 0.0, rate = 1.0;
        int remaining = 0, total = 0;
        float ampL = 0.0f, ampR = 0.0f;
        float rise = 0.5f, hold = 0.0f;
    };

    static std::shared_ptr<Corpus> analyzeCorpus(const std::array<std::string, kFiles>& uris,
                                                 double engineRate);

    bool refreshUris();
    void publishCorpus(std::shared_ptr<Corpus>);
    void applyPending();
    double between(double low, double high);
    double between(double low, double high, double shared, double apart);
    int voicesSounding() const;
    void spawnGrain();
    void publishFlash(const Picked& picked, float seconds);
    void renderAdd(float* left, float* right, int numSamples);
    Picked pickGrain(float x, float y, double rate) const;
    void writeLive(const float* left, const float* right, int numSamples, float gain);
    void feedLiveBack(const float* left, const float* right, int numSamples, float amount);
    double samplesToGrid(const Transport& transport, int offset) const;
    void forgetLive();
    void rebuildDisplay(std::int64_t written);

    std::shared_ptr<Corpus> corpus_;
    std::array<Voice, kMaxGrainVoices> voices_;
    double spawnCountdown_ = 0.0;
    bool onGrid_ = false;
    bool mute_ = false;
    std::uint32_t rng_ = 0x9e3779b9u;
    std::array<FlashSlot, kFlashRing> flashes_;
    std::uint32_t flashSerial_ = 0;

    juce::CriticalSection loadLock_;
    std::shared_ptr<Corpus> pending_;
    std::atomic<bool> hasPending_{false};

    std::array<std::string, kFiles> uris_;
    std::vector<MapPoint> displayPoints_, corpusPoints_;
    Projection projection_;
    LevelMeter inletMeter_;
    std::vector<float> liveLeft_, liveRight_;
    std::atomic<std::int64_t> liveWritten_{0};
    std::array<LiveGrain, kLiveGrains> liveGrains_;
    std::array<Feature, kLiveGrains> liveFeatures_{};
    std::unique_ptr<Analyser, AnalyserDeleter> liveAnalyser_;
    std::int64_t liveAnalysed_ = 0;
    int liveSlot_ = 0;
    bool liveWas_ = false;
    unsigned generation_ = 0;
    unsigned loadSeq_ = 0;
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);

    class Lifecycle;
    friend class Lifecycle;
    std::unique_ptr<Lifecycle> lifecycle_;
    LiveWavWriter writer_;
    std::atomic<bool> writerLive_{false};
    std::atomic<bool> inWrite_{false};
    std::atomic<std::int64_t> capturedSamples_{0};
    std::atomic<float> capturedPeak_{0.0f};
    bool armLatch_ = false;
    std::string capturePath_;
    juce::CriticalSection captureLock_;
    Take completed_;
    bool hasCompleted_ = false;
};

}
