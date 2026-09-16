// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <string>

#include <juce_graphics/juce_graphics.h>

#include "Hands/HandPose.h"

namespace hum {

bool handsPortableReady(const std::string& modelPath);
bool handsBuiltInReady();

int detectHandLandmarksPortable(const juce::Image& frame, bool mirror, float minConfidence,
                                HandLandmarks* out, int maxHands);

}
