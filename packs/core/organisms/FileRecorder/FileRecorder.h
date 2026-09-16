// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/caps/Files.h"
#include "hum/Organism.h"
#include "hum/dsp/LevelMeter.h"
#include "hum/dsp/LiveWavWriter.h"

namespace hum {

class FileRecorder : public Organism, public Recorder, public LevelMeterSource {
public:
    using RecordTarget = Recorder::RecordTarget;

    explicit FileRecorder(int channels = 2) : channels_(std::max(1, channels)) {}

    int numAudioInputs() const override { return channels_; }
    int numAudioOutputs() const override { return channels_; }

    void prepare(double sampleRate, int maxBlock) override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    bool startRecording(const std::vector<RecordTarget>& targets, int punchMode,
                        double durationSeconds, double sampleRate, bool append = false) override;
    void stopRecording() override;
    bool isRecording() const override;
    int channels() const override { return channels_; }

    int meterChannels() const override { return channels_; }
    float meterLevel(int channel) const override { return meter_.level(channel); }

    bool consumeAutoStop() override { return autoStop_.exchange(false); }

private:
    LevelMeter meter_;
    int channels_ = 2;
    std::vector<float> zero_;
    std::vector<const float*> writePtrs_;
    std::vector<std::unique_ptr<LiveWavWriter>> writers_;
    std::vector<int> firstChannel_;
    std::vector<int> writerChannels_;

    int punchMode_ = 0;
    std::int64_t recorded_ = 0;
    std::int64_t limitSamples_ = 0;
    std::atomic<bool> autoStop_ {false};
};

}
