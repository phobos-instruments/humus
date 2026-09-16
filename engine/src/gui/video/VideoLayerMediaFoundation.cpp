// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VideoLayer.h"
#include "gui/video/VideoLog.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include <windows.h>
#include <objbase.h>
#include <propidl.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

namespace hum {

namespace {

template <class T> class ComPtr {
public:
    ComPtr() = default;
    ~ComPtr() { reset(); }
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    T** put() {
        reset();
        return &p_;
    }
    T* get() const { return p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const { return p_ != nullptr; }
    void reset() {
        if (p_ != nullptr) {
            p_->Release();
            p_ = nullptr;
        }
    }

private:
    T* p_ = nullptr;
};

constexpr DWORD kVideoStream = (DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM;
constexpr double kTicksPerSecond = 1e7;

void startMediaFoundationOnce() {
    static std::once_flag once;
    std::call_once(once, [] { MFStartup(MF_VERSION, MFSTARTUP_LITE); });
}

juce::String fourccName(const GUID& subtype) {
    char cc[5] = {};
    std::memcpy(cc, &subtype.Data1, 4);
    for (char c : cc) {
        if (c == 0) break;
        if (c < 0x20 || c > 0x7e) return "video";
    }
    return juce::String(cc).trim().toLowerCase();
}

void copyRows(unsigned char* dst, const BYTE* scanline0, LONG pitch, int w, int h) {
    const size_t row = (size_t) w * 4u;
    for (int y = 0; y < h; ++y)
        std::memcpy(dst + row * (size_t) y, scanline0 + (ptrdiff_t) y * (ptrdiff_t) pitch, row);
}

class MediaFoundationVideoLayer : public VideoLayer, private juce::Thread {
public:
    MediaFoundationVideoLayer() : juce::Thread("mf-decode") {}

    ~MediaFoundationVideoLayer() override { stopThread(4000); }

    void load(const juce::String& path) override {
        stopThread(4000);
        {
            const juce::ScopedLock sl(lock_);
            current_.reset();
        }
        path_ = path;
        length_.store(0.0);
        shownPosition_.store(0.0);
        seekTo_.store(-1.0);
        rewind_.store(false);
        if (path_.isNotEmpty()) startThread();
    }

    void setRate(float rate) override { rate_.store(rate); }

    void restart() override { rewind_.store(true); }

    void setPaused(bool paused) override { paused_.store(paused); }

    bool isPaused() const override { return paused_.load(); }

    double positionSeconds() override { return shownPosition_.load(); }

    double lengthSeconds() override { return length_.load(); }

    void seekSeconds(double t) override { seekTo_.store(std::max(0.0, t)); }

    void chase(double seconds, double rate) override {
        chaseTo_.store(std::max(0.0, seconds));
        chaseRate_.store(rate);
        chaseStamp_.fetch_add(1);
    }

    void setLoopRange(const LoopRange& r) override {
        loopIn_.store(std::max(0.0, r.in));
        loopOut_.store(r.out);
        loop_.store(r.loop);
    }

    std::shared_ptr<const Frame> latestFrame() override {
        const juce::ScopedLock sl(lock_);
        return current_;
    }

private:
    struct Reader {
        ComPtr<IMFSourceReader> reader;
        int width = 0, height = 0;
        LONG stride = 0;
        double span = 0.0;
        juce::String codec = "video";
    };

    static bool readShape(Reader& r) {
        ComPtr<IMFMediaType> cur;
        if (FAILED(r.reader->GetCurrentMediaType(kVideoStream, cur.put()))) return false;
        UINT32 w = 0, h = 0;
        if (FAILED(MFGetAttributeSize(cur.get(), MF_MT_FRAME_SIZE, &w, &h)) || w == 0 || h == 0)
            return false;
        r.width = (int) w;
        r.height = (int) h;
        UINT32 stride = 0;
        r.stride = SUCCEEDED(cur->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride))
                       ? (LONG) (INT32) stride
                       : (LONG) w * 4;
        return true;
    }

    static juce::String codecName(Reader& r) {
        ComPtr<IMFMediaType> native;
        GUID sub{};
        if (SUCCEEDED(r.reader->GetNativeMediaType(kVideoStream, 0, native.put()))
            && SUCCEEDED(native->GetGUID(MF_MT_SUBTYPE, &sub)))
            return fourccName(sub);
        return "video";
    }

    bool open(Reader& r) {
        const auto leaf = juce::File(path_).getFileName();
        ComPtr<IMFAttributes> attrs;
        if (FAILED(MFCreateAttributes(attrs.put(), 2))) return false;
        attrs->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
        attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        if (FAILED(MFCreateSourceReaderFromURL(path_.toWideCharPointer(), attrs.get(),
                                               r.reader.put()))) {
            videoLog(leaf + ": not a tape this build can open");
            return false;
        }
        r.reader->SetStreamSelection((DWORD) MF_SOURCE_READER_ALL_STREAMS, FALSE);
        if (FAILED(r.reader->SetStreamSelection(kVideoStream, TRUE))) {
            videoLog(leaf + ": no video stream");
            return false;
        }
        r.codec = codecName(r);
        ComPtr<IMFMediaType> want;
        if (FAILED(MFCreateMediaType(want.put()))) return false;
        want->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        want->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        if (FAILED(r.reader->SetCurrentMediaType(kVideoStream, nullptr, want.get()))) {
            videoLog(leaf + ": no decoder for " + r.codec + " on this system");
            return false;
        }
        if (!readShape(r)) return false;
        PROPVARIANT v;
        PropVariantInit(&v);
        if (SUCCEEDED(r.reader->GetPresentationAttribute((DWORD) MF_SOURCE_READER_MEDIASOURCE,
                                                         MF_PD_DURATION, &v))) {
            if (v.vt == VT_UI8) r.span = (double) v.uhVal.QuadPart / kTicksPerSecond;
            PropVariantClear(&v);
        }
        videoLog(leaf + ": " + r.codec + " through Media Foundation");
        return true;
    }

    void seekReader(Reader& r, double seconds) {
        PROPVARIANT v;
        PropVariantInit(&v);
        v.vt = VT_I8;
        v.hVal.QuadPart = (LONGLONG) std::llround(seconds * kTicksPerSecond);
        r.reader->SetCurrentPosition(GUID_NULL, v);
        PropVariantClear(&v);
        pending_.reset();
        pendingValid_ = false;
        atEof_ = false;
    }

    bool decodeNext(Reader& r, double& ptsOut) {
        for (int tries = 0; tries < 8; ++tries) {
            DWORD stream = 0, flags = 0;
            LONGLONG ts = 0;
            ComPtr<IMFSample> sample;
            if (FAILED(r.reader->ReadSample(kVideoStream, 0, &stream, &flags, &ts, sample.put())))
                return false;
            if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
                atEof_ = true;
                return false;
            }
            if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0) readShape(r);
            if (!sample) continue;
            *pending_.put() = sample.get();
            pending_->AddRef();
            ptsOut = (double) ts / kTicksPerSecond;
            return true;
        }
        return false;
    }

    void present(Reader& r, double pts) {
        if (!pending_) return;
        const int w = r.width, h = r.height;
        if (w <= 0 || h <= 0) return;
        ComPtr<IMFMediaBuffer> buf;
        if (FAILED(pending_->ConvertToContiguousBuffer(buf.put()))) return;
        auto out = std::make_shared<Frame>();
        out->width = w;
        out->height = h;
        out->fmt = Frame::BGRA;
        out->bgra.resize((size_t) w * (size_t) h * 4u);

        bool copied = false;
        ComPtr<IMF2DBuffer> planar;
        if (SUCCEEDED(buf->QueryInterface(IID_PPV_ARGS(planar.put())))) {
            BYTE* scanline0 = nullptr;
            LONG pitch = 0;
            if (SUCCEEDED(planar->Lock2D(&scanline0, &pitch))) {
                copyRows(out->bgra.data(), scanline0, pitch, w, h);
                planar->Unlock2D();
                copied = true;
            }
        }
        if (!copied) {
            BYTE* data = nullptr;
            DWORD len = 0;
            if (FAILED(buf->Lock(&data, nullptr, &len))) return;
            const LONG pitch = r.stride != 0 ? r.stride : (LONG) w * 4;
            const size_t rowBytes = (size_t) (pitch >= 0 ? pitch : -pitch);
            if (len >= rowBytes * (size_t) h) {
                const BYTE* top = pitch >= 0 ? data : data + rowBytes * (size_t) (h - 1);
                copyRows(out->bgra.data(), top, pitch, w, h);
                copied = true;
            }
            buf->Unlock();
        }
        if (!copied) return;
        out->pts = pts;
        const juce::ScopedLock sl(lock_);
        current_ = std::move(out);
    }

    void run() override {
        startMediaFoundationOnce();
        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        Reader d;
        if (open(d)) drive(d);
        d.reader.reset();
        pending_.reset();
        if (com == S_OK || com == S_FALSE) CoUninitialize();
    }

    void drive(Reader& d) {
        length_.store(d.span);
        double position = 0.0;
        double lastPresented = -1.0;
        auto lastMs = juce::Time::getMillisecondCounterHiRes();
        pendingValid_ = false;
        atEof_ = false;
        double pendingPts = 0.0;
        unsigned chaseSeen = 0;
        bool chasing = false;

        while (!threadShouldExit()) {
            const auto nowMs = juce::Time::getMillisecondCounterHiRes();
            const auto range = loopWindow(loopIn_.load(), loopOut_.load(), d.span);
            bool jumped = false;
            if (rewind_.exchange(false)) {
                position = range.first;
                jumped = true;
                chasing = false;
            }
            if (const double target = seekTo_.exchange(-1.0); target >= 0.0) {
                position = d.span > 0.0 ? std::min(target, d.span) : target;
                jumped = true;
                chasing = false;
            }
            if (const unsigned stamp = chaseStamp_.load(); stamp != chaseSeen) {
                chaseSeen = stamp;
                chasing = true;
                position = chaseTo_.load();
                if (lastPresented >= 0.0 && position > lastPresented + 1.0) jumped = true;
            } else if (chasing) {
                position += (nowMs - lastMs) * 0.001 * chaseRate_.load();
            } else if (const float rate = rate_.load(); !paused_.load() && rate > 0.0f) {
                position += (nowMs - lastMs) * 0.001 * (double) rate;
            }
            lastMs = nowMs;
            if (chasing) {
                if (d.span > 0.0) position = std::min(position, d.span);
            } else if (range.second > range.first && position >= range.second) {
                if (loop_.load()) {
                    position = range.first
                             + std::fmod(position - range.first, range.second - range.first);
                    jumped = true;
                } else {
                    position = range.second;
                }
            }
            shownPosition_.store(position);

            if (jumped || (lastPresented >= 0.0 && position < lastPresented - 0.05)) {
                seekReader(d, position);
                lastPresented = -1.0;
            }

            if (!pendingValid_) {
                double pts = 0.0;
                if (decodeNext(d, pts)) {
                    pendingPts = pts;
                    pendingValid_ = true;
                } else if (atEof_) {
                    if (d.span <= 0.0) d.span = std::max(lastPresented, 0.0);
                    if (!loop_.load() || chasing) {
                        wait(chasing ? 10 : 2);
                        continue;
                    }
                    seekReader(d, range.first);
                    position = range.first;
                    lastPresented = -1.0;
                    wait(2);
                    continue;
                }
            }
            if (pendingValid_ && pendingPts <= position + 0.001) {
                if (pendingPts >= position - 0.5 || lastPresented < 0.0) {
                    present(d, pendingPts);
                    lastPresented = pendingPts;
                }
                pending_.reset();
                pendingValid_ = false;
                continue;
            }
            wait(paused_.load() ? 20 : 2);
        }
    }

    juce::String path_;
    std::atomic<float> rate_{1.0f};
    std::atomic<bool> rewind_{false};
    std::atomic<double> loopIn_{0.0};
    std::atomic<double> loopOut_{0.0};
    std::atomic<bool> loop_{true};
    std::atomic<bool> paused_{false};
    std::atomic<double> chaseTo_{0.0};
    std::atomic<double> chaseRate_{0.0};
    std::atomic<unsigned> chaseStamp_{0};
    std::atomic<double> seekTo_{-1.0};
    std::atomic<double> shownPosition_{0.0};
    std::atomic<double> length_{0.0};
    ComPtr<IMFSample> pending_;
    bool pendingValid_ = false;
    bool atEof_ = false;
    juce::CriticalSection lock_;
    std::shared_ptr<const Frame> current_;
};

}

std::unique_ptr<VideoLayer> VideoLayer::createPlatform() {
    return std::make_unique<MediaFoundationVideoLayer>();
}

}
