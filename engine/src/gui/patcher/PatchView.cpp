// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatchView.h"

#include <algorithm>
#include <cmath>

#include "gui/app/AppSettings.h"
#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

void PatchView::setZoom(float z, juce::Point<int> anchor) {
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

void PatchView::resetView() {
    zoom_ = 1.0f;
    updateContentSize();
    zoomBar_.repaint();
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        vp->setViewPosition(0, 0);
    repaint();
}

void PatchView::frameContent() {
    auto* vp = findParentComponentOfClass<juce::Viewport>();
    if (vp == nullptr || vp->getMaximumVisibleWidth() <= 0) return;
    juce::Rectangle<int> box;
    for (const auto& d : displayNodes())
        box = box.isEmpty() ? nodeBounds(d.name) : box.getUnion(nodeBounds(d.name));
    if (box.isEmpty()) {
        resetView();
        return;
    }
    constexpr int kFramePad = 48;
    box = box.expanded(kFramePad);
    const float fit = std::min((float) vp->getMaximumVisibleWidth() / (float) box.getWidth(),
                               (float) vp->getMaximumVisibleHeight() / (float) box.getHeight());
    zoom_ = juce::jlimit(kZoomMin, 1.0f, fit);
    updateContentSize();
    zoomBar_.repaint();
    const auto centre = viewPos(box.getCentre());
    vp->setViewPosition(centre.x - vp->getMaximumVisibleWidth() / 2, centre.y - vp->getMaximumVisibleHeight() / 2);
    repaint();
}

void PatchView::mouseWheelMove(const juce::MouseEvent& e,
                                   const juce::MouseWheelDetails& w) {
    if (!e.mods.isCommandDown()) { Component::mouseWheelMove(e, w); return; }
    setZoom(zoom_ * (1.0f + w.deltaY * 0.6f), e.getPosition());
}

void PatchView::mouseMagnify(const juce::MouseEvent& e, float factor) {
    setZoom(zoom_ * factor, e.getPosition());
}

void PatchView::updateContentSize() {
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

juce::Point<int> PatchView::viewOffset() const {
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        return vp->getViewPosition();
    return {};
}

void PatchView::autoScrollWhileDragging(const juce::MouseEvent& e) {
    if (auto* vp = findParentComponentOfClass<juce::Viewport>()) {
        const auto rel = e.getEventRelativeTo(vp).getPosition();
        vp->autoScroll(rel.x, rel.y, 24, 12);
    }
}

void PatchView::ZoomBar::layoutInViewport() {
    if (auto* vp = dynamic_cast<juce::Viewport*>(getParentComponent()))
        setBounds(vp->getMaximumVisibleWidth() - kZoomCell * 4 - 8,
                  vp->getMaximumVisibleHeight() - kZoomBarH - 8,
                  kZoomCell * 4, kZoomBarH);
}

void PatchView::ZoomBar::paint(juce::Graphics& g) {
    g.setColour(Palette::panel.withAlpha(alpha::nearOpaque));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 1.0f);
    const bool atRest = std::abs(view_.zoom() - 1.0f) < 1.0e-4f;
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("-", cell(0), juce::Justification::centred);
    g.drawText("+", cell(2), juce::Justification::centred);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(juce::String(juce::roundToInt(view_.zoom() * 100.0f)) + "%",
               cell(1), juce::Justification::centred);
    drawIconGlyph(g, IconGlyph::Undo, cell(3).withSizeKeepingCentre(13, 13).toFloat(),
                  atRest ? Palette::border : Palette::text, !atRest);
}

void PatchView::ZoomBar::mouseDown(const juce::MouseEvent& e) {
    const auto mid = view_.getLocalBounds().getCentre();
    if (cell(0).contains(e.getPosition())) view_.setZoom(view_.zoom() / 1.2f, mid);
    else if (cell(2).contains(e.getPosition())) view_.setZoom(view_.zoom() * 1.2f, mid);
    else if (cell(3).contains(e.getPosition())) view_.resetView();
    else view_.setZoom(1.0f, mid);
    repaint();
}

void PatchView::timerCallback() {
    if (!isShowing()) return;
    if (const auto stamp = src_.changeStamp(); stamp != paintedStamp_) {
        paintedStamp_ = stamp;
        repaint();
    }
    if (AppSettings::instance().getInt("ui.flowLights", 1) == 0) {
        if (!flow_.empty()) { flow_.clear(); repaint(); }
        return;
    }
    const auto now = juce::Time::getMillisecondCounter();
    for (auto& d : displayNodes()) {
        float peak = 0.0f, midi = 0.0f;
        if (d.pod) {
            for (auto& n : pods::outlets(src_.model(), d.name)) {
                float pk, m;
                src_.nodeActivity(n, pk, m);
                peak = std::max(peak, pk);
                midi += m;
            }
        } else {
            src_.nodeActivity(d.name, peak, midi);
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
        const bool off = src_.bypassed(d.name);
        if (!f.bypassSeen || off != f.bypassed) {
            f.bypassed = off;
            f.bypassSeen = true;
            dirty = true;
        }
        if (dirty) repaintNode(d.name);
    }
}

void PatchView::repaintNode(const std::string& name) {
    lastNodeRepaint_ =
        (nodeBounds(name).expanded(8).toFloat() * zoom_).getSmallestIntegerContainer();
    repaint(lastNodeRepaint_);
}

void PatchView::parentHierarchyChanged() {
    if (auto* vp = findParentComponentOfClass<juce::Viewport>();
        vp != nullptr && zoomBar_.getParentComponent() != vp) {
        vp->addAndMakeVisible(zoomBar_);
        zoomBar_.layoutInViewport();
    }
}

juce::Point<int> PatchView::snapMove(juce::Point<int> d, juce::Point<int> leadOrigin, bool free) {
    alignGuideX_ = alignGuideY_ = -1;
    if (free) return d;
    juce::Point<int> lead = leadOrigin + d;
    constexpr int kReach = 6;
    int bestDx = kReach + 1, bestDy = kReach + 1, snapX = 0, snapY = 0;
    for (auto& other : displayNodes()) {
        if (selection_.count(other.name) > 0) continue;
        const auto ob = nodeBounds(other.name);
        if (std::abs(ob.getX() - lead.x) < bestDx) {
            bestDx = std::abs(ob.getX() - lead.x);
            snapX = ob.getX();
        }
        if (std::abs(ob.getY() - lead.y) < bestDy) {
            bestDy = std::abs(ob.getY() - lead.y);
            snapY = ob.getY();
        }
    }
    if (bestDx <= kReach) { lead.x = snapX; alignGuideX_ = snapX; }
    else lead.x = ((lead.x + kGrid / 2) / kGrid) * kGrid;
    if (bestDy <= kReach) { lead.y = snapY; alignGuideY_ = snapY; }
    else lead.y = ((lead.y + kGrid / 2) / kGrid) * kGrid;
    return lead - leadOrigin;
}

bool PatchView::fitsDrag(const PortRef& t) const {
    return drag_ == Drag::Cord && !t.outlet && t.node != dragNode_ && t.midi == cordMidi_
           && t.video == cordVideo_ && t.control == cordControl_;
}

bool PatchView::isDragAnchor(const PortRef& t) const {
    return drag_ == Drag::Cord && t.outlet && t.node == dragNode_ && t.port == cordOutlet_
           && t.midi == cordMidi_ && t.video == cordVideo_ && t.control == cordControl_;
}

}
