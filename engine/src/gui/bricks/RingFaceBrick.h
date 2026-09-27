// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/RingGeometry.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"

namespace hum {

class RingFaceView : public PolledBrick {
public:
    RingFaceView(BrickHost& host, std::string name) : PolledBrick(host, std::move(name)) {
        picture_.read(host_, name_);
    }

    int preferredContentWidth() const override { return 220; }
    int preferredContentHeight(int) const override { return 220; }

    const readout::RingPicture& pictureForTest() const { return picture_; }

    void paint(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(ink::ring::ground);
        g.fillRoundedRectangle(bounds, 6.0f);
        const auto centre = bounds.getCentre();
        const float outer = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f - 14.0f;
        const int count = (int) picture_.rings.size();
        for (int r = 0; r < count; ++r) drawRing(g, r, count, centre, outer);
        g.setColour(ink::ring::frame);
        g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
    }

private:
    void drawRing(juce::Graphics& g, int r, int count, juce::Point<float> centre, float outer) const {
        const auto& ring = picture_.rings[(size_t) r];
        const float radius = readout::ringRadius(r, count, outer);
        const auto ink = ink::ring::inks[r % 4];
        const bool silent = ring.hits == 0;
        g.setColour(ink::ring::track);
        g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
        const float size = readout::dotSize(ring.steps, radius);
        for (int s = 0; s < ring.steps; ++s) {
            const auto spot = readout::ringSpot(s, ring.steps, radius);
            const juce::Rectangle<float> dot(centre.x + spot.x - size * 0.5f, centre.y + spot.y - size * 0.5f, size, size);
            const bool hit = (ring.hits >> s) & 1u;
            const bool now = s == ring.now && !silent;
            if (hit) {
                g.setColour(now ? ink::ring::now : ink);
                g.fillEllipse(now ? dot.expanded(2.0f) : dot);
                continue;
            }
            g.setColour(ink::ring::ground);
            g.fillEllipse(dot);
            g.setColour(now ? ink.withAlpha(alpha::heavy) : ink::ring::track.brighter(0.25f));
            g.drawEllipse(dot.reduced(0.5f), now ? 1.6f : 1.0f);
        }
    }

    void poll() override {
        if (picture_.read(host_, name_)) repaint();
    }

    readout::RingPicture picture_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RingFaceView)
};

}
