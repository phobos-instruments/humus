// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "gui/editor/BrickBindings.h"
#include "gui/editor/inputs/RingInputs.h"
#include "gui/editor/readouts/RingGeometry.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"

namespace hum {

class RingFaceView : public PolledBrick, public juce::SettableTooltipClient {
public:
    RingFaceView(BrickHost& host, std::string name, const Bindings& bound)
        : PolledBrick(host, name),
          mutes_(host, name, bound(bind::kHitsPrefix), bound(bind::kTurnPrefix), bound(bind::kMutePrefix)) {
        picture_.read(host_, name_);
        readMuted();
        if (mutes_.editable())
            setTooltip(tr("ring-face.click-a-hit", "Click a hit to silence it, and again to bring it back"));
    }

    int preferredContentWidth() const override { return 220; }
    int preferredContentHeight(int) const override { return 220; }

    const readout::RingPicture& pictureForTest() const { return picture_; }

    void reloadValues() override { poll(); }

    void paint(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(ink::ring::ground);
        g.fillRoundedRectangle(bounds, 6.0f);
        const auto centre = bounds.getCentre();
        const float outer = outerRadius();
        const int count = (int) picture_.rings.size();
        for (int r = 0; r < count; ++r) drawRing(g, r, count, centre, outer);
        g.setColour(ink::ring::frame);
        g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!mutes_.editable() || e.mods.isPopupMenu()) return;
        const auto centre = getLocalBounds().toFloat().reduced(1.0f).getCentre();
        const auto dot = readout::ringDotAt(picture_, e.position.x - centre.x, e.position.y - centre.y, outerRadius());
        if (dot.ring < 0) return;
        if (mutes_.toggle(dot.ring, dot.step, picture_.rings[(size_t) dot.ring].steps)) poll();
    }

private:
    float outerRadius() const {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        return std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f - 14.0f;
    }

    void drawRing(juce::Graphics& g, int r, int count, juce::Point<float> centre, float outer) const {
        const auto& ring = picture_.rings[(size_t) r];
        const std::uint64_t muted = r < (int) muted_.size() ? muted_[(size_t) r] : 0;
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
            if ((muted >> s) & 1u) {
                g.setColour(ink.withAlpha(alpha::scrim));
                g.fillEllipse(dot);
                g.setColour(ink.withAlpha(now ? alpha::heavy : alpha::dim));
                g.drawEllipse(dot.reduced(0.5f), now ? 1.6f : 1.0f);
                continue;
            }
            g.setColour(ink::ring::ground);
            g.fillEllipse(dot);
            g.setColour(now ? ink.withAlpha(alpha::heavy) : ink::ring::track.brighter(0.25f));
            g.drawEllipse(dot.reduced(0.5f), now ? 1.6f : 1.0f);
        }
    }

    bool readMuted() {
        std::vector<std::uint64_t> now(picture_.rings.size());
        for (size_t r = 0; r < now.size(); ++r) now[r] = mutes_.mutedHits((int) r, picture_.rings[r].steps);
        if (now == muted_) return false;
        muted_ = std::move(now);
        return true;
    }

    void poll() override {
        const bool moved = picture_.read(host_, name_);
        if (readMuted() || moved) repaint();
    }

    readout::RingPicture picture_;
    input::RingMuteModel mutes_;
    std::vector<std::uint64_t> muted_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RingFaceView)
};

}
