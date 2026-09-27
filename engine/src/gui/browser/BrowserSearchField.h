// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserQuery.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum::browser {

class BrowserSearchField : public juce::Component, private juce::TextEditor::Listener, private juce::KeyListener {
public:
    std::function<void(const std::string&)> onChange;
    std::function<void()> onDown;

    BrowserSearchField() {
        restyle();
        editor_.setFont(juce::FontOptions(13.0f));
        editor_.addListener(this);
        editor_.addKeyListener(this);
        addAndMakeVisible(editor_);
    }

    void restyle() {
        editor_.setTextToShowWhenEmpty(tr("browser.search-hint", "Search - try #tag, >3, kind:ir, bpm:120"),
                                       Palette::textDim.withAlpha(alpha::strong));
        editor_.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        editor_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        editor_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        editor_.setColour(juce::TextEditor::textColourId, Palette::text);
        editor_.applyColourToAllText(Palette::text);
        repaint();
    }
    void lookAndFeelChanged() override { restyle(); }

    void setLocked(std::vector<std::string> chips) {
        locked_ = std::move(chips);
        resized();
        repaint();
    }

    void clear() {
        chips_.clear();
        editor_.clear();
        resized();
        changed();
    }

    std::string text() const {
        std::string out;
        for (const auto& c : chips_) out += c + " ";
        return out + editor_.getText().toStdString();
    }

    void focus() { editor_.grabKeyboardFocus(); }

    void paint(juce::Graphics& g) override {
        const auto box = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(Palette::background);
        g.fillRoundedRectangle(box, 5.0f);
        g.setColour(editor_.hasKeyboardFocus(true) ? Palette::accent.withAlpha(alpha::strong) : Palette::border);
        g.drawRoundedRectangle(box, 5.0f, 1.0f);
        for (const auto& chip : placed()) {
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(chip.bounds, 4.0f);
            if (chip.locked) {
                const float dash[] = {3.0f, 2.0f};
                juce::Path outline;
                outline.addRoundedRectangle(chip.bounds.reduced(0.5f), 4.0f);
                juce::Path dashed;
                juce::PathStrokeType(1.0f).createDashedStroke(dashed, outline, dash, 2);
                g.setColour(Palette::textDim);
                g.fillPath(dashed);
            }
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(11.5f));
            g.drawText(juce::String::fromUTF8(chip.text.c_str()), chip.bounds.reduced(7.0f, 0.0f), juce::Justification::centredLeft, false);
        }
    }

    void resized() override {
        float x = 8.0f;
        for (const auto& chip : placed()) x = chip.bounds.getRight() + 4.0f;
        editor_.setBounds(getLocalBounds().withTrimmedLeft((int) x).reduced(2, 3));
    }

    void mouseUp(const juce::MouseEvent& e) override {
        const auto chips = placed();
        for (size_t i = 0; i < chips.size(); ++i)
            if (!chips[i].locked && chips[i].bounds.contains(e.position)) {
                chips_.erase(chips_.begin() + (long) (i - locked_.size()));
                resized();
                changed();
                return;
            }
        editor_.grabKeyboardFocus();
    }

private:
    struct Chip {
        std::string text;
        bool locked = false;
        juce::Rectangle<float> bounds;
    };

    std::vector<Chip> placed() const {
        std::vector<Chip> out;
        float x = 6.0f;
        auto add = [&](const std::string& text, bool locked) {
            const float w = juce::GlyphArrangement::getStringWidth(juce::FontOptions(11.5f), juce::String::fromUTF8(text.c_str())) + 14.0f;
            out.push_back({text, locked, {x, 5.0f, w, (float) getHeight() - 10.0f}});
            x += w + 4.0f;
        };
        for (const auto& c : locked_) add(c, true);
        for (const auto& c : chips_) add(tokenOf(c).chip(), false);
        return out;
    }

    void textEditorTextChanged(juce::TextEditor&) override {
        auto t = editor_.getText().toStdString();
        if (!t.empty() && (t.back() == ' ' || t.back() == '\t')) {
            std::string word;
            for (const char c : t) {
                if (c != ' ' && c != '\t') { word += c; continue; }
                if (!word.empty()) chips_.push_back(word);
                word.clear();
            }
            editor_.setText({}, juce::dontSendNotification);
            resized();
            repaint();
        }
        changed();
    }

    void textEditorFocusLost(juce::TextEditor&) override { repaint(); }

    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override {
        if (key == juce::KeyPress::backspaceKey && editor_.isEmpty() && !chips_.empty()) {
            chips_.pop_back();
            resized();
            changed();
            return true;
        }
        if (key == juce::KeyPress::downKey && onDown) { onDown(); return true; }
        if (key == juce::KeyPress::escapeKey && !text().empty()) { clear(); return true; }
        return false;
    }

    void changed() {
        repaint();
        if (onChange) onChange(text());
    }

    juce::TextEditor editor_;
    std::vector<std::string> chips_;
    std::vector<std::string> locked_;
};

}
