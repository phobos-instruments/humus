// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"

namespace hum {

class StartHereList : public juce::Component {
public:
    struct Row {
        juce::String title, sub;
        std::function<void()> go;
    };

    static constexpr int kRowH = 40, kGap = 4;

    StartHereList(juce::Colour ink, std::vector<Row> rows)
        : ink_(ink), rows_(std::move(rows)) {
        setInterceptsMouseClicks(!rows_.empty(), false);
    }

    int rowCount() const { return (int) rows_.size(); }
    const Row& row(int i) const { return rows_[(size_t) i]; }
    int preferredHeight() const {
        return rows_.empty() ? 0 : (int) rows_.size() * (kRowH + kGap) - kGap;
    }
    void clickRow(int i) {
        if (i >= 0 && i < rowCount() && rows_[(size_t) i].go) rows_[(size_t) i].go();
    }

    void paint(juce::Graphics& g) override {
        for (int i = 0; i < rowCount(); ++i) {
            const auto r = rowBounds(i);
            g.setColour(ink_.withAlpha(i == hover_ ? 0.12f : 0.05f));
            g.fillRoundedRectangle(r.toFloat(), 4.0f);
            const auto inner = r.reduced(10, 5);
            g.setColour(ink_);
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText(rows_[(size_t) i].title, inner.getX(), inner.getY(), inner.getWidth(), 17,
                       juce::Justification::centredLeft, true);
            g.setColour(ink_.withAlpha(alpha::dim));
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText(rows_[(size_t) i].sub, inner.getX(), inner.getY() + 16, inner.getWidth(), 14,
                       juce::Justification::centredLeft, true);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int h = rowAt(e.getPosition());
        if (h != hover_) { hover_ = h; repaint(); }
        setMouseCursor(h >= 0 ? juce::MouseCursor::PointingHandCursor
                              : juce::MouseCursor::NormalCursor);
    }
    void mouseExit(const juce::MouseEvent&) override { hover_ = -1; repaint(); }
    void mouseUp(const juce::MouseEvent& e) override { clickRow(rowAt(e.getPosition())); }

private:
    juce::Rectangle<int> rowBounds(int i) const {
        return {0, i * (kRowH + kGap), getWidth(), kRowH};
    }
    int rowAt(juce::Point<int> p) const {
        for (int i = 0; i < rowCount(); ++i)
            if (rowBounds(i).contains(p)) return i;
        return -1;
    }

    juce::Colour ink_;
    std::vector<Row> rows_;
    int hover_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StartHereList)
};

}
