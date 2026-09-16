// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/ClassString.h"
#include "core/app/FuzzyMatch.h"
#include "gui/patcher/ClassPickerMenu.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/patcher/OrganismBlurb.h"
#include "gui/help/TextHighlight.h"
#include "gui/common/Localisation.h"

namespace hum {

class QuickAddPalette : public juce::Component, private juce::ListBoxModel {
public:
    std::function<void(const std::string& cls)> onPick;

    QuickAddPalette() {
        for (const auto& origin : classPickerGroups())
            for (const auto& group : origin.second)
                for (const auto& cls : group.second) {
                    Entry e;
                    e.cls = cls;
                    e.display = juce::String::fromUTF8(parseClassString(cls).display.c_str());
                    e.tag = juce::String::fromUTF8(origin.first.c_str())
                          + juce::String::fromUTF8(" \xe2\x80\xba ")
                          + juce::String::fromUTF8(group.first.c_str());
                    all_.push_back(std::move(e));
                }

        search_.onNav = [this](const juce::KeyPress& k) { return navigate(k); };
        search_.setFont(juce::FontOptions(14.5f));
        search_.setTextToShowWhenEmpty(juce::String::fromUTF8("Search organisms\xe2\x80\xa6"),
                                       Palette::textDim);
        search_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
        search_.setColour(juce::TextEditor::textColourId, Palette::text);
        search_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        search_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
        search_.onTextChange = [this] { refilter(); };
        search_.onEscapeKey = [this] { dismiss(); };
        search_.onReturnKey = [this] { pick(list_.getSelectedRow()); };
        addAndMakeVisible(search_);

        list_.setModel(this);
        list_.setRowHeight(kRowH);
        list_.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list_.setWantsKeyboardFocus(false);
        addAndMakeVisible(list_);

        refilter();
    }

    static void show(juce::Rectangle<int> screenAnchor,
                     std::function<void(const std::string&)> onPick) {
        auto content = std::make_unique<QuickAddPalette>();
        content->onPick = std::move(onPick);
        juce::CallOutBox::launchAsynchronously(std::move(content), screenAnchor, nullptr);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        search_.setBounds(r.removeFromTop(28));
        r.removeFromTop(6);
        list_.setBounds(r);
    }

    void parentHierarchyChanged() override {
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<QuickAddPalette>(this)] {
            if (safe != nullptr) safe->search_.grabKeyboardFocus();
        });
    }

    std::vector<std::string> resultsForTest(const juce::String& q) {
        search_.setText(q, false);
        refilter();
        std::vector<std::string> out;
        for (const auto& e : shown_) out.push_back(e.cls);
        return out;
    }

private:
    static constexpr int kRowH = 24;
    static constexpr int kMaxRows = 10;

    struct Entry {
        std::string cls;
        juce::String display, tag;
        int score = 0;
    };

    struct SearchBox : juce::TextEditor {
        std::function<bool(const juce::KeyPress&)> onNav;
        bool keyPressed(const juce::KeyPress& k) override {
            if (onNav && onNav(k)) return true;
            return juce::TextEditor::keyPressed(k);
        }
    };

    bool navigate(const juce::KeyPress& k) {
        const int n = (int) shown_.size();
        if (n == 0) return false;
        int row = list_.getSelectedRow();
        if (k.getKeyCode() == juce::KeyPress::downKey) row = (row + 1) % n;
        else if (k.getKeyCode() == juce::KeyPress::upKey) row = (row + n - 1) % n;
        else return false;
        list_.selectRow(row);
        return true;
    }

    void refilter() {
        const auto q = search_.getText().toStdString();
        shown_.clear();
        for (auto& e : all_) {
            const int score = fuzzyScoreNameMetaText(q, e.display.toStdString(), e.tag.toStdString(),
                                                     organismBlurb(e.cls).toStdString());
            if (score < 0) continue;
            Entry s = e;
            s.score = score;
            shown_.push_back(std::move(s));
        }
        std::stable_sort(shown_.begin(), shown_.end(), [](const Entry& a, const Entry& b) {
            return a.score != b.score ? a.score > b.score : a.display < b.display;
        });
        list_.updateContent();
        list_.selectRow(0);
        const int rows = juce::jlimit(1, kMaxRows, (int) shown_.size());
        setSize(340, 8 + 28 + 6 + rows * kRowH + 8);
        list_.repaint();
    }

    void pick(int row) {
        if (row < 0 || row >= (int) shown_.size()) return;
        const auto cls = shown_[(size_t) row].cls;
        dismiss();
        if (onPick) onPick(cls);
    }

    void dismiss() {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
    }

    int getNumRows() override { return std::max(1, (int) shown_.size()); }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
        if (shown_.empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(juce::String(tr("quick-add-palette.no-matches-esc-to-close", "no matches - Esc to close")),
                       10, 0, w - 20, h, juce::Justification::centredLeft);
            return;
        }
        const auto& e = shown_[(size_t) row];
        if (selected) {
            g.setColour(Palette::accent.withAlpha(alpha::scrim));
            g.fillRoundedRectangle(2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
        }
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(Palette::textDim);
        const int tagW = juce::jmin(w / 2, 160);
        g.drawText(e.tag, w - tagW - 8, 0, tagW, h, juce::Justification::centredRight);
        drawMatchLit(g, juce::FontOptions(13.5f), e.display, search_.getText(),
                     {10, 0, w - tagW - 26, h}, Palette::text, Palette::accent);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override { pick(row); }
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override { pick(row); }

    SearchBox search_;
    juce::ListBox list_;
    std::vector<Entry> all_, shown_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickAddPalette)
};

}
