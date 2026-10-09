// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"

#include "core/params/RollScope.h"
#include "gui/app/FloatingWindows.h"
#include "gui/app/TransportStrip.h"
#include "gui/common/Localisation.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"
#include "gui/tracks/TracksPane.h"

namespace hum {

void MainComponent::buildPatchPanes() {
    canvas_ = std::make_unique<PatcherCanvas>(host_);
    propsPane_ = std::make_unique<PropertiesPane>(host_);
    canvas_->onActivate = [this](const std::string& n) { propsPane_->openFor(n); };

    canvas_->onSelect = [this](const std::string& n) { propsPane_->setSelected(n); };
    propsPane_->onSelect = [this](const std::string& n) {
        canvas_->select(n);
        canvas_->refresh();
    };

    canvas_->onAddRequest = [this](const std::string& cls, juce::Point<int> at) { addAt(cls, at); };
    propsPane_->onOpenPluginUI = [this](const std::string& n) { openPluginUI(n); };
    propsPane_->onFloatKey = [this](const juce::KeyPress& k) { return keyPressed(k); };
    propsPane_->onFloatKeyState = [this](bool down) { return keyStateChanged(down); };
    propsPane_->onAddOrganism = [this](const std::string& cls) {
        auto centre = patcherView_.getViewArea().getCentre();
        addAt(cls, {centre.x - 75, centre.y - 36});
    };
    canvas_->onRandomisePod = [this](const std::string& pod) { rollScope(pod); };
    canvas_->onMapPodRandom = [this](const std::string& pod, juce::Point<int> at) {
        showAutomateMenu(host_, host_.clockNodeName(), rollscope::podAction(pod), at, nullptr, false);
    };
    canvas_->onOpenPluginUI = [this](const std::string& n) { openPluginUI(n); };
    canvas_->onOpenVisuals = [this](const std::string& n) { openVisualUI(n); };
    canvas_->onRecordNode = [this](const std::string& tk) {
        refreshTimelinePanes();
        if (!viewAutomation_.getToggleState())
            viewAutomation_.setToggleState(true, juce::sendNotification);
        notify(tr("main-panes.armed-press-record-then-play",
                  "Armed \"{take}\" - press {record} Record, then {play} Play to capture the take",
                  {{"take", juce::String(tk)},
                   {"record", juce::String::fromUTF8("\xe2\x8f\xba")},
                   {"play", juce::String::fromUTF8("\xe2\x96\xb6")}}));
    };
    canvas_->onUndoRedo = [this] { propsPane_->reload(); refreshTimelinePanes(); };
    propsPane_->onAutomationChanged = [this] {
        refreshTimelinePanes();
        if (!viewAutomation_.getToggleState())
            viewAutomation_.setToggleState(true, juce::sendNotification);
    };
}

void MainComponent::buildTracksPane() {
    tracksPane_ = std::make_unique<TracksPane>(host_);
    tracksPane_->onOpenClip = [this](const std::string& n, int clip) { openClipEditor(n, clip); };
    tracksPane_->onPatchChanged = [this] { refreshTimelinePanes(); };
    tracksPane_->onSelect = [this](const std::string& n) {
        if (canvas_) canvas_->select(n);
        if (propsPane_) propsPane_->setSelected(n);
    };
    host_.noteCaptureHint = [this] {
        return tracksPane_ ? tracksPane_->selectedNoteNode() : std::string();
    };
    tracksPane_->onOpenAutomation = [this](const std::string& node, const std::string&) {
        if (tracksPane_) tracksPane_->expandRow(node);
    };
    tracksPane_->onOpenBoxDetail = [this](const std::string& organism) {
        if (tracksPane_) tracksPane_->enterBoxMode(organism);
    };
}

void MainComponent::rebuildPanes() {
    Dockable* docks[] = {&center_, &right_, &bottom_};
    bool floated[3] = {};
    for (int i = 0; i < 3; ++i) {
        floated[i] = docks[i]->floated;
        if (floated[i]) redock(*docks[i]);
    }
    const float zoom = canvas_->zoom();
    const auto view = patcherView_.getViewPosition();
    propsPane_->releaseEmbeddedEditors();
    patcherView_.setViewedComponent(nullptr, false);
    paneRight_.setContent(nullptr);
    paneBottom_.setContent(nullptr);
    tracksPane_.reset();
    propsPane_.reset();
    canvas_.reset();

    buildPatchPanes();
    buildTracksPane();
    patcherView_.setViewedComponent(canvas_.get(), false);
    paneRight_.setContent(propsPane_.get());
    paneBottom_.setContent(tracksPane_.get());
    right_.content = propsPane_.get();
    bottom_.content = tracksPane_.get();
    canvas_->setZoom(zoom, {});
    patcherView_.setViewPosition(view);
    for (int i = 0; i < 3; ++i)
        if (floated[i]) detach(*docks[i]);
    propsPane_->syncFromModel();
    refreshTimelinePanes();
    updateDock();
}

}
