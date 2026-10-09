// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

struct TextSpan {
    const juce::TextLayout* layout = nullptr;
    juce::Point<float> origin;
    juce::String text;
    juce::String lead;
    juce::Rectangle<float> area;
};

class SelectableText : public juce::Component {
public:
    SelectableText() {
        setWantsKeyboardFocus(true);
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    }

    juce::String selectedText() const;
    bool hasSelection() const { return anchor_ != caret_; }
    void selectAll();
    void selectBetween(juce::Point<float> from, juce::Point<float> to);
    void selectWordAt(juce::Point<float> p);
    void copySelection() const;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override { selectWordAt(e.position); }
    bool keyPressed(const juce::KeyPress& k) override;

protected:
    virtual std::vector<TextSpan> textSpans() const = 0;
    void paintSelection(juce::Graphics& g) const;
    bool draggedToSelect() const { return dragged_; }

private:
    struct Place {
        int span = 0, index = 0;
        bool operator==(const Place& o) const { return span == o.span && index == o.index; }
        bool operator!=(const Place& o) const { return !(*this == o); }
        bool operator<(const Place& o) const { return span != o.span ? span < o.span : index < o.index; }
    };

    Place placeAt(const std::vector<TextSpan>& spans, juce::Point<float> p) const;
    static int indexInSpan(const TextSpan& s, juce::Point<float> p);
    std::pair<Place, Place> ordered() const { return caret_ < anchor_ ? std::pair{caret_, anchor_} : std::pair{anchor_, caret_}; }
    void showMenu();

    Place anchor_, caret_;
    bool dragged_ = false;
};

inline SelectableText::Place SelectableText::placeAt(const std::vector<TextSpan>& spans, juce::Point<float> p) const {
    if (spans.empty()) return {};
    for (int i = 0; i < (int) spans.size(); ++i) {
        const auto& s = spans[(size_t) i];
        if (p.y < s.area.getY()) return {i, 0};
        if (p.y < s.area.getBottom()) return {i, indexInSpan(s, p)};
    }
    return {(int) spans.size() - 1, spans.back().text.length()};
}

inline int SelectableText::indexInSpan(const TextSpan& s, juce::Point<float> p) {
    if (s.layout == nullptr) return p.x < s.area.getCentreX() ? 0 : s.text.length();
    if (p.x < s.origin.x) return 0;
    const auto& layout = *s.layout;
    for (int l = 0; l < layout.getNumLines(); ++l) {
        const auto& line = layout.getLine(l);
        const float bottom = s.origin.y + line.lineOrigin.y + line.descent + line.leading;
        if (p.y > bottom && l + 1 < layout.getNumLines()) continue;
        for (const auto* run : line.runs) {
            if (run == nullptr) continue;
            for (int g = 0; g < run->glyphs.size(); ++g) {
                const auto& glyph = run->glyphs.getReference(g);
                const float x = s.origin.x + line.lineOrigin.x + glyph.anchor.x;
                if (p.x < x + glyph.width * 0.5f)
                    return std::min(run->stringRange.getStart() + g, run->stringRange.getEnd());
            }
        }
        return line.stringRange.getEnd();
    }
    return s.text.length();
}

inline juce::String SelectableText::selectedText() const {
    const auto spans = textSpans();
    if (spans.empty() || !hasSelection()) return {};
    const auto [from, to] = ordered();
    juce::StringArray pieces;
    for (int i = from.span; i <= to.span && i < (int) spans.size(); ++i) {
        const auto& s = spans[(size_t) i];
        const int a = i == from.span ? from.index : 0;
        const int b = i == to.span ? to.index : s.text.length();
        if (b <= a) continue;
        auto piece = s.text.substring(a, b);
        if (a == 0 && b >= s.text.length() && s.lead.isNotEmpty()) piece = s.lead + "\t" + piece;
        pieces.add(piece);
    }
    return pieces.joinIntoString("\n");
}

inline void SelectableText::selectAll() {
    const auto spans = textSpans();
    anchor_ = {};
    caret_ = spans.empty() ? Place{} : Place{(int) spans.size() - 1, spans.back().text.length()};
    repaint();
}

inline void SelectableText::selectBetween(juce::Point<float> from, juce::Point<float> to) {
    const auto spans = textSpans();
    anchor_ = placeAt(spans, from);
    caret_ = placeAt(spans, to);
    repaint();
}

inline void SelectableText::selectWordAt(juce::Point<float> p) {
    const auto spans = textSpans();
    const auto at = placeAt(spans, p);
    if (spans.empty()) return;
    const auto& text = spans[(size_t) at.span].text;
    auto inWord = [&](int i) { return i >= 0 && i < text.length() && juce::CharacterFunctions::isLetterOrDigit(text[i]); };
    int a = at.index, b = at.index;
    while (inWord(a - 1)) --a;
    while (inWord(b)) ++b;
    anchor_ = {at.span, a};
    caret_ = {at.span, b};
    repaint();
}

inline void SelectableText::copySelection() const {
    if (hasSelection()) juce::SystemClipboard::copyTextToClipboard(selectedText());
}

inline void SelectableText::mouseDown(const juce::MouseEvent& e) {
    dragged_ = false;
    if (e.mods.isPopupMenu()) { showMenu(); return; }
    const auto at = placeAt(textSpans(), e.position);
    if (!e.mods.isShiftDown()) anchor_ = at;
    caret_ = at;
    beginDragAutoRepeat(40);
    repaint();
}

inline void SelectableText::mouseDrag(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu() || !e.mouseWasDraggedSinceMouseDown()) return;
    dragged_ = true;
    caret_ = placeAt(textSpans(), e.position);
    if (auto* vp = findParentComponentOfClass<juce::Viewport>()) {
        const auto inView = e.getEventRelativeTo(vp).getPosition();
        vp->autoScroll(inView.x, inView.y, 24, 12);
    }
    repaint();
}

inline bool SelectableText::keyPressed(const juce::KeyPress& k) {
    const auto cmd = juce::ModifierKeys::commandModifier;
    if (k == juce::KeyPress('c', cmd, 0)) { copySelection(); return hasSelection(); }
    if (k == juce::KeyPress('a', cmd, 0)) { selectAll(); return true; }
    if (k.getKeyCode() == juce::KeyPress::escapeKey && hasSelection()) {
        anchor_ = caret_;
        repaint();
        return true;
    }
    return false;
}

inline void SelectableText::showMenu() {
    juce::PopupMenu menu;
    menu.addItem(1, tr("selectable-text.copy", "Copy"), hasSelection());
    menu.addItem(2, tr("selectable-text.select-all", "Select All"));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition(),
                       [safe = juce::Component::SafePointer<SelectableText>(this)](int r) {
                           if (safe == nullptr) return;
                           if (r == 1) safe->copySelection();
                           if (r == 2) safe->selectAll();
                       });
}

inline void SelectableText::paintSelection(juce::Graphics& g) const {
    if (!hasSelection()) return;
    const auto spans = textSpans();
    const auto [from, to] = ordered();
    g.setColour(Palette::accentDim);
    for (int i = from.span; i <= to.span && i < (int) spans.size(); ++i) {
        const auto& s = spans[(size_t) i];
        const int a = i == from.span ? from.index : 0;
        const int b = i == to.span ? to.index : s.text.length();
        if (b <= a) continue;
        if (s.layout == nullptr) {
            g.fillRect(s.area);
            continue;
        }
        if (a == 0 && b >= s.text.length() && s.lead.isNotEmpty())
            g.fillRect(s.area.withRight(s.origin.x));
        for (int l = 0; l < s.layout->getNumLines(); ++l) {
            const auto& line = s.layout->getLine(l);
            float left = 1.0e9f, right = -1.0e9f;
            for (const auto* run : line.runs) {
                if (run == nullptr) continue;
                for (int gi = 0; gi < run->glyphs.size(); ++gi) {
                    const int c = run->stringRange.getStart() + gi;
                    if (c < a || c >= b) continue;
                    const auto& glyph = run->glyphs.getReference(gi);
                    const float x = s.origin.x + line.lineOrigin.x + glyph.anchor.x;
                    left = std::min(left, x);
                    right = std::max(right, x + glyph.width);
                }
            }
            if (right <= left) continue;
            const float top = s.origin.y + line.lineOrigin.y - line.ascent;
            g.fillRect(juce::Rectangle<float>(left, top, right - left, line.ascent + line.descent));
        }
    }
}

}
