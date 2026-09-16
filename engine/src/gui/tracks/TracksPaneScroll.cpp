// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TracksPane.h"

#include <algorithm>
#include <cmath>

namespace hum {

namespace {
constexpr double kRollRowLo = 4.0, kRollRowHi = 44.0;
double zoomLo(int axis, bool roll) { return axis == 0 ? 2.0 : roll ? kRollRowLo : (double) SongView::kRowHMin; }
}

double TracksPane::zoomNorm(int axis) const {
    const bool roll = mode_ == Mode::Track;
    const double lo = zoomLo(axis, roll);
    const double hi = axis == 0 ? maxPpb() : roll ? kRollRowHi : (double) SongView::kRowHMax;
    const double v = axis == 0 ? view_.ppb : roll ? (double) rollView_.rowHeight() : (double) song_.rowHeight();
    return juce::jlimit(0.0, 1.0, std::log(v / lo) / std::log(hi / lo));
}

void TracksPane::setZoomNorm(int axis, double t) {
    const bool roll = mode_ == Mode::Track;
    const double lo = zoomLo(axis, roll);
    const double hi = axis == 0 ? maxPpb() : roll ? kRollRowHi : (double) SongView::kRowHMax;
    const double v = lo * std::pow(hi / lo, juce::jlimit(0.0, 1.0, t));
    if (axis == 0) setPixelsPerBeat(v);
    else if (roll) { rollView_.setRowHeight((float) v); repaint(); }
    else { song_.setRowHeight((int) std::lround(v)); repaint(); }
}

void TracksPane::zoomBy(int axis, double factor) {
    if (axis == 0) {
        setPixelsPerBeat(view_.ppb * factor);
    } else if (mode_ == Mode::Track) {
        rollView_.setRowHeight((float) juce::jlimit(kRollRowLo, kRollRowHi, rollView_.rowHeight() * factor));
    } else {
        song_.setRowHeight(juce::jlimit(SongView::kRowHMin, SongView::kRowHMax,
                                        (int) std::lround(song_.rowHeight() * factor)));
    }
    repaint();
}

void TracksPane::zoomAbout(float x, double factor) {
    const double beatAt = view_.xToBeat(x, kStripW);
    view_.ppb = juce::jlimit(2.0, maxPpb(), view_.ppb * factor);
    view_.scrollBeats = juce::jlimit(0.0, std::max(0.0, contentEndBeat() - 1.0),
                                     beatAt - (x - kStripW) / view_.ppb);
    repaint();
}

void TracksPane::mouseMagnify(const juce::MouseEvent& e, float scaleFactor) {
    zoomAbout((float) e.getPosition().x, scaleFactor);
}

void TracksPane::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
        zoomAbout((float) e.getPosition().x, wheel.deltaY > 0 ? 1.15 : 1.0 / 1.15);
        return;
    }
    const auto scrollBy = [this](double d) {
        view_.scrollBeats = std::max(0.0, view_.scrollBeats - d * 0.5 * (getWidth() - kStripW) / view_.ppb);
    };
    if (e.mods.isShiftDown() || mode_ == Mode::Clip) {
        scrollBy(std::abs(wheel.deltaX) > std::abs(wheel.deltaY) ? wheel.deltaX : wheel.deltaY);
    } else if (mode_ == Mode::Track) {
        rollView_.mouseWheelMove(e.getEventRelativeTo(&rollView_), wheel);
        return;
    } else {
        if (wheel.deltaX != 0.0f) scrollBy(wheel.deltaX);
        if (wheel.deltaY != 0.0f) {
            traceView("vscroll");
            song_.applyVScroll(view_.vScroll - (int) std::round(wheel.deltaY * 240.0));
        }
    }
    repaint();
}

bool TracksPane::edgeScroll(juce::Point<int> p) {
    constexpr int kMargin = 30, kStep = 14;
    const int left = kStripW, right = getWidth() - kZoomGut;
    double reach = 0.0;
    if (p.x > right - kMargin)     reach = (p.x - (right - kMargin)) / (double) kMargin;
    else if (p.x < left + kMargin) reach = (p.x - (left + kMargin)) / (double) kMargin;

    int rows = 0;
    if (mode_ == Mode::Song) {
        if (p.y > fieldBottom() - kMargin) rows = 1;
        else if (p.y < tracksgeo::headerH() + kMargin && view_.vScroll > 0) rows = -1;
    }
    if (std::abs(reach) < 1e-9 && rows == 0) {
        juce::Desktop::getInstance().beginDragAutoRepeat(0);
        return false;
    }
    const double was = view_.scrollBeats;
    view_.scrollBeats = std::max(0.0, view_.scrollBeats
                                     + juce::jlimit(-1.0, 1.0, reach) * kStep / view_.ppb);
    if (rows != 0) {
        traceView("vscroll");
        song_.applyVScroll(view_.vScroll + rows * 12);
    }
    juce::Desktop::getInstance().beginDragAutoRepeat(16);
    if (view_.scrollBeats != was) repaint();
    return true;
}

}
