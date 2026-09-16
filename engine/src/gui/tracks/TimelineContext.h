// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_graphics/juce_graphics.h>

namespace hum {

class TracksHost;
namespace noteedit { enum class Tool; }

class TimelineContext {
public:
    virtual ~TimelineContext() = default;
    virtual TracksHost& timelineHost() = 0;
    virtual double gridBeats() const = 0;
    virtual double snapBeats(double beat, bool bypass) const = 0;
    virtual noteedit::Tool effectiveTool() const = 0;
    virtual void selectTool(noteedit::Tool tool) = 0;
    virtual bool edgeScroll(juce::Point<int> panePoint) = 0;
    virtual void beatsChanged(double from, double to) = 0;
    virtual void viewChanged() = 0;
};

}
