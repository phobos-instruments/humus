// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/patcher/PatcherCanvas.h"

#include "gui/style/IconGlyph.h"

#include <algorithm>
#include <cmath>

#include "core/packs/Categories.h"
#include "core/graph/PodModel.h"
#include "gui/app/AppSettings.h"

namespace hum {

void PatcherCanvas::refresh() {
    if (!scope_.empty() && !pods::isPod(host_.model(), scope_)) {
        exitToScope("");
        return;
    }
    updateContentSize();
    repaint();
}

void PatcherCanvas::enterPod(const std::string& pod) { exitToScope(pod); }

void PatcherCanvas::select(const std::string& n) {
    bool navigated = false;
    if (!n.empty() && host_.model().byName(n) != nullptr) {
        const auto home = pods::parentOf(n);
        if (home != scope_) { exitToScope(home); navigated = true; }
    }
    selection_.clear();
    if (!n.empty()) selection_.insert(n);
    primary_ = n;
    repaint();
    if (n.empty()) return;
    if (auto* vp = findParentComponentOfClass<juce::Viewport>()) {
        const auto b = nodeBounds(n);
        const auto vis = vp->getViewArea();
        if (!b.isEmpty() && (navigated || !vis.intersects(b)))
            vp->setViewPosition(juce::jmax(0, b.getCentreX() - vis.getWidth() / 2),
                                juce::jmax(0, b.getCentreY() - vis.getHeight() / 2));
    }
}

void PatcherCanvas::exitToScope(const std::string& scope) {
    if (scope_ == scope) return;
    scope_ = scope;
    selection_.clear();
    primary_.clear();
    selectedCords_.clear();
    cordHovered_ = portHover_ = cordTarget_ = false;
    drag_ = Drag::None;
    notifySelection();
    updateCrumbBar();
    refresh();
}

void PatcherCanvas::parentHierarchyChanged() {
    if (auto* vp = findParentComponentOfClass<juce::Viewport>();
        vp != nullptr && crumbBar_.getParentComponent() != vp) {
        vp->addChildComponent(crumbBar_);
        crumbBar_.layoutInViewport();
        PatchView::parentHierarchyChanged();
        updateCrumbBar();
    }
}

void PatcherCanvas::updateCrumbBar() {
    crumbBar_.setVisible(!scope_.empty());
    crumbBar_.layoutInViewport();
    crumbBar_.repaint();
}

void PatcherCanvas::CrumbBar::layoutInViewport() {
    if (auto* vp = dynamic_cast<juce::Viewport*>(getParentComponent()))
        setBounds(0, 0, vp->getMaximumVisibleWidth(), kCrumbH);
}

void PatcherCanvas::CrumbBar::mouseDown(const juce::MouseEvent& e) {
    for (auto& [r, target] : crumbs_)
        if (r.contains(e.getPosition())) { canvas_.exitToScope(target); return; }
}

void PatcherCanvas::notifySelection() {
    if (onSelect) onSelect(selection_.size() == 1 ? primary_ : std::string());
}

}
