#include "gui/VideoLayer.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace hum {

namespace {

class FfmpegVideoLayer : public VideoLayer, private juce::Thread {
public:
    FfmpegVideoLayer() : juce::Thread("ffmpeg-decode") {}

    ~FfmpegVideoLayer() override { stopThread(4000); }

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

    std::shared_ptr<const Frame> latestFrame() override {
        const juce::ScopedLock sl(lock_);
        return current_;
    }

private:
    struct Decoder {
        AVFormatContext* fmt = nullptr;
        AVCodecContext* ctx = nullptr;
        AVBufferRef* hwDevice = nullptr;
        AVPixelFormat hwPix = AV_PIX_FMT_NONE;
        AVPacket* packet = nullptr;
        AVFrame* frame = nullptr;
        AVFrame* swFrame = nullptr;
        SwsContext* sws = nullptr;
        int stream = -1;
        AVRational tb{1, 1};
        double span = 0.0;

        ~Decoder() {
            if (sws != nullptr) sws_freeContext(sws);
            if (swFrame != nullptr) av_frame_free(&swFrame);
            if (frame != nullptr) av_frame_free(&frame);
            if (packet != nullptr) av_packet_free(&packet);
            if (ctx != nullptr) avcodec_free_context(&ctx);
            if (hwDevice != nullptr) av_buffer_unref(&hwDevice);
            if (fmt != nullptr) avformat_close_input(&fmt);
        }
    };

    static AVPixelFormat pickHwFormat(AVCodecContext* ctx, const AVPixelFormat* formats) {
        auto* self = static_cast<Decoder*>(ctx->opaque);
        for (const AVPixelFormat* p = formats; *p != AV_PIX_FMT_NONE; ++p)
            if (self != nullptr && *p == self->hwPix) return *p;
        return formats[0];
    }

    static bool openCodec(Decoder& d, const AVCodec* codec, AVHWDeviceType type) {
        d.ctx = avcodec_alloc_context3(codec);
        if (d.ctx == nullptr) return false;
        if (avcodec_parameters_to_context(d.ctx, d.fmt->streams[d.stream]->codecpar) < 0) {
            avcodec_free_context(&d.ctx);
            return false;
        }
        d.ctx->opaque = &d;
        d.hwPix = AV_PIX_FMT_NONE;
        if (type != AV_HWDEVICE_TYPE_NONE) {
            for (int i = 0;; ++i) {
                const auto* cfg = avcodec_get_hw_config(codec, i);
                if (cfg == nullptr) break;
                if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) != 0
                    && cfg->device_type == type) {
                    d.hwPix = cfg->pix_fmt;
                    break;
                }
            }
            if (d.hwPix == AV_PIX_FMT_NONE
                || av_hwdevice_ctx_create(&d.hwDevice, type, nullptr, nullptr, 0) < 0) {
                avcodec_free_context(&d.ctx);
                return false;
            }
            d.ctx->hw_device_ctx = av_buffer_ref(d.hwDevice);
            d.ctx->get_format = pickHwFormat;
        }
        if (avcodec_open2(d.ctx, codec, nullptr) < 0) {
            avcodec_free_context(&d.ctx);
            if (d.hwDevice != nullptr) av_buffer_unref(&d.hwDevice);
            return false;
        }
        return true;
    }

    bool open(Decoder& d) {
        const auto leaf = juce::File(path_).getFileName();
        if (avformat_open_input(&d.fmt, path_.toRawUTF8(), nullptr, nullptr) < 0) {
            std::cout << "[video] " << leaf << ": not a tape this build can open" << std::endl;
            return false;
        }
        if (avformat_find_stream_info(d.fmt, nullptr) < 0) return false;
        d.stream = av_find_best_stream(d.fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (d.stream < 0) {
            std::cout << "[video] " << leaf << ": no video stream" << std::endl;
            return false;
        }
        const auto codecId = d.fmt->streams[d.stream]->codecpar->codec_id;
        const AVCodec* codec = avcodec_find_decoder(codecId);
        if (codec == nullptr) {
            std::cout << "[video] " << leaf << ": no decoder for " << avcodec_get_name(codecId)
                      << " in this build" << std::endl;
            return false;
        }
        const auto* st = d.fmt->streams[d.stream];
        d.tb = st->time_base;
        if (st->duration > 0) d.span = (double) st->duration * av_q2d(d.tb);
        else if (d.fmt->duration > 0) d.span = (double) d.fmt->duration / (double) AV_TIME_BASE;

        const AVHWDeviceType tries[] = {
#if defined(_WIN32)
            AV_HWDEVICE_TYPE_D3D11VA,
#endif
            AV_HWDEVICE_TYPE_CUDA, AV_HWDEVICE_TYPE_VAAPI, AV_HWDEVICE_TYPE_NONE};
        for (const auto type : tries)
            if (openCodec(d, codec, type)) {
                std::cout << "[video] " << juce::File(path_).getFileName() << ": "
                          << codec->name << " "
                          << (type == AV_HWDEVICE_TYPE_NONE
                                  ? "in software"
                                  : juce::String("on ") + av_hwdevice_get_type_name(type))
                          << std::endl;
                break;
            }
        if (d.ctx == nullptr) return false;
        d.packet = av_packet_alloc();
        d.frame = av_frame_alloc();
        d.swFrame = av_frame_alloc();
        return d.packet != nullptr && d.frame != nullptr && d.swFrame != nullptr;
    }

    void seekDecoder(Decoder& d, double seconds) {
        const auto ts = (std::int64_t) (seconds / av_q2d(d.tb));
        av_seek_frame(d.fmt, d.stream, ts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(d.ctx);
        pendingValid_ = false;
        atEof_ = false;
    }

    bool decodeNext(Decoder& d, double& ptsOut) {
        for (;;) {
            const int got = avcodec_receive_frame(d.ctx, d.frame);
            if (got == 0) {
                const auto best = d.frame->best_effort_timestamp;
                ptsOut = best == AV_NOPTS_VALUE ? -1.0 : (double) best * av_q2d(d.tb);
                return true;
            }
            if (got != AVERROR(EAGAIN)) return false;
            if (atEof_) return false;
            const int rd = av_read_frame(d.fmt, d.packet);
            if (rd < 0) {
                atEof_ = true;
                avcodec_send_packet(d.ctx, nullptr);
                continue;
            }
            if (d.packet->stream_index == d.stream) avcodec_send_packet(d.ctx, d.packet);
            av_packet_unref(d.packet);
        }
    }

    void present(Decoder& d) {
        AVFrame* src = d.frame;
        if (d.hwPix != AV_PIX_FMT_NONE && d.frame->format == d.hwPix) {
            av_frame_unref(d.swFrame);
            if (av_hwframe_transfer_data(d.swFrame, d.frame, 0) < 0) return;
            src = d.swFrame;
        }
        const int w = src->width, h = src->height;
        if (w <= 0 || h <= 0) return;
        d.sws = sws_getCachedContext(d.sws, w, h, (AVPixelFormat) src->format, w, h,
                                     AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr,
                                     nullptr);
        if (d.sws == nullptr) return;
        auto out = std::make_shared<Frame>();
        out->width = w;
        out->height = h;
        out->fmt = Frame::BGRA;
        out->bgra.resize((size_t) w * (size_t) h * 4u);
        std::uint8_t* dst[4] = {out->bgra.data(), nullptr, nullptr, nullptr};
        const int dstStride[4] = {w * 4, 0, 0, 0};
        sws_scale(d.sws, src->data, src->linesize, 0, h, dst, dstStride);
        const juce::ScopedLock sl(lock_);
        current_ = std::move(out);
    }

    void run() override {
        Decoder d;
        if (!open(d)) return;
        length_.store(d.span);
        double position = 0.0;
        double lastPresented = -1.0;
        auto lastMs = juce::Time::getMillisecondCounterHiRes();
        pendingValid_ = false;
        atEof_ = false;
        double pendingPts = 0.0;

        while (!threadShouldExit()) {
            const auto nowMs = juce::Time::getMillisecondCounterHiRes();
            bool jumped = false;
            if (rewind_.exchange(false)) {
                position = 0.0;
                jumped = true;
            }
            if (const double target = seekTo_.exchange(-1.0); target >= 0.0) {
                position = d.span > 0.0 ? std::min(target, d.span) : target;
                jumped = true;
            }
            const float rate = rate_.load();
            if (!paused_.load() && rate > 0.0f)
                position += (nowMs - lastMs) * 0.001 * (double) rate;
            lastMs = nowMs;
            if (d.span > 0.0 && position >= d.span) {
                position -= d.span * std::floor(position / d.span);
                jumped = true;
            }
            shownPosition_.store(position);

            if (jumped || (lastPresented >= 0.0 && position < lastPresented - 0.05)) {
                seekDecoder(d, position);
                lastPresented = -1.0;
            }

            if (!pendingValid_) {
                double pts = 0.0;
                if (decodeNext(d, pts)) {
                    pendingPts = pts < 0.0 ? position : pts;
                    pendingValid_ = true;
                } else if (atEof_) {
                    if (d.span <= 0.0) d.span = std::max(lastPresented, 0.0);
                    seekDecoder(d, 0.0);
                    position = 0.0;
                    lastPresented = -1.0;
                    wait(2);
                    continue;
                }
            }
            if (pendingValid_ && pendingPts <= position + 0.001) {
                if (pendingPts >= position - 0.5 || lastPresented < 0.0) {
                    present(d);
                    lastPresented = pendingPts;
                }
                pendingValid_ = false;
                continue;
            }
            wait(2);
        }
    }

    juce::String path_;
    std::atomic<float> rate_{1.0f};
    std::atomic<bool> rewind_{false};
    std::atomic<bool> paused_{false};
    std::atomic<double> seekTo_{-1.0};
    std::atomic<double> shownPosition_{0.0};
    std::atomic<double> length_{0.0};
    bool pendingValid_ = false;
    bool atEof_ = false;
    juce::CriticalSection lock_;
    std::shared_ptr<const Frame> current_;
};

}

std::unique_ptr<VideoLayer> VideoLayer::createPlatform() {
    return std::make_unique<FfmpegVideoLayer>();
}

}
