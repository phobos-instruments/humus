#include "gui/MainComponent.h"

#include "gui/AppSettings.h"
#include "gui/NotesView.h"

namespace hum {

void MainComponent::buildDock() {
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

    patcherView_.setViewedComponent(canvas_.get(), false);
    patcherView_.setScrollBarsShown(true, true);
    paneCenter_.setContent(&patcherView_);
    paneRight_.setContent(propsPane_.get());
    paneBottom_.setContent(tracksPane_.get());
    paneBottom_.setTitle("Timeline");
    addAndMakeVisible(paneCenter_);
    addAndMakeVisible(paneRight_);
    addAndMakeVisible(paneBottom_);
    addAndMakeVisible(splitV_);
    addAndMakeVisible(splitH_);

    center_ = {&paneCenter_, &patcherView_, &viewPatcher_, "Patcher"};
    right_  = {&paneRight_, propsPane_.get(), &viewProperties_, "Properties"};
    bottom_ = {&paneBottom_, tracksPane_.get(), &viewAutomation_, "Timeline"};
    bottom_.wantsTransport = true;

    paneCenter_.onClose = [this] { viewPatcher_.setToggleState(false, juce::sendNotification); };
    paneRight_.onClose  = [this] { viewProperties_.setToggleState(false, juce::sendNotification); };
    paneBottom_.onClose = [this] { viewAutomation_.setToggleState(false, juce::sendNotification); };
    paneCenter_.onDetach = [this] { detach(center_); };
    paneRight_.onDetach  = [this] { detach(right_); };
    paneBottom_.onDetach = [this] { detach(bottom_); };

    splitV_.onDrag = [this](int d) {
        rightW_ = juce::jlimit(180, juce::jmax(200, getWidth() - 320), rightW_ - d);
        resized();
    };
    splitH_.onDrag = [this](int d) {
        bottomH_ = juce::jlimit(80, juce::jmax(120, getHeight() - 220), bottomH_ - d);
        resized();
    };
    splitV_.onDragEnd = [this] { persistDock(); };
    splitH_.onDragEnd = [this] { persistDock(); };
}

void MainComponent::updateDock() {
    auto vis = [](Dockable& d) { return d.toggle->getToggleState() && !d.floated; };
    paneCenter_.setVisible(vis(center_));
    paneRight_.setVisible(vis(right_));
    paneBottom_.setVisible(vis(bottom_));
    resized();
}

void MainComponent::detach(Dockable& d) {
    if (d.floated) return;
    if (d.content == propsPane_.get()) propsPane_->releaseEmbeddedEditors();
    d.floated = true;
    d.toggle->setFloated(true);
    if (d.wantsTransport) {
        d.strip = std::make_unique<TransportStrip>(host_);
        d.strip->onPlay          = [this] { playBtn_.triggerClick(); };
        d.strip->onPlayFromStart = [this] { playFromStartBtn_.triggerClick(); };
        d.strip->onStop          = [this] { stopBtn_.triggerClick(); };
        d.strip->onRecord        = [this] { recordBtn_.triggerClick(); };
        d.strip->onLoop          = [this] { loopBtn_.triggerClick(); };
        d.strip->onTempoMenu     = [this](juce::Point<int> at) { showTempoMenu(at); };
    }
    d.win = std::make_unique<FloatingPaneWindow>(d.pane->title(), d.content, d.floatBounds,
                                                 [this, &d] { redock(d); },
                                                 d.strip.get(),
                                                 d.strip ? TransportStrip::kHeight : 0);
    d.win->onUnhandledKey = [this](const juce::KeyPress& k) { return keyPressed(k); };
    d.win->onUnhandledKeyState = [this](bool down) { return keyStateChanged(down); };
    updateDock();
    persistDock();
}

void MainComponent::redock(Dockable& d) {
    if (!d.floated) return;
    if (d.content == propsPane_.get()) propsPane_->releaseEmbeddedEditors();
    if (d.win) d.floatBounds = d.win->getBounds();
    d.floated = false;
    if (d.win) d.win->releaseContent();
    d.strip.reset();
    d.pane->setContent(d.content);
    if (auto* w = d.win.release())
        juce::MessageManager::callAsync([w] { delete w; });
    d.toggle->setToggleState(true, juce::dontSendNotification);
    d.toggle->setFloated(false);
    updateDock();
    persistDock();
}

void MainComponent::persistDock() {
    auto& s = AppSettings::instance();
    s.beginBatch();
    s.set("dock.rightW", rightW_);
    s.set("dock.bottomH", bottomH_);
    auto save = [&](Dockable& d) {
        s.set("dock." + d.key + ".visible", d.toggle->getToggleState() ? 1 : 0);
        s.set("dock." + d.key + ".floated", d.floated ? 1 : 0);
        auto b = (d.floated && d.win) ? d.win->getBounds() : d.floatBounds;
        s.set("dock." + d.key + ".bounds", b.toString());
    };
    save(center_); save(right_); save(bottom_);
    s.endBatch();
}

void MainComponent::restoreDock() {
    auto& s = AppSettings::instance();
    rightW_ = s.getInt("dock.rightW", rightW_);
    bottomH_ = s.getInt("dock.bottomH", bottomH_);
    auto load = [&](Dockable& d) {
        d.toggle->setToggleState(s.getInt("dock." + d.key + ".visible", 1) != 0, juce::dontSendNotification);
        auto bs = s.getString("dock." + d.key + ".bounds", "");
        if (bs.isNotEmpty()) d.floatBounds = juce::Rectangle<int>::fromString(bs);
        if (s.getInt("dock." + d.key + ".floated", 0) != 0) detach(d);
    };
    load(center_); load(right_); load(bottom_);
    paneBottom_.setTitle("Timeline");
    updateDock();
}

void MainComponent::refreshTimelinePanes() {
    if (tracksPane_) tracksPane_->rebuild();
    if (canvas_) canvas_->refresh();
    if (notesWindow_ != nullptr)
        if (auto* v = dynamic_cast<NotesView*>(notesWindow_->getContentComponent()))
            v->reload();
}

void MainComponent::openClipEditor(const std::string& node, int clip) {
    if (!viewProperties_.getToggleState())
        viewProperties_.setToggleState(true, juce::sendNotification);
    propsPane_->openFor(node);
    propsPane_->expandFor(node);
    if (auto* editor = propsPane_->editorFor(node)) editor->openClip(clip);
}

}
