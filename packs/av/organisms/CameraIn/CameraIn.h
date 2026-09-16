// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <memory>
#include <string>

#include "hum/CameraCapture.h"
#include "hum/NativeCamera.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"

namespace hum {

class CameraIn : public Organism, public VideoNode, public CamPreviewSource {
public:
    CameraIn();
    ~CameraIn() override;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int numVideoInputs() const override { return 0; }
    int numVideoOutputs() const override { return 1; }

    bool camActive() const override { return deviceOpen_.load(); }
    unsigned camGeneration() const override { return frameGen_.load(); }
    Frame camFrame() const override;
    void camSignals(float& x, float& y, float& motion, float& brightness) const override {
        x = 0.5f; y = 0.5f; motion = 0.0f; brightness = 0.0f;
    }

    void injectPreviewFrame(const juce::Image& img);

    NativePicture camNativePicture() const override;

private:
    class Lifecycle;
    friend class Lifecycle;
    void updateCamera(bool wantOpen, int camIndex);
    void frameArrived(const juce::Image& img);

    void nativeFrameArrived(const NativeCamera::FrameRef& f);

    std::unique_ptr<Lifecycle> lifecycle_;
    std::unique_ptr<NativeCamera> native_;
    std::unique_ptr<CameraCapture> device_;
    class FrameListener;
    std::unique_ptr<FrameListener> listener_;

    int openCam_ = 0;
    std::atomic<bool> deviceOpen_{false};
    std::atomic<unsigned> frameGen_{0};

    mutable juce::CriticalSection frameLock_;
    Frame frame_;
    NativeCamera::FrameRef nativeFrame_;
    mutable Frame nativeRgba_;
    mutable unsigned nativeShownGen_ = ~0u;
};

}
