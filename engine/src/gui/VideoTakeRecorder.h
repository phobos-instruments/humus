#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/VideoTakeClock.h"
#include "gui/AppSettings.h"
#include "gui/VideoEncoder.h"
#include "gui/VideoLog.h"
#include "gui/VideoTakeSink.h"

namespace hum {

namespace videotake {

inline constexpr const char* kHeightKey = "video.takeHeight";
inline constexpr int kDefaultHeight = 720;
inline constexpr int kHeights[] = {1080, 720, 540, 360};
inline constexpr double kFps = 30.0;

inline int heightSetting() {
    const int h = AppSettings::instance().getInt(kHeightKey, kDefaultHeight);
    for (const int k : kHeights)
        if (k == h) return h;
    return kDefaultHeight;
}

inline void setHeightSetting(int h) { AppSettings::instance().set(kHeightKey, h); }

inline int widthFor(int height) { return height * 16 / 9; }

}

class VideoTakeRecorder : public VideoTakeSink, private juce::Thread {
public:
    static constexpr int kQueueDepth = 6;

    VideoTakeRecorder(const juce::File& file, int width, int height, double fps, double sampleRate)
        : juce::Thread("video take"), file_(file), width_(width), height_(height),
          sampleRate_(sampleRate), clock_(fps) {
        writer_ = makeMovieWriter(MovieKind::H264, file, width, height, fps, kQualityDefault, true);
        if (writer_ != nullptr && writer_->ok()) startThread(juce::Thread::Priority::high);
    }

    ~VideoTakeRecorder() override { stop(); }

    bool ok() const { return writer_ != nullptr && writer_->ok(); }
    const juce::File& file() const { return file_; }

    void pushFrame(const std::uint8_t* bottomUpRgba, int w, int h, double beat, double tempo,
                   bool rolling) override {
        if (!ok() || bottomUpRgba == nullptr || w != width_ || h != height_) return;
        std::vector<std::uint8_t> buf;
        {
            const juce::ScopedLock sl(lock_);
            if (closed_) return;
            if ((int) queue_.size() >= kQueueDepth) {
                ++dropped_;
                return;
            }
            const auto a = clock_.tick(beat, tempo, rolling);
            if (!a.write) return;
            if (!spare_.empty()) {
                buf = std::move(spare_.back());
                spare_.pop_back();
            }
            flip(bottomUpRgba, buf);
            queue_.push_back({a.hold, std::move(buf)});
        }
        notify();
    }

    void stop() {
        {
            const juce::ScopedLock sl(lock_);
            closed_ = true;
        }
        notify();
        stopThread(30000);
        if (writer_ != nullptr) {
            writer_->close();
            writer_.reset();
        }
    }

    bool started() const {
        const juce::ScopedLock sl(lock_);
        return clock_.started();
    }
    double takeStartBeat() const {
        const juce::ScopedLock sl(lock_);
        return clock_.startBeat();
    }
    std::int64_t takeLengthSamples() const {
        const juce::ScopedLock sl(lock_);
        return clock_.samplesOf(clock_.frames(), sampleRate_);
    }
    std::int64_t frames() const {
        const juce::ScopedLock sl(lock_);
        return clock_.frames();
    }
    int framesWritten() const { return written_.load(); }
    int framesDropped() const {
        const juce::ScopedLock sl(lock_);
        return dropped_;
    }
    int takeLaps(double* beats, std::int64_t* samples, int maxLaps) const {
        const juce::ScopedLock sl(lock_);
        int n = 0;
        for (const auto& lap : clock_.laps()) {
            if (n >= maxLaps) break;
            beats[n] = lap.beat;
            samples[n] = clock_.samplesOf(lap.frame, sampleRate_);
            ++n;
        }
        return n;
    }

private:
    struct Item {
        int hold = 0;
        std::vector<std::uint8_t> rgba;
    };

    void flip(const std::uint8_t* bottomUp, std::vector<std::uint8_t>& topDown) const {
        const size_t row = (size_t) width_ * 4u;
        topDown.resize(row * (size_t) height_);
        for (int y = 0; y < height_; ++y)
            std::memcpy(topDown.data() + row * (size_t) y,
                        bottomUp + row * (size_t) (height_ - 1 - y), row);
    }

    void run() override {
        std::vector<std::uint8_t> last;
        for (;;) {
            Item item;
            bool have = false, done = false;
            {
                const juce::ScopedLock sl(lock_);
                if (!queue_.empty()) {
                    item = std::move(queue_.front());
                    queue_.pop_front();
                    have = true;
                } else {
                    done = closed_;
                }
            }
            if (!have) {
                if (done) return;
                wait(50);
                continue;
            }
            for (int i = 0; i < item.hold && !last.empty(); ++i)
                if (writer_->addFrame(last.data())) ++written_;
            if (writer_->addFrame(item.rgba.data())) ++written_;
            {
                const juce::ScopedLock sl(lock_);
                if (!last.empty()) spare_.push_back(std::move(last));
            }
            last = std::move(item.rgba);
        }
    }

    juce::File file_;
    int width_, height_;
    double sampleRate_;
    std::unique_ptr<VideoEncoder> writer_;
    juce::CriticalSection lock_;
    videotake::Clock clock_;
    std::deque<Item> queue_;
    std::vector<std::vector<std::uint8_t>> spare_;
    std::atomic<int> written_{0};
    int dropped_ = 0;
    bool closed_ = false;
};

}
