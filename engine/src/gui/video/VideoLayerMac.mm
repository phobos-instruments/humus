// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
// macOS video decode for the Lumen window (see VideoLayer.h): AVPlayer +
// AVPlayerItemVideoOutput, muted, hardware-decoded, looping via an
// end-of-item observer. Manual retain/release (JUCE builds .mm without ARC).
#include "gui/video/VideoLayer.h"

#if JUCE_MAC

#include <cmath>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <cstdlib>

namespace hum {
namespace {

bool copyRows(CVPixelBufferRef px, int w, int h, std::vector<unsigned char>& out) {
    if (w <= 0 || h <= 0) return false;
    CVPixelBufferLockBaseAddress(px, kCVPixelBufferLock_ReadOnly);
    const auto stride = (size_t) CVPixelBufferGetBytesPerRow(px);
    const auto* src = (const unsigned char*) CVPixelBufferGetBaseAddress(px);
    if (src != nullptr) {
        const size_t rowBytes = (size_t) w * 4;
        out.resize(rowBytes * (size_t) h);
        for (int y = 0; y < h; ++y)
            memcpy(out.data() + rowBytes * (size_t) y, src + stride * (size_t) y, rowBytes);
    }
    CVPixelBufferUnlockBaseAddress(px, kCVPixelBufferLock_ReadOnly);
    return src != nullptr;
}

std::shared_ptr<const VideoLayer::Frame> frameOfPixelBuffer(CVPixelBufferRef px, double pts) {
    auto frame = std::make_shared<VideoLayer::Frame>();
    frame->width = (int) CVPixelBufferGetWidth(px);
    frame->height = (int) CVPixelBufferGetHeight(px);
    frame->pts = pts;
    static const bool forceUpload = getenv("HUMUS_GL_UPLOAD") != nullptr;
    const bool surfaceBacked = !forceUpload && CVPixelBufferGetIOSurface(px) != nullptr;
    if (!surfaceBacked) copyRows(px, frame->width, frame->height, frame->bgra);
    if (frame->width <= 0 || frame->height <= 0 || (!surfaceBacked && frame->bgra.empty())) {
        CVBufferRelease(px);
        return nullptr;
    }
    frame->native = px;
    frame->nativeHold = std::shared_ptr<const void>(
        (const void*) px, [](const void* p) { CVBufferRelease((CVPixelBufferRef) p); });
    if (surfaceBacked) {
        const int w = frame->width, h = frame->height;
        frame->readPixels = [px, w, h](std::vector<unsigned char>& out) {
            return copyRows(px, w, h, out);
        };
    }
    return frame;
}

class AvReaderLayer : public VideoLayer {
public:
    ~AvReaderLayer() override { unload(); }

    void load(const juce::String& path) override {
        @autoreleasepool {
            unload();
            NSString* p = [NSString stringWithUTF8String:path.toRawUTF8()];
            if (p == nil || path.isEmpty()) return;
            asset_ = [[AVURLAsset alloc] initWithURL:[NSURL fileURLWithPath:p] options:nil];
            dispatch_semaphore_t done = dispatch_semaphore_create(0);
            [asset_ loadValuesAsynchronouslyForKeys:@[ @"tracks", @"duration" ]
                                  completionHandler:^{ dispatch_semaphore_signal(done); }];
            dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW,
                                                        (int64_t) (10 * NSEC_PER_SEC)));
            dispatch_release(done);
            AVAssetTrack* track = [[asset_ tracksWithMediaType:AVMediaTypeVideo] firstObject];
            if (track == nil) {
                unload();
                return;
            }
            track_ = [track retain];
            const CMTime d = asset_.duration;
            length_ = CMTIME_IS_NUMERIC(d) ? CMTimeGetSeconds(d) : 0.0;
            const float fps = track_.nominalFrameRate;
            step_ = fps > 1.0f ? 1.0 / (double) fps : 1.0 / 30.0;
        }
    }

    void setRate(float) override {}
    void restart() override { decodeTo(loopIn_); }
    void setPaused(bool) override {}
    bool isPaused() const override { return true; }
    double positionSeconds() override { return current_ != nullptr ? current_->pts : 0.0; }
    double lengthSeconds() override { return length_; }
    void seekSeconds(double t) override { decodeTo(t); }
    void setLoopRange(const LoopRange& r) override { loopIn_ = std::max(0.0, r.in); }
    void chase(double seconds, double) override { decodeTo(seconds); }
    std::shared_ptr<const Frame> latestFrame() override { return current_; }

private:
    void decodeTo(double want) {
        if (track_ == nil) return;
        const double t = std::max(0.0, want);
        if (reader_ == nil || t + kAhead < readAt_ || t - readAt_ > kFarJump) openReader(t);
        if (reader_ == nil) return;
        for (;;) {
            if (pending_ == nullptr && !nextSample()) return;
            if (pendingPts_ > t + kAhead) return;
            adopt();
        }
    }

    bool nextSample() {
        if (reader_.status != AVAssetReaderStatusReading) return false;
        CMSampleBufferRef sb = [output_ copyNextSampleBuffer];
        if (sb == nullptr) return false;
        pending_ = sb;
        pendingPts_ = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sb));
        return true;
    }

    void adopt() {
        if (CVImageBufferRef px = CMSampleBufferGetImageBuffer(pending_); px != nullptr) {
            CVBufferRetain(px);
            if (auto f = frameOfPixelBuffer(px, pendingPts_)) current_ = std::move(f);
        }
        readAt_ = pendingPts_;
        CFRelease(pending_);
        pending_ = nullptr;
    }

    void openReader(double t) {
        @autoreleasepool {
            closeReader();
            NSError* err = nil;
            reader_ = [[AVAssetReader alloc] initWithAsset:asset_ error:&err];
            if (reader_ == nil) return;
            NSDictionary* settings = @{
                (id) kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
            };
            output_ = [[AVAssetReaderTrackOutput alloc] initWithTrack:track_
                                                        outputSettings:settings];
            output_.alwaysCopiesSampleData = NO;
            if (![reader_ canAddOutput:output_]) {
                closeReader();
                return;
            }
            [reader_ addOutput:output_];
            const double from = std::max(0.0, t - 2.0 * step_);
            reader_.timeRange = CMTimeRangeMake(CMTimeMakeWithSeconds(from, 600),
                                                kCMTimePositiveInfinity);
            if (![reader_ startReading]) {
                closeReader();
                return;
            }
            readAt_ = from;
        }
    }

    void closeReader() {
        if (pending_ != nullptr) {
            CFRelease(pending_);
            pending_ = nullptr;
        }
        if (reader_ != nil) {
            [reader_ cancelReading];
            [reader_ release];
            reader_ = nil;
        }
        if (output_ != nil) {
            [output_ release];
            output_ = nil;
        }
        readAt_ = -1.0;
    }

    void unload() {
        closeReader();
        current_.reset();
        if (track_ != nil) {
            [track_ release];
            track_ = nil;
        }
        if (asset_ != nil) {
            [asset_ release];
            asset_ = nil;
        }
        length_ = 0.0;
    }

    static constexpr double kFarJump = 1.0, kAhead = 0.001;

    AVURLAsset* asset_ = nil;
    AVAssetTrack* track_ = nil;
    AVAssetReader* reader_ = nil;
    AVAssetReaderTrackOutput* output_ = nil;
    CMSampleBufferRef pending_ = nullptr;
    double pendingPts_ = -1.0, readAt_ = -1.0, length_ = 0.0, step_ = 1.0 / 30.0;
    double loopIn_ = 0.0;
    std::shared_ptr<const Frame> current_;
};

class AvVideoLayer : public VideoLayer {
public:
    AvVideoLayer() {
        player_ = [[AVPlayer alloc] init];
        player_.muted = YES;   // audio belongs to the patch, not the projector
        player_.actionAtItemEnd = AVPlayerActionAtItemEndNone;
        NSDictionary* attrs = @{
            (id) kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
        };
        output_ = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:attrs];
    }

    ~AvVideoLayer() override {
        unload();
        [player_ pause];
        [output_ release];
        [player_ release];
    }

    void load(const juce::String& path) override {
        unload();
        NSString* p = [NSString stringWithUTF8String:path.toRawUTF8()];
        if (p == nil || path.isEmpty()) return;
        NSURL* url = [NSURL fileURLWithPath:p];
        item_ = [[AVPlayerItem alloc] initWithURL:url];
        [item_ addOutput:output_];
        [player_ replaceCurrentItemWithPlayerItem:item_];
        // Loop: on end, rewind and restore the rate (actionAtItemEndNone left
        // the player "playing" at position end). The block retains player_
        // and item_; the observer is removed before they are released.
        AVPlayer* player = player_;
        AVPlayerItem* item = item_;
        AvVideoLayer* owner = this;
        endToken_ = [[[NSNotificationCenter defaultCenter]
            addObserverForName:AVPlayerItemDidPlayToEndTimeNotification
                        object:item_
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification*) {
                        if (!owner->loop_) return;
                        const float r = player.rate > 0.0f ? player.rate : 1.0f;
                        [item seekToTime:CMTimeMakeWithSeconds(owner->loopIn_, 600)
                            toleranceBefore:kCMTimeZero
                             toleranceAfter:kCMTimeZero
                          completionHandler:^(BOOL) { player.rate = r; }];
                    }] retain];
        applyLoopRange();
        if (!paused_) player_.rate = rate_;
    }

    void setLoopRange(const LoopRange& r) override {
        loopIn_ = std::max(0.0, r.in);
        loopOut_ = r.out;
        loop_ = r.loop;
        applyLoopRange();
    }

    void applyLoopRange() {
        if (item_ == nil) return;
        item_.forwardPlaybackEndTime = loopOut_ > loopIn_
            ? CMTimeMakeWithSeconds(loopOut_, 600) : kCMTimeInvalid;
    }

    void setRate(float rate) override {
        rate_ = rate;
        if (item_ != nil && !paused_) player_.rate = rate;
    }

    void setPaused(bool paused) override {
        if (paused == paused_) return;
        paused_ = paused;
        if (item_ == nil) return;
        if (paused) [player_ pause];
        else player_.rate = rate_;
    }

    bool isPaused() const override { return paused_; }

    double positionSeconds() override {
        if (item_ == nil) return 0.0;
        const auto t = [item_ currentTime];
        return CMTIME_IS_NUMERIC(t) ? CMTimeGetSeconds(t) : 0.0;
    }

    double lengthSeconds() override {
        if (item_ == nil) return 0.0;
        const auto d = item_.duration;
        return CMTIME_IS_NUMERIC(d) ? CMTimeGetSeconds(d) : 0.0;
    }

    void seekSeconds(double t) override {
        if (item_ == nil) return;
        [item_ seekToTime:CMTimeMakeWithSeconds(t, 600)
            toleranceBefore:kCMTimeZero
             toleranceAfter:kCMTimeZero
          completionHandler:nil];
    }

    void chase(double seconds, double rate) override {
        if (item_ == nil) return;
        if (std::abs(positionSeconds() - seconds) > kChaseSlack)
            [item_ seekToTime:CMTimeMakeWithSeconds(seconds, 600)
                toleranceBefore:CMTimeMakeWithSeconds(kChaseSlack * 0.5, 600)
                 toleranceAfter:CMTimeMakeWithSeconds(kChaseSlack * 0.5, 600)
              completionHandler:nil];
        player_.rate = (float) rate;
    }

    void restart() override {
        if (item_ == nil) return;
        [item_ seekToTime:CMTimeMakeWithSeconds(loopIn_, 600)
            toleranceBefore:kCMTimeZero
             toleranceAfter:kCMTimeZero
          completionHandler:nil];
        player_.rate = rate_ > 0.0f ? rate_ : 1.0f;
    }

    std::shared_ptr<const Frame> latestFrame() override {
        if (item_ == nil) return current_;
        const CMTime t = [item_ currentTime];
        if (![output_ hasNewPixelBufferForItemTime:t]) return current_;
        CMTime shown = kCMTimeInvalid;
        CVPixelBufferRef px = [output_ copyPixelBufferForItemTime:t
                                                itemTimeForDisplay:&shown];
        if (px == nullptr) return current_;
        if (auto f = frameOfPixelBuffer(px, CMTimeGetSeconds(CMTIME_IS_NUMERIC(shown) ? shown : t)))
            current_ = std::move(f);
        return current_;
    }

private:
    void unload() {
        current_.reset();
        if (endToken_ != nil) {
            [[NSNotificationCenter defaultCenter] removeObserver:endToken_];
            [endToken_ release];
            endToken_ = nil;
        }
        if (item_ != nil) {
            [player_ pause];
            [item_ removeOutput:output_];
            [player_ replaceCurrentItemWithPlayerItem:nil];
            [item_ release];
            item_ = nil;
        }
    }

    AVPlayer* player_ = nil;
    AVPlayerItemVideoOutput* output_ = nil;
    AVPlayerItem* item_ = nil;
    id endToken_ = nil;
    static constexpr double kChaseSlack = 0.04;
    float rate_ = 1.0f;
    bool paused_ = false;
    double loopIn_ = 0.0;
    double loopOut_ = 0.0;
    bool loop_ = true;
    std::shared_ptr<const Frame> current_;
};

}  // namespace

std::unique_ptr<VideoLayer> VideoLayer::createPlatform() {
    return std::make_unique<AvVideoLayer>();
}

std::unique_ptr<VideoLayer> VideoLayer::createOffline() {
    return std::make_unique<AvReaderLayer>();
}

double VideoLayer::probeLengthSeconds(const juce::File& file) {
    NSString* p = [NSString stringWithUTF8String:file.getFullPathName().toRawUTF8()];
    if (p == nil) return 0.0;
    AVURLAsset* asset = [AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:p] options:nil];
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    __block double seconds = 0.0;
    [asset loadValuesAsynchronouslyForKeys:@[ @"duration" ] completionHandler:^{
        NSError* err = nil;
        if ([asset statusOfValueForKey:@"duration" error:&err] == AVKeyValueStatusLoaded) {
            const CMTime d = asset.duration;
            if (CMTIME_IS_NUMERIC(d)) seconds = CMTimeGetSeconds(d);
        }
        dispatch_semaphore_signal(done);
    }];
    dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, (int64_t) (5 * NSEC_PER_SEC)));
    dispatch_release(done);
    return seconds > 0.0 ? seconds : 0.0;
}

}  // namespace hum

#endif  // JUCE_MAC
