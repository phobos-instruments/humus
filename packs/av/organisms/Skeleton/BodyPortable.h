// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <string>

#include <juce_graphics/juce_graphics.h>

#include "Skeleton/BodyPose.h"

namespace hum {

struct BodyTrack {
    bool tracking = false;
    float cx = 0.0f, cy = 0.0f, ax = 0.0f, ay = 0.0f;
};

bool bodyPortableReady(const std::string& modelPath);
bool bodyBuiltInReady();
bool detectBodyLandmarksPortable(const juce::Image& frame, bool mirror,
                                 float minConfidence, BodyLandmarks& out,
                                 BodyTrack& track);

}
