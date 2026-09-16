// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

inline constexpr int kWindowFloorW = 200;
inline constexpr int kWindowFloorH = 140;
inline constexpr int kWindowMaxSide = 10000;
inline constexpr int kFreeShrinkW = 380;
inline constexpr int kFreeShrinkH = 280;

juce::BorderSize<int> windowFrameSize(const juce::ResizableWindow& w);
void setWindowMinimumSize(juce::ResizableWindow& w, int contentW, int contentH);
void refreshWindowMinimum(const juce::ResizableWindow& w, int contentW, int contentH);
juce::Point<int> windowManagerMinimum(const juce::ResizableWindow& w);

}
