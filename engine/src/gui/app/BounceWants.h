// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/video/VideoEncoder.h"
#include "io/Mp3Writer.h"

namespace hum {

struct BounceWants {
    bool audio = true, video = false, midi = false;
    std::string soundKind = "wav";
    int mp3Rate = kMp3RateBest;
    std::string videoNode;
    MovieKind kind = MovieKind::H264;
    int quality = kQualityDefault;
    int width = 1280, height = 720;
    double fps = 30.0;
    double fromBeat = 0.0, toBeat = 0.0;
    juce::File folder;
    juce::String stem;

    juce::File fileFor(const char* extension) const {
        return folder.getChildFile(stem + "." + juce::String(extension));
    }
    juce::File movieFile() const { return fileFor(movieKindExtension(kind)); }
    juce::File soundFile() const { return fileFor(soundKind.c_str()); }
    juce::File notesFile() const { return fileFor("mid"); }
    bool nothingChosen() const { return !audio && !video && !midi; }
};

struct BounceOffer {
    std::vector<std::string> videoNodes;
    bool hasSound = false;
    bool hasNotes = false;
    bool cameraOnly = false;
    double tempo = 120.0;
    double songEndBeat = 0.0;
    double loopFrom = 0.0, loopTo = 0.0;
    double selectFrom = 0.0, selectTo = 0.0;
    juce::File folder;
    juce::String stem;
};

}
