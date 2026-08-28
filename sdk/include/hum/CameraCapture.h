#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_video/juce_video.h>

namespace hum {

class CameraCapture {
public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void cameraFrame(const juce::Image& image) = 0;
    };

    virtual ~CameraCapture() = default;

    static std::vector<std::string> availableDevices();

    static std::unique_ptr<CameraCapture> open(int index, int minWidth, int minHeight,
                                               int maxWidth, int maxHeight);

    virtual void addListener(Listener*) = 0;
    virtual void removeListener(Listener*) = 0;

protected:
    CameraCapture() = default;
};

inline void yuyvRowToRgb(const unsigned char* src, unsigned char* dst, int width) {
    auto clamp8 = [](int v) -> unsigned char {
        return (unsigned char) (v < 0 ? 0 : v > 255 ? 255 : v);
    };
    for (int x = 0; x + 1 < width; x += 2) {
        const int y0 = src[0], u = src[1], y1 = src[2], v = src[3];
        const int d = u - 128, e = v - 128;
        for (int i = 0; i < 2; ++i) {
            const int c = 298 * ((i == 0 ? y0 : y1) - 16);
            dst[0] = clamp8((c + 409 * e + 128) >> 8);
            dst[1] = clamp8((c - 100 * d - 208 * e + 128) >> 8);
            dst[2] = clamp8((c + 516 * d + 128) >> 8);
            dst += 3;
        }
        src += 4;
    }
}

}

#if JUCE_USE_CAMERA

namespace hum {
namespace camera_detail {

class JuceCamera final : public CameraCapture, private juce::CameraDevice::Listener {
public:
    explicit JuceCamera(std::unique_ptr<juce::CameraDevice> d) : dev_(std::move(d)) {
        dev_->addListener(this);
    }
    ~JuceCamera() override { dev_->removeListener(this); }

    void addListener(CameraCapture::Listener* l) override { listeners_.add(l); }
    void removeListener(CameraCapture::Listener* l) override { listeners_.remove(l); }

private:
    void imageReceived(const juce::Image& img) override {
        listeners_.call([&](CameraCapture::Listener& l) { l.cameraFrame(img); });
    }
    std::unique_ptr<juce::CameraDevice> dev_;
    juce::ListenerList<CameraCapture::Listener> listeners_;
};

}

inline std::vector<std::string> CameraCapture::availableDevices() {
    std::vector<std::string> out;
    for (const auto& n : juce::CameraDevice::getAvailableDevices())
        out.push_back(n.toStdString());
    return out;
}

inline std::unique_ptr<CameraCapture> CameraCapture::open(int index, int minWidth,
                                                          int minHeight, int maxWidth,
                                                          int maxHeight) {
    std::unique_ptr<juce::CameraDevice> d(
        juce::CameraDevice::openDevice(index, minWidth, minHeight, maxWidth, maxHeight));
    if (d == nullptr) return nullptr;
    return std::make_unique<camera_detail::JuceCamera>(std::move(d));
}

}

#elif defined(__linux__) && !defined(__ANDROID__)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include <cerrno>
#include <cstring>

namespace hum {
namespace camera_detail {

inline int xioctl(int fd, unsigned long request, void* arg) {
    int r;
    do { r = ::ioctl(fd, request, arg); } while (r == -1 && errno == EINTR);
    return r;
}

struct V4l2Info { std::string path, name; };

inline std::vector<V4l2Info> v4l2Devices() {
    std::vector<V4l2Info> out;
    for (int i = 0; i < 64; ++i) {
        const std::string path = "/dev/video" + std::to_string(i);
        const int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK);
        if (fd < 0) continue;
        v4l2_capability cap{};
        if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
            const auto caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps
                                                                        : cap.capabilities;
            if ((caps & V4L2_CAP_VIDEO_CAPTURE) != 0 && (caps & V4L2_CAP_STREAMING) != 0)
                out.push_back({path, std::string((const char*) cap.card)});
        }
        ::close(fd);
    }
    return out;
}

class V4l2Camera final : public CameraCapture, private juce::Thread {
public:
    static std::unique_ptr<V4l2Camera> tryOpen(const std::string& path, int maxW, int maxH) {
        std::unique_ptr<V4l2Camera> cam(new V4l2Camera());
        if (!cam->openDevice(path, maxW, maxH)) return nullptr;
        cam->startThread();
        return cam;
    }

    ~V4l2Camera() override {
        stopThread(2000);
        shutdown();
    }

    void addListener(CameraCapture::Listener* l) override { listeners_.add(l); }
    void removeListener(CameraCapture::Listener* l) override { listeners_.remove(l); }

private:
    V4l2Camera() : juce::Thread("V4L2 camera") {}

    bool openDevice(const std::string& path, int maxW, int maxH) {
        fd_ = ::open(path.c_str(), O_RDWR | O_NONBLOCK);
        if (fd_ < 0) return false;

        v4l2_format fmt{};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = (unsigned) maxW;
        fmt.fmt.pix.height = (unsigned) maxH;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        if (xioctl(fd_, VIDIOC_S_FMT, &fmt) != 0
            || fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
            fmt.fmt.pix.width = (unsigned) maxW;
            fmt.fmt.pix.height = (unsigned) maxH;
            if (xioctl(fd_, VIDIOC_S_FMT, &fmt) != 0
                || fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG) {
                shutdown();
                return false;
            }
        }
        pixelFormat_ = fmt.fmt.pix.pixelformat;
        width_ = (int) fmt.fmt.pix.width;
        height_ = (int) fmt.fmt.pix.height;
        stride_ = (int) fmt.fmt.pix.bytesperline;

        v4l2_requestbuffers req{};
        req.count = kBuffers;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (xioctl(fd_, VIDIOC_REQBUFS, &req) != 0 || req.count < 2) {
            shutdown();
            return false;
        }
        for (unsigned i = 0; i < req.count; ++i) {
            v4l2_buffer buf{};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if (xioctl(fd_, VIDIOC_QUERYBUF, &buf) != 0) { shutdown(); return false; }
            void* p = ::mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                             fd_, (off_t) buf.m.offset);
            if (p == MAP_FAILED) { shutdown(); return false; }
            maps_.push_back({p, (size_t) buf.length});
            if (xioctl(fd_, VIDIOC_QBUF, &buf) != 0) { shutdown(); return false; }
        }
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(fd_, VIDIOC_STREAMON, &type) != 0) { shutdown(); return false; }
        streaming_ = true;
        return true;
    }

    void shutdown() {
        if (fd_ >= 0 && streaming_) {
            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            xioctl(fd_, VIDIOC_STREAMOFF, &type);
            streaming_ = false;
        }
        for (auto& m : maps_) ::munmap(m.first, m.second);
        maps_.clear();
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    }

    void run() override {
        while (!threadShouldExit()) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd_, &fds);
            timeval tv{0, 200 * 1000};
            if (::select(fd_ + 1, &fds, nullptr, nullptr, &tv) <= 0) continue;
            v4l2_buffer buf{};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if (xioctl(fd_, VIDIOC_DQBUF, &buf) != 0) continue;
            if (buf.index < maps_.size())
                deliver((const unsigned char*) maps_[buf.index].first, (size_t) buf.bytesused);
            xioctl(fd_, VIDIOC_QBUF, &buf);
        }
    }

    void deliver(const unsigned char* data, size_t bytes) {
        juce::Image img;
        if (pixelFormat_ == V4L2_PIX_FMT_YUYV) {
            if (bytes < (size_t) stride_ * (size_t) height_) return;
            img = juce::Image(juce::Image::RGB, width_, height_, false);
            juce::Image::BitmapData bd(img, juce::Image::BitmapData::writeOnly);
            std::vector<unsigned char> row((size_t) width_ * 3);
            for (int y = 0; y < height_; ++y) {
                yuyvRowToRgb(data + (size_t) y * (size_t) stride_, row.data(), width_);
                const unsigned char* s = row.data();
                for (int x = 0; x < width_; ++x, s += 3)
                    bd.setPixelColour(x, y, juce::Colour(s[0], s[1], s[2]));
            }
        } else {
            juce::MemoryInputStream in(data, bytes, false);
            img = juce::JPEGImageFormat().decodeImage(in);
            if (!img.isValid()) return;
        }
        listeners_.call([&](CameraCapture::Listener& l) { l.cameraFrame(img); });
    }

    static constexpr unsigned kBuffers = 4;
    int fd_ = -1;
    bool streaming_ = false;
    unsigned pixelFormat_ = 0;
    int width_ = 0, height_ = 0, stride_ = 0;
    std::vector<std::pair<void*, size_t>> maps_;
    juce::ListenerList<CameraCapture::Listener> listeners_;
};

}

inline std::vector<std::string> CameraCapture::availableDevices() {
    std::vector<std::string> out;
    for (auto& d : camera_detail::v4l2Devices()) out.push_back(d.name);
    return out;
}

inline std::unique_ptr<CameraCapture> CameraCapture::open(int index, int, int,
                                                          int maxWidth, int maxHeight) {
    const auto devs = camera_detail::v4l2Devices();
    if (index < 0 || index >= (int) devs.size()) return nullptr;
    return camera_detail::V4l2Camera::tryOpen(devs[(size_t) index].path, maxWidth, maxHeight);
}

}

#else

namespace hum {

inline std::vector<std::string> CameraCapture::availableDevices() { return {}; }

inline std::unique_ptr<CameraCapture> CameraCapture::open(int, int, int, int, int) {
    return nullptr;
}

}

#endif
