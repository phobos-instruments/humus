// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserEntry.h"
#include "gui/browser/Audition.h"
#include "gui/browser/AuditionPicture.h"

namespace hum::browser {

class AuditionPane : public juce::Component {
public:
    static constexpr int kPad = 14, kGap = 8, kWaveH = 64, kScrubH = 14, kRowH = 28, kVolumeH = 22, kHintH = 30;
    static constexpr int kButtonW = 54, kPowerW = 130, kPictureScale = 2, kSpeakerW = 18;
    static constexpr double kClockFrom = 10.0, kUnityAt = 0.4;

    explicit AuditionPane(Audition& audition, AuditionPicture::MakeLayer makeLayer = {});

    std::function<bool()> engineRunning;
    std::function<void()> turnEngineOn;

    void show(const Entry& entry, const juce::String& hint);
    void tick();
    void restyle();

    int heightFor(int width) const;
    bool showsPicture() const { return entry_.kind == Kind::Video; }
    bool engineOff() const { return engineRunning && !engineRunning(); }
    bool offersPower() const { return power_.isVisible(); }
    bool offersSync() const { return sync_.isVisible(); }
    double shownSeconds() const;
    juce::Rectangle<float> seekArea() const;
    juce::Rectangle<float> playBounds() const;
    juce::Slider& volume() { return volume_; }
    void pressPower() { power_.onClick(); }
    AuditionPicture& picture() { return picture_; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void lookAndFeelChanged() override { restyle(); }
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    int mediaHeight(int width) const;
    juce::Rectangle<int> mediaBounds() const;
    juce::Rectangle<int> scrubBounds() const;
    juce::Rectangle<int> rowBounds() const;
    juce::Rectangle<int> volumeBounds() const;
    juce::Rectangle<int> hintBounds() const;
    double lengthSeconds() const;
    float playedFraction() const;
    bool mine() const { return audition_.current() == entry_.path; }
    double secondsAt(float x) const;
    void togglePlay();
    void showControls();
    juce::String audioOffText() const;

    void paintMedia(juce::Graphics& g) const;
    void paintScrub(juce::Graphics& g) const;
    void paintPlay(juce::Graphics& g) const;
    void paintTime(juce::Graphics& g) const;
    void paintPlayOverPicture(juce::Graphics& g) const;
    void paintSpeaker(juce::Graphics& g) const;

    Audition& audition_;
    AuditionPicture picture_;
    Entry entry_;
    juce::String hint_;
    juce::TextButton loop_, sync_, power_;
    juce::Slider volume_;
    bool wasPlaying_ = false, scrubbing_ = false;
    double lastPos_ = 0.0, scrubAt_ = 0.0, holdAt_ = -1.0;
};

}
