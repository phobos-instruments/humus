// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/AuditionPane.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "gui/browser/BrowserPaint.h"
#include "gui/common/Localisation.h"

namespace hum::browser {

AuditionPane::AuditionPane(Audition& audition, AuditionPicture::MakeLayer makeLayer)
    : audition_(audition), picture_(std::move(makeLayer)) {
    for (auto* b : {&loop_, &sync_}) {
        b->setClickingTogglesState(true);
        addAndMakeVisible(*b);
    }
    loop_.setButtonText(tr("browser.loop", "Loop"));
    sync_.setButtonText(tr("browser.sync", "Sync"));
    sync_.setTooltip(tr("browser.sync-tip", "Play loops at the patch tempo"));
    power_.setButtonText(tr("browser.turn-on-audio", "Turn on audio"));
    power_.onClick = [this] {
        if (turnEngineOn) turnEngineOn();
    };
    addChildComponent(power_);
    loop_.onClick = [this] { audition_.setLoop(loop_.getToggleState()); };
    sync_.onClick = [this] { audition_.setSync(sync_.getToggleState()); };
    volume_.setSliderStyle(juce::Slider::LinearHorizontal);
    volume_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volume_.setRange(0.0, 1.5, 0.0);
    volume_.setSkewFactorFromMidPoint(kUnityAt);
    volume_.setSliderSnapsToMousePosition(false);
    volume_.setDoubleClickReturnValue(true, 1.0);
    volume_.textFromValueFunction = [](double v) { return juce::String(juce::roundToInt(v * 100.0)) + "%"; };
    volume_.setPopupDisplayEnabled(true, false, nullptr);
    volume_.setValue(audition_.gain(), juce::dontSendNotification);
    volume_.setTooltip(tr("browser.volume", "Preview volume"));
    volume_.onValueChange = [this] { audition_.setGain((float) volume_.getValue()); };
    addAndMakeVisible(volume_);
    loop_.setToggleState(audition_.loop(), juce::dontSendNotification);
    sync_.setToggleState(audition_.sync(), juce::dontSendNotification);
    restyle();
}

void AuditionPane::restyle() {
    for (auto* b : {&loop_, &sync_}) {
        b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        b->setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        b->setColour(juce::TextButton::textColourOnId, paint::onAccent());
        b->setColour(juce::TextButton::textColourOffId, Palette::text);
    }
    volume_.setColour(juce::Slider::trackColourId, Palette::textDim.withAlpha(alpha::mid));
    volume_.setColour(juce::Slider::backgroundColourId, Palette::panelLight);
    power_.setColour(juce::TextButton::buttonColourId, Palette::accent);
    power_.setColour(juce::TextButton::textColourOffId, paint::onAccent());
    repaint();
}

void AuditionPane::show(const Entry& entry, const juce::String& hint) {
    entry_ = entry;
    hint_ = hint;
    if (showsPicture()) picture_.open(entry_.path);
    else picture_.close();
    showControls();
    repaint();
}

void AuditionPane::showControls() {
    const bool off = power_.isVisible();
    loop_.setVisible(!off);
    sync_.setVisible(!off && !showsPicture());
    resized();
}

void AuditionPane::visibilityChanged() {
    if (!isVisible()) picture_.close();
}

void AuditionPane::tick() {
    if (const bool off = engineOff(); off != power_.isVisible()) {
        power_.setVisible(off);
        if (!off && mine()) audition_.play(entry_.path, entry_.facts.bpm);
        showControls();
        repaint();
    }
    const bool playing = audition_.playing();
    if (holdAt_ >= 0.0 && (!mine() || audition_.holds(entry_.path))) holdAt_ = -1.0;
    const double pos = shownSeconds();
    if (playing != wasPlaying_ || std::abs(pos - lastPos_) > 0.01)
        repaint(seekArea().toNearestInt().expanded(4).getUnion(rowBounds()).getUnion(mediaBounds()));
    wasPlaying_ = playing;
    lastPos_ = pos;
    if (showsPicture()) {
        picture_.follow(pos, mine() && playing && !scrubbing_);
        const auto box = mediaBounds();
        if (picture_.refresh(box.getWidth() * kPictureScale, box.getHeight() * kPictureScale)) repaint(box);
    }
}

int AuditionPane::mediaHeight(int width) const {
    return showsPicture() ? (width - 2 * kPad) * 9 / 16 : kWaveH;
}

int AuditionPane::heightFor(int width) const {
    const int scrub = showsPicture() ? kScrubH + kGap : 0;
    return kPad + mediaHeight(width) + kGap + scrub + kRowH + kGap + kVolumeH + kGap + kHintH + kGap;
}

juce::Rectangle<int> AuditionPane::mediaBounds() const {
    return {kPad, kPad, getWidth() - 2 * kPad, mediaHeight(getWidth())};
}

juce::Rectangle<int> AuditionPane::scrubBounds() const {
    if (!showsPicture()) return {};
    return {kPad, mediaBounds().getBottom() + kGap, getWidth() - 2 * kPad, kScrubH};
}

juce::Rectangle<int> AuditionPane::rowBounds() const {
    const int top = (showsPicture() ? scrubBounds().getBottom() : mediaBounds().getBottom()) + kGap;
    return {kPad, top, getWidth() - 2 * kPad, kRowH};
}

juce::Rectangle<float> AuditionPane::playBounds() const { return rowBounds().removeFromLeft(kRowH).toFloat(); }

juce::Rectangle<int> AuditionPane::volumeBounds() const {
    return {kPad, rowBounds().getBottom() + kGap, getWidth() - 2 * kPad, kVolumeH};
}

juce::Rectangle<int> AuditionPane::hintBounds() const {
    return {kPad, volumeBounds().getBottom() + kGap, getWidth() - 2 * kPad, kHintH};
}

juce::Rectangle<float> AuditionPane::seekArea() const {
    return (showsPicture() ? scrubBounds() : mediaBounds()).toFloat();
}

double AuditionPane::lengthSeconds() const {
    return entry_.facts.seconds > 0.0 ? entry_.facts.seconds : mine() ? audition_.length() : 0.0;
}

double AuditionPane::shownSeconds() const {
    if (scrubbing_) return scrubAt_;
    if (holdAt_ >= 0.0) return holdAt_;
    return audition_.holds(entry_.path) ? audition_.position() : 0.0;
}

float AuditionPane::playedFraction() const {
    const double length = lengthSeconds();
    if (length <= 0.0) return 0.0f;
    return juce::jlimit(0.0f, 1.0f, (float) (shownSeconds() / length));
}

double AuditionPane::secondsAt(float x) const {
    const auto area = seekArea();
    if (area.getWidth() <= 0.0f) return 0.0;
    return lengthSeconds() * juce::jlimit(0.0, 1.0, (double) ((x - area.getX()) / area.getWidth()));
}

void AuditionPane::resized() {
    auto right = rowBounds().reduced(0, 2);
    power_.setBounds(right.removeFromRight(kPowerW));
    right = rowBounds().reduced(0, 2);
    if (sync_.isVisible()) {
        sync_.setBounds(right.removeFromRight(kButtonW));
        right.removeFromRight(6);
    }
    loop_.setBounds(right.removeFromRight(kButtonW));
    volume_.setBounds(volumeBounds().withTrimmedLeft(kSpeakerW + 4));
}

void AuditionPane::paint(juce::Graphics& g) {
    g.fillAll(Palette::panel);
    g.setColour(Palette::border.withAlpha(alpha::strong));
    g.drawVerticalLine(0, 0.0f, (float) getHeight());
    g.setColour(Palette::border.withAlpha(alpha::muted));
    g.drawHorizontalLine(getHeight() - 1, (float) kPad, (float) (getWidth() - kPad));
    paintMedia(g);
    if (showsPicture()) paintScrub(g);
    paintPlay(g);
    paintTime(g);
    paintSpeaker(g);
    g.setColour(power_.isVisible() ? Palette::warnAmber() : Palette::textDim);
    g.setFont(juce::FontOptions(11.0f));
    g.drawFittedText(power_.isVisible() ? audioOffText() : hint_, hintBounds(), juce::Justification::topLeft, 2);
}

void AuditionPane::paintMedia(juce::Graphics& g) const {
    const auto box = mediaBounds().toFloat();
    if (showsPicture()) {
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(box, 4.0f);
        if (const auto& img = picture_.picture(); img.isValid())
            g.drawImage(img, box.reduced(1.0f), juce::RectanglePlacement::centred);
        if (!(mine() && audition_.playing()) && !scrubbing_) paintPlayOverPicture(g);
        return;
    }
    g.setColour(Palette::background);
    g.fillRoundedRectangle(box, 4.0f);
    const auto peaks = box.reduced(4.0f, 5.0f);
    paint::drawPeaks(g, entry_.facts.peaks, peaks, Palette::textDim.withAlpha(alpha::strong));
    if (const float played = playedFraction(); played > 0.0f) {
        {
            juce::Graphics::ScopedSaveState keep(g);
            g.reduceClipRegion(box.withWidth(box.getWidth() * played).toNearestInt());
            paint::drawPeaks(g, entry_.facts.peaks, peaks, Palette::accent);
        }
        g.setColour(Palette::accent);
        g.fillRect(box.getX() + box.getWidth() * played, box.getY(), 1.5f, box.getHeight());
    }
}

void AuditionPane::paintScrub(juce::Graphics& g) const {
    const auto bar = scrubBounds().toFloat();
    const auto track = bar.withSizeKeepingCentre(bar.getWidth(), 4.0f);
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(track, 2.0f);
    const float played = playedFraction();
    g.setColour(Palette::accent);
    g.fillRoundedRectangle(track.withWidth(track.getWidth() * played), 2.0f);
    const float x = track.getX() + track.getWidth() * played;
    g.fillEllipse(juce::Rectangle<float>(10.0f, 10.0f).withCentre({x, track.getCentreY()}));
}

void AuditionPane::paintPlay(juce::Graphics& g) const {
    const auto r = playBounds();
    const bool playing = audition_.playing() && mine();
    g.setColour(playing ? Palette::accent : Palette::panelLight);
    g.fillEllipse(r);
    g.setColour(playing ? Palette::background : Palette::text);
    const auto c = r.getCentre();
    if (playing) {
        g.fillRoundedRectangle(c.x - 5.0f, c.y - 5.5f, 3.5f, 11.0f, 1.0f);
        g.fillRoundedRectangle(c.x + 1.5f, c.y - 5.5f, 3.5f, 11.0f, 1.0f);
        return;
    }
    juce::Path tri;
    tri.addTriangle(c.x - 3.5f, c.y - 5.5f, c.x - 3.5f, c.y + 5.5f, c.x + 6.5f, c.y);
    g.fillPath(tri);
}

void AuditionPane::paintPlayOverPicture(juce::Graphics& g) const {
    const auto c = mediaBounds().toFloat().getCentre();
    g.setColour(juce::Colours::black.withAlpha(alpha::dim));
    g.fillEllipse(juce::Rectangle<float>(44.0f, 44.0f).withCentre(c));
    juce::Path tri;
    tri.addTriangle(c.x - 6.0f, c.y - 10.0f, c.x - 6.0f, c.y + 10.0f, c.x + 11.0f, c.y);
    g.setColour(juce::Colours::white.withAlpha(alpha::nearOpaque));
    g.fillPath(tri);
}

void AuditionPane::paintSpeaker(juce::Graphics& g) const {
    const auto r = volumeBounds().removeFromLeft(kSpeakerW).toFloat();
    const auto c = r.getCentre();
    juce::Path cone;
    cone.startNewSubPath(c.x - 7.0f, c.y - 3.0f);
    cone.lineTo(c.x - 3.5f, c.y - 3.0f);
    cone.lineTo(c.x + 1.0f, c.y - 6.5f);
    cone.lineTo(c.x + 1.0f, c.y + 6.5f);
    cone.lineTo(c.x - 3.5f, c.y + 3.0f);
    cone.lineTo(c.x - 7.0f, c.y + 3.0f);
    cone.closeSubPath();
    g.setColour(Palette::textDim);
    g.fillPath(cone);
    juce::Path wave;
    wave.addCentredArc(c.x + 2.0f, c.y, 4.5f, 4.5f, 0.0f, 0.6f, 2.55f, true);
    g.strokePath(wave, juce::PathStrokeType(1.3f));
}

void AuditionPane::paintTime(juce::Graphics& g) const {
    const double length = lengthSeconds();
    if (length <= 0.0) return;
    const bool placed = (audition_.holds(entry_.path) || scrubbing_ || holdAt_ >= 0.0) && length >= kClockFrom;
    const auto text = placed ? paint::clockText(shownSeconds()) + " / " + paint::clockText(length)
                             : paint::lengthText(length);
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(11.5f));
    g.drawText(text, rowBounds().withTrimmedLeft(kRowH + kGap), juce::Justification::centredLeft, false);
}

juce::String AuditionPane::audioOffText() const {
    return showsPicture() ? tr("browser.audio-off-video", "Audio is off, so video previews stand still")
                          : tr("browser.audio-off", "Audio is off, so previews are silent");
}

void AuditionPane::togglePlay() {
    if (mine()) audition_.toggle();
    else audition_.play(entry_.path, entry_.facts.bpm);
    repaint();
}

void AuditionPane::mouseDown(const juce::MouseEvent& e) {
    scrubbing_ = !power_.isVisible() && seekArea().expanded(0.0f, 4.0f).contains(e.position) && lengthSeconds() > 0.0;
    if (scrubbing_) mouseDrag(e);
}

void AuditionPane::mouseDrag(const juce::MouseEvent& e) {
    if (!scrubbing_) return;
    scrubAt_ = secondsAt(e.position.x);
    if (audition_.holds(entry_.path)) audition_.seek(scrubAt_);
    repaint();
}

void AuditionPane::mouseUp(const juce::MouseEvent& e) {
    if (std::exchange(scrubbing_, false)) {
        if (!audition_.holds(entry_.path)) {
            audition_.play(entry_.path, entry_.facts.bpm);
            audition_.seek(scrubAt_);
            holdAt_ = scrubAt_;
        }
        repaint();
        return;
    }
    if (power_.isVisible()) return;
    const bool onPicture = showsPicture() && mediaBounds().toFloat().contains(e.position);
    if (playBounds().contains(e.position) || onPicture) togglePlay();
}

}
