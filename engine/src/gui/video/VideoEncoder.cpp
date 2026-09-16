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

#if HUM_MOVIE_NATIVE
#include "gui/video/VideoEncoderNative.h"
#endif

namespace hum {

namespace {

class HapMovie : public VideoEncoder {
public:
    HapMovie(const juce::File& file, int width, int height, double fps)
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

class NoMovie : public VideoEncoder {
public:
    bool ok() const override { return false; }
    int frameCount() const override { return 0; }
    bool openSound(int, double) override { return false; }
    bool addSound(const float* const*, int, int) override { return false; }
    bool addFrame(const std::uint8_t*) override { return false; }
    bool close() override { return false; }
};

}

MovieThread::MovieThread() {
#if JUCE_WINDOWS
    joined_ = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
#endif
}

MovieThread::~MovieThread() {
#if JUCE_WINDOWS
    if (joined_) CoUninitialize();
#endif
}

bool movieKindAvailable(MovieKind kind) {
    if (kind == MovieKind::Hap) return true;
#if HUM_MOVIE_NATIVE
    return true;
#else
    return HUM_H264_WRITER != 0;
#endif
}

bool movieLogIsLoud() {
#if HUM_H264_WRITER
    return av_log_get_level() > AV_LOG_ERROR;
#else
    return false;
#endif
}

const char* movieKindExtension(MovieKind kind) {
    return kind == MovieKind::Hap ? "mov" : "mp4";
}

std::unique_ptr<VideoEncoder> makeMovieWriter(MovieKind kind, const juce::File& file, int width,
                                              int height, double fps, int quality, bool live) {
    if (kind == MovieKind::Hap) return std::make_unique<HapMovie>(file, width, height, fps);
    const int graded = juce::jlimit(kQualityFinest, kQualityCoarsest, quality);
#if HUM_MOVIE_NATIVE
    return makeNativeMovieWriter(file, width, height, fps, graded, live);
#elif HUM_H264_WRITER
    return std::make_unique<H264Writer>(file, width, height, fps, graded, live);
#else
    juce::ignoreUnused(graded, live);
    return std::make_unique<NoMovie>();
#endif
}

}
