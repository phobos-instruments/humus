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
