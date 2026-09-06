#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "gui/MovieSound.h"
#include "gui/VideoEncoder.h"
#include "gui/VideoLog.h"

#if HUM_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#endif

namespace hum {

#if HUM_FFMPEG && LIBAVCODEC_VERSION_MAJOR >= 59
#define HUM_H264_WRITER 1
#else
#define HUM_H264_WRITER 0
#endif

#if HUM_H264_WRITER

class H264Writer : public VideoEncoder {
public:
    H264Writer(const juce::File& file, int width, int height, double fps, int quality,
               bool live = false)
        : width_(width & ~1), height_(height & ~1), fps_(fps > 0.0 ? fps : 30.0),
          quality_(quality), live_(live) {
        av_log_set_level(videoLogOn() ? AV_LOG_INFO : AV_LOG_ERROR);
        if (width_ < 16 || height_ < 16) return;
        path_ = file.getFullPathName().toStdString();
        file.deleteFile();
        if (avformat_alloc_output_context2(&format_, nullptr, nullptr, path_.c_str()) < 0
            || format_ == nullptr)
            return;
        if (!openPicture()) return;
        ready_ = true;
    }

    ~H264Writer() override { close(); }

    bool ok() const override { return ready_; }
    int frameCount() const override { return frames_; }

    bool openSound(int channels, double sampleRate) override {
        if (!ready_ || started_ || channels <= 0 || sampleRate <= 0.0) return false;
        const auto* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (codec == nullptr) return false;
        soundStream_ = avformat_new_stream(format_, nullptr);
        if (soundStream_ == nullptr) return false;
        sound_ = avcodec_alloc_context3(codec);
        if (sound_ == nullptr) return false;
        sound_->sample_fmt = AV_SAMPLE_FMT_FLTP;
        sound_->sample_rate = (int) std::lround(sampleRate);
        sound_->bit_rate = 256000;
        av_channel_layout_default(&sound_->ch_layout, std::min(channels, 2));
        sound_->time_base = AVRational{1, sound_->sample_rate};
        if (format_->oformat->flags & AVFMT_GLOBALHEADER)
            sound_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        if (avcodec_open2(sound_, codec, nullptr) < 0) return false;
        if (avcodec_parameters_from_context(soundStream_->codecpar, sound_) < 0) return false;
        soundStream_->time_base = sound_->time_base;
        channels_ = sound_->ch_layout.nb_channels;
        soundFrame_ = av_frame_alloc();
        soundFrame_->format = AV_SAMPLE_FMT_FLTP;
        soundFrame_->nb_samples = sound_->frame_size > 0 ? sound_->frame_size : 1024;
        av_channel_layout_copy(&soundFrame_->ch_layout, &sound_->ch_layout);
        soundFrame_->sample_rate = sound_->sample_rate;
        if (av_frame_get_buffer(soundFrame_, 0) < 0) return false;
        return true;
    }

    bool addSound(const float* const* channels, int count, int frames) override {
        if (sound_ == nullptr || count <= 0 || frames <= 0) return false;
        hold_.take(channels, count, frames, channels_);
        return true;
    }

    bool addFrame(const std::uint8_t* rgba) override {
        if (!ready_ || rgba == nullptr) return false;
        if (!started_ && !start()) return false;
        if (av_frame_make_writable(frame_) < 0) return false;
        const std::uint8_t* src[1] = {rgba};
        const int stride[1] = {4 * width_};
        sws_scale(scaler_, src, stride, 0, height_, frame_->data, frame_->linesize);
        frame_->pts = (std::int64_t) frames_ * kTicks;
        if (!send(picture_, frame_, pictureStream_)) return false;
        ++frames_;
        return pushSoundUpTo((double) frames_ / fps_);
    }

    bool close() override {
        if (!ready_) return false;
        ready_ = false;
        bool wrote = frames_ > 0;
        if (started_) {
            pushSoundUpTo(1.0e9);
            send(picture_, nullptr, pictureStream_);
            if (sound_ != nullptr) send(sound_, nullptr, soundStream_);
            wrote = av_write_trailer(format_) >= 0 && wrote;
        }
        release();
        return wrote;
    }

private:
    bool openPicture() {
        const auto* codec = avcodec_find_encoder_by_name("libx264");
        if (codec == nullptr) codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (codec == nullptr) return false;
        pictureStream_ = avformat_new_stream(format_, nullptr);
        if (pictureStream_ == nullptr) return false;
        picture_ = avcodec_alloc_context3(codec);
        if (picture_ == nullptr) return false;
        picture_->width = width_;
        picture_->height = height_;
        picture_->pix_fmt = AV_PIX_FMT_YUV420P;
        picture_->time_base = AVRational{1, (int) std::lround(fps_ * kTicks)};
        picture_->framerate = AVRational{(int) std::lround(fps_ * kTicks), kTicks};
        picture_->gop_size = keyframeInterval(fps_, live_);
        picture_->max_b_frames = 2;
        av_opt_set(picture_->priv_data, "preset", live_ ? "veryfast" : "medium", 0);
        av_opt_set(picture_->priv_data, "crf", std::to_string(quality_).c_str(), 0);
        if (format_->oformat->flags & AVFMT_GLOBALHEADER)
            picture_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        if (avcodec_open2(picture_, codec, nullptr) < 0) return false;
        if (avcodec_parameters_from_context(pictureStream_->codecpar, picture_) < 0) return false;
        pictureStream_->time_base = picture_->time_base;

        frame_ = av_frame_alloc();
        frame_->format = AV_PIX_FMT_YUV420P;
        frame_->width = width_;
        frame_->height = height_;
        if (av_frame_get_buffer(frame_, 0) < 0) return false;
        scaler_ = sws_getContext(width_, height_, AV_PIX_FMT_RGBA, width_, height_,
                                 AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);
        return scaler_ != nullptr;
    }

    bool start() {
        if ((format_->oformat->flags & AVFMT_NOFILE) == 0
            && avio_open(&format_->pb, path_.c_str(), AVIO_FLAG_WRITE) < 0)
            return false;
        if (avformat_write_header(format_, nullptr) < 0) return false;
        started_ = true;
        return true;
    }

    bool send(AVCodecContext* ctx, AVFrame* frame, AVStream* stream) {
        if (ctx == nullptr) return true;
        if (avcodec_send_frame(ctx, frame) < 0) return false;
        for (;;) {
            AVPacket* packet = av_packet_alloc();
            const int got = avcodec_receive_packet(ctx, packet);
            if (got == AVERROR(EAGAIN) || got == AVERROR_EOF) {
                av_packet_free(&packet);
                return true;
            }
            if (got < 0) {
                av_packet_free(&packet);
                return false;
            }
            av_packet_rescale_ts(packet, ctx->time_base, stream->time_base);
            packet->stream_index = stream->index;
            const int wrote = av_interleaved_write_frame(format_, packet);
            av_packet_free(&packet);
            if (wrote < 0) return false;
        }
    }

    bool pushSoundUpTo(double seconds) {
        if (sound_ == nullptr || hold_.empty()) return true;
        const int block = soundFrame_->nb_samples;
        const double rate = (double) sound_->sample_rate;
        for (int take = hold_.ready(block, seconds, rate); take > 0;
             take = hold_.ready(block, seconds, rate)) {
            if (av_frame_make_writable(soundFrame_) < 0) return false;
            for (int c = 0; c < channels_; ++c) {
                auto* dst = (float*) soundFrame_->data[c];
                for (int i = 0; i < take; ++i) dst[i] = hold_.at(c, hold_.sent() + i);
                for (int i = take; i < block; ++i) dst[i] = 0.0f;
            }
            soundFrame_->pts = hold_.sent();
            if (!send(sound_, soundFrame_, soundStream_)) return false;
            hold_.advance(take);
        }
        return true;
    }

    void release() {
        if (format_ != nullptr && started_ && (format_->oformat->flags & AVFMT_NOFILE) == 0)
            avio_closep(&format_->pb);
        if (scaler_ != nullptr) { sws_freeContext(scaler_); scaler_ = nullptr; }
        if (frame_ != nullptr) av_frame_free(&frame_);
        if (soundFrame_ != nullptr) av_frame_free(&soundFrame_);
        if (picture_ != nullptr) avcodec_free_context(&picture_);
        if (sound_ != nullptr) avcodec_free_context(&sound_);
        if (format_ != nullptr) { avformat_free_context(format_); format_ = nullptr; }
        hold_.clear();
    }

    static constexpr int kTicks = 1000;

    int width_ = 0, height_ = 0, frames_ = 0, channels_ = 0, quality_ = 19;
    bool live_ = false;
    double fps_ = 30.0;
    bool ready_ = false, started_ = false;
    std::string path_;
    AVFormatContext* format_ = nullptr;
    AVCodecContext* picture_ = nullptr;
    AVCodecContext* sound_ = nullptr;
    AVStream* pictureStream_ = nullptr;
    AVStream* soundStream_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVFrame* soundFrame_ = nullptr;
    SwsContext* scaler_ = nullptr;
    SoundHold hold_;
};

#endif

}
