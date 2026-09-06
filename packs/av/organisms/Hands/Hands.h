#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "Hands/HandPose.h"
#include "hum/CameraCapture.h"
#include "hum/NativeCamera.h"
#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

bool handsDetectorAvailable();

bool handsSystemTracker();

bool handsBuiltInReady();

class Hands : public Organism, public MidiNode, public CamPreviewSource,
              public OscValueSource, public GestureFeatureSource,
              public ControlSource, public VideoNode, public VideoFrameSink {
public:
    static constexpr int kHands = 2;
    static constexpr int kPerHand = 9;
    static constexpr int kLive = kHands * kPerHand;
    static constexpr int kSignals = kLive + handpose::kGestureSlots;

    Hands();
    ~Hands() override;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
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
        return handsDetectorAvailable() ? std::string()
                                        : "No hand tracking on this system";
    }
    unsigned camGeneration() const override { return frameGen_.load(); }
    Frame camFrame() const override;
    Skeleton camSkeleton() const override;
    void camSignals(float& x, float& y, float& motion, float& brightness) const override {
        const int h = sig_[7].load() >= 0.5f ? 0 : (sig_[kPerHand + 7].load() >= 0.5f ? 1 : 0);
        const int b = h * kPerHand;
        x = sig_[(size_t) (b + 5)].load(); y = 1.0f - sig_[(size_t) (b + 6)].load();
        motion = sig_[(size_t) (b + 7)].load();
        brightness = (sig_[(size_t) b].load() + sig_[(size_t) (b + 1)].load()
                      + sig_[(size_t) (b + 2)].load() + sig_[(size_t) (b + 3)].load()
                      + sig_[(size_t) (b + 4)].load()) / 5.0f;
    }

    HandValues liveValues(int hand) const {
        HandValues v;
        const int b = std::clamp(hand, 0, kHands - 1) * kPerHand;
        for (int i = 0; i < 5; ++i) v.finger[(size_t) i] = sig_[(size_t) (b + i)].load();
        v.x = sig_[(size_t) (b + 5)].load();
        v.y = sig_[(size_t) (b + 6)].load();
        v.present = sig_[(size_t) (b + 7)].load();
        v.pinch = sig_[(size_t) (b + 8)].load();
        return v;
    }
    HandValues liveValues() const {
        const auto l = liveValues(0);
        return l.present >= 0.5f ? l : liveValues(1);
    }

    int gestureFeatureCount() const override { return 5; }
    bool gestureFeaturesLive(float* out) const override {
        const auto v = liveValues();
        for (int i = 0; i < 5; ++i) out[i] = v.finger[(size_t) i];
        return v.present >= 0.5f;
    }
    const char* gestureAbsentPrompt() const override { return "show a hand..."; }

    int controlValues(ControlVal* out, int capacity) const override {
        OscVal vals[kSignals];
        const int n = oscValues(vals, std::min(capacity, (int) kSignals));
        for (int i = 0; i < n; ++i) out[i] = {vals[i].suffix, vals[i].value};
        return n;
    }

    bool oscEnabled() const override { return params.get("SendOSC", 0.0) >= 0.5; }
    int oscValues(OscVal* out, int capacity) const override {
        static const char* kSuffix[kSignals] = {
            "l/thumb", "l/index", "l/middle", "l/ring", "l/pinky",
            "l/x", "l/y", "l/present", "l/pinch",
            "r/thumb", "r/index", "r/middle", "r/ring", "r/pinky",
            "r/x", "r/y", "r/present", "r/pinch",
            "gesture/1", "gesture/2", "gesture/3", "gesture/4"};
        const int n = std::min(capacity, (int) kSignals);
        for (int i = 0; i < n; ++i) out[i] = {kSuffix[i], oscOut_[(size_t) i].load()};
        return n;
    }

    void setTestSignals(int hand, const HandValues& v) {
        const int b = std::clamp(hand, 0, kHands - 1) * kPerHand;
        for (int i = 0; i < 5; ++i) sig_[(size_t) (b + i)].store(v.finger[(size_t) i]);
        sig_[(size_t) (b + 5)].store(v.x);
        sig_[(size_t) (b + 6)].store(v.y);
        sig_[(size_t) (b + 7)].store(v.present);
        sig_[(size_t) (b + 8)].store(v.pinch);
    }

private:
    class Lifecycle;
    friend class Lifecycle;
    void updateCamera(bool wantOpen, int camIndex);
#if JUCE_MAC
    void pixelFrameArrived(void* cvPixelBuffer);
#else
    void frameArrived(const juce::Image& img);
#endif

    class Worker;
    friend class Worker;
    std::unique_ptr<Worker> worker_;
    juce::CriticalSection jobLock_;
    juce::Image job_;
    Picture jobPicture_;
    void publish(const std::array<HandLandmarks, kHands>& lm);
    void offerFrame(const juce::Image& img);
    bool wantsBuiltIn() const;

    std::unique_ptr<Lifecycle> lifecycle_;
#if JUCE_MAC
    std::unique_ptr<NativeCamera> nativeCam_;
#else
    std::unique_ptr<CameraCapture> device_;
    class FrameListener;
    std::unique_ptr<FrameListener> listener_;
#endif

    std::atomic<bool> deviceOpen_{false};
    std::atomic<bool> corded_{false};
    std::atomic<bool> sourceHeld_{false};
    std::atomic<unsigned> frameGen_{0};
    int openCam_ = 0;
    std::atomic<bool> closing_{false};
    std::atomic<bool> inFrame_{false};
    int frameCount_ = 0;
    std::array<std::atomic<float>, kLive> sig_{};

    juce::CriticalSection previewLock_;
    Frame preview_;
    std::array<HandLandmarks, kHands> lastLm_;

    std::string cachedGestures_;
    handpose::GestureSet gestures_;

    std::array<float, kSignals> smoothed_{0, 0, 0, 0, 0, 0.5f, 0.5f, 0, 0,
                                          0, 0, 0, 0, 0, 0.5f, 0.5f, 0, 0,
                                          0, 0, 0, 0};
    std::array<std::atomic<float>, kSignals> oscOut_{};
    std::array<int, kSignals> lastCcSent_;
    std::array<bool, handpose::kGestureSlots> noteOn_{};
    std::array<int, handpose::kGestureSlots> noteNum_{};
};

#if JUCE_MAC
int detectHandLandmarks(void* cvPixelBuffer, bool mirror, float minConfidence,
                        HandLandmarks* out, int maxHands);

void handsPreviewFromPixelBuffer(void* cvPixelBuffer, bool mirror,
                                 CamPreviewSource::Frame& out);
#endif

int detectHandLandmarks(const juce::Image& frame, bool mirror, float minConfidence,
                        HandLandmarks* out, int maxHands);

}
