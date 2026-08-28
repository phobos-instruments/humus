#include "CameraIn/CameraIn.h"

#include <algorithm>

namespace hum {

class CameraIn::FrameListener : public CameraCapture::Listener {
public:
    explicit FrameListener(CameraIn& o) : owner_(o) {}
    void cameraFrame(const juce::Image& image) override { owner_.frameArrived(image); }
private:
    CameraIn& owner_;
};

class CameraIn::Lifecycle : public juce::Timer {
public:
    explicit Lifecycle(CameraIn& o) : owner_(o) { startTimer(500); }
    ~Lifecycle() override { stopTimer(); }
    void timerCallback() override {
        owner_.updateCamera(owner_.params.get("Enabled", 0.0) >= 0.5,
                            (int) owner_.params.get("Camera", 1.0));
    }
private:
    CameraIn& owner_;
};

CameraIn::CameraIn() = default;

CameraIn::~CameraIn() {
    lifecycle_.reset();
    updateCamera(false, 0);
}

void CameraIn::prepare(double, int) {
    if (!lifecycle_ && juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        lifecycle_ = std::make_unique<Lifecycle>(*this);
}

void CameraIn::updateCamera(bool wantOpen, int camIndex) {
    const bool isOpen = device_ != nullptr;
    if (isOpen == wantOpen && (!isOpen || camIndex == openCam_)) return;
    if (isOpen) {
        if (device_ && listener_) device_->removeListener(listener_.get());
        device_.reset();
        listener_.reset();
        deviceOpen_.store(false);
    }
    if (!wantOpen) return;
    const int count = (int) CameraCapture::availableDevices().size();
    const int idx = juce::jlimit(0, std::max(0, count - 1), camIndex - 1);
    device_ = CameraCapture::open(idx, 640, 480, 1280, 720);
    if (!device_) return;
    listener_ = std::make_unique<FrameListener>(*this);
    device_->addListener(listener_.get());
    deviceOpen_.store(true);
    openCam_ = camIndex;
}

void CameraIn::frameArrived(const juce::Image& image) {
    const int w = image.getWidth(), h = image.getHeight();
    if (w <= 0 || h <= 0) return;
    const bool mirror = params.get("Mirror", 1.0) >= 0.5;

    Frame f;
    f.width = w; f.height = h;
    f.rgba.resize((size_t) w * (size_t) h * 4);
    juce::Image::BitmapData bd(image, juce::Image::BitmapData::readOnly);
    for (int y = 0; y < h; ++y) {
        auto* out = f.rgba.data() + (size_t) y * (size_t) w * 4;
        for (int x = 0; x < w; ++x, out += 4) {
            const auto* p = bd.getPixelPointer(mirror ? w - 1 - x : x, y);
            if (bd.pixelFormat == juce::Image::ARGB) {
                const auto* px = reinterpret_cast<const juce::PixelARGB*>(p);
                out[0] = px->getRed(); out[1] = px->getGreen(); out[2] = px->getBlue();
            } else if (bd.pixelFormat == juce::Image::RGB) {
                const auto* px = reinterpret_cast<const juce::PixelRGB*>(p);
                out[0] = px->getRed(); out[1] = px->getGreen(); out[2] = px->getBlue();
            } else {
                const auto c = bd.getPixelColour(mirror ? w - 1 - x : x, y);
                out[0] = c.getRed(); out[1] = c.getGreen(); out[2] = c.getBlue();
            }
            out[3] = 255;
        }
    }
    {
        const juce::ScopedLock sl(frameLock_);
        frame_ = std::move(f);
    }
    frameGen_.fetch_add(1);
}

void CameraIn::injectPreviewFrame(const juce::Image& img) {
    deviceOpen_.store(true);
    frameArrived(img);
}

CamPreviewSource::Frame CameraIn::camFrame() const {
    const juce::ScopedLock sl(frameLock_);
    return frame_;
}

void CameraIn::process(const float* const*, int, float* const*, int, int,
                       const Transport&) {}

}
