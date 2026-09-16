// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_graphics/juce_graphics.h>

namespace hum::tracksgeo {

constexpr int kTopH = 21;
constexpr int kStripW = 200, kLoopH = 14, kRulerH = 24;
constexpr int kLoopGrip = 7, kLoopHit = 8;
constexpr int kToolCount = 5;
constexpr int kChipH = 22, kFadeGrip = 9, kClipEdgeGrab = 6, kClipRibbonH = 18;
constexpr double kClipMaxPpb = 40000.0;

constexpr int headerH() { return kTopH + kLoopH + kRulerH; }
constexpr int loopTop() { return kTopH; }
constexpr int rulerTop() { return kTopH + kLoopH; }

inline juce::Rectangle<int> toolBox(int i) { return {6 + i * 24, rulerTop() + 1, 22, kRulerH - 3}; }
inline juce::Rectangle<int> snapBox() { return {kStripW - 44, rulerTop() + 1, 40, kRulerH - 3}; }
inline juce::Rectangle<int> addTrackBox() { return {6 + kToolCount * 24, rulerTop() + 1, 22, kRulerH - 3}; }
inline juce::Rectangle<int> followBox() { return {kStripW - 88, 1, 84, kTopH - 2}; }

}
