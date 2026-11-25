// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin/PatchPanes.h"

#include "gui/common/Localisation.h"

namespace hum {

PatchPanes::PatchPanes(EngineHost& host, PluginWindows& windows, PaneLayout& layout)
    : host_(host), windows_(windows), layout_(layout), canvas_(host), props_(host),
      patchFrame_(tr("main-dock.patcher", "Patcher")), propsFrame_(tr("main-dock.properties", "Properties")) {
    patchView_.setViewedComponent(&canvas_, false);
    patchView_.setScrollBarsShown(true, true);
    patchFrame_.setContent(&patchView_);
    propsFrame_.setContent(&props_);
    for (auto* pane : {&patchFrame_, &propsFrame_}) {
        pane->showButtons(false, true);
        addChildComponent(*pane);
    }
    patchFrame_.onClose = [this] { setShown(Pane::Patcher, false); };
    propsFrame_.onClose = [this] { setShown(Pane::Properties, false); };
    addChildComponent(split_);
    split_.onDrag = [this](int dx) { dragSplit(dx); };

    canvas_.onActivate = [this](const std::string& n) { props_.openFor(n); };
    canvas_.onSelect = [this](const std::string& n) { props_.setSelected(n); };
    props_.onSelect = [this](const std::string& n) {
        canvas_.select(n);
        canvas_.refresh();
    };
    canvas_.onAddRequest = [this](const std::string& cls, juce::Point<int> at) { addAt(cls, at); };
    canvas_.onUndoRedo = [this] { props_.reload(); };
    canvas_.onOpenPluginUI = [this](const std::string& n) { windows_.open(n); };
    props_.onOpenPluginUI = [this](const std::string& n) { windows_.open(n); };
    props_.onAddOrganism = [this](const std::string& cls) {
        const auto centre = patchView_.getViewArea().getCentre();
        addAt(cls, {centre.x - 75, centre.y - 36});
    };
    windows_.beforeOpen = [this](const std::string& n) { props_.releaseEmbedded(n); };
    wireHost();
    refresh();
}

PatchPanes::~PatchPanes() {
    windows_.beforeOpen = nullptr;
    host_.onBeforeRebuild = nullptr;
    host_.onTopologyChanged = nullptr;
    host_.onNodeRolled = nullptr;
    host_.onPanelEdit = nullptr;
}

void PatchPanes::wireHost() {
    host_.onBeforeRebuild = [this](const std::function<bool(const std::string&)>& survives) {
        windows_.dropWhere([&](const std::string& n) { return !survives(n); });
        props_.releaseEmbeddedEditorsWhere([&](const std::string& n) { return !survives(n); });
    };
    host_.onTopologyChanged = [this] {
        refresh();
        if (onTopologyChanged) onTopologyChanged();
    };
    host_.onNodeRolled = [this](const std::string& n) { props_.reloadValuesFor(n); };
    host_.onPanelEdit = [this](const std::string& n) { props_.reloadValuesFor(n); };
}

void PatchPanes::refresh() {
    canvas_.refresh();
    props_.reload();
}

void PatchPanes::addAt(const std::string& className, juce::Point<int> at) {
    const auto name = host_.addOrganism(className, at, canvas_.scope());
    canvas_.select(name);
    props_.openFor(name);
    canvas_.refresh();
}

bool PatchPanes::setShown(Pane pane, bool shown) {
    auto& flag = pane == Pane::Patcher ? layout_.patcher : layout_.properties;
    const bool other = pane == Pane::Patcher ? layout_.properties : layout_.patcher;
    if (!shown && !other) return false;
    flag = shown;
    resized();
    if (onLayoutChanged) onLayoutChanged();
    return true;
}

void PatchPanes::dragSplit(int dx) {
    const docklayout::Span side{layout_.sideWidth - dx, kMinSide, kMinPatch + SplitterBar::kThick};
    layout_.sideWidth = docklayout::clampSpan(side, getWidth());
    resized();
}

juce::Rectangle<int> PatchPanes::paneBoundsForTest(Pane pane) const {
    const auto& frame = pane == Pane::Patcher ? patchFrame_ : propsFrame_;
    return frame.isVisible() ? frame.getBounds() : juce::Rectangle<int>();
}

void PatchPanes::resized() {
    patchFrame_.setVisible(layout_.patcher);
    propsFrame_.setVisible(layout_.properties);
    split_.setVisible(layout_.patcher && layout_.properties);
    const docklayout::Shown shown{layout_.patcher, layout_.properties, false};
    const docklayout::Span side{layout_.sideWidth, kMinSide, kMinPatch + SplitterBar::kThick};
    const auto areas = docklayout::lay(getLocalBounds(), shown, side, {}, SplitterBar::kThick);
    patchFrame_.setBounds(areas.center);
    propsFrame_.setBounds(areas.right);
    split_.setBounds(areas.splitV);
    canvas_.refresh();
}

}
