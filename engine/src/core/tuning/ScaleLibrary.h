// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

struct ScaleInfo {
    std::string path;
    std::string name;
    std::string collection;
    std::string description;
    int degrees = 0;
    double periodCents = 0.0;
};

juce::File userScalesDir();
std::vector<juce::File> scaleLibraryRoots();

std::vector<ScaleInfo> scanScaleDirs(const std::vector<juce::File>& roots);

std::string latin1FallbackToUtf8(const std::string& raw);

}
