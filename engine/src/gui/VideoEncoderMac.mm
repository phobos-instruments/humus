#include "gui/VideoEncoderNative.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "gui/MovieSound.h"

namespace hum {

namespace {

constexpr int kSoundBlock = 1024;
constexpr double kSoundLead = 1.0;
constexpr juce::uint32 kPatienceMs = 30000;
constexpr int kTimeScale = 90000;
constexpr int kSealPatience = 60000;

class MacMovie : public VideoEncoder {
public:
    MacMovie(const juce::File& file, int width, int height, double fps, int quality, bool live)
        : width_(width & ~1), height_(height & ~1), fps_(fps > 0.0 ? fps : 30.0), live_(live) {
        if (width_ < 16 || height_ < 16) return;
        file.deleteFile();
        NSString* path = [NSString stringWithUTF8String:file.getFullPathName().toRawUTF8()];
        NSURL* url = [NSURL fileURLWithPath:path];
        NSError* trouble = nil;
        writer_ = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeMPEG4 error:&trouble];
        if (writer_ == nil) return;
        if (!openPicture(quality)) return;
        ready_ = true;
    }

    ~MacMovie() override {
        close();
        [pool_ release];
        [picture_ release];
        [soundIn_ release];
        [writer_ release];
        if (soundShape_ != nullptr) CFRelease(soundShape_);
    }

    bool ok() const override { return ready_; }
    int frameCount() const override { return frames_; }

    bool openSound(int channels, double sampleRate) override {
        if (!ready_ || started_ || channels <= 0 || sampleRate <= 0.0) return false;
        channels_ = std::min(channels, 2);
        rate_ = sampleRate;
        AudioChannelLayout layout = {};
        layout.mChannelLayoutTag = channels_ == 1 ? kAudioChannelLayoutTag_Mono
                                                  : kAudioChannelLayoutTag_Stereo;
        NSData* shape = [NSData dataWithBytes:&layout length:sizeof(layout)];
        NSDictionary* settings = @{
            AVFormatIDKey : @(kAudioFormatMPEG4AAC),
            AVSampleRateKey : @(rate_),
            AVNumberOfChannelsKey : @(channels_),
            AVEncoderBitRateKey : @(channels_ > 1 ? 256000 : 128000),
            AVChannelLayoutKey : shape
        };
        soundIn_ = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio
                                                  outputSettings:settings];
        soundIn_.expectsMediaDataInRealTime = NO;
        if (![writer_ canAddInput:soundIn_]) return false;
        [writer_ addInput:soundIn_];
        return describeSound();
    }

    bool addSound(const float* const* channels, int count, int frames) override {
        if (soundIn_ == nil || count <= 0 || frames <= 0) return false;
        hold_.take(channels, count, frames, channels_);
        return true;
    }

    bool addFrame(const std::uint8_t* rgba) override {
        @autoreleasepool {
            return takeFrame(rgba);
        }
    }

    bool close() override {
        @autoreleasepool {
            return seal();
        }
    }

private:
    bool takeFrame(const std::uint8_t* rgba) {
        if (!ready_ || rgba == nullptr) return false;
        if (!started_ && !start()) return false;
        CVPixelBufferRef buffer = nullptr;
        if (pool_.pixelBufferPool == nullptr
            || CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pool_.pixelBufferPool,
                                                 &buffer) != kCVReturnSuccess)
            return false;
        CVPixelBufferLockBaseAddress(buffer, 0);
        auto* out = (std::uint8_t*) CVPixelBufferGetBaseAddress(buffer);
        const size_t stride = CVPixelBufferGetBytesPerRow(buffer);
        for (int y = 0; y < height_; ++y) {
            const std::uint8_t* src = rgba + (size_t) y * (size_t) width_ * 4u;
            std::uint8_t* dst = out + (size_t) y * stride;
            for (int x = 0; x < width_; ++x) {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                dst[3] = 255;
                src += 4;
                dst += 4;
            }
        }
        CVPixelBufferUnlockBaseAddress(buffer, 0);
        const auto when = CMTimeMake((std::int64_t) std::llround((double) frames_ / fps_
                                                                * (double) kTimeScale),
                                     kTimeScale);
        const bool took = feedSound((double) frames_ / fps_ + kSoundLead) >= 0 && awaitPicture()
                          && [pool_ appendPixelBuffer:buffer withPresentationTime:when];
        CVPixelBufferRelease(buffer);
        if (!took) return false;
        ++frames_;
        return feedSound((double) frames_ / fps_ + kSoundLead) >= 0;
    }

    bool seal() {
        if (!ready_) return false;
        ready_ = false;
        bool wrote = frames_ > 0;
        if (started_) {
            [picture_ markAsFinished];
            if (!drainSound()) wrote = false;
            finishSound();
            juce::WaitableEvent done;
            auto* sealed = &done;
            [writer_ finishWritingWithCompletionHandler:^{ sealed->signal(); }];
            wrote = done.wait(kSealPatience) && writer_.status == AVAssetWriterStatusCompleted
                    && wrote;
        }
        hold_.clear();
        return wrote;
    }

    bool openPicture(int quality) {
        const auto bits = (NSInteger) std::llround(movieBitsPerSecond(width_, height_, fps_,
                                                                     quality));
        NSDictionary* press = @{
            AVVideoAverageBitRateKey : @(bits),
            AVVideoMaxKeyFrameIntervalKey : @(keyframeInterval(fps_, live_)),
            AVVideoExpectedSourceFrameRateKey : @((NSInteger) std::llround(fps_)),
            AVVideoProfileLevelKey : AVVideoProfileLevelH264HighAutoLevel,
            AVVideoAllowFrameReorderingKey : @YES
        };
        NSDictionary* settings = @{
            AVVideoCodecKey : AVVideoCodecTypeH264,
            AVVideoWidthKey : @(width_),
            AVVideoHeightKey : @(height_),
            AVVideoCompressionPropertiesKey : press
        };
        picture_ = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo
                                                  outputSettings:settings];
        picture_.expectsMediaDataInRealTime = live_ ? YES : NO;
        NSDictionary* source = @{
            (id) kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
            (id) kCVPixelBufferWidthKey : @(width_),
            (id) kCVPixelBufferHeightKey : @(height_),
            (id) kCVPixelBufferIOSurfacePropertiesKey : @{}
        };
        pool_ = [[AVAssetWriterInputPixelBufferAdaptor alloc]
            initWithAssetWriterInput:picture_
         sourcePixelBufferAttributes:source];
        if (![writer_ canAddInput:picture_]) return false;
        [writer_ addInput:picture_];
        return true;
    }

    bool describeSound() {
        AudioStreamBasicDescription form = {};
        form.mSampleRate = rate_;
        form.mFormatID = kAudioFormatLinearPCM;
        form.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        form.mBitsPerChannel = 32;
        form.mChannelsPerFrame = (UInt32) channels_;
        form.mFramesPerPacket = 1;
        form.mBytesPerFrame = 4 * (UInt32) channels_;
        form.mBytesPerPacket = form.mBytesPerFrame;
        return CMAudioFormatDescriptionCreate(kCFAllocatorDefault, &form, 0, nullptr, 0, nullptr,
                                              nullptr, &soundShape_) == noErr;
    }

    bool start() {
        if (![writer_ startWriting]) return false;
        [writer_ startSessionAtSourceTime:kCMTimeZero];
        started_ = true;
        return true;
    }

    bool awaitPicture() {
        const auto until = juce::Time::getMillisecondCounter() + kPatienceMs;
        while (!picture_.isReadyForMoreMediaData) {
            if (writer_.status == AVAssetWriterStatusFailed) return false;
            if (juce::Time::getMillisecondCounter() > until) return false;
            const int fed = feedSound(1.0e9);
            if (fed < 0) return false;
            if (fed == 0) [NSThread sleepForTimeInterval:0.002];
        }
        return writer_.status == AVAssetWriterStatusWriting;
    }

    bool drainSound() {
        if (soundIn_ == nil) return true;
        const auto until = juce::Time::getMillisecondCounter() + kPatienceMs;
        while (!hold_.empty() && hold_.sent() < hold_.total()) {
            if (writer_.status == AVAssetWriterStatusFailed) return false;
            if (juce::Time::getMillisecondCounter() > until) return false;
            const int fed = feedSound(1.0e9);
            if (fed < 0) return false;
            if (fed == 0) [NSThread sleepForTimeInterval:0.002];
        }
        return true;
    }

    int feedSound(double upTo) {
        if (soundIn_ == nil || hold_.empty() || soundShape_ == nullptr) return 0;
        int fed = 0;
        for (int take = hold_.ready(kSoundBlock, upTo, rate_);
             take > 0 && soundIn_.isReadyForMoreMediaData;
             take = hold_.ready(kSoundBlock, upTo, rate_)) {
            if (writer_.status == AVAssetWriterStatusFailed) return -1;
            packed_.resize((size_t) take * (size_t) channels_);
            for (int i = 0; i < take; ++i)
                for (int c = 0; c < channels_; ++c)
                    packed_[(size_t) i * (size_t) channels_ + (size_t) c]
                        = hold_.at(c, hold_.sent() + i);
            if (!appendSound(take)) return -1;
            hold_.advance(take);
            fed += take;
        }
        if (hold_.sent() >= hold_.total()) finishSound();
        return fed;
    }

    void finishSound() {
        if (soundIn_ == nil || soundDone_) return;
        soundDone_ = true;
        [soundIn_ markAsFinished];
    }

    bool appendSound(int frames) {
        const size_t bytes = packed_.size() * sizeof(float);
        CMBlockBufferRef block = nullptr;
        if (CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, nullptr, bytes,
                                               kCFAllocatorDefault, nullptr, 0, bytes,
                                               kCMBlockBufferAssureMemoryNowFlag, &block) != noErr)
            return false;
        bool sent = CMBlockBufferReplaceDataBytes(packed_.data(), block, 0, bytes) == noErr;
        CMSampleBufferRef sample = nullptr;
        if (sent) {
            CMSampleTimingInfo timing = {};
            timing.duration = CMTimeMake(1, (std::int32_t) std::llround(rate_));
            timing.presentationTimeStamp = CMTimeMake(hold_.sent(),
                                                      (std::int32_t) std::llround(rate_));
            timing.decodeTimeStamp = kCMTimeInvalid;
            sent = CMSampleBufferCreate(kCFAllocatorDefault, block, TRUE, nullptr, nullptr,
                                        soundShape_, frames, 1, &timing, 0, nullptr, &sample)
                   == noErr;
        }
        if (sent) sent = [soundIn_ appendSampleBuffer:sample];
        if (sample != nullptr) CFRelease(sample);
        CFRelease(block);
        return sent;
    }

    int width_ = 0, height_ = 0, frames_ = 0, channels_ = 2;
    double fps_ = 30.0, rate_ = 48000.0;
    bool ready_ = false, started_ = false, soundDone_ = false, live_ = false;
    AVAssetWriter* writer_ = nil;
    AVAssetWriterInput* picture_ = nil;
    AVAssetWriterInput* soundIn_ = nil;
    AVAssetWriterInputPixelBufferAdaptor* pool_ = nil;
    CMAudioFormatDescriptionRef soundShape_ = nullptr;
    SoundHold hold_;
    std::vector<float> packed_;
};

}

std::unique_ptr<VideoEncoder> makeNativeMovieWriter(const juce::File& file, int width, int height,
                                                    double fps, int quality, bool live) {
    return std::make_unique<MacMovie>(file, width, height, fps, quality, live);
}

}
