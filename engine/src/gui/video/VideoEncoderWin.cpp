// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VideoEncoderNative.h"

#include <windows.h>

#include <codecapi.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "gui/video/MovieSound.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

constexpr int kSoundBlock = 1024;
constexpr double kHundredNanos = 1.0e7;

template <typename T>
void letGo(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

constexpr double kAacRateLow = kDefaultSampleRate, kAacRateHigh = 48000.0;

bool aacTakesRate(double rate) {
    const double whole = std::floor(rate + 0.5);
    return whole == kAacRateLow || whole == kAacRateHigh;
}

class WinMovie : public VideoEncoder {
public:
    WinMovie(const juce::File& file, int width, int height, double fps, int quality, bool live)
        : width_(width & ~1), height_(height & ~1), fps_(fps > 0.0 ? fps : 30.0), live_(live) {
        if (width_ < 16 || height_ < 16) return;
        if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) return;
        up_ = true;
        file.deleteFile();
        if (FAILED(MFCreateSinkWriterFromURL(file.getFullPathName().toWideCharPointer(), nullptr,
                                             nullptr, &writer_)))
            return;
        if (!openPicture(quality)) return;
        ready_ = true;
    }

    ~WinMovie() override {
        close();
        letGo(writer_);
        if (up_) MFShutdown();
    }

    bool ok() const override { return ready_; }
    int frameCount() const override { return frames_; }

    bool openSound(int channels, double sampleRate) override {
        if (!ready_ || started_ || channels <= 0 || !aacTakesRate(sampleRate)) return false;
        channels_ = std::min(channels, 2);
        rate_ = sampleRate;
        const UINT32 whole = (UINT32) std::llround(rate_);
        IMFMediaType* out = nullptr;
        if (FAILED(MFCreateMediaType(&out))) return false;
        bool made = SUCCEEDED(out->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))
                    && SUCCEEDED(out->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC))
                    && SUCCEEDED(out->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16))
                    && SUCCEEDED(out->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, whole))
                    && SUCCEEDED(out->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, (UINT32) channels_))
                    && SUCCEEDED(out->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000))
                    && SUCCEEDED(out->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0))
                    && SUCCEEDED(writer_->AddStream(out, &soundStream_));
        letGo(out);
        if (!made) return false;
        IMFMediaType* in = nullptr;
        if (FAILED(MFCreateMediaType(&in))) return false;
        const UINT32 align = 2u * (UINT32) channels_;
        made = SUCCEEDED(in->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))
               && SUCCEEDED(in->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM))
               && SUCCEEDED(in->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16))
               && SUCCEEDED(in->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, whole))
               && SUCCEEDED(in->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, (UINT32) channels_))
               && SUCCEEDED(in->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, align))
               && SUCCEEDED(in->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, whole * align))
               && SUCCEEDED(writer_->SetInputMediaType(soundStream_, in, nullptr));
        letGo(in);
        hasSound_ = made;
        return made;
    }

    bool addSound(const float* const* channels, int count, int frames) override {
        if (!hasSound_ || count <= 0 || frames <= 0) return false;
        hold_.take(channels, count, frames, channels_);
        return true;
    }

    bool addFrame(const std::uint8_t* rgba) override {
        if (!ready_ || rgba == nullptr) return false;
        if (!started_ && !start()) return false;
        const size_t bytes = (size_t) width_ * (size_t) height_ * 4u;
        IMFSample* sample = nullptr;
        IMFMediaBuffer* buffer = nullptr;
        if (FAILED(MFCreateMemoryBuffer((DWORD) bytes, &buffer))) return false;
        BYTE* out = nullptr;
        bool sent = SUCCEEDED(buffer->Lock(&out, nullptr, nullptr));
        if (sent) {
            for (size_t i = 0; i < bytes; i += 4) {
                out[i] = rgba[i + 2];
                out[i + 1] = rgba[i + 1];
                out[i + 2] = rgba[i];
                out[i + 3] = 255;
            }
            buffer->Unlock();
            sent = SUCCEEDED(buffer->SetCurrentLength((DWORD) bytes));
        }
        if (sent) sent = SUCCEEDED(MFCreateSample(&sample))
                         && SUCCEEDED(sample->AddBuffer(buffer));
        if (sent) {
            const auto when = (LONGLONG) std::llround((double) frames_ / fps_ * kHundredNanos);
            sent = SUCCEEDED(sample->SetSampleTime(when))
                   && SUCCEEDED(sample->SetSampleDuration(
                       (LONGLONG) std::llround(kHundredNanos / fps_)))
                   && SUCCEEDED(writer_->WriteSample(pictureStream_, sample));
        }
        letGo(sample);
        letGo(buffer);
        if (!sent) return false;
        ++frames_;
        return pushSoundUpTo((double) frames_ / fps_);
    }

    bool close() override {
        if (!ready_) return false;
        ready_ = false;
        bool wrote = frames_ > 0;
        if (started_) {
            pushSoundUpTo(1.0e9);
            wrote = SUCCEEDED(writer_->Finalize()) && wrote;
        }
        hold_.clear();
        return wrote;
    }

private:
    bool openPicture(int quality) {
        const auto bits = (UINT32) std::llround(movieBitsPerSecond(width_, height_, fps_, quality));
        const UINT32 num = (UINT32) std::llround(fps_ * 1000.0);
        IMFMediaType* out = nullptr;
        if (FAILED(MFCreateMediaType(&out))) return false;
        bool made = SUCCEEDED(out->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))
                    && SUCCEEDED(out->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264))
                    && SUCCEEDED(out->SetUINT32(MF_MT_AVG_BITRATE, bits))
                    && SUCCEEDED(out->SetUINT32(MF_MT_MAX_KEYFRAME_SPACING,
                                                (UINT32) keyframeInterval(fps_, live_)))
                    && SUCCEEDED(out->SetUINT32(MF_MT_INTERLACE_MODE,
                                                MFVideoInterlace_Progressive))
                    && SUCCEEDED(out->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_High))
                    && SUCCEEDED(MFSetAttributeSize(out, MF_MT_FRAME_SIZE, (UINT32) width_,
                                                    (UINT32) height_))
                    && SUCCEEDED(MFSetAttributeRatio(out, MF_MT_FRAME_RATE, num, 1000))
                    && SUCCEEDED(MFSetAttributeRatio(out, MF_MT_PIXEL_ASPECT_RATIO, 1, 1))
                    && SUCCEEDED(writer_->AddStream(out, &pictureStream_));
        letGo(out);
        if (!made) return false;
        IMFMediaType* in = nullptr;
        if (FAILED(MFCreateMediaType(&in))) return false;
        made = SUCCEEDED(in->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))
               && SUCCEEDED(in->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32))
               && SUCCEEDED(in->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive))
               && SUCCEEDED(in->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32) (width_ * 4)))
               && SUCCEEDED(MFSetAttributeSize(in, MF_MT_FRAME_SIZE, (UINT32) width_,
                                               (UINT32) height_))
               && SUCCEEDED(MFSetAttributeRatio(in, MF_MT_FRAME_RATE, num, 1000))
               && SUCCEEDED(MFSetAttributeRatio(in, MF_MT_PIXEL_ASPECT_RATIO, 1, 1))
               && SUCCEEDED(writer_->SetInputMediaType(pictureStream_, in, nullptr));
        letGo(in);
        return made;
    }

    bool start() {
        if (FAILED(writer_->BeginWriting())) return false;
        started_ = true;
        return true;
    }

    bool pushSoundUpTo(double seconds) {
        if (!hasSound_ || hold_.empty()) return true;
        for (int take = hold_.ready(kSoundBlock, seconds, rate_); take > 0;
             take = hold_.ready(kSoundBlock, seconds, rate_)) {
            if (!appendSound(take)) return false;
            hold_.advance(take);
        }
        return true;
    }

    bool appendSound(int frames) {
        const size_t bytes = (size_t) frames * (size_t) channels_ * sizeof(std::int16_t);
        IMFSample* sample = nullptr;
        IMFMediaBuffer* buffer = nullptr;
        if (FAILED(MFCreateMemoryBuffer((DWORD) bytes, &buffer))) return false;
        BYTE* out = nullptr;
        bool sent = SUCCEEDED(buffer->Lock(&out, nullptr, nullptr));
        if (sent) {
            auto* words = (std::int16_t*) out;
            for (int i = 0; i < frames; ++i)
                for (int c = 0; c < channels_; ++c)
                    words[i * channels_ + c] = (std::int16_t) std::lround(
                        32767.0 * juce::jlimit(-1.0f, 1.0f, hold_.at(c, hold_.sent() + i)));
            buffer->Unlock();
            sent = SUCCEEDED(buffer->SetCurrentLength((DWORD) bytes));
        }
        if (sent) sent = SUCCEEDED(MFCreateSample(&sample))
                         && SUCCEEDED(sample->AddBuffer(buffer));
        if (sent) {
            const auto when = (LONGLONG) std::llround((double) hold_.sent() / rate_
                                                      * kHundredNanos);
            sent = SUCCEEDED(sample->SetSampleTime(when))
                   && SUCCEEDED(sample->SetSampleDuration(
                       (LONGLONG) std::llround((double) frames / rate_ * kHundredNanos)))
                   && SUCCEEDED(writer_->WriteSample(soundStream_, sample));
        }
        letGo(sample);
        letGo(buffer);
        return sent;
    }

    int width_ = 0, height_ = 0, frames_ = 0, channels_ = 2;
    double fps_ = 30.0, rate_ = 48000.0;
    bool ready_ = false, started_ = false, up_ = false, hasSound_ = false, live_ = false;
    DWORD pictureStream_ = 0, soundStream_ = 0;
    IMFSinkWriter* writer_ = nullptr;
    SoundHold hold_;
};

}

std::unique_ptr<VideoEncoder> makeNativeMovieWriter(const juce::File& file, int width, int height,
                                                    double fps, int quality, bool live) {
    return std::make_unique<WinMovie>(file, width, height, fps, quality, live);
}

}
