#include "gui/PatcherCanvas.h"

#include "gui/IconGlyph.h"

#include <algorithm>
#include <cmath>

#include "core/Categories.h"
#include "core/PodModel.h"
#include "gui/AppSettings.h"

namespace hum {

void PatcherCanvas::refresh() {
    if (!scope_.empty() && !pods::isPod(host_.model(), scope_)) {
        exitToScope("");
        return;
    }
    updateContentSize();
    repaint();
}

void PatcherCanvas::setZoom(float z, juce::Point<int> anchor) {
    z = juce::jlimit(kZoomMin, kZoomMax, z);
    if (std::abs(z - zoom_) < 1.0e-4f) return;
    auto* vp = findParentComponentOfClass<juce::Viewport>();
    const auto model = (anchor.toFloat() / zoom_);
    zoom_ = z;
    updateContentSize();
    zoomBar_.repaint();
    if (vp != nullptr) {
        const auto want = (model * zoom_).roundToInt() - (anchor - vp->getViewPosition());
        vp->setViewPosition(std::max(0, want.x), std::max(0, want.y));
    }
    repaint();
}

void PatcherCanvas::resetView() {
    zoom_ = 1.0f;
    updateContentSize();
    zoomBar_.repaint();
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        vp->setViewPosition(0, 0);
    repaint();
}

void PatcherCanvas::mouseWheelMove(const juce::MouseEvent& e,
                                   const juce::MouseWheelDetails& w) {
    if (!e.mods.isCommandDown()) { Component::mouseWheelMove(e, w); return; }
    setZoom(zoom_ * (1.0f + w.deltaY * 0.6f), e.getPosition());
}

void PatcherCanvas::mouseMagnify(const juce::MouseEvent& e, float factor) {
    setZoom(zoom_ * factor, e.getPosition());
}

void PatcherCanvas::updateContentSize() {
    int right = 0, bottom = 0;
    for (auto& d : displayNodes()) {
        const auto b = nodeBounds(d.name);
        right = std::max(right, b.getRight());
        bottom = std::max(bottom, b.getBottom());
    }
    int w = (int) std::lround((right + 240) * zoom_);
    int h = (int) std::lround((bottom + 240) * zoom_);
    if (auto* vp = findParentComponentOfClass<juce::Viewport>()) {
        w = std::max(w, vp->getMaximumVisibleWidth());
        h = std::max(h, vp->getMaximumVisibleHeight());
    }
    if (w != getWidth() || h != getHeight()) setSize(w, h);
    setBufferedToImage((long long) w * h <= 6000000);
}

juce::Point<int> PatcherCanvas::viewOffset() const {
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        return vp->getViewPosition();
    return {};
}

void PatcherCanvas::autoScrollWhileDragging(const juce::MouseEvent& e) {
    if (auto* vp = findParentComponentOfClass<juce::Viewport>()) {
        const auto rel = e.getEventRelativeTo(vp).getPosition();
        vp->autoScroll(rel.x, rel.y, 24, 12);
    }
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
    cordSelected_ = cordHovered_ = portHover_ = cordTarget_ = false;
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
        vp->addAndMakeVisible(zoomBar_);
        zoomBar_.layoutInViewport();
        updateCrumbBar();
    }
}

void PatcherCanvas::updateCrumbBar() {
    crumbBar_.setVisible(!scope_.empty());
    crumbBar_.layoutInViewport();
    crumbBar_.repaint();
}

void PatcherCanvas::ZoomBar::layoutInViewport() {
    if (auto* vp = dynamic_cast<juce::Viewport*>(getParentComponent()))
        setBounds(vp->getMaximumVisibleWidth() - kZoomCell * 4 - 8,
                  vp->getMaximumVisibleHeight() - kZoomBarH - 8,
                  kZoomCell * 4, kZoomBarH);
}

void PatcherCanvas::ZoomBar::paint(juce::Graphics& g) {
    g.setColour(Palette::panel.withAlpha(0.92f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 1.0f);
    const bool atRest = std::abs(canvas_.zoom() - 1.0f) < 1.0e-4f;
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("-", cell(0), juce::Justification::centred);
    g.drawText("+", cell(2), juce::Justification::centred);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(juce::String(juce::roundToInt(canvas_.zoom() * 100.0f)) + "%",
               cell(1), juce::Justification::centred);
    drawIconGlyph(g, IconGlyph::Undo, cell(3).withSizeKeepingCentre(13, 13).toFloat(),
                  atRest ? Palette::border : Palette::text, !atRest);
}

void PatcherCanvas::ZoomBar::mouseDown(const juce::MouseEvent& e) {
    const auto mid = canvas_.getLocalBounds().getCentre();
    if (cell(0).contains(e.getPosition())) canvas_.setZoom(canvas_.zoom() / 1.2f, mid);
    else if (cell(2).contains(e.getPosition())) canvas_.setZoom(canvas_.zoom() * 1.2f, mid);
    else if (cell(3).contains(e.getPosition())) canvas_.resetView();
    else canvas_.setZoom(1.0f, mid);
    repaint();
}

void PatcherCanvas::CrumbBar::layoutInViewport() {
    if (auto* vp = dynamic_cast<juce::Viewport*>(getParentComponent()))
        setBounds(0, 0, vp->getMaximumVisibleWidth(), kCrumbH);
}

void PatcherCanvas::CrumbBar::mouseDown(const juce::MouseEvent& e) {
    for (auto& [r, target] : crumbs_)
        if (r.contains(e.getPosition())) { canvas_.exitToScope(target); return; }
}

void PatcherCanvas::timerCallback() {
    if (!isShowing()) return;
    if (AppSettings::instance().getInt("ui.flowLights", 1) == 0) {
        if (!flow_.empty()) { flow_.clear(); repaint(); }
        return;
    }
    const auto now = juce::Time::getMillisecondCounter();
    for (auto& d : displayNodes()) {
        float peak = 0.0f, midi = 0.0f;
        if (d.pod) {
            for (auto& n : pods::outlets(host_.model(), d.name)) {
                float pk, m;
                host_.nodeActivity(n, pk, m);
                peak = std::max(peak, pk);
                midi += m;
            }
        } else {
            host_.nodeActivity(d.name, peak, midi);
        }
        auto& f = flow_[d.name];
        int level = f.level;
        if (peak > 0.25f) level = 2;
        else if (level == 2 && peak < 0.15f) level = 1;
        if (peak > 0.003f) level = juce::jmax(level, 1);
        else if (peak < 0.001f) level = 0;
        bool dirty = level != f.level;
        f.level = level;
        if (f.midiCount < 0.0f) f.midiCount = midi;
        if (midi != f.midiCount) {
            f.midiCount = midi;
            if (f.flashUntil < now) dirty = true;
            f.flashUntil = now + 160;
        } else if (f.flashUntil != 0 && now >= f.flashUntil) {
            f.flashUntil = 0;
            dirty = true;
        }
        const bool off = host_.bypassed(d.name);
        if (!f.bypassSeen || off != f.bypassed) {
            f.bypassed = off;
            f.bypassSeen = true;
            dirty = true;
        }
        if (dirty) repaintNode(d.name);
    }
}

void PatcherCanvas::repaintNode(const std::string& name) {
    lastNodeRepaint_ =
        (nodeBounds(name).expanded(8).toFloat() * zoom_).getSmallestIntegerContainer();
    repaint(lastNodeRepaint_);
}

void PatcherCanvas::notifySelection() {
    if (onSelect) onSelect(selection_.size() == 1 ? primary_ : std::string());
}

}
