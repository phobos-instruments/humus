// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <memory>

#include <juce_audio_devices/juce_audio_devices.h>

#include "gui/host/HostAudioIo.h"

namespace hum {

class DawDevice final : public juce::AudioIODevice {
public:
    DawDevice() : juce::AudioIODevice("DAW", "Plug-in") {}

    void configure(double rate, int block, int outputs) {
        rate_ = rate;
        block_ = block;
        outputs_ = juce::jlimit(1, 2, outputs);
    }

    juce::StringArray getOutputChannelNames() override { return {"Left", "Right"}; }
    juce::StringArray getInputChannelNames() override { return {"Left", "Right"}; }
    juce::Array<double> getAvailableSampleRates() override { return {rate_}; }
    juce::Array<int> getAvailableBufferSizes() override { return {block_}; }
    int getDefaultBufferSize() override { return block_; }
    juce::String open(const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start(juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return block_; }
    double getCurrentSampleRate() override { return rate_; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { return channels(outputs_); }
    juce::BigInteger getActiveInputChannels() const override { return channels(2); }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }

private:
    static juce::BigInteger channels(int n) {
        juce::BigInteger b;
        b.setRange(0, n, true);
        return b;
    }

    double rate_ = 48000.0;
    int block_ = 512;
    int outputs_ = 2;
};

class PluginAudioIo final : public HostAudioIo {
public:
    void prepare(double rate, int block, int outputs) {
        device_.configure(rate, block, outputs);
        if (auto* cb = callback_.load()) cb->audioDeviceAboutToStart(&device_);
    }

    void render(const float* const* in, int numIn, float* const* out, int numOut, int numSamples) {
        rendering_.store(true);
        if (auto* cb = callback_.load())
            cb->audioDeviceIOCallbackWithContext(in, numIn, out, numOut, numSamples, {});
        else
            for (int c = 0; c < numOut; ++c) juce::FloatVectorOperations::clear(out[c], numSamples);
        rendering_.store(false);
    }

    bool attached() const { return callback_.load() != nullptr; }

    juce::String open(int, int, const juce::XmlElement*) override { return {}; }
    void close() override {}
    void attach(juce::AudioIODeviceCallback& callback) override {
        callback.audioDeviceAboutToStart(&device_);
        callback_.store(&callback);
    }
    void detach(juce::AudioIODeviceCallback& callback) override {
        if (callback_.load() == &callback) callback_.store(nullptr);
        while (rendering_.load()) juce::Thread::yield();
        callback.audioDeviceStopped();
    }
    juce::AudioIODevice* current() override { return &device_; }
    std::unique_ptr<juce::XmlElement> state() override { return nullptr; }
    juce::AudioDeviceManager::AudioDeviceSetup setup() override {
        juce::AudioDeviceManager::AudioDeviceSetup s;
        s.sampleRate = device_.getCurrentSampleRate();
        s.bufferSize = device_.getCurrentBufferSizeSamples();
        return s;
    }
    void applySetup(const juce::AudioDeviceManager::AudioDeviceSetup&) override {}
    juce::AudioDeviceManager& manager() override { return idle_; }

private:
    DawDevice device_;
    std::atomic<juce::AudioIODeviceCallback*> callback_{nullptr};
    std::atomic<bool> rendering_{false};
    juce::AudioDeviceManager idle_;
};

}
