// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserPlaces.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum::browser {

class Breadcrumbs : public juce::Component {
public:
    std::function<void(const std::string& dir)> onChoose;

    void setTrail(std::vector<NamedFolder> trail, juce::String after) {
        trail_ = std::move(trail);
        after_ = std::move(after);
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto boxes = layout();
        for (size_t i = 0; i < boxes.size(); ++i) {
            const bool last = i + 1 == trail_.size();
            g.setColour(last ? Palette::text : (i == (size_t) hover_ ? Palette::accent : Palette::textDim));
            g.setFont(juce::FontOptions(11.5f, last ? juce::Font::bold : juce::Font::plain));
            g.drawText(label(i), boxes[i], juce::Justification::centredLeft, true);
            if (!last) {
                g.setColour(Palette::textDim.withAlpha(alpha::mid));
                g.drawText(">", boxes[i].withX(boxes[i].getRight()).withWidth(kSep), juce::Justification::centred, false);
            }
        }
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.5f));
        const float x = boxes.empty() ? 0.0f : boxes.back().getRight() + 12.0f;
        g.drawText(after_, juce::Rectangle<float>(x, 0.0f, (float) getWidth() - x, (float) getHeight()),
                   juce::Justification::centredLeft, true);
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int over = crumbAt(e.position);
        if (over != hover_) { hover_ = over; repaint(); }
        setMouseCursor(over >= 0 && over + 1 < (int) trail_.size() ? juce::MouseCursor::PointingHandCursor
                                                                   : juce::MouseCursor::NormalCursor);
    }
    void mouseExit(const juce::MouseEvent&) override { hover_ = -1; repaint(); }
    void mouseUp(const juce::MouseEvent& e) override {
        const int i = crumbAt(e.position);
        if (i >= 0 && i + 1 < (int) trail_.size() && onChoose) onChoose(trail_[(size_t) i].path);
    }

private:
    static constexpr float kSep = 16.0f;

    juce::String label(size_t i) const { return juce::String::fromUTF8(trail_[i].label.c_str()); }

    std::vector<juce::Rectangle<float>> layout() const {
        std::vector<juce::Rectangle<float>> out;
        float x = 0.0f;
        for (size_t i = 0; i < trail_.size(); ++i) {
            const float w = juce::GlyphArrangement::getStringWidth(juce::FontOptions(11.5f, juce::Font::bold), label(i)) + 2.0f;
            out.push_back({x, 0.0f, w, (float) getHeight()});
            x += w + kSep;
        }
        return out;
    }

    int crumbAt(juce::Point<float> p) const {
        const auto boxes = layout();
        for (int i = 0; i < (int) boxes.size(); ++i)
            if (boxes[(size_t) i].contains(p)) return i;
        return -1;
    }

    std::vector<NamedFolder> trail_;
    juce::String after_;
    int hover_ = -1;
};

}
