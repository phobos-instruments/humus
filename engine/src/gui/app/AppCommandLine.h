// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_core/juce_core.h>

namespace hum::appcli {

struct Outcome {
    bool handled = false;
    int returnValue = 0;
};

Outcome run(const juce::StringArray& args);

}
