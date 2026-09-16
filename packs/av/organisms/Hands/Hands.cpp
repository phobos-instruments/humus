// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Hands/Hands.h"

#include "Hands/HandsPortable.h"

#include "common/FrameSample.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "hum/dsp/DspMath.h"

namespace hum {

#if !JUCE_MAC
int detectHandLandmarks(const juce::Image& img, bool mirror, float conf,
                        HandLandmarks* out, int maxHands) {
    return detectHandLandmarksPortable(img, mirror, conf, out, maxHands);
}
bool handsDetectorAvailable() { return handsBuiltInReady(); }
bool handsSystemTracker() { return false; }

class Hands::FrameListener : public CameraCapture::Listener {
public:
    explicit FrameListener(Hands& o) : owner_(o) {}
    void cameraFrame(const juce::Image& image) override { owner_.frameArrived(image); }
private:
    Hands& owner_;
};
#endif

class Hands::Worker : public juce::Thread {
public:
    explicit Worker(Hands& o) : juce::Thread("hands-detect"), owner_(o) { startThread(); }
    ~Worker() override { stopThread(2000); }

    void run() override {
        while (!threadShouldExit()) {
            juce::Image frame;
            Picture picture;
            {
                const juce::ScopedLock sl(owner_.jobLock_);
                frame = owner_.job_;
                picture = owner_.jobPicture_;
                owner_.job_ = juce::Image();
                owner_.jobPicture_ = {};
            }
            const bool mirror =
                (owner_.params.get("Mirror", 0.0) >= 0.5) != picture.mirrored;
            const bool corded = picture.pixels != nullptr || picture.native != nullptr;
            if (corded) {
                Frame preview;
                framesample::previewFrom(picture, mirror, preview);
                {
                    const juce::ScopedLock sl(owner_.previewLock_);
                    owner_.preview_ = std::move(preview);
                }
                owner_.frameGen_.fetch_add(1);
                if ((detectTick_++ & 1) != 0) continue;
            }
            const float conf = (float) std::clamp(owner_.params.get("Confidence", 0.3),
                                                  0.0, 1.0);
            const bool two = owner_.params.get("TwoHands", 1.0) >= 0.5;
            std::array<HandLandmarks, kHands> lm{};
#if JUCE_MAC
            if (corded && picture.native != nullptr && !owner_.wantsBuiltIn()) {
                detectHandLandmarks(picture.native, mirror, conf, lm.data(),
                                    two ? kHands : 1);
                owner_.publish(lm);
                continue;
            }
#endif
            if (corded) {
                framesample::imageFrom(picture, scratch_);
                frame = scratch_;
            }
            if (!frame.isValid()) { wait(15); continue; }
            if (owner_.wantsBuiltIn())
                detectHandLandmarksPortable(frame, mirror, conf, lm.data(),
                                            two ? kHands : 1);
            else
                detectHandLandmarks(frame, mirror, conf, lm.data(), two ? kHands : 1);
            owner_.publish(lm);
        }
    }

private:
    Hands& owner_;
    juce::Image scratch_;
    int detectTick_ = 0;
};

class Hands::Lifecycle : public juce::Timer {
public:
    explicit Lifecycle(Hands& o) : owner_(o) { startTimer(500); }
    ~Lifecycle() override { stopTimer(); }
    void timerCallback() override {
        owner_.updateCamera(owner_.params.get("Enabled", 0.0) >= 0.5,
                            (int) owner_.params.get("Camera", 1.0));
    }
private:
    Hands& owner_;
};

Hands::Hands() {
    sig_[5].store(0.5f);
    sig_[6].store(0.5f);
    lastCcSent_.fill(-1);
}

Hands::~Hands() {
    lifecycle_.reset();
    updateCamera(false, 0);
}

void Hands::syncGestures() {
    const std::string text = params.getText("Gestures");
    if (text == appliedGestures_) return;
    appliedGestures_ = text;
    gestures_ = handpose::decodeGestures(text.c_str());
}

void Hands::loadFrom(const OrganismState& state) {
    Organism::loadFrom(state);
    syncGestures();
}

void Hands::onTextChanged(const std::string& param, const std::string& text) {
    if (param != "Gestures") return;
    appliedGestures_ = text;
    pendingGestures_.publish(handpose::decodeGestures(text.c_str()));
}

void Hands::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    syncGestures();
    if (!lifecycle_ && juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        lifecycle_ = std::make_unique<Lifecycle>(*this);
}

#if JUCE_MAC
void Hands::updateCamera(bool wantOpen, int camIndex) {
    wantOpen = wantOpen && handsDetectorAvailable() && !corded_.load();
    const bool isOpen = nativeCam_ != nullptr;
    if (isOpen == wantOpen && (!isOpen || camIndex == openCam_)) return;
    if (isOpen) {
        closing_.store(true);
        nativeCam_.reset();
        deviceOpen_.store(false);
        closing_.store(false);
    }
    if (!wantOpen) return;
    std::string devName;
    const auto devs = juce::CameraDevice::getAvailableDevices();
    if (camIndex >= 1 && camIndex <= devs.size())
        devName = devs[camIndex - 1].toStdString();
    nativeCam_ = NativeCamera::open(
        devName, [this](const NativeCamera::FrameRef& f) { pixelFrameArrived(f.buffer); });
    deviceOpen_.store(nativeCam_ != nullptr);
    openCam_ = camIndex;
}
#else
void Hands::updateCamera(bool wantOpen, int camIndex) {
    wantOpen = wantOpen && handsDetectorAvailable() && !corded_.load();
    const bool isOpen = device_ != nullptr;
    if (isOpen == wantOpen && (!isOpen || camIndex == openCam_)) return;
    if (isOpen) {
        closing_.store(true);
        if (device_ && listener_) device_->removeListener(listener_.get());
        for (int i = 0; i < 400 && inFrame_.load(); ++i)
            juce::Thread::sleep(1);
        device_.reset();
        listener_.reset();
        deviceOpen_.store(false);
        closing_.store(false);
    }
    if (!wantOpen) return;
    const int count = (int) CameraCapture::availableDevices().size();
    const int idx = juce::jlimit(0, std::max(0, count - 1), camIndex - 1);
    device_ = CameraCapture::open(idx, 320, 240, 1280, 720);
    if (!device_) return;
    listener_ = std::make_unique<FrameListener>(*this);
    device_->addListener(listener_.get());
    deviceOpen_.store(true);
    openCam_ = camIndex;
}
#endif

bool Hands::wantsBuiltIn() const {
    return params.get("BuiltIn", 0.0) >= 0.5 && handsBuiltInReady();
}

void Hands::offerFrame(const juce::Image& img) {
    if (worker_ == nullptr) worker_ = std::make_unique<Worker>(*this);
    const juce::ScopedLock sl(jobLock_);
    job_ = img;
}

void Hands::publish(const std::array<HandLandmarks, kHands>& lm) {
    for (int h = 0; h < kHands; ++h) {
        const int b = h * kPerHand;
        const HandValues v = handpose::values(lm[(size_t) h]);
        if (lm[(size_t) h].present) {
            for (int i = 0; i < 5; ++i)
                sig_[(size_t) (b + i)].store(v.finger[(size_t) i]);
            sig_[(size_t) (b + 5)].store(v.x);
            sig_[(size_t) (b + 6)].store(v.y);
            sig_[(size_t) (b + 8)].store(v.pinch);
        }
        sig_[(size_t) (b + 7)].store(v.present);
    }
    const juce::ScopedLock sl(previewLock_);
    lastLm_ = lm;
}

#if JUCE_MAC
void Hands::pixelFrameArrived(void* pbRaw) {
    if (closing_.load()) return;
    struct InFlight {
        std::atomic<bool>& f;
        explicit InFlight(std::atomic<bool>& a) : f(a) { f.store(true); }
        ~InFlight() { f.store(false); }
    } guard(inFrame_);

    const bool mirror = params.get("Mirror", 0.0) >= 0.5;
    const float minConf = (float) std::clamp(params.get("Confidence", 0.3), 0.0, 1.0);

    if ((frameCount_++ & 1) == 0) {
        const bool two = params.get("TwoHands", 1.0) >= 0.5;
        if (wantsBuiltIn()) {
            Frame raw;
            handsPreviewFromPixelBuffer(pbRaw, false, raw);
            if (raw.width > 0)
                offerFrame(framesample::imageFromRgba(raw.width, raw.height,
                                                      raw.rgba.data()));
        } else {
            std::array<HandLandmarks, kHands> lm{};
            detectHandLandmarks(pbRaw, mirror, minConf, lm.data(), two ? kHands : 1);
            publish(lm);
        }
    }

    Frame f;
    handsPreviewFromPixelBuffer(pbRaw, mirror, f);
    if (f.width > 0) {
        const juce::ScopedLock sl(previewLock_);
        preview_ = std::move(f);
    }
    frameGen_.fetch_add(1);
}
#else
void Hands::frameArrived(const juce::Image& image) {
    if (closing_.load()) return;
    struct InFlight {
        std::atomic<bool>& f;
        explicit InFlight(std::atomic<bool>& a) : f(a) { f.store(true); }
        ~InFlight() { f.store(false); }
    } guard(inFrame_);

    const bool mirror = params.get("Mirror", 0.0) >= 0.5;
    const float minConf = (float) std::clamp(params.get("Confidence", 0.3), 0.0, 1.0);

    juce::ignoreUnused(mirror, minConf);
    if ((frameCount_++ & 1) == 0) offerFrame(image);

    Frame f;
    framesample::previewFromImage(image, mirror, f);
    {
        const juce::ScopedLock sl(previewLock_);
        preview_ = std::move(f);
    }
    frameGen_.fetch_add(1);
}
#endif

void Hands::pushVideoFrame(const Picture& picture) {
    if (picture.width <= 0 || picture.height <= 0 || closing_.load()
        || (picture.pixels == nullptr && picture.native == nullptr))
        return;
    handsDetectorAvailable();
    if (worker_ == nullptr) worker_ = std::make_unique<Worker>(*this);
    const juce::ScopedLock sl(jobLock_);
    jobPicture_ = picture;
}

void Hands::injectPreviewFrame(const juce::Image& img) {
    if (!img.isValid()) return;
    handsDetectorAvailable();
    const bool mirror = params.get("Mirror", 0.0) >= 0.5;
    std::array<HandLandmarks, kHands> lm{};
    detectHandLandmarks(img, mirror,
                        (float) std::clamp(params.get("Confidence", 0.3), 0.0, 1.0),
                        lm.data(), kHands);
    Frame f;
    framesample::previewFromImage(img, mirror, f);
    {
        const juce::ScopedLock sl(previewLock_);
        preview_ = std::move(f);
        lastLm_ = lm;
    }
    deviceOpen_.store(true);
    frameGen_.fetch_add(1);
}

CamPreviewSource::Skeleton Hands::camSkeleton() const {
    static const int kBones[][2] = {
        {0, 1},  {1, 2},   {2, 3},   {3, 4},
        {0, 5},  {5, 6},   {6, 7},   {7, 8},
        {5, 9},  {9, 10},  {10, 11}, {11, 12},
        {9, 13}, {13, 14}, {14, 15}, {15, 16},
        {13, 17}, {17, 18}, {18, 19}, {19, 20},
        {0, 17},
    };
    Skeleton sk;
    sk.supported = true;
    sk.groupSize = 21;
    sk.bones = kBones;
    sk.boneCount = (int) (sizeof(kBones) / sizeof(kBones[0]));
    const juce::ScopedLock sl(previewLock_);
    for (const auto& lm : lastLm_) {
        if (!lm.present) continue;
        for (int i = 0; i < 21; ++i)
            sk.pt[(size_t) (sk.points + i)] = lm.pt[(size_t) i];
        sk.points += 21;
    }
    return sk;
}

CamPreviewSource::Frame Hands::camFrame() const {
    const juce::ScopedLock sl(previewLock_);
    return preview_;
}

void Hands::process(const float* const*, int, float* const* out, int numOut,
                    int numSamples, const Transport&) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double ms = std::max(1.0, params.get("Smooth", 80.0));
    const float coef =
        (float) std::exp(-1.0 / (ms * 0.001 * sr / (double) std::max(1, numSamples)));

    pendingGestures_.adopt(gestures_);
    const float tol = (float) std::clamp(params.get("Tolerance", 0.35), 0.05, 1.0);

    float target[kSignals];
    for (int i = 0; i < kLive; ++i) target[i] = sig_[(size_t) i].load();
    for (int k = 0; k < handpose::kGestureSlots; ++k) target[kLive + k] = 0.0f;
    for (int h = 0; h < kHands; ++h) {
        const int b = h * kPerHand;
        if (sig_[(size_t) (b + 7)].load() < 0.5f) continue;
        std::array<float, 5> cur;
        for (int i = 0; i < 5; ++i) cur[(size_t) i] = sig_[(size_t) (b + i)].load();
        float m[handpose::kGestureSlots];
        handpose::matchAll(gestures_, cur, tol, m);
        for (int k = 0; k < handpose::kGestureSlots; ++k)
            target[kLive + k] = std::max(target[kLive + k], m[k]);
    }
    for (int i = 0; i < kSignals; ++i) {
        smoothed_[(size_t) i] = target[i] + coef * (smoothed_[(size_t) i] - target[i]);
        oscOut_[(size_t) i].store(smoothed_[(size_t) i], std::memory_order_relaxed);
    }

    juce::ignoreUnused(out, numOut, numSamples);
}

int Hands::collectMidi(int, MidiEvent* out, int capacity) {
    auto sendFlag = [&](const char* nm) {
        if (const auto* pp = params.byName(nm)) return pp->value >= 0.5;
        return params.get("SendMidi", 1.0) >= 0.5;
    };
    const bool sendCC = sendFlag("SendCC");
    const bool sendNotes = sendFlag("SendNotes");
    const int base = std::clamp((int) params.get("MidiCC", 30.0), 0, 119);
    const int ch = std::clamp((int) params.get("MidiChannel", 1.0), 1, 16);
    int n = 0;
    for (int i = 0; sendCC && i < kLive && n < capacity; ++i) {
        const int v7 = std::clamp((int) std::lround(smoothed_[(size_t) i] * kMidiMaxF), 0, kMidiMax);
        if (v7 == lastCcSent_[(size_t) i]) continue;
        lastCcSent_[(size_t) i] = v7;
        MidiEvent e;
        e.sampleOffset = 0;
        e.data[0] = (unsigned char) (0xB0 | (ch - 1));
        e.data[1] = (unsigned char) (base + i);
        e.data[2] = (unsigned char) v7;
        e.size = 3;
        out[n++] = e;
    }
    for (int k = 0; k < handpose::kGestureSlots && n < capacity; ++k) {
        const std::string sfx = std::to_string(k + 1);
        const float thr =
            (float) std::clamp(params.get("GThresh_" + sfx, 0.6), 0.05, 1.0);
        const float m = smoothed_[(size_t) (kLive + k)];
        const bool want = sendNotes
                          && (noteOn_[(size_t) k] ? m > std::max(0.02f, thr - 0.07f)
                                                  : m > thr);
        if (want == noteOn_[(size_t) k]) continue;
        noteOn_[(size_t) k] = want;
        if (want)
            noteNum_[(size_t) k] =
                std::clamp((int) params.get("GNote_" + sfx, 60.0 + k), 0, kMidiMax);
        MidiEvent e;
        e.sampleOffset = 0;
        e.data[0] = (unsigned char) ((want ? 0x90 : 0x80) | (ch - 1));
        e.data[1] = (unsigned char) noteNum_[(size_t) k];
        e.data[2] = (unsigned char) (want ? std::clamp((int) std::lround(m * kMidiMaxF), 1, kMidiMax) : 0);
        e.size = 3;
        out[n++] = e;
    }
    return n;
}

}
