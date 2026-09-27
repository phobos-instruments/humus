// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VideoEncoder.h"

#include <cstdint>
#include <memory>

#include "core/video/HapWrite.h"
#include "gui/video/VideoEncoderH264.h"

#if JUCE_WINDOWS
#include <objbase.h>
#endif

#if HUM_VIDEO_NATIVE
#include "gui/video/VideoEncoderNative.h"
#endif

namespace hum {

namespace {

class HapVideo : public VideoEncoder {
public:
    HapVideo(const juce::File& file, int width, int height, double fps)
        : writer_(file, width, height, fps) {}

    bool ok() const override { return writer_.ok(); }
    int frameCount() const override { return writer_.frameCount(); }
    bool openSound(int channels, double sampleRate) override {
        return writer_.openSound(channels, sampleRate);
    }
    bool addSound(const float* const* channels, int count, int frames) override {
        return writer_.addSound(channels, count, frames);
    }
    bool addFrame(const std::uint8_t* rgba) override { return writer_.addFrame(rgba); }
    bool close() override { return writer_.close(); }

private:
    hap::Writer writer_;
};

class NoVideo : public VideoEncoder {
public:
    bool ok() const override { return false; }
    int frameCount() const override { return 0; }
    bool openSound(int, double) override { return false; }
    bool addSound(const float* const*, int, int) override { return false; }
    bool addFrame(const std::uint8_t*) override { return false; }
    bool close() override { return false; }
};

}

VideoThread::VideoThread() {
#if JUCE_WINDOWS
    joined_ = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
#endif
}

VideoThread::~VideoThread() {
#if JUCE_WINDOWS
    if (joined_) CoUninitialize();
#endif
}

bool videoKindAvailable(VideoKind kind) {
    if (kind == VideoKind::Hap) return true;
#if HUM_VIDEO_NATIVE
    return true;
#else
    return HUM_H264_WRITER != 0;
#endif
}

bool videoLogIsLoud() {
#if HUM_H264_WRITER
    return av_log_get_level() > AV_LOG_ERROR;
#else
    return false;
#endif
}

const char* videoKindExtension(VideoKind kind) {
    return kind == VideoKind::Hap ? "mov" : "mp4";
}

std::unique_ptr<VideoEncoder> makeVideoWriter(VideoKind kind, const juce::File& file, int width,
                                              int height, double fps, int quality, bool live) {
    if (kind == VideoKind::Hap) return std::make_unique<HapVideo>(file, width, height, fps);
    const int graded = juce::jlimit(kQualityFinest, kQualityCoarsest, quality);
#if HUM_VIDEO_NATIVE
    return makeNativeVideoWriter(file, width, height, fps, graded, live);
#elif HUM_H264_WRITER
    return std::make_unique<H264Writer>(file, width, height, fps, graded, live);
#else
    juce::ignoreUnused(graded, live);
    return std::make_unique<NoVideo>();
#endif
}

}
