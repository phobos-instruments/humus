// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/UiTicker.h"
#include "gui/editor/files/Marquee.h"

namespace hum {

class RowMarquee : private juce::MouseListener {
public:
    explicit RowMarquee(juce::ListBox& list) : list_(list) { list_.addMouseListener(this, true); }
    ~RowMarquee() override {
        list_.removeMouseListener(this);
        rest();
    }

    int row() const { return row_; }

    double offsetFor(int row, double textW, double roomW) const {
        if (row != row_) return 0.0;
        return files::marqueeOffset(textW, roomW, juce::Time::getMillisecondCounterHiRes() - sinceMs_);
    }

    void hoverForTest(int row, double msAgo) {
        hover(row);
        sinceMs_ = juce::Time::getMillisecondCounterHiRes() - msAgo;
    }
    bool runningForTest() const { return ticker_ != 0; }

private:
    void mouseMove(const juce::MouseEvent& e) override {
        const auto p = e.getEventRelativeTo(&list_).getPosition();
        hover(list_.getRowContainingPosition(p.x, p.y));
    }
    void mouseExit(const juce::MouseEvent& e) override {
        if (!list_.getLocalBounds().contains(e.getEventRelativeTo(&list_).getPosition())) hover(-1);
    }

    void hover(int row) {
        if (row == row_) return;
        const int was = row_;
        row_ = row;
        sinceMs_ = juce::Time::getMillisecondCounterHiRes();
        if (was >= 0) list_.repaintRow(was);
        if (row_ < 0) { rest(); return; }
        if (ticker_ == 0)
            ticker_ = UiTicker::instance().add([this] { if (row_ >= 0) list_.repaintRow(row_); });
    }

    void rest() {
        if (ticker_ != 0) UiTicker::instance().remove(ticker_);
        ticker_ = 0;
    }

    juce::ListBox& list_;
    int row_ = -1;
    double sinceMs_ = 0.0;
    int ticker_ = 0;
};

}
