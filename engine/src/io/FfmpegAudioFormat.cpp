// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/FfmpegAudioFormat.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "hum/dsp/SoundFileBuffer.h"

#if HUM_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}
#endif

namespace hum {

namespace {

const char* const kSystemWillNotOpen =
    "*.mkv;*.webm;*.wmv;*.asf;*.flv;*.ogv;*.mpg;*.mpeg;*.ts;*.vob";

const char* const kContainers = "*.mkv;*.webm;*.avi;*.wmv;*.asf;*.flv;*.ogv;*.mpg;*.mpeg;*.ts;*.vob;*.m4v;*.mp4;*.mov";

}

std::string FfmpegAudioFormat::extraContainers() { return kSystemWillNotOpen; }

juce::StringArray FfmpegAudioFormat::containers() {
    return juce::StringArray::fromTokens(juce::String(kContainers).replace("*", ""), ";", "");
}

#if !HUM_FFMPEG

FfmpegAudioFormat::FfmpegAudioFormat(int track)
    : juce::AudioFormat("Video sound", juce::StringArray()), track_(track) {}
bool FfmpegAudioFormat::available() { return false; }
std::vector<std::string> FfmpegAudioFormat::audioTrackNames(const juce::File&) { return {}; }
juce::AudioFormatReader* FfmpegAudioFormat::createReaderFor(juce::InputStream* stream, bool deleteIfFails) {
    if (deleteIfFails) delete stream;
    return nullptr;
}
void installExtraSoundFormats() {}

#else

namespace {

constexpr int kIoBuffer = 32768;

std::string trackName(int ordinal, const AVStream* stream) {
    std::string name = std::to_string(ordinal);
    const auto* language = av_dict_get(stream->metadata, "language", nullptr, 0);
    const auto* title = av_dict_get(stream->metadata, "title", nullptr, 0);
    if (language != nullptr) name += " " + std::string(language->value);   // utf8-ok: data
    if (title != nullptr) name += " " + std::string(title->value);         // utf8-ok: data
    if (const auto* codec = avcodec_get_name(stream->codecpar->codec_id))
        name += " " + std::string(codec);
    const int channels = stream->codecpar->ch_layout.nb_channels;
    if (channels > 0) name += " " + std::to_string(channels) + "ch";
    return name;
}

int readStream(void* opaque, std::uint8_t* buf, int size) {
    auto* in = static_cast<juce::InputStream*>(opaque);
    const int got = in->read(buf, size);
    return got > 0 ? got : AVERROR_EOF;
}

std::int64_t seekStream(void* opaque, std::int64_t offset, int whence) {
    auto* in = static_cast<juce::InputStream*>(opaque);
    if (whence == AVSEEK_SIZE) return in->getTotalLength();
    std::int64_t want = offset;
    if (whence == SEEK_CUR) want += in->getPosition();
    else if (whence == SEEK_END) want += in->getTotalLength();
    return in->setPosition(want) ? want : -1;
}

class FfmpegReader : public juce::AudioFormatReader {
public:
    FfmpegReader(juce::InputStream* in, const juce::String& name, int track)
        : juce::AudioFormatReader(in, name), track_(track) {
        usesFloatingPointData = true;
        bitsPerSample = 32;
    }

    ~FfmpegReader() override { closeAll(); }

    int trackStream(int wanted) const {
        if (wanted <= 0) return av_find_best_stream(format_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        int seen = 0;
        for (unsigned i = 0; i < format_->nb_streams; ++i) {
            if (format_->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
            if (seen == wanted) return (int) i;
            ++seen;
        }
        return av_find_best_stream(format_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    }

    bool open() {
        io_ = (std::uint8_t*) av_malloc(kIoBuffer);
        if (io_ == nullptr) return false;
        avio_ = avio_alloc_context(io_, kIoBuffer, 0, input, &readStream, nullptr, &seekStream);
        if (avio_ == nullptr) return false;
        format_ = avformat_alloc_context();
        if (format_ == nullptr) return false;
        format_->pb = avio_;
        format_->flags |= AVFMT_FLAG_CUSTOM_IO;
        if (avformat_open_input(&format_, "", nullptr, nullptr) < 0) {
            format_ = nullptr;
            return false;
        }
        if (avformat_find_stream_info(format_, nullptr) < 0) return false;
        stream_ = trackStream(track_);
        if (stream_ < 0) return false;
        auto* par = format_->streams[stream_]->codecpar;
        const auto* codec = avcodec_find_decoder(par->codec_id);
        if (codec == nullptr) return false;
        codec_ = avcodec_alloc_context3(codec);
        if (codec_ == nullptr || avcodec_parameters_to_context(codec_, par) < 0) return false;
        if (avcodec_open2(codec_, codec, nullptr) < 0) return false;

        numChannels = (unsigned int) videoChannels(codec_->ch_layout.nb_channels);
        sampleRate = codec_->sample_rate > 0 ? (double) codec_->sample_rate : kDefaultSampleRate;
        const auto* st = format_->streams[stream_];
        const double seconds = st->duration > 0
            ? (double) st->duration * av_q2d(st->time_base)
            : (format_->duration > 0 ? (double) format_->duration / (double) AV_TIME_BASE : 0.0);
        lengthInSamples = (juce::int64) (seconds * sampleRate);

        AVChannelLayout out;
        av_channel_layout_default(&out, (int) numChannels);
        if (swr_alloc_set_opts2(&swr_, &out, AV_SAMPLE_FMT_FLTP, codec_->sample_rate,
                                &codec_->ch_layout, codec_->sample_fmt, codec_->sample_rate,
                                0, nullptr) < 0)
            return false;
        av_channel_layout_uninit(&out);
        if (swr_init(swr_) < 0) return false;

        frame_ = av_frame_alloc();
        packet_ = av_packet_alloc();
        return frame_ != nullptr && packet_ != nullptr && lengthInSamples > 0;
    }

    bool readSamples(int* const* destChannels, int numDestChannels, int startOffsetInDestBuffer,
                     juce::int64 startSampleInFile, int numSamples) override {
        if (startSampleInFile != at_ && !seekTo(startSampleInFile)) return false;
        while (numSamples > 0) {
            if (held_.empty() && !decodeMore()) {
                for (int c = 0; c < numDestChannels; ++c)
                    if (destChannels[c] != nullptr)
                        juce::FloatVectorOperations::clear((float*) destChannels[c] + startOffsetInDestBuffer,
                                                           numSamples);
                at_ += numSamples;
                return true;
            }
            const int take = std::min(numSamples, heldFrames());
            for (int c = 0; c < numDestChannels; ++c) {
                auto* dest = (float*) destChannels[c];
                if (dest == nullptr) continue;
                const int src = std::min(c, (int) numChannels - 1);
                std::copy(held_[(size_t) src].begin() + heldAt_,
                          held_[(size_t) src].begin() + heldAt_ + take,
                          dest + startOffsetInDestBuffer);
            }
            heldAt_ += take;
            startOffsetInDestBuffer += take;
            numSamples -= take;
            at_ += take;
            if (heldAt_ >= (int) held_[0].size()) held_.clear();
        }
        return true;
    }

private:
    int heldFrames() const { return held_.empty() ? 0 : (int) held_[0].size() - heldAt_; }

    bool seekTo(juce::int64 sample) {
        const auto* st = format_->streams[stream_];
        const auto ts = (std::int64_t) ((double) sample / sampleRate / av_q2d(st->time_base));
        if (av_seek_frame(format_, stream_, ts, AVSEEK_FLAG_BACKWARD) < 0) return false;
        avcodec_flush_buffers(codec_);
        held_.clear();
        heldAt_ = 0;
        at_ = sample;
        return true;
    }

    bool decodeMore() {
        held_.clear();
        heldAt_ = 0;
        for (;;) {
            int got = avcodec_receive_frame(codec_, frame_);
            if (got == 0) return convert();
            if (got != AVERROR(EAGAIN)) return false;
            if (av_read_frame(format_, packet_) < 0) return false;
            const int index = packet_->stream_index;
            const int sent = index == stream_ ? avcodec_send_packet(codec_, packet_) : 0;
            av_packet_unref(packet_);
            if (index == stream_ && sent < 0) return false;
        }
    }

    bool convert() {
        const int frames = frame_->nb_samples;
        if (frames <= 0) return false;
        held_.assign((size_t) numChannels, std::vector<float>((size_t) frames));
        std::vector<std::uint8_t*> planes((size_t) numChannels);
        for (unsigned c = 0; c < numChannels; ++c)
            planes[c] = (std::uint8_t*) held_[c].data();
        const int done = swr_convert(swr_, planes.data(), frames,
                                     (const std::uint8_t**) frame_->extended_data, frames);
        av_frame_unref(frame_);
        if (done <= 0) { held_.clear(); return false; }
        for (auto& ch : held_) ch.resize((size_t) done);
        return true;
    }

    void closeAll() {
        if (frame_ != nullptr) av_frame_free(&frame_);
        if (packet_ != nullptr) av_packet_free(&packet_);
        if (swr_ != nullptr) swr_free(&swr_);
        if (codec_ != nullptr) avcodec_free_context(&codec_);
        if (format_ != nullptr) avformat_close_input(&format_);
        else if (avio_ != nullptr) av_freep(&avio_->buffer);
        if (avio_ != nullptr) avio_context_free(&avio_);
    }

    AVFormatContext* format_ = nullptr;
    AVIOContext* avio_ = nullptr;
    std::uint8_t* io_ = nullptr;
    AVCodecContext* codec_ = nullptr;
    SwrContext* swr_ = nullptr;
    const int track_ = 0;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    int stream_ = -1;
    juce::int64 at_ = 0;
    std::vector<std::vector<float>> held_;
    int heldAt_ = 0;
};

}

FfmpegAudioFormat::FfmpegAudioFormat(int track)
    : juce::AudioFormat("Video sound", containers()), track_(track) {}

bool FfmpegAudioFormat::available() { return true; }

juce::AudioFormatReader* FfmpegAudioFormat::createReaderFor(juce::InputStream* stream,
                                                            bool deleteStreamIfOpeningFails) {
    auto reader = std::make_unique<FfmpegReader>(stream, "Video sound", track_);
    if (reader->open()) return reader.release();
    if (!deleteStreamIfOpeningFails) reader->input = nullptr;
    return nullptr;
}

std::vector<std::string> FfmpegAudioFormat::audioTrackNames(const juce::File& file) {
    std::vector<std::string> names;
    if (!file.existsAsFile()) return names;
    AVFormatContext* format = nullptr;
    const auto path = file.getFullPathName().toStdString();
    if (avformat_open_input(&format, path.c_str(), nullptr, nullptr) < 0) return names;
    if (avformat_find_stream_info(format, nullptr) >= 0)
        for (unsigned i = 0; i < format->nb_streams; ++i) {
            const auto* stream = format->streams[i];
            if (stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
            names.push_back(trackName((int) names.size() + 1, stream));
        }
    avformat_close_input(&format);
    return names;
}

void installExtraSoundFormats() {
    av_log_set_level(AV_LOG_ERROR);
    extraSoundFormats() = [](int track) {
        std::vector<std::unique_ptr<juce::AudioFormat>> out;
        out.push_back(std::make_unique<FfmpegAudioFormat>(track));
        return out;
    };
    soundFileTrackNames() = [](const std::string& uri) {
        return FfmpegAudioFormat::audioTrackNames(
            juce::File(juce::String(juce::CharPointer_UTF8(uri.c_str()))));
    };
}

#endif

}
