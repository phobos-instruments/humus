// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/video/HapFile.h"
#include "gui/video/VideoLayer.h"

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

    void chase(double seconds, double rate) override {
        chaseTo_.store(std::max(0.0, seconds));
        chaseRate_.store(rate);
        chaseStamp_.fetch_add(1);
    }

    void seekSeconds(double t) override { seekTo_.store(t); }

    void setLoopRange(const LoopRange& r) override {
        loopIn_.store(std::max(0.0, r.in));
        loopOut_.store(r.out);
        loop_.store(r.loop);
    }

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
        unsigned chaseSeen = 0;
        bool chasing = false;
        while (!threadShouldExit()) {
            const auto nowMs = juce::Time::getMillisecondCounterHiRes();
            const double span = (double) movie_.samples.size() / movie_.fps;
            const auto range = loopWindow(loopIn_.load(), loopOut_.load(), span);
            const double len = range.second > range.first ? range.second - range.first : 1.0;
            if (rewind_.exchange(false)) { position_ = range.first; chasing = false; }
            if (const double target = seekTo_.exchange(-1.0); target >= 0.0) {
                position_ = target;
                chasing = false;
            }
            if (const unsigned stamp = chaseStamp_.load(); stamp != chaseSeen) {
                chaseSeen = stamp;
                chasing = true;
                position_ = chaseTo_.load();
            } else if (chasing) {
                position_ += (nowMs - lastMs) * 0.001 * chaseRate_.load();
            } else if (!paused_.load()) {
                position_ += (nowMs - lastMs) * 0.001 * (double) rate_.load();
            }
            lastMs = nowMs;
            if (chasing) {
                position_ = juce::jlimit(0.0, std::max(0.0, span), position_);
            } else {
                if (range.second > range.first && position_ >= range.second)
                    position_ = loop_.load()
                        ? range.first + std::fmod(position_ - range.first, len)
                        : range.second;
                while (position_ < range.first) position_ += len;
            }
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
                        frame->pts = (double) idx / movie_.fps;
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
    std::atomic<double> loopIn_{0.0};
    std::atomic<double> loopOut_{0.0};
    std::atomic<bool> loop_{true};
    std::atomic<bool> rewind_{false};
    std::atomic<bool> paused_{false};
    std::atomic<double> seekTo_{-1.0};
    std::atomic<double> chaseTo_{0.0};
    std::atomic<double> chaseRate_{0.0};
    std::atomic<unsigned> chaseStamp_{0};
    std::atomic<double> shownPosition_{0.0};
    double position_ = 0.0;
    int lastShown_ = -1;
    juce::CriticalSection lock_;
    std::shared_ptr<const Frame> current_;
};

}
