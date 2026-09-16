// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "Skeleton/BodyPortable.h"
#include "Skeleton/BodyPose.h"
#include "common/GestureVec.h"
#include "hum/CameraCapture.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Osc.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"
#include "hum/dsp/Prepared.h"

namespace hum {

class Skeleton : public Organism, public MidiNode, public CamPreviewSource,
                 public OscValueSource, public GestureFeatureSource,
                 public ControlSource, public VideoNode, public VideoFrameSink {
public:
    static constexpr int kLive = bodypose::kPerBody;
    static constexpr int kSignals = kLive + gvec::kSlots;

    Skeleton();
    ~Skeleton() override;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void loadFrom(const OrganismState& state) override;
    void onTextChanged(const std::string& param, const std::string& text) override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int port, MidiEvent* out, int capacity) override;

    void injectPreviewFrame(const juce::Image& img);

    int numVideoInputs() const override { return 1; }
    int numVideoOutputs() const override { return 0; }
    void pushVideoFrame(const Picture& picture) override;
    void setVideoCordAttached(bool on) override { corded_.store(on); }
    void setVideoSourceHeld(bool held) override { sourceHeld_.store(held); }
    bool camSourceHeld() const override { return sourceHeld_.load(); }

    bool camActive() const override { return deviceOpen_.load() || corded_.load(); }
    std::string camUnavailable() const override {
        return bodyBuiltInReady() ? std::string() : "No body model on this system";
    }
    unsigned camGeneration() const override { return frameGen_.load(); }
    Frame camFrame() const override;
    CamPreviewSource::Skeleton camSkeleton() const override;
    void camSignals(float& x, float& y, float& motion, float& brightness) const override {
        x = sig_[bodypose::kSigHeadX].load();
        y = 1.0f - sig_[bodypose::kSigHeadY].load();
        motion = std::max(sig_[bodypose::kSigRaiseL].load(),
                          sig_[bodypose::kSigRaiseR].load());
        brightness = sig_[bodypose::kSigPresent].load();
    }

    int gestureFeatureCount() const override { return bodypose::kFeatures; }
    bool gestureFeaturesLive(float* out) const override {
        for (int i = 0; i < bodypose::kFeatures; ++i) out[i] = feat_[(size_t) i].load();
        return featLive_.load();
    }

    int controlValues(ControlVal* out, int capacity) const override {
        OscVal vals[kSignals];
        const int n = oscValues(vals, std::min(capacity, (int) kSignals));
        for (int i = 0; i < n; ++i) out[i] = {vals[i].suffix, vals[i].value};
        return n;
    }

    bool oscEnabled() const override { return params.get("SendOSC", 0.0) >= 0.5; }
    int oscValues(OscVal* out, int capacity) const override {
        static const char* kSuffix[kSignals] = {
            "head/x", "head/y", "hand/l/x", "hand/l/y", "hand/r/x", "hand/r/y",
            "raise/l", "raise/r", "lean", "crouch", "stance", "present",
            "gesture/1", "gesture/2", "gesture/3", "gesture/4"};
        const int n = std::min(capacity, (int) kSignals);
        for (int i = 0; i < n; ++i) out[i] = {kSuffix[i], oscOut_[(size_t) i].load()};
        return n;
    }

    void setTestSignals(const bodypose::BodyValues& v) {
        for (int i = 0; i < kLive; ++i)
            if (v.valid[(size_t) i]) sig_[(size_t) i].store(v.sig[(size_t) i]);
    }
    void setTestFeatures(const float* f, bool live) {
        for (int i = 0; i < bodypose::kFeatures; ++i) feat_[(size_t) i].store(f[i]);
        featLive_.store(live);
    }

private:
    class Lifecycle;
    friend class Lifecycle;
    class Worker;
    friend class Worker;
    class FrameListener;

    void updateCamera(bool wantOpen, int camIndex);
    void frameArrived(const juce::Image& img);
    void offerFrame(const juce::Image& img);
    void publish(const BodyLandmarks& lm);

    std::unique_ptr<Worker> worker_;
    juce::CriticalSection jobLock_;
    juce::Image job_;
    Picture jobPicture_;

    std::unique_ptr<Lifecycle> lifecycle_;
    std::unique_ptr<CameraCapture> device_;
    std::unique_ptr<FrameListener> listener_;

    std::atomic<bool> deviceOpen_{false};
    std::atomic<bool> corded_{false};
    std::atomic<bool> sourceHeld_{false};
    std::atomic<unsigned> frameGen_{0};
    int openCam_ = 0;
    std::atomic<bool> closing_{false};
    std::atomic<bool> inFrame_{false};
    int frameCount_ = 0;

    std::array<std::atomic<float>, kLive> sig_{};
    std::array<std::atomic<float>, bodypose::kFeatures> feat_{};
    std::atomic<bool> featLive_{false};

    juce::CriticalSection previewLock_;
    Frame preview_;
    BodyLandmarks lastLm_;
    BodyTrack track_;

    void syncGestures();
    std::string appliedGestures_;
    gvec::Set gestures_;
    Prepared<gvec::Set> pendingGestures_;

    std::array<float, kSignals> smoothed_{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                                          0, 0, 0.5f, 0, 0, 0,
                                          0, 0, 0, 0};
    std::array<std::atomic<float>, kSignals> oscOut_{};
    std::array<int, kSignals> lastCcSent_;
    std::array<bool, gvec::kSlots> noteOn_{};
    std::array<int, gvec::kSlots> noteNum_{};
};

}
