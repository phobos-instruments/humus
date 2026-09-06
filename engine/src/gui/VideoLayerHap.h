#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/HapFile.h"
#include "gui/VideoLayer.h"

namespace hum {

class HapVideoLayer : public VideoLayer, private juce::Thread {
public:
    HapVideoLayer() : juce::Thread("hap-decode") {}

    ~HapVideoLayer() override { stopThread(2000); }

    void load(const juce::String& path) override {
        stopThread(2000);
        {
            const juce::ScopedLock sl(lock_);
            current_.reset();
        }
        file_ = juce::File(path);
        movie_ = hap::open(file_);
        position_ = 0.0;
        lastShown_ = -1;
        if (movie_.ok) startThread();
    }

    void setRate(float rate) override { rate_.store(rate); }

    void restart() override { rewind_.store(true); }

    void setPaused(bool paused) override { paused_.store(paused); }

    bool isPaused() const override { return paused_.load(); }

    double positionSeconds() override { return shownPosition_.load(); }

    double lengthSeconds() override {
        return movie_.ok ? (double) movie_.samples.size() / movie_.fps : 0.0;
    }

    void seekSeconds(double t) override { seekTo_.store(t); }

    std::shared_ptr<const Frame> latestFrame() override {
        const juce::ScopedLock sl(lock_);
        return current_;
    }

private:
    void run() override {
        juce::FileInputStream in(file_);
        if (!in.openedOk()) return;
        auto lastMs = juce::Time::getMillisecondCounterHiRes();
        std::vector<std::uint8_t> raw;
        while (!threadShouldExit()) {
            const auto nowMs = juce::Time::getMillisecondCounterHiRes();
            if (rewind_.exchange(false)) position_ = 0.0;
            if (const double target = seekTo_.exchange(-1.0); target >= 0.0)
                position_ = target;
            if (!paused_.load())
                position_ += (nowMs - lastMs) * 0.001 * (double) rate_.load();
            lastMs = nowMs;
            const double span = (double) movie_.samples.size() / movie_.fps;
            if (span > 0.0)
                while (position_ >= span) position_ -= span;
            while (position_ < 0.0) position_ += span > 0.0 ? span : 1.0;
            shownPosition_.store(position_);
            const int idx = juce::jlimit(0, (int) movie_.samples.size() - 1,
                                         (int) (position_ * movie_.fps));
            if (idx != lastShown_) {
                const auto& s = movie_.samples[(size_t) idx];
                raw.resize(s.size);
                in.setPosition(s.offset);
                if (in.read(raw.data(), (int) raw.size()) == (int) raw.size()) {
                    hap::Frame decoded;
                    if (hap::decodeFrame(raw.data(), raw.size(), decoded)) {
                        auto frame = std::make_shared<Frame>();
                        frame->width = movie_.width;
                        frame->height = movie_.height;
                        frame->fmt = decoded.tex == hap::Tex::DXT1 ? Frame::DXT1
                                   : decoded.tex == hap::Tex::DXT5
                                       ? Frame::DXT5
                                       : Frame::YCoCgDXT5;
                        frame->blocks = std::move(decoded.blocks);
                        const juce::ScopedLock sl(lock_);
                        current_ = std::move(frame);
                    }
                }
                lastShown_ = idx;
            }
            wait(4);
        }
    }

    juce::File file_;
    hap::Movie movie_;
    std::atomic<float> rate_{1.0f};
    std::atomic<bool> rewind_{false};
    std::atomic<bool> paused_{false};
    std::atomic<double> seekTo_{-1.0};
    std::atomic<double> shownPosition_{0.0};
    double position_ = 0.0;
    int lastShown_ = -1;
    juce::CriticalSection lock_;
    std::shared_ptr<const Frame> current_;
};

}
