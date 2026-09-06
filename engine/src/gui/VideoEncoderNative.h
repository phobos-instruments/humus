#pragma once
#include <memory>

#include <juce_core/juce_core.h>

#include "gui/VideoEncoder.h"

namespace hum {

std::unique_ptr<VideoEncoder> makeNativeMovieWriter(const juce::File& file, int width, int height,
                                                    double fps, int quality, bool live);

}
