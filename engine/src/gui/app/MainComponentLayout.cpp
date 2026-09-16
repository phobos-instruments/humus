// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/common/PerfLog.h"
#include "gui/app/MainComponent.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"
#include "gui/tracks/TracksPane.h"

#include "gui/app/Duff.h"
#include "gui/style/LookAndFeel.h"
#include "gui/app/QwertyPiano.h"
#include "gui/app/ToolbarLadder.h"

namespace hum {

#if JUCE_MAC
namespace { constexpr int kMenuH = 0, kRowH = 30; }
#else
namespace { constexpr int kMenuH = 24, kRowH = 30; }
#endif
namespace { constexpr int kRailW = 38; }

bool MainComponent::keyPressed(const juce::KeyPress& k) {
    if (QwertyPiano::instance().handleKeyPress(k)) return true;
    using juce::KeyPress;
    const auto cmd = juce::ModifierKeys::commandModifier;
    const auto cmdShift = cmd | juce::ModifierKeys::shiftModifier;
    if (k == KeyPress('z', cmdShift, 0)) { canvas_->redo(); return true; }
    if (k == KeyPress('z', cmd, 0)) { canvas_->undo(); return true; }
    if (k == KeyPress('a', cmd, 0)) { canvas_->selectAll(); return true; }
    if (k == KeyPress('c', cmd, 0)) { canvas_->copySelection(); return true; }
    if (k == KeyPress('x', cmd, 0)) { canvas_->cutSelection(); return true; }
    if (k == KeyPress('v', cmd, 0)) { canvas_->pasteClipboard(); return true; }
    if (k == KeyPress('d', cmd, 0)) { canvas_->duplicateSelection(); return true; }
    if (k == KeyPress('r', cmd, 0)) { host_.record().captureToggle(); return true; }
    if (k == KeyPress(juce::KeyPress::F2Key)) { canvas_->renameSelection(); return true; }
    if (k == KeyPress('s', cmdShift, 0)) { savePatchAs(); return true; }
    if (k == KeyPress('s', cmd, 0)) { savePatch(); return true; }
    if (k == KeyPress('n', cmd, 0)) { newPatch(); return true; }
    if (k == KeyPress('o', cmd, 0)) { openPatch(); return true; }
    if (k == KeyPress('q', cmd, 0)) { juce::JUCEApplication::getInstance()->systemRequestedQuit(); return true; }
    if (k == KeyPress('w', cmd, 0)) { closeProject(); return true; }
    if (k == KeyPress(',', cmd, 0)) { openSettings(); return true; }
    if (k == KeyPress(juce::KeyPress::spaceKey)) { togglePlay(); return true; }
    if (k == KeyPress(juce::KeyPress::F3Key)) { openParameterControl(); return true; }
    if (k == KeyPress(juce::KeyPress::F9Key)) { openDocSwitcher(); return true; }
    if (k.getKeyCode() == KeyPress::deleteKey || k.getKeyCode() == KeyPress::backspaceKey) {
        canvas_->deleteSelection(); return true;
    }
    return false;
}

bool MainComponent::keyStateChanged(bool) {
    return QwertyPiano::instance().handleKeyState();
}

void MainComponent::demoScene() {
    auto h = host_.addOrganism("10Harmonics", {250, 120});
    auto h2 = host_.addOrganism("SDelay", {250, 250});
    auto out = host_.model().organisms.empty() ? std::string() : std::string();
    std::string soundOut = host_.masterOutputName();
    if (!soundOut.empty()) {
        host_.setPosition(soundOut, {250, 380});
        host_.connect(h, 0, soundOut, 0);
        host_.connect(h, 1, soundOut, 1);
    }
    canvas_->select(h);
    propsPane_->openFor(h);
    propsPane_->openFor(h2);
    if (!soundOut.empty()) propsPane_->openFor(soundOut);
    propsPane_->setSelected(h);

    host_.automation().add(h, "Frequency");
    host_.automation().addPoint(h, "Frequency", 4.0, 1200.0);
    host_.automation().addPoint(h, "Frequency", 8.0, 200.0);
    if (tracksPane_) tracksPane_->rebuild();

    canvas_->refresh();
    (void) out;
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    if (!timeWell_.isEmpty()) {
        const auto w = timeWell_.toFloat();
        g.setColour(Palette::background.darker(0.45f));
        g.fillRoundedRectangle(w, 4.0f);
        g.setColour(Palette::border.darker(0.25f));
        g.drawRoundedRectangle(w.reduced(0.5f), 4.0f, 1.0f);
        g.setColour(Palette::panelLight.withAlpha(alpha::muted));
        g.drawLine(w.getX() + 4.0f, w.getBottom() - 0.5f,
                   w.getRight() - 4.0f, w.getBottom() - 0.5f, 1.0f);
    }
    g.setColour(Palette::border);
    g.fillRect(0, kMenuH + kRowH, getWidth(), 1);
    g.fillRect(kRailW - 1, kMenuH + kRowH, 1, getHeight());

    for (size_t i = 0; i < sepX_.size(); ++i)
        if (hypha_[i & 1].isValid())
            g.drawImageAt(hypha_[i & 1], sepX_[i] - 3, kMenuH);
}

void MainComponent::rebuildChromeTextures() {
    if (!hypha_[0].isValid()) {
        hypha_[0] = duff::hyphaImage(kRowH, 0);
        hypha_[1] = duff::hyphaImage(kRowH, 1);
    }
}

void MainComponent::resized() {
    perf::Scope scope("main.resized");
    auto area = getLocalBounds();
    rebuildChromeTextures();
    menuBar_.setBounds(area.removeFromTop(kMenuH));

    auto iconIn = [](juce::Rectangle<int>& row, IconButton& b) {
        b.setBounds(row.removeFromLeft(28).reduced(0, 3)); row.removeFromLeft(3);
    };
    sepX_.clear();
    auto sep = [this](juce::Rectangle<int>& row) {
        sepX_.push_back(row.removeFromLeft(toolbar::kSep).getCentreX());
    };

    using namespace toolbar;
    const int shed = shedFor(area.getWidth() - kInsets);

    const bool showLocate = shed < ShedLocate, showTaps = shed < ShedSetup;
    const bool showClick = shed < ShedClick, showLink = shed < ShedLink;
    const bool showDsp = shed < ShedDsp, showQwerty = shed < ShedSetup;
    const bool showLimiter = shed < ShedLimiter, showKeep = shed < ShedKeep;
    const bool showCaptions = shed < ShedCaptions;
    goStartBtn_.setVisible(showLocate); goEndBtn_.setVisible(showLocate);
    loopBtn_.setVisible(showLocate);
    tapBtn_.setVisible(showTaps); beat1Btn_.setVisible(showTaps);
    metroBtn_.setVisible(showClick);
    linkBtn_.setVisible(showLink);
    dspLabel_.setVisible(showDsp);
    qwertyBtn_.setVisible(showQwerty);
    limiterBtn_.setVisible(showLimiter);
    globalDiceBtn_.setVisible(showKeep); keepBtn_.setVisible(showKeep);
    masterCaption_.setVisible(showCaptions);
    groove_.setCompact(!showCaptions);
    overflowBtn_.setVisible(shed > 0);

    auto row2 = area.removeFromTop(kRowH).reduced(6, 0);
    auto iconW = [](juce::Rectangle<int>& row, IconButton& b, int w) {
        b.setBounds(row.removeFromLeft(w).reduced(0, 3)); row.removeFromLeft(3);
    };
    iconW(row2, playFromStartBtn_, 28);
    iconW(row2, playBtn_, kPlayW);
    iconW(row2, stopBtn_, 28);
    iconW(row2, recordBtn_, 28);
    sep(row2);
    if (showLocate) {
        iconW(row2, goStartBtn_, kNavW); iconW(row2, goEndBtn_, kNavW); iconW(row2, loopBtn_, kNavW);
        sep(row2);
    }
    if (showKeep) { iconIn(row2, globalDiceBtn_); iconIn(row2, keepBtn_); }
    iconIn(row2, undoBtn_); iconIn(row2, redoBtn_);
    sep(row2);
    meter_.setBounds(row2.removeFromRight(kMeterW).reduced(0, 6));
    row2.removeFromRight(6);
    if (showDsp) {
        dspLabel_.setBounds(row2.removeFromRight(58));
        row2.removeFromRight(2);
    }
    masterLevel_.setBounds(row2.removeFromRight(kOutW));
    if (showCaptions) masterCaption_.setBounds(row2.removeFromRight(kOutCaptionW));
    row2.removeFromRight(4);
    if (showLimiter) {
        limiterBtn_.setBounds(row2.removeFromRight(kLimW).reduced(0, 6));
        row2.removeFromRight(4);
    }
    groove_.setBounds(row2.removeFromRight(groove_.preferredWidth()));
    row2.removeFromRight(8);
    const int wellX = row2.getX();
    clock_.setBounds(row2.removeFromLeft(128).reduced(0, 4));
    sep(row2);
    tempo_.setBounds(row2.removeFromLeft(kTempoW));
    row2.removeFromLeft(3);
    tsig_.setBounds(row2.removeFromLeft(toolbar::kTsigW).reduced(0, 4));
    row2.removeFromLeft(4);
    if (showTaps) {
        tapBtn_.setBounds(row2.removeFromLeft(toolbar::kTapW).reduced(0, 3));
        row2.removeFromLeft(toolbar::kGap);
        beat1Btn_.setBounds(row2.removeFromLeft(toolbar::kBeatW).reduced(0, 3));
        row2.removeFromLeft(toolbar::kGap);
    }
    if (showClick) {
        metroBtn_.setBounds(row2.removeFromLeft(toolbar::kClickW).reduced(0, 3));
        row2.removeFromLeft(toolbar::kGap);
    }
    if (showLink) linkBtn_.setBounds(row2.removeFromLeft(toolbar::kLinkW).reduced(0, 3));
    timeWell_ = juce::Rectangle<int>(wellX - 6, row2.getY() + 2,
                                     row2.getX() - wellX + 10, row2.getHeight() - 4);
    sep(row2);
    iconIn(row2, enableAudioBtn_); iconIn(row2, enableMidiBtn_);
    if (showQwerty) iconIn(row2, qwertyBtn_);
    if (shed > 0) iconIn(row2, overflowBtn_);
    row2.removeFromLeft(8);
    statusLabel_.setBounds(row2);

    auto rail = area.removeFromLeft(kRailW);
    {
        auto col = rail.reduced(0, 4);
        settingsBtn_.setBounds(col.removeFromBottom(28));
        auto railIn = [&col](IconButton& b, int h) {
            b.setBounds(col.removeFromTop(h)); col.removeFromTop(2);
        };
        railIn(viewPatcher_, 30); railIn(viewProperties_, 30); railIn(viewAutomation_, 30);
        col.removeFromTop(10);
        railIn(viewMetapad_, 28); railIn(viewParamControl_, 28); railIn(viewNotes_, 28);
        railIn(viewDocSwitcher_, 28); railIn(viewHelp_, 28); railIn(viewLibrary_, 28);
    }

    splitV_.setVisible(paneCenter_.isVisible() && paneRight_.isVisible());
    splitH_.setVisible(paneBottom_.isVisible() && (paneCenter_.isVisible() || paneRight_.isVisible()));

    if (paneBottom_.isVisible()) {
        int h = juce::jlimit(80, juce::jmax(100, area.getHeight() - 120), bottomH_);
        paneBottom_.setBounds(area.removeFromBottom(h));
        if (splitH_.isVisible()) splitH_.setBounds(area.removeFromBottom(SplitterBar::kThick));
    }
    if (paneRight_.isVisible()) {
        int w = juce::jlimit(180, juce::jmax(200, area.getWidth() - 200), rightW_);
        paneRight_.setBounds(area.removeFromRight(w));
        if (splitV_.isVisible()) splitV_.setBounds(area.removeFromRight(SplitterBar::kThick));
    }
    if (paneCenter_.isVisible()) paneCenter_.setBounds(area);
    if (canvas_) canvas_->refresh();

    if (!startupOutCentered_ && canvas_ && canvas_->getWidth() > 0) {
        startupOutCentered_ = true;
        const auto master = host_.masterOutputName();
        if (!master.empty() && currentFile_.isEmpty()
            && host_.model().organisms.size() == 1) {
            host_.setPosition(master, canvas_->visibleCentreForNode());
            canvas_->refresh();
        }
    }

    placeUpdateNotice();
}

}
