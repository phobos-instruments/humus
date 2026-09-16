// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>

#include <juce_core/juce_core.h>

#include "gui/video/VideoEncoder.h"

namespace hum {

std::unique_ptr<VideoEncoder> makeNativeMovieWriter(const juce::File& file, int width, int height,
                                                    double fps, int quality, bool live);

}
