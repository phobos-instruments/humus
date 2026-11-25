// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin/HumusEditor.h"

#include <algorithm>
#include <vector>

#include "core/app/AppPaths.h"
#include "gui/app/BrandPanel.h"
#include "gui/app/UsageLog.h"
#include "gui/common/CallOut.h"
#include "gui/common/Languages.h"
#include "gui/common/Localisation.h"
#include "gui/common/UiTicker.h"
#include "gui/editor/LearnModes.h"
#include "gui/settings/Appearance.h"
#include "gui/settings/AppearanceSettingsView.h"
#include "gui/video/VideoRenderService.h"
#include "io/PatchFormat.h"
#include "plugin/DesktopSession.h"

namespace hum {

namespace {

std::vector<HumusEditor*>& openEditors() {
    static std::vector<HumusEditor*> editors;
    return editors;
}

juce::LookAndFeel*& lookBefore() {
    static juce::LookAndFeel* before = nullptr;
    return before;
}

constexpr int kBarH = 34, kButtonW = 30, kMacroH = 40, kTitleW = 110, kGutter = 6;

}

juce::AudioProcessorEditor* HumusProcessor::createEditor() { return new HumusEditor(*this); }

HumusEditor::HumusEditor(HumusProcessor& p)
    : juce::AudioProcessorEditor(p), proc_(p), host_(p.host()), windows_(host_), macros_(p),
      start_(IconButton::Glyph::GoToStart, tr("plugin-transport.start", "Back to the start")),
      play_(IconButton::Glyph::Play,
            tr("plugin-transport.play", "Play the patch while the DAW is stopped; the DAW's own Play always runs it")),
      stop_(IconButton::Glyph::Stop,
            tr("plugin-transport.stop", "Stop the patch while the DAW is stopped; press again to go back to the start")),
      undo_(IconButton::Glyph::Undo, tr("humus-editor.undo-tip", "Undo the last change to the patch (Cmd-Z / Ctrl-Z)")),
      redo_(IconButton::Glyph::Redo, tr("humus-editor.redo-tip", "Redo what was undone (Cmd-Shift-Z / Ctrl-Y)")),
      showPatcher_(IconButton::Glyph::Patcher, tr("humus-editor.show-patcher", "Show or hide the patcher")),
      showProperties_(IconButton::Glyph::Properties,
                      tr("humus-editor.show-properties", "Show or hide the properties of the boxes you open")),
      about_(IconButton::Glyph::Help, tr("humus-editor.about", "About Humus: version, credits, website and support")),
      gear_(IconButton::Glyph::Gear, tr("humus-editor.settings", "Settings: themes and language")),
      macrosFrame_(tr("humus-editor.daw-macros", "DAW Macros")) {
    if (openEditors().empty()) {
        lookBefore() = &juce::LookAndFeel::getDefaultLookAndFeel();
        i18n::applySaved();
        applySavedAppearance();
        juce::LookAndFeel::setDefaultLookAndFeel(&laf_.getObject());
    }
    openEditors().push_back(this);
    setLookAndFeel(&laf_.getObject());
    macrosFrame_.showButtons(false, false);
    macrosFrame_.setContent(&macros_);
    addAndMakeVisible(macrosFrame_);

    play_.setLead(true);
    start_.onClick = [this] { host_.goToStart(); };
    play_.onClick = [this] { host_.play(); };
    stop_.onClick = [this] {
        if (host_.isPlaying()) host_.stop();
        else host_.goToStart();
    };
    undo_.onClick = [this] { panes_->canvas().undo(); };
    redo_.onClick = [this] { panes_->canvas().redo(); };
    gear_.onClick = [this] { openSettings(); };
    about_.onClick = [this] { toggleAbout(); };
    showPatcher_.onClick = [this] {
        panes_->setShown(PatchPanes::Pane::Patcher, !panes_->isShown(PatchPanes::Pane::Patcher));
    };
    showProperties_.onClick = [this] {
        panes_->setShown(PatchPanes::Pane::Properties, !panes_->isShown(PatchPanes::Pane::Properties));
    };
    load_.setButtonText(tr("humus-editor.load-patch", "Load patch..."));
    load_.onClick = [this] { chooseFile(); };
    for (auto* c : std::initializer_list<juce::Component*>{&start_, &play_, &stop_, &undo_, &redo_, &showPatcher_,
                                                          &showProperties_, &about_, &gear_, &load_})
        addAndMakeVisible(*c);

    rebuildPanes();
    if (sessionCanOpenWindows()) {
        previews_ = std::make_unique<VideoRenderService>(host_);
        previews_->setSuppressed([this](const std::string& n) { return proc_.videoWindows().isOpen(n); });
    }
    setResizable(true, true);
    setResizeLimits(720, 480, 2600, 1700);
    setSize(1240, 760);
    startTimerHz(30);
}

HumusEditor::~HumusEditor() {
    stopTimer();
    aboutPanel_.reset();
    cancelLearningOn(host_);
    previews_.reset();
    panes_.reset();
    setLookAndFeel(nullptr);
    auto& all = openEditors();
    all.erase(std::remove(all.begin(), all.end(), this), all.end());
    if (all.empty()) juce::LookAndFeel::setDefaultLookAndFeel(lookBefore());
}

void HumusEditor::rebuildPanes() {
    panes_.reset();
    panes_ = std::make_unique<PatchPanes>(host_, windows_, proc_.paneLayout());
    panes_->onTopologyChanged = [this] { macros_.refresh(); };
    panes_->onLayoutChanged = [this] { showPaneToggles(); };
    macrosFrame_.setTitle(tr("humus-editor.daw-macros", "DAW Macros"));
    showPaneToggles();
    panes_->canvas().onOpenVisuals = [this](const std::string& n) { proc_.videoWindows().open(n); };
    addAndMakeVisible(*panes_, 0);
    framePending_ = true;
    resized();
    repaint();
}

void HumusEditor::toggleAbout() {
    if (aboutPanel_ != nullptr) {
        aboutPanel_.reset();
        return;
    }
    AboutWindow::Actions actions;
    actions.onDismiss = [safe = juce::Component::SafePointer<HumusEditor>(this)] {
        juce::MessageManager::callAsync([safe] {
            if (safe != nullptr) safe->aboutPanel_.reset();
        });
    };
    actions.onSupport = [] { usagelog::openDonatePage(); };
    aboutPanel_ = std::make_unique<AboutWindow>(std::move(actions));
    addAndMakeVisible(*aboutPanel_);
    resized();
    aboutPanel_->toFront(true);
}

void HumusEditor::showPaneToggles() {
    showPatcher_.setOn(panes_->isShown(PatchPanes::Pane::Patcher));
    showProperties_.setOn(panes_->isShown(PatchPanes::Pane::Properties));
}

void HumusEditor::openSettings() {
    const auto restyleAll = [] {
        for (auto* editor : openEditors()) {
            editor->laf_->refreshColours();
            editor->rebuildPanes();
            editor->macros_.repaint();
        }
    };
    auto view = std::make_unique<AppearanceSettingsView>(restyleAll);
    view->onLanguageChanged = restyleAll;
    view->setSize(560, 520);
    launchCallOut(std::move(view), gear_.getScreenBounds(), this);
}

void HumusEditor::chooseFile() {
    const auto patches = userPatchesDir();
    chooser_ = std::make_unique<juce::FileChooser>(tr("humus-editor.load-patch-2", "Load patch"),
                                                   patches.isDirectory() ? patches : juce::File(), kPatchOpenFilter);
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this](const juce::FileChooser& fc) {
                              const auto f = fc.getResult();
                              if (f == juce::File{}) return;
                              std::string error;
                              proc_.loadPatch(f, error);
                              panes_->refresh();
                              macros_.refresh();
                              framePending_ = true;
                          });
}

void HumusEditor::timerCallback() {
    UiTicker::instance().setBusy(host_.audioRunning());
    play_.setOn(host_.isPlaying());
    undo_.setEnabled(host_.canUndo());
    redo_.setEnabled(host_.canRedo());
    macros_.refresh();
    if (panes_ != nullptr) panes_->tick();
    if (framePending_ && panes_ != nullptr && panes_->getWidth() > 0) {
        framePending_ = false;
        panes_->frame();
    }
}

bool HumusEditor::keyPressed(const juce::KeyPress& key) {
    return panes_ != nullptr && panes_->canvas().keyPressed(key);
}

void HumusEditor::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    drawHumusEmblem(g, {10.0f, 5.0f, (float) kBarH - 10.0f, (float) kBarH - 10.0f}, Palette::text);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    g.drawText("Humus", 10 + kBarH - 4, 0, 80, kBarH, juce::Justification::centredLeft, false);
}

void HumusEditor::resized() {
    auto r = getLocalBounds();
    auto bar = r.removeFromTop(kBarH).reduced(8, 4);
    bar.removeFromLeft(kTitleW);
    for (auto* b : {&start_, &play_, &stop_}) {
        b->setBounds(bar.removeFromLeft(kButtonW));
        bar.removeFromLeft(2);
    }
    bar.removeFromLeft(12);
    for (auto* b : {&undo_, &redo_}) {
        b->setBounds(bar.removeFromLeft(kButtonW));
        bar.removeFromLeft(2);
    }
    bar.removeFromLeft(12);
    for (auto* b : {&showPatcher_, &showProperties_}) {
        b->setBounds(bar.removeFromLeft(kButtonW));
        bar.removeFromLeft(2);
    }
    gear_.setBounds(bar.removeFromRight(kButtonW));
    bar.removeFromRight(2);
    about_.setBounds(bar.removeFromRight(kButtonW));
    bar.removeFromRight(6);
    load_.setBounds(bar.removeFromRight(120));
    r = r.withTrimmedLeft(kGutter).withTrimmedRight(kGutter).withTrimmedBottom(kGutter);
    macrosFrame_.setBounds(r.removeFromBottom(kMacroH + DockPane::kHeader));
    r.removeFromBottom(kGutter);
    if (panes_ != nullptr) panes_->setBounds(r);
    if (aboutPanel_ != nullptr)
        aboutPanel_->setBounds(getLocalBounds().withSizeKeepingCentre(
            AboutWindow::kW, juce::jmin(AboutWindow::kH, getHeight() - 2 * kBarH)));
}

}
