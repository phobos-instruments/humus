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

#include "hum/caps/Files.h"
#include "hum/caps/Samples.h"
#include "hum/Organism.h"
#include "hum/dsp/LiveWavWriter.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class SoundSpace : public Organism, public FileLoader, public SoundMapSource,
                   public LiveCaptureSource {
public:
    static constexpr int kFiles = 4;
    static constexpr int kMaxGrainVoices = 24;
    static constexpr int kGrainsPerFile = 800;
    static constexpr double kCaptureCapSeconds = 120.0;

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

    bool fetchCompletedTake(Take& out) override;

    void captureTick();

private:
    struct Grain { int file = 0; int start = 0; int len = 0; float x = 0.5f, y = 0.5f; };
    struct Corpus {
        std::array<juce::AudioBuffer<float>, kFiles> files;
        std::array<double, kFiles> rates {kDefaultSampleRate, kDefaultSampleRate, kDefaultSampleRate, kDefaultSampleRate};
        std::vector<Grain> grains;
    };
    struct Voice {
        int file = -1;
        double pos = 0.0, rate = 1.0;
        int remaining = 0, total = 0;
        float ampL = 0.0f, ampR = 0.0f;
    };

    static std::shared_ptr<Corpus> analyzeCorpus(const std::array<std::string, kFiles>& uris,
                                                 double engineRate);

    bool refreshUris();
    void publishCorpus(std::shared_ptr<Corpus>);
    void applyPending();
    void spawnGrain();
    void renderAdd(float* left, float* right, int numSamples);

    std::shared_ptr<Corpus> corpus_;
    std::array<Voice, kMaxGrainVoices> voices_;
    double spawnCountdown_ = 0.0;
    bool mute_ = false;
    std::uint32_t rng_ = 0x9e3779b9u;

    juce::CriticalSection loadLock_;
    std::shared_ptr<Corpus> pending_;
    std::atomic<bool> hasPending_{false};

    std::array<std::string, kFiles> uris_;
    std::vector<MapPoint> displayPoints_;
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
