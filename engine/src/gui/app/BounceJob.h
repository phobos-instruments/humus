// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/BounceWants.h"
#include "gui/host/EngineHost.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"
#include "gui/video/VideoFilm.h"
#include "io/MidiExport.h"
#include "io/WavWriter.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class BounceJob : private juce::Thread, private juce::Timer {
public:
    struct Told {
        bool ok = false;
        juce::String said;
    };

    BounceJob(EngineHost& host, BounceWants wants)
        : juce::Thread("bounce-sound"), host_(host), wants_(std::move(wants)) {}

    ~BounceJob() override {
        stopThread(4000);
        film_.reset();
    }

    juce::File soundFile() const { return wants_.soundFile(); }
    juce::File movieFile() const { return wants_.movieFile(); }
    juce::File notesFile() const { return wants_.notesFile(); }

    void run(std::function<void(Told)> done) {
        done_ = std::move(done);
        if (wants_.nothingChosen()) {
            finish({false, tr("bounce.nothing-chosen", "Nothing was chosen to write.")});
            return;
        }
        if (wants_.toBeat <= wants_.fromBeat) wants_.toBeat = wants_.fromBeat + 4.0;
        if (wants_.midi
            && !midiexport::write(host_.model(), notesFile(),
                                  {wants_.fromBeat, wants_.toBeat}))
            trouble_ = tr("bounce.no-notes-written", "No notes were written.");
        show();
        if (wants_.audio) {
            std::string error;
            if (!host_.openOfflineSound(sound_, error)) {
                finish({false, error.empty()
                                   ? tr("bounce.no-sound-made", "Nothing was rendered.")
                                   : juce::String(error)});
                return;
            }
            stage(tr("bounce.rendering-audio", "Rendering audio..."), 0.0, 0.5);
            startThread();
            return;
        }
        startPicture();
    }

    void stop() {
        stopped_ = true;
        signalThreadShouldExit();
        if (film_ != nullptr) film_->stop();
        else if (!isThreadRunning()) finish({false, tr("bounce.stopped", "Stopped.")});
    }

private:
    struct Progress : juce::Component {
        juce::Label title, message, timeLeft;
        juce::ProgressBar bar;
        juce::TextButton stop;

        explicit Progress(double& value) : bar(value) {
            title.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            title.setColour(juce::Label::textColourId, Palette::text);
            message.setFont(juce::FontOptions(12.0f));
            message.setColour(juce::Label::textColourId, Palette::textDim);
            timeLeft.setFont(juce::FontOptions(12.0f));
            timeLeft.setColour(juce::Label::textColourId, Palette::textDim);
            timeLeft.setJustificationType(juce::Justification::centredRight);
            for (auto* c : {&title, &message, &timeLeft}) addAndMakeVisible(c);
            addAndMakeVisible(bar);
            addAndMakeVisible(stop);
            setSize(400, 132);
        }

        void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

        void resized() override {
            auto b = getLocalBounds().reduced(20, 16);
            title.setBounds(b.removeFromTop(20));
            b.removeFromTop(4);
            auto foot = b.removeFromBottom(26);
            stop.setBounds(foot.removeFromRight(96));
            timeLeft.setBounds(foot.withTrimmedRight(12));
            b.removeFromBottom(8);
            message.setBounds(b.removeFromTop(18));
            b.removeFromTop(6);
            bar.setBounds(b.removeFromTop(20));
        }
    };

    class ProgressDialog : public juce::DialogWindow {
    public:
        ProgressDialog(Progress& content, std::function<void()> onClose)
            : juce::DialogWindow(tr("bounce.bouncing", "Bouncing"), Palette::background, true,
                                 true),
              onClose_(std::move(onClose)) {
            setUsingNativeTitleBar(true);
            setContentNonOwned(&content, true);
            setResizable(false, false);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }
        void closeButtonPressed() override { if (onClose_) onClose_(); }

    private:
        std::function<void()> onClose_;
    };

    void show() {
        progressView_ = std::make_unique<Progress>(progress_);
        progressView_->title.setText(wants_.stem, juce::dontSendNotification);
        progressView_->timeLeft.setText(tr("bounce.estimating", "estimating time left..."),
                                        juce::dontSendNotification);
        progressView_->stop.setButtonText(tr("bounce.stop", "Stop"));
        progressView_->stop.onClick = [this] { stop(); };
        window_ = std::make_unique<ProgressDialog>(*progressView_, [this] { stop(); });
        keep_ = window_.get();
        window_->enterModalState(true, nullptr, false);
        startTimer(500);
    }

    void say(const juce::String& what) {
        if (progressView_ != nullptr)
            progressView_->message.setText(what, juce::dontSendNotification);
    }

    void stage(const juce::String& what, double base, double span) {
        say(what);
        stageBase_ = base;
        stageSpan_ = span;
        stageBegan_ = juce::Time::getMillisecondCounter();
        if (progressView_ != nullptr)
            progressView_->timeLeft.setText(tr("bounce.estimating", "estimating time left..."),
                                            juce::dontSendNotification);
    }

    static juce::String clock(double seconds) {
        const int whole = (int) std::lround(std::max(0.0, seconds));
        const int h = whole / 3600, m = (whole % 3600) / 60, sec = whole % 60;
        auto two = [](int v) { return juce::String(v).paddedLeft('0', 2); };
        return h > 0 ? juce::String(h) + ":" + two(m) + ":" + two(sec)
                     : juce::String(m) + ":" + two(sec);
    }

    void timerCallback() override {
        const double done = std::clamp((progress_ - stageBase_) / std::max(1.0e-9, stageSpan_),
                                       0.0, 1.0);
        const double elapsed = (double) (juce::Time::getMillisecondCounter() - stageBegan_) / 1000.0;
        if (done < 0.02 || elapsed < 1.5) return;
        const double left = elapsed * (1.0 - done) / done;
        if (progressView_ != nullptr)
            progressView_->timeLeft.setText(clock(left) + " " + tr("bounce.time-left", "left"),
                                            juce::dontSendNotification);
    }

    void run() override {
        const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
        const double from = wants_.fromBeat * kSecondsPerMinute / bpm;
        const double to = wants_.toBeat * kSecondsPerMinute / bpm;
        std::string error;
        pcm_.clear();
        const double span = std::max(1.0e-6, to - from);
        const bool made = host_.renderOfflineSound(sound_, from, to,
                                            [this, span](const float* const* in, int channels, int n) {
            if (threadShouldExit()) return false;
            if (pcm_.empty()) pcm_.resize((size_t) channels);
            for (int c = 0; c < channels; ++c)
                pcm_[(size_t) c].insert(pcm_[(size_t) c].end(), in[c], in[c] + n);
            progress_ = 0.5 * (double) pcm_[0].size() / std::max(1.0, span * host_.sampleRate());
            return true;
        }, error);
        const auto why = juce::String(error);
        juce::MessageManager::callAsync([this, made, why] { soundDone(made, why); });
    }

    void soundDone(bool made, const juce::String& why) {
        sound_ = {};
        if (stopped_) {
            finish({false, tr("bounce.stopped", "Stopped.")});
            return;
        }
        if (!made || pcm_.empty() || pcm_[0].empty()) {
            finish({false, why.isEmpty() ? tr("bounce.no-sound-made", "Nothing was rendered.")
                                         : why});
            return;
        }
        if (!wants_.video) {
            const bool wrote = writeSound(soundFile().getFullPathName().toStdString(), pcm_,
                                          host_.sampleRate(), 24, wants_.mp3Rate);
            finish({wrote, wrote ? said() : tr("bounce.no-sound-file", "The sound file could not be written.")});
            return;
        }
        startPicture();
    }

    void startPicture() {
        if (!wants_.video) {
            finish({true, said()});
            return;
        }
        const double base = wants_.audio ? 0.5 : 0.0;
        const double span = wants_.audio ? 0.5 : 1.0;
        stage(tr("bounce.rendering-video", "Rendering video..."), base, span);
        VideoFilm::Options film;
        film.node = wants_.videoNode;
        film.fromBeat = wants_.fromBeat;
        const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
        film.seconds = (wants_.toBeat - wants_.fromBeat) * kSecondsPerMinute / bpm;
        film.fps = wants_.fps;
        film.width = wants_.width;
        film.height = wants_.height;
        film.kind = wants_.kind;
        film.quality = wants_.quality;
        film_ = std::make_unique<VideoFilm>(host_, movieFile(), film);
        if (!pcm_.empty() && !pcm_[0].empty()) {
            film_->sound = [this](VideoFilm& f) {
                std::vector<const float*> ptrs(pcm_.size());
                for (size_t c = 0; c < pcm_.size(); ++c) ptrs[c] = pcm_[c].data();
                return f.writeSound(ptrs.data(), (int) pcm_.size(), (int) pcm_[0].size(),
                                    host_.sampleRate());
            };
        }
        film_->onProgress = [this, base, span](double at) { progress_ = base + span * at; };
        film_->run([this](VideoFilm::Result r) {
            finish({r.ok, r.ok ? said() : r.trouble});
        });
    }

    juce::String said() const {
        juce::String out = tr("bounce.wrote", "bounced ") + wants_.stem;
        if (trouble_.isNotEmpty()) out += " (" + trouble_ + ")";
        return out;
    }

    void finish(Told told) {
        stopTimer();
        if (keep_ != nullptr) keep_->exitModalState(0);
        keep_ = nullptr;
        window_.reset();
        progressView_.reset();
        pcm_.clear();
        if (done_) done_(told);
    }

    EngineHost& host_;
    BounceWants wants_;
    EngineHost::OfflineSound sound_;
    std::vector<std::vector<float>> pcm_;
    std::unique_ptr<VideoFilm> film_;
    std::unique_ptr<Progress> progressView_;
    std::unique_ptr<ProgressDialog> window_;
    juce::DialogWindow* keep_ = nullptr;
    std::function<void(Told)> done_;
    juce::String trouble_;
    double progress_ = 0.0, stageBase_ = 0.0, stageSpan_ = 1.0;
    juce::uint32 stageBegan_ = 0;
    bool stopped_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BounceJob)
};

}
