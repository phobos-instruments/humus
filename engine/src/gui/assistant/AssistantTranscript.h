// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/assistant/TextSpan.h"
#include "gui/assistant/TranscriptLines.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class AssistantTranscript : public juce::Component {
public:
    using Voice = transcript::Voice;
    using Say = transcript::Say;

    struct Caret {
        int say = 0, line = 0, ch = 0;
        bool operator<(const Caret& o) const {
            return std::tie(say, line, ch) < std::tie(o.say, o.line, o.ch);
        }
        bool operator==(const Caret& o) const {
            return std::tie(say, line, ch) == std::tie(o.say, o.line, o.ch);
        }
    };

    AssistantTranscript() {
        setWantsKeyboardFocus(true);
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    }

    static constexpr int kPad = 12;
    static constexpr int kGutter = 10;
    static constexpr int kBubbleGap = 10;
    static constexpr float kRadius = 9.0f;
    static constexpr int kIndent = 44;

    std::function<void(const juce::String&)> onCopy = [](const juce::String& text) {
        juce::SystemClipboard::copyTextToClipboard(text);
    };

    void say(const juce::String& role, const juce::String& text) {
        const auto trimmed = text.trimEnd();
        if (trimmed.isEmpty()) return;
        Say s;
        s.voice = transcript::voiceOf(role);
        s.raw = trimmed;
        s.lines = transcript::build(s.voice, trimmed);
        says_.push_back(std::move(s));
        layoutTo(getWidth());
    }

    void clear() {
        says_.clear();
        anchor_ = head_ = {};
        layoutTo(getWidth());
    }

    void layoutTo(int width) {
        const float inner = (float) std::max(80, width - 2 * kPad - kGutter);
        float y = (float) kPad;
        for (auto& s : says_) {
            const bool bubble = s.voice != Voice::Tool;
            const float indent = s.voice == Voice::You ? (float) kIndent : 0.0f;
            const float wrap = inner - indent - (bubble ? 2.0f * (float) kGutter : 0.0f);
            s.top = y;
            float at = y + (bubble ? (float) kGutter * 0.6f : 0.0f);
            for (auto& line : s.lines) {
                line.layout = {};
                line.layout.createLayout(line.attr, wrap);
                line.height = line.layout.getHeight();
                at += line.gap;
                line.at = {(float) kPad + indent + (bubble ? (float) kGutter : 0.0f), at};
                at += line.height;
            }
            s.height = at - y + (bubble ? (float) kGutter * 0.6f : 0.0f);
            y += s.height + (float) kBubbleGap;
        }
        setSize(width, (int) std::max(y + (float) kPad, 10.0f));
        repaint();
    }

    void resized() override {
        if (getWidth() != laidOutAt_) {
            laidOutAt_ = getWidth();
            layoutTo(getWidth());
        }
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::background);
        const float inner = (float) std::max(80, getWidth() - 2 * kPad - kGutter);
        const auto from = std::min(anchor_, head_), to = std::max(anchor_, head_);
        for (int i = 0; i < (int) says_.size(); ++i) {
            const auto& s = says_[(size_t) i];
            const float indent = s.voice == Voice::You ? (float) kIndent : 0.0f;
            const auto box =
                juce::Rectangle<float>((float) kPad + indent, s.top, inner - indent, s.height);
            if (s.voice != Voice::Tool) {
                g.setColour(fill(s.voice));
                g.fillRoundedRectangle(box, kRadius);
                g.setColour(edge(s.voice));
                g.drawRoundedRectangle(box.reduced(0.5f), kRadius, 1.0f);
            }
            for (int l = 0; l < (int) s.lines.size(); ++l) {
                const auto& line = s.lines[(size_t) l];
                const float wide = line.layout.getWidth();
                if (line.code) {
                    g.setColour(Palette::background.withAlpha(alpha::mid));
                    g.fillRoundedRectangle(line.at.x - 4.0f, line.at.y - 2.0f, wide + 8.0f,
                                           line.height + 4.0f, 4.0f);
                }
                g.setColour(Palette::accent.withAlpha(alpha::mid));
                for (const auto& r : textspan::rectsOf(line.layout, chosenIn(i, l, from, to)))
                    g.fillRect(r.translated(line.at.x, line.at.y));
                line.layout.draw(g, {line.at.x, line.at.y, wide, line.height});
            }
        }
    }

    Caret caretAt(juce::Point<int> p) const {
        Caret c;
        const auto at = p.toFloat();
        for (int i = 0; i < (int) says_.size(); ++i) {
            const auto& s = says_[(size_t) i];
            c.say = i;
            if (at.y > s.top + s.height && i + 1 < (int) says_.size()) continue;
            for (int l = 0; l < (int) s.lines.size(); ++l) {
                const auto& line = s.lines[(size_t) l];
                c.line = l;
                if (at.y > line.at.y + line.height && l + 1 < (int) s.lines.size()) continue;
                c.ch = textspan::charAt(line.layout, at - line.at);
                return c;
            }
            return c;
        }
        return c;
    }

    void select(Caret from, Caret to) {
        anchor_ = from;
        head_ = to;
        repaint();
    }

    void selectAll() {
        if (says_.empty()) return;
        const int last = (int) says_.size() - 1;
        const int lastLine = (int) says_[(size_t) last].lines.size() - 1;
        select({}, {last, lastLine, lengthOf(last, lastLine)});
    }

    bool hasSelection() const { return !(anchor_ == head_); }

    juce::String selectedText() const {
        const auto from = std::min(anchor_, head_), to = std::max(anchor_, head_);
        juce::StringArray bubbles;
        for (int i = from.say; i <= to.say && i < (int) says_.size(); ++i) {
            juce::StringArray lines;
            for (int l = 0; l < (int) says_[(size_t) i].lines.size(); ++l) {
                const auto chars = chosenIn(i, l, from, to);
                if (chars.isEmpty()) continue;
                lines.add(says_[(size_t) i].lines[(size_t) l].attr.getText().substring(
                    chars.getStart(), chars.getEnd()));
            }
            if (!lines.isEmpty()) bubbles.add(lines.joinIntoString("\n"));
        }
        return bubbles.joinIntoString("\n\n");
    }

    juce::String conversationText() const {
        juce::StringArray parts;
        for (const auto& s : says_) parts.add(transcript::spoken(s));
        return parts.joinIntoString("\n\n");
    }

    void copySelection() { if (onCopy && hasSelection()) onCopy(selectedText()); }
    void copyConversation() { if (onCopy) onCopy(conversationText()); }

    void mouseDown(const juce::MouseEvent& e) override {
        grabKeyboardFocus();
        if (e.mods.isPopupMenu()) { showMenu(); return; }
        const auto at = caretAt(e.getPosition());
        if (e.getNumberOfClicks() >= 2) { selectBubble(at.say); return; }
        select(e.mods.isShiftDown() ? anchor_ : at, at);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) return;
        select(anchor_, caretAt(e.getPosition()));
        if (auto* view = findParentComponentOfClass<juce::Viewport>()) {
            const auto inView = e.getEventRelativeTo(view).getPosition();
            view->autoScroll(inView.x, inView.y, 24, 12);
        }
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (k == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0)) {
            copySelection();
            return true;
        }
        if (k == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)) {
            selectAll();
            return true;
        }
        return false;
    }

    int countForTest() const { return (int) says_.size(); }
    int linesForTest(int index) const {
        return index >= 0 && index < (int) says_.size() ? (int) says_[(size_t) index].lines.size()
                                                        : 0;
    }
    bool codeForTest(int index, int line) const {
        if (index < 0 || index >= (int) says_.size()) return false;
        const auto& lines = says_[(size_t) index].lines;
        return line >= 0 && line < (int) lines.size() && lines[(size_t) line].code;
    }
    juce::String textForTest(int index, int line) const {
        if (index < 0 || index >= (int) says_.size()) return {};
        const auto& lines = says_[(size_t) index].lines;
        return line >= 0 && line < (int) lines.size() ? lines[(size_t) line].attr.getText()
                                                      : juce::String();
    }
    juce::Point<int> lineOriginForTest(int index, int line) const {
        return says_[(size_t) index].lines[(size_t) line].at.roundToInt();
    }

private:
    int lengthOf(int say, int line) const {
        return says_[(size_t) say].lines[(size_t) line].attr.getText().length();
    }

    juce::Range<int> chosenIn(int say, int line, Caret from, Caret to) const {
        const Caret start{say, line, 0}, end{say, line, lengthOf(say, line)};
        if (end < from || to < start) return {};
        return {from < start ? 0 : from.ch, to < end ? to.ch : end.ch};
    }

    void selectBubble(int say) {
        if (say < 0 || say >= (int) says_.size()) return;
        const int lastLine = (int) says_[(size_t) say].lines.size() - 1;
        select({say, 0, 0}, {say, lastLine, lengthOf(say, lastLine)});
    }

    void showMenu() {
        if (says_.empty()) return;
        juce::PopupMenu menu;
        menu.addItem(1, tr("assistant-transcript.copy", "Copy"), hasSelection());
        menu.addItem(2, tr("assistant-transcript.select-all", "Select all"));
        menu.addItem(3, tr("assistant-transcript.copy-all", "Copy the conversation"));
        juce::Component::SafePointer<AssistantTranscript> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withMousePosition(), [safe](int picked) {
            if (safe == nullptr) return;
            if (picked == 1) safe->copySelection();
            if (picked == 2) safe->selectAll();
            if (picked == 3) safe->copyConversation();
        });
    }

    static juce::Colour fill(Voice v) {
        switch (v) {
            case Voice::You:   return Palette::accent.withAlpha(alpha::scrim);
            case Voice::Error: return Palette::warnAmber().withAlpha(alpha::scrim);
            default:           return Palette::panelLight.withAlpha(alpha::dim);
        }
    }
    static juce::Colour edge(Voice v) {
        switch (v) {
            case Voice::You:   return Palette::accent.withAlpha(alpha::muted);
            case Voice::Error: return Palette::warnAmber().withAlpha(alpha::dim);
            default:           return Palette::border;
        }
    }

    std::vector<Say> says_;
    Caret anchor_, head_;
    int laidOutAt_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AssistantTranscript)
};

}
