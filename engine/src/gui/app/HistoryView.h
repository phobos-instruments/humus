// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "io/HistoryRules.h"
#include "io/PatchHistory.h"

namespace hum {

class HistoryView : public juce::Component, private juce::ListBoxModel, private juce::Timer {
public:
    using Entry = PatchHistory::Entry;

    struct Actions {
        std::function<std::vector<Entry>()> list;
        std::function<void(const Entry&)> restore;
        std::function<void(const Entry&)> openCopy;
        std::function<void(const Entry&, const juce::String&)> keep;
        std::function<void(const Entry&)> release;
        std::function<void()> clear;
    };

    static constexpr int kRowH = 42;
    static constexpr int kRefreshSeconds = 20;

    explicit HistoryView(Actions actions) : actions_(std::move(actions)) {
        title_.setText(tr("history.title", "Patch history"), juce::dontSendNotification);
        title_.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        title_.setColour(juce::Label::textColourId, Palette::text);
        addAndMakeVisible(title_);
        hint_.setText(tr("history.hint",
                         "A snapshot every few minutes while the patch changes, and when you save. "
                         "Restoring keeps the current state here too."),
                      juce::dontSendNotification);
        hint_.setFont(juce::FontOptions(11.5f));
        hint_.setColour(juce::Label::textColourId, Palette::textDim);
        hint_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(hint_);

        list_.setModel(this);
        list_.setRowHeight(kRowH);
        list_.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(list_);

        empty_.setText(tr("history.empty", "Nothing yet. Snapshots appear once the patch changes."),
                       juce::dontSendNotification);
        empty_.setColour(juce::Label::textColourId, Palette::textDim);
        empty_.setJustificationType(juce::Justification::centred);
        addChildComponent(empty_);

        restore_.setButtonText(tr("history.restore", "Restore"));
        restore_.setTooltip(tr("history.restore-tip", "Bring this version back in place of the current patch"));
        restore_.onClick = [this] { if (auto* e = selected()) if (actions_.restore) actions_.restore(*e); };
        copy_.setButtonText(tr("history.open-copy", "Open as Copy"));
        copy_.setTooltip(tr("history.open-copy-tip", "Open this version as a new untitled patch"));
        copy_.onClick = [this] { if (auto* e = selected()) if (actions_.openCopy) actions_.openCopy(*e); };
        keep_.onClick = [this] { toggleKeep(); };
        clear_.setButtonText(tr("history.clear", "Clear..."));
        clear_.setTooltip(tr("history.clear-tip", "Remove every snapshot except the kept ones. The patch stays as it is"));
        clear_.onClick = [this] { askToClear(); };
        for (auto* b : {&restore_, &copy_, &keep_, &clear_}) addAndMakeVisible(*b);

        refresh();
        startTimer(kRefreshSeconds * 1000);
        setSize(420, 520);
    }

    void refresh() {
        const auto chosen = selected() != nullptr ? selected()->at : 0;
        entries_ = actions_.list ? actions_.list() : std::vector<Entry>{};
        list_.updateContent();
        int row = entries_.empty() ? -1 : 0;
        for (int i = 0; i < (int) entries_.size(); ++i)
            if (entries_[(size_t) i].at == chosen) row = i;
        list_.selectRow(row);
        list_.repaint();
        empty_.setVisible(entries_.empty());
        updateButtons();
    }

    int shownCount() const { return (int) entries_.size(); }
    void selectForTest(int row) { list_.selectRow(row); }
    juce::String descriptionForTest(int row) const { return description(entries_[(size_t) row]); }

    void resized() override {
        auto r = getLocalBounds().reduced(12);
        title_.setBounds(r.removeFromTop(22));
        hint_.setBounds(r.removeFromTop(34));
        r.removeFromTop(6);
        auto buttons = r.removeFromBottom(28);
        r.removeFromBottom(8);
        list_.setBounds(r);
        empty_.setBounds(r);
        clear_.setBounds(buttons.removeFromRight(clear_.getBestWidthForHeight(28) + 8));
        for (auto* b : {&restore_, &copy_, &keep_}) {
            b->setBounds(buttons.removeFromLeft(b->getBestWidthForHeight(28) + 8));
            buttons.removeFromLeft(6);
        }
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

private:
    int getNumRows() override { return (int) entries_.size(); }

    void selectedRowsChanged(int) override { updateButtons(); }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override {
        if (row >= 0 && row < (int) entries_.size() && actions_.restore) actions_.restore(entries_[(size_t) row]);
    }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected) override {
        if (row < 0 || row >= (int) entries_.size()) return;
        const auto& e = entries_[(size_t) row];
        if (isSelected) {
            g.setColour(Palette::accent.withAlpha(alpha::mist));
            g.fillRoundedRectangle(juce::Rectangle<float>(0.0f, 1.0f, (float) w, (float) h - 2.0f), 6.0f);
        }
        auto r = juce::Rectangle<int>(0, 0, w, h).reduced(10, 5);
        auto top = r.removeFromTop(17);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.setColour(Palette::text);
        const auto when = whenText(e.at);
        g.drawText(when, top, juce::Justification::centredLeft, false);
        const int whenW = (int) juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), when) + 10;
        g.setFont(juce::FontOptions(12.0f));
        g.setColour(Palette::textDim);
        g.drawText(agoText(e.at), top.withTrimmedLeft(whenW), juce::Justification::centredLeft, true);
        if (e.kept) {
            g.setColour(Palette::accent);
            g.drawText(e.name.isNotEmpty() ? e.name : tr("history.kept", "Kept"), top,
                       juce::Justification::centredRight, true);
        }
        g.setFont(juce::FontOptions(11.5f));
        g.setColour(Palette::textDim);
        g.drawText(description(e), r, juce::Justification::centredLeft, true);
    }

    void timerCallback() override { refresh(); }

    const Entry* selected() const {
        const int row = list_.getSelectedRow();
        return row >= 0 && row < (int) entries_.size() ? &entries_[(size_t) row] : nullptr;
    }

    void updateButtons() {
        const auto* e = selected();
        restore_.setEnabled(e != nullptr);
        copy_.setEnabled(e != nullptr);
        keep_.setEnabled(e != nullptr);
        clear_.setEnabled(!entries_.empty());
        keep_.setButtonText(e != nullptr && e->kept ? tr("history.unkeep", "Unkeep")
                                                    : tr("history.keep", "Keep..."));
        keep_.setTooltip(tr("history.keep-tip", "A kept snapshot is never thinned out"));
        resized();
    }

    void toggleKeep() {
        const auto* e = selected();
        if (e == nullptr) return;
        if (e->kept) {
            if (actions_.release) actions_.release(*e);
            refresh();
            return;
        }
        const Entry chosen = *e;
        auto* aw = new juce::AlertWindow(tr("history.keep-title", "Keep this version"),
                                         tr("history.keep-message", "Give it a name you will recognise later."),
                                         juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor("name", tr("history.keep-default", "Good version"));
        aw->addButton(tr("history.keep-button", "Keep"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton(tr("history.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        juce::Component::SafePointer<HistoryView> safe(this);
        aw->enterModalState(true, juce::ModalCallbackFunction::create([safe, aw, chosen](int result) {
            if (result == 1 && safe != nullptr && safe->actions_.keep)
                safe->actions_.keep(chosen, aw->getTextEditorContents("name").trim());
            if (safe != nullptr) safe->refresh();
        }), true);
    }

    void askToClear() {
        auto* aw = new juce::AlertWindow(tr("history.clear-title", "Clear history"),
                                         tr("history.clear-message",
                                            "Every snapshot of this patch goes, except the ones you kept. "
                                            "The patch stays as it is and becomes the first snapshot again."),
                                         juce::MessageBoxIconType::WarningIcon);
        aw->addButton(tr("history.clear-button", "Clear"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton(tr("history.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        juce::Component::SafePointer<HistoryView> safe(this);
        aw->enterModalState(true, juce::ModalCallbackFunction::create([safe](int result) {
            if (safe == nullptr) return;
            if (result == 1 && safe->actions_.clear) safe->actions_.clear();
            safe->refresh();
        }), true);
    }

    static juce::String whenText(std::int64_t at) {
        const juce::Time t(at), now = juce::Time::getCurrentTime();
        const bool today = t.getYear() == now.getYear() && t.getDayOfYear() == now.getDayOfYear();
        if (today) return t.formatted("%H:%M:%S");
        if (now.toMilliseconds() - at < 6 * history::kDayMs) return t.formatted("%a %H:%M:%S");
        return t.formatted("%d %b %H:%M:%S");
    }

    static juce::String agoText(std::int64_t at) {
        const auto ago = history::agoOf(at, juce::Time::currentTimeMillis());
        switch (ago.unit) {
            case history::Ago::JustNow: return tr("history.just-now", "just now");
            case history::Ago::Minutes: return juce::String(ago.count) + tr("history.minutes-ago", " min ago");
            case history::Ago::Hours: return juce::String(ago.count) + tr("history.hours-ago", " h ago");
            case history::Ago::Yesterday: return tr("history.yesterday", "yesterday");
            case history::Ago::Days: return juce::String(ago.count) + tr("history.days-ago", " days ago");
        }
        return {};
    }

    static juce::String reasonText(const juce::String& reason) {
        if (reason == "opened") return tr("history.reason-opened", "As opened");
        if (reason == "saved") return tr("history.reason-saved", "Saved");
        if (reason == "before closing") return tr("history.reason-closing", "Before closing");
        if (reason == "before restoring") return tr("history.reason-restoring", "Before a restore");
        if (reason == "before revert") return tr("history.reason-revert", "Before revert");
        if (reason == "cleared") return tr("history.reason-cleared", "History cleared");
        return {};
    }

    static juce::String description(const Entry& e) {
        const auto why = reasonText(e.reason);
        if (why.isEmpty() && e.summary.isEmpty()) return tr("history.no-changes", "No changes described");
        if (why.isEmpty()) return e.summary;
        if (e.summary.isEmpty()) return why;
        return why + juce::String::fromUTF8(" \xc2\xb7 ") + e.summary;
    }

    Actions actions_;
    std::vector<Entry> entries_;
    juce::Label title_, hint_, empty_;
    juce::ListBox list_;
    juce::TextButton restore_, copy_, keep_, clear_;
};

}
