// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <deque>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/video/VideoEncoder.h"
#include "gui/host/VideoHost.h"
#include "gui/common/Localisation.h"
#include "gui/video/VisualGlCanvas.h"
#include "gui/video/VisualPlanBuilder.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class FrameSink : private juce::Thread {
public:
    explicit FrameSink(VideoEncoder& writer) : juce::Thread("bounce-encode"), writer_(writer) {
        startThread(juce::Thread::Priority::high);
    }

    ~FrameSink() override { finish(); }

    bool hasRoom() const {
        const juce::ScopedLock sl(lock_);
        return !broke_ && queue_.size() < kDepth;
    }

    bool idle() const {
        const juce::ScopedLock sl(lock_);
        return queue_.empty();
    }

    bool broke() const {
        const juce::ScopedLock sl(lock_);
        return broke_;
    }

    void push(std::vector<std::uint8_t>&& frame) {
        {
            const juce::ScopedLock sl(lock_);
            queue_.push_back(std::move(frame));
        }
        ready_.signal();
    }

    void finish() {
        draining_ = true;
        ready_.signal();
        stopThread(20000);
    }

private:
    static constexpr size_t kDepth = 3;

    void run() override {
        const MovieThread onThisThread;

        for (;;) {
            std::vector<std::uint8_t> frame;
            {
                const juce::ScopedLock sl(lock_);
                if (!queue_.empty()) {
                    frame = std::move(queue_.front());
                    queue_.pop_front();
                }
            }
            if (frame.empty()) {
                if (draining_ || threadShouldExit()) return;
                ready_.wait(4);
                continue;
            }
            if (!writer_.addFrame(frame.data())) {
                const juce::ScopedLock sl(lock_);
                broke_ = true;
                queue_.clear();
                return;
            }
        }
    }

    VideoEncoder& writer_;
    mutable juce::CriticalSection lock_;
    std::deque<std::vector<std::uint8_t>> queue_;
    juce::WaitableEvent ready_;
    std::atomic<bool> draining_{false};
    bool broke_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrameSink)
};

class VideoFilm : private juce::Timer {
public:
    struct Options {
        std::string node;
        double fromBeat = 0.0;
        double seconds = 4.0;
        double fps = 30.0;
        int width = 1280, height = 720;
        MovieKind kind = MovieKind::H264;
        int quality = kQualityDefault;
        bool driveEngine = true;
    };

    struct Result {
        bool ok = false;
        int frames = 0;
        juce::String trouble;
    };

    std::function<void(double)> onProgress;
    std::function<bool(VideoFilm&)> sound;

    VideoFilm(VideoHost& host, const juce::File& out, Options o)
        : host_(host), file_(out), opt_(std::move(o)),
          builder_(host, opt_.node, isVideoOutputNode(host, opt_.node)) {
        opt_.fps = juce::jlimit(1.0, 120.0, opt_.fps);
        opt_.width = juce::jlimit(16, 7680, opt_.width & ~1);
        opt_.height = juce::jlimit(16, 4320, opt_.height & ~1);
        wanted_ = std::max(1, (int) std::llround(opt_.seconds * opt_.fps));
        const double perFrame = host.sampleRate() / opt_.fps / std::max(1, host.blockSize());
        blocksPerFrame_ = juce::jlimit(1, 32, (int) std::ceil(perFrame));
        builder_.setExactFrames(true);
    }

    ~VideoFilm() override {
        stopTimer();
        letEngineGo();
    }

    void run(std::function<void(Result)> done) {
        done_ = std::move(done);
        if (opt_.node.empty() || host_.videoSourceInto(opt_.node, 0).empty()) {
            finish({false, 0, tr("film.nothing-wired", "Nothing is wired into that video output.")});
            return;
        }
        writer_ = makeMovieWriter(opt_.kind, file_, opt_.width, opt_.height, opt_.fps,
                                  opt_.quality);
        if (!writer_->ok()) {
            finish({false, 0, tr("film.no-file", "The movie file could not be opened.")});
            return;
        }
        if (sound && !sound(*this)) {
            finish({false, 0, tr("film.no-sound", "The sound could not be written.")});
            return;
        }
        if (opt_.driveEngine) {
            wasPlaying_ = host_.isPlaying();
            wasAt_ = host_.positionBeats();
            drove_ = true;
            host_.holdAudio(true);
            if (!wasPlaying_) host_.play();
        }
        builder_.buildAt(opt_.fromBeat, tempoOf(host_), (float) (1.0 / opt_.fps));
        sink_ = std::make_unique<FrameSink>(*writer_);
        stage_ = std::make_unique<Stage>(opt_.width, opt_.height);
        startTimer(2);
    }

    void stop() {
        if (writer_ == nullptr) return;
        const bool sealed = made_ > 0 && writer_->close();
        finish({sealed, made_,
                sealed ? juce::String()
                       : tr("film.stopped", "Stopped before a frame was written.")});
    }

    int framesWanted() const { return wanted_; }

    const visual::Plan& lastPlanForTest() const { return shown_; }
    const std::string& deckPrefix() const { return builder_.poolPrefix(); }

    bool writeSound(const float* const* channels, int count, int frames, double sampleRate) {
        return writer_ != nullptr && writer_->openSound(count, sampleRate)
               && writer_->addSound(channels, count, frames);
    }

    static double tempoOf(VideoHost& host) {
        const double t = host.tempo();
        return t > 0.0 ? t : 120.0;
    }

private:
    struct Stage : juce::Component {
        GlCanvas canvas;

        Stage(int w, int h) {
            setOpaque(true);
            canvas.setBounds(0, 0, w, h);
            addAndMakeVisible(canvas);
            const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
            const auto area = display != nullptr ? display->totalArea
                                                 : juce::Rectangle<int>(0, 0, 800, 600);
            setBounds(area.getRight() - 2, area.getBottom() - 2, 2, 2);
            addToDesktop(juce::ComponentPeer::windowIsTemporary
                         | juce::ComponentPeer::windowIgnoresMouseClicks);
            setVisible(true);
            canvas.setPaused(true);
        }

        ~Stage() override { removeFromDesktop(); }

        void paint(juce::Graphics&) override {}
    };

    void timerCallback() override {
        if (waiting_) {
            std::vector<std::uint8_t> rgba;
            if (stage_->canvas.takeGrab(rgba)) {
                waiting_ = false;
                waitMs_ = juce::Time::getMillisecondCounterHiRes() - askedAt_;
                if (loud_) report(rgba);
                sink_->push(std::move(rgba));
                ++made_;
                if (onProgress) onProgress((double) made_ / (double) std::max(1, wanted_));
                return;
            }
            if (++stalled_ < kFrameTries) return;
            finish({false, made_, tr("film.stalled", "The picture stopped arriving.")});
            return;
        }
        if (sink_->broke()) {
            finish({false, made_, tr("film.no-frame", "A frame could not be written.")});
            return;
        }
        if (made_ >= wanted_) {
            if (!sink_->idle()) return;
            sink_->finish();
            const bool sealed = !sink_->broke() && writer_->close();
            finish({sealed, made_,
                    sealed ? juce::String()
                           : tr("film.no-close", "The movie file could not be finished.")});
            return;
        }
        if (!sink_->hasRoom()) return;
        const double planFrom = juce::Time::getMillisecondCounterHiRes();
        const double tempo = tempoOf(host_);
        const double beat = opt_.fromBeat
                          + (double) made_ / opt_.fps * tempo / kSecondsPerMinute;
        if (opt_.driveEngine) {
            host_.setPositionBeats(beat);
            host_.primeOffline(blocksPerFrame_);
            host_.advanceModulation(1.0 / opt_.fps);
        }
        engineMs_ = juce::Time::getMillisecondCounterHiRes() - planFrom;
        auto plan = builder_.buildAt(beat, tempo, (float) (1.0 / opt_.fps));
        shown_ = plan;
        atBeat_ = beat;
        stage_->canvas.setPlan(plan);
        planMs_ = juce::Time::getMillisecondCounterHiRes() - planFrom;
        stage_->canvas.requestGrab(opt_.width, opt_.height);
        stage_->canvas.pump();
        askedAt_ = juce::Time::getMillisecondCounterHiRes();
        waiting_ = true;
        stalled_ = 0;
    }

    void report(const std::vector<std::uint8_t>& rgba) const {
        double sum = 0.0;
        for (size_t i = 0; i + 3 < rgba.size(); i += 4)
            sum += rgba[i] + rgba[i + 1] + rgba[i + 2];
        const double lit = rgba.empty() ? -1.0 : sum / (double) (rgba.size() / 4 * 3);
        int decks = 0, lively = 0;
        for (const auto& step : shown_.steps)
            if (step.kind == visual::Step::Deck) {
                ++decks;
                if (step.active && step.frame != nullptr) ++lively;
            }
        std::fprintf(stderr,
                     "[bounce] frame %d beat %.3f grab %.1f steps %d root %d fade %.2f "
                     "decks %d with-frame %d | engine %.1fms plan %.1fms wait %.1fms queued %d\n",
                     made_, atBeat_, lit, (int) shown_.steps.size(), shown_.root,
                     shown_.masterFade, decks, lively, engineMs_, planMs_ - engineMs_, waitMs_,
                     sink_ != nullptr && !sink_->idle() ? 1 : 0);
        std::fflush(stderr);
    }

    void finish(Result r) {
        stopTimer();
        letEngineGo();
        if (sink_ != nullptr) sink_->finish();
        sink_.reset();
        if (writer_ != nullptr && !r.ok) writer_->close();
        writer_.reset();
        stage_.reset();
        if (!r.ok && r.frames == 0) file_.deleteFile();
        if (done_) done_(r);
    }

    void letEngineGo() {
        if (!drove_) return;
        drove_ = false;
        if (!wasPlaying_) host_.stop();
        host_.setPositionBeats(wasAt_);
        host_.holdAudio(false);
    }

    static constexpr int kFrameTries = 500;

    VideoHost& host_;
    juce::File file_;
    Options opt_;
    VisualPlanBuilder builder_;
    std::unique_ptr<Stage> stage_;
    std::unique_ptr<VideoEncoder> writer_;
    std::unique_ptr<FrameSink> sink_;
    std::function<void(Result)> done_;
    int wanted_ = 0, made_ = 0, stalled_ = 0, blocksPerFrame_ = 1;
    double wasAt_ = 0.0, atBeat_ = 0.0;
    double planMs_ = 0.0, engineMs_ = 0.0, waitMs_ = 0.0, askedAt_ = 0.0;
    visual::Plan shown_;
    const bool loud_ = std::getenv("HUMUS_BOUNCE_DEBUG") != nullptr;
    bool wasPlaying_ = false, drove_ = false;
    bool waiting_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoFilm)
};

}
