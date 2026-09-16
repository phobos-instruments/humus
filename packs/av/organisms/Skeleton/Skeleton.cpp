// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Skeleton/Skeleton.h"

#include "common/FrameSample.h"

#include <cmath>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum {

class Skeleton::FrameListener : public CameraCapture::Listener {
public:
    explicit FrameListener(Skeleton& o) : owner_(o) {}
    void cameraFrame(const juce::Image& image) override { owner_.frameArrived(image); }
private:
    Skeleton& owner_;
};

class Skeleton::Worker : public juce::Thread {
public:
    explicit Worker(Skeleton& o) : juce::Thread("skeleton-detect"), owner_(o) { startThread(); }
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
            if (picture.pixels != nullptr || picture.native != nullptr) {
                Frame preview;
                framesample::previewFrom(picture, mirror, preview);
                {
                    const juce::ScopedLock sl(owner_.previewLock_);
                    owner_.preview_ = std::move(preview);
                }
                owner_.frameGen_.fetch_add(1);
                if ((detectTick_++ & 1) != 0) continue;
                framesample::imageFrom(picture, scratch_);
                frame = scratch_;
            }
            if (!frame.isValid()) { wait(15); continue; }
            const float conf = (float) std::clamp(owner_.params.get("Confidence", 0.5),
                                                  0.0, 1.0);
            BodyLandmarks lm;
            detectBodyLandmarksPortable(frame, mirror, conf, lm, owner_.track_);
            owner_.publish(lm);
        }
    }

private:
    Skeleton& owner_;
    juce::Image scratch_;
    int detectTick_ = 0;
};

class Skeleton::Lifecycle : public juce::Timer {
public:
    explicit Lifecycle(Skeleton& o) : owner_(o) { startTimer(500); }
    ~Lifecycle() override { stopTimer(); }
    void timerCallback() override {
        owner_.updateCamera(owner_.params.get("Enabled", 0.0) >= 0.5,
                            (int) owner_.params.get("Camera", 1.0));
    }
private:
    Skeleton& owner_;
};

Skeleton::Skeleton() {
    sig_[bodypose::kSigHeadX].store(0.5f);
    sig_[bodypose::kSigHeadY].store(0.5f);
    sig_[bodypose::kSigHandLX].store(0.5f);
    sig_[bodypose::kSigHandLY].store(0.5f);
    sig_[bodypose::kSigHandRX].store(0.5f);
    sig_[bodypose::kSigHandRY].store(0.5f);
    sig_[bodypose::kSigLean].store(0.5f);
    lastCcSent_.fill(-1);
}

Skeleton::~Skeleton() {
    lifecycle_.reset();
    updateCamera(false, 0);
}

void Skeleton::syncGestures() {
    const std::string text = params.getText("Gestures");
    if (text == appliedGestures_) return;
    appliedGestures_ = text;
    gestures_ = gvec::decode(text.c_str(), bodypose::kFeatures);
}

void Skeleton::loadFrom(const OrganismState& state) {
    Organism::loadFrom(state);
    syncGestures();
}

void Skeleton::onTextChanged(const std::string& param, const std::string& text) {
    if (param != "Gestures") return;
    appliedGestures_ = text;
    pendingGestures_.publish(gvec::decode(text.c_str(), bodypose::kFeatures));
}

void Skeleton::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    syncGestures();
    if (!lifecycle_ && juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        lifecycle_ = std::make_unique<Lifecycle>(*this);
}

void Skeleton::updateCamera(bool wantOpen, int camIndex) {
    wantOpen = wantOpen && bodyBuiltInReady() && !corded_.load();
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

void Skeleton::offerFrame(const juce::Image& img) {
    if (worker_ == nullptr) worker_ = std::make_unique<Worker>(*this);
    const juce::ScopedLock sl(jobLock_);
    job_ = img;
}

void Skeleton::publish(const BodyLandmarks& lm) {
    const auto v = bodypose::values(lm);
    for (int i = 0; i < kLive; ++i)
        if (v.valid[(size_t) i]) sig_[(size_t) i].store(v.sig[(size_t) i]);
    if (!lm.present) sig_[bodypose::kSigPresent].store(0.0f);

    float f[bodypose::kFeatures];
    const bool live = bodypose::features(lm, f);
    for (int i = 0; i < bodypose::kFeatures; ++i) feat_[(size_t) i].store(f[i]);
    featLive_.store(live);

    const juce::ScopedLock sl(previewLock_);
    lastLm_ = lm;
}

void Skeleton::frameArrived(const juce::Image& image) {
    if (closing_.load()) return;
    struct InFlight {
        std::atomic<bool>& f;
        explicit InFlight(std::atomic<bool>& a) : f(a) { f.store(true); }
        ~InFlight() { f.store(false); }
    } guard(inFrame_);

    const bool mirror = params.get("Mirror", 0.0) >= 0.5;
    if ((frameCount_++ & 1) == 0) offerFrame(image);

    Frame f;
    framesample::previewFromImage(image, mirror, f);
    {
        const juce::ScopedLock sl(previewLock_);
        preview_ = std::move(f);
    }
    frameGen_.fetch_add(1);
}

void Skeleton::pushVideoFrame(const Picture& picture) {
    if (picture.width <= 0 || picture.height <= 0 || closing_.load()
        || (picture.pixels == nullptr && picture.native == nullptr))
        return;
    bodyBuiltInReady();
    if (worker_ == nullptr) worker_ = std::make_unique<Worker>(*this);
    const juce::ScopedLock sl(jobLock_);
    jobPicture_ = picture;
}

void Skeleton::injectPreviewFrame(const juce::Image& img) {
    if (!img.isValid()) return;
    bodyBuiltInReady();
    const bool mirror = params.get("Mirror", 0.0) >= 0.5;
    BodyLandmarks lm;
    detectBodyLandmarksPortable(
        img, mirror, (float) std::clamp(params.get("Confidence", 0.5), 0.0, 1.0), lm,
        track_);
    publish(lm);
    Frame f;
    framesample::previewFromImage(img, mirror, f);
    {
        const juce::ScopedLock sl(previewLock_);
        preview_ = std::move(f);
    }
    deviceOpen_.store(true);
    frameGen_.fetch_add(1);
}

CamPreviewSource::Skeleton Skeleton::camSkeleton() const {
    CamPreviewSource::Skeleton sk;
    sk.supported = true;
    sk.groupSize = 33;
    sk.bones = bodypose::bones(sk.boneCount);
    const juce::ScopedLock sl(previewLock_);
    if (lastLm_.present) {
        for (int i = 0; i < 33; ++i) sk.pt[(size_t) i] = lastLm_.pt[(size_t) i];
        sk.points = 33;
    }
    return sk;
}

CamPreviewSource::Frame Skeleton::camFrame() const {
    const juce::ScopedLock sl(previewLock_);
    return preview_;
}

void Skeleton::process(const float* const*, int, float* const* out, int numOut,
                   int numSamples, const Transport&) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double ms = std::max(1.0, params.get("Smooth", 80.0));
    const float coef =
        (float) std::exp(-1.0 / (ms * 0.001 * sr / (double) std::max(1, numSamples)));

    pendingGestures_.adopt(gestures_);
    const float tol = (float) std::clamp(params.get("Tolerance", 0.35), 0.05, 1.0);

    float target[kSignals];
    for (int i = 0; i < kLive; ++i) target[i] = sig_[(size_t) i].load();
    for (int k = 0; k < gvec::kSlots; ++k) target[kLive + k] = 0.0f;
    if (featLive_.load() && sig_[bodypose::kSigPresent].load() >= 0.5f) {
        float cur[bodypose::kFeatures];
        for (int i = 0; i < bodypose::kFeatures; ++i) cur[i] = feat_[(size_t) i].load();
        float m[gvec::kSlots];
        gvec::matchAll(gestures_, cur, tol, m);
        for (int k = 0; k < gvec::kSlots; ++k) target[kLive + k] = m[k];
    }
    for (int i = 0; i < kSignals; ++i) {
        smoothed_[(size_t) i] = target[i] + coef * (smoothed_[(size_t) i] - target[i]);
        oscOut_[(size_t) i].store(smoothed_[(size_t) i], std::memory_order_relaxed);
    }

    juce::ignoreUnused(out, numOut, numSamples);
}

int Skeleton::collectMidi(int, MidiEvent* out, int capacity) {
    const bool sendCC = params.get("SendCC", 1.0) >= 0.5;
    const bool sendNotes = params.get("SendNotes", 1.0) >= 0.5;
    const int base = std::clamp((int) params.get("MidiCC", 70.0), 0, 119);
    const int ch = std::clamp((int) params.get("MidiChannel", 1.0), 1, 16);
    int n = 0;
    for (int i = 0; sendCC && i < kLive && n < capacity; ++i) {
        const int v7 = std::clamp((int) std::lround(smoothed_[(size_t) i] * kMidiMaxF), 0, kMidiMax);
        if (v7 == lastCcSent_[(size_t) i]) continue;
        lastCcSent_[(size_t) i] = v7;
        MidiEvent e;
        e.sampleOffset = 0;
        e.data[0] = (unsigned char) (0xB0 | (ch - 1));
        e.data[1] = (unsigned char) std::min(119, base + i);
        e.data[2] = (unsigned char) v7;
        e.size = 3;
        out[n++] = e;
    }
    for (int k = 0; k < gvec::kSlots && n < capacity; ++k) {
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
                std::clamp((int) params.get("GNote_" + sfx, 48.0 + k), 0, kMidiMax);
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
