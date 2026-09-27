// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/graph/TapeNames.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "gui/editor/inputs/NumberInputs.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class TapeLabelBrick : public PolledBrick {
public:
    TapeLabelBrick(BrickHost& host, std::string organism, std::string param, int firstInlet, int width)
        : PolledBrick(host, organism, 4), text_(host, organism, std::move(param)), firstInlet_(firstInlet),
          width_(width) {
        editor_.setJustification(juce::Justification::centred);
        editor_.onReturnKey = [this] { commit(); };
        editor_.onFocusLost = [this] { commit(); };
        editor_.onEscapeKey = [this] { editor_.setVisible(false); };
        addChildComponent(editor_);
        shown_ = currentName();
    }

    std::string shownForTest() const { return currentName(); }
    void renameForTest(const std::string& typed) { store(typed); }

    int preferredContentWidth() const override { return 60; }
    int preferredContentHeight(int) const override { return 20; }
    void resized() override { editor_.setBounds(getLocalBounds()); }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(Palette::text.withAlpha(alpha::heavy));
        g.fillRoundedRectangle(r, 2.0f);
        g.setColour(Palette::panel.withAlpha(alpha::veil));
        for (float x = r.getX() + 3.0f; x < r.getRight(); x += 6.0f) g.fillRect(x, r.getY(), 1.0f, 2.0f);
        g.setColour(Palette::panel);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawFittedText(juce::String::fromUTF8(shown_.c_str()), getLocalBounds().reduced(4, 0),
                         juce::Justification::centred, 1, 0.7f);
    }

    void mouseDoubleClick(const juce::MouseEvent&) override { beginRename(); }
    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) return;
        juce::PopupMenu m;
        m.addItem(1, tr("tape-label.rename", "Rename"));
        m.addItem(2, tr("tape-label.use-connected", "Use the connected name"), !text_.text().empty());
        m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
            if (r == 1) beginRename();
            if (r == 2) store({});
        });
    }

private:
    std::string corded() const { return tape::fromCords(host_.model(), name_, firstInlet_, width_); }
    std::string currentName() const { return tape::shown(text_.text(), corded()); }

    void poll() override {
        if (editor_.isVisible()) return;
        if (auto now = currentName(); now != shown_) {
            shown_ = std::move(now);
            repaint();
        }
    }

    void beginRename() {
        editor_.setColour(juce::TextEditor::backgroundColourId, Palette::background);
        editor_.setColour(juce::TextEditor::textColourId, Palette::text);
        editor_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accent);
        editor_.setText(juce::String::fromUTF8(shown_.c_str()), juce::dontSendNotification);
        editor_.setVisible(true);
        editor_.grabKeyboardFocus();
        editor_.selectAll();
    }

    void commit() {
        if (!editor_.isVisible()) return;
        editor_.setVisible(false);
        store(editor_.getText().toStdString());  // utf8-ok
    }

    void store(const std::string& typed) {
        text_.commit(tape::toStore(typed, corded()));
        shown_ = currentName();
        repaint();
    }

    input::TextInput text_;
    int firstInlet_ = 0, width_ = 2;
    std::string shown_;
    juce::TextEditor editor_;
};

}
