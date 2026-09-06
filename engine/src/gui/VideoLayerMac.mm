// macOS video decode for the Lumen window (see VideoLayer.h): AVPlayer +
// AVPlayerItemVideoOutput, muted, hardware-decoded, looping via an
// end-of-item observer. Manual retain/release (JUCE builds .mm without ARC).
#include "gui/VideoLayer.h"

#if JUCE_MAC

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#include <cstdlib>

namespace hum {
namespace {

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
        endToken_ = [[[NSNotificationCenter defaultCenter]
            addObserverForName:AVPlayerItemDidPlayToEndTimeNotification
                        object:item_
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification*) {
                        const float r = player.rate > 0.0f ? player.rate : 1.0f;
                        [item seekToTime:kCMTimeZero
                            toleranceBefore:kCMTimeZero
                             toleranceAfter:kCMTimeZero
                          completionHandler:^(BOOL) { player.rate = r; }];
                    }] retain];
        if (!paused_) player_.rate = rate_;
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

    void restart() override {
        if (item_ == nil) return;
        [item_ seekToTime:kCMTimeZero
            toleranceBefore:kCMTimeZero
             toleranceAfter:kCMTimeZero
          completionHandler:nil];
        player_.rate = rate_ > 0.0f ? rate_ : 1.0f;
    }

    std::shared_ptr<const Frame> latestFrame() override {
        if (item_ == nil) return current_;
        const CMTime t = [item_ currentTime];
        if (![output_ hasNewPixelBufferForItemTime:t]) return current_;
        CVPixelBufferRef px = [output_ copyPixelBufferForItemTime:t
                                                itemTimeForDisplay:nil];
        if (px == nullptr) return current_;
        auto frame = std::make_shared<Frame>();
        frame->width = (int) CVPixelBufferGetWidth(px);
        frame->height = (int) CVPixelBufferGetHeight(px);
        static const bool forceUpload = getenv("HUMUS_GL_UPLOAD") != nullptr;
        const bool surfaceBacked = !forceUpload && CVPixelBufferGetIOSurface(px) != nullptr;
        if (!surfaceBacked) {
            CVPixelBufferLockBaseAddress(px, kCVPixelBufferLock_ReadOnly);
            const auto stride = (size_t) CVPixelBufferGetBytesPerRow(px);
            const auto* src = (const unsigned char*) CVPixelBufferGetBaseAddress(px);
            if (src != nullptr && frame->width > 0 && frame->height > 0) {
                const size_t rowBytes = (size_t) frame->width * 4;
                frame->bgra.resize(rowBytes * (size_t) frame->height);
                for (int y = 0; y < frame->height; ++y)
                    memcpy(frame->bgra.data() + rowBytes * (size_t) y,
                           src + stride * (size_t) y, rowBytes);
            }
            CVPixelBufferUnlockBaseAddress(px, kCVPixelBufferLock_ReadOnly);
        }
        if (frame->width <= 0 || frame->height <= 0
            || (!surfaceBacked && frame->bgra.empty())) {
            CVBufferRelease(px);
            return current_;
        }
        frame->native = px;
        frame->nativeHold = std::shared_ptr<const void>(
            (const void*) px, [](const void* p) { CVBufferRelease((CVPixelBufferRef) p); });
        current_ = std::move(frame);
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
    float rate_ = 1.0f;
    bool paused_ = false;
    std::shared_ptr<const Frame> current_;
};

}  // namespace

std::unique_ptr<VideoLayer> VideoLayer::createPlatform() {
    return std::make_unique<AvVideoLayer>();
}

}  // namespace hum

#endif  // JUCE_MAC
