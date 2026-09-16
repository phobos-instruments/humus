// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/help/HelpMarkdown.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class HelpBody : public juce::Component {
public:
    HelpBody(const HelpDoc& doc, const std::string& cls,
             std::function<void(const std::string&)> onOrganism = {})
        : onOrganism_(std::move(onOrganism)) {
        auto [subtitle, blocks] = help_detail::parseHelpDoc(doc, juce::String(cls));
        const juce::Colour body = Palette::text, dim = Palette::textDim, acc = Palette::accent;
        const juce::Font fBody(juce::FontOptions(13.0f));
        const juce::Font fHead(juce::FontOptions(13.5f).withStyle("Bold"));
        const juce::Font fName(juce::FontOptions(13.0f).withStyle("Bold"));
        const juce::Font fSub(juce::FontOptions(11.0f));

        if (subtitle.isNotEmpty()) {
            Chunk c;
            c.attr.append(subtitle, fSub, dim);
            c.attr.append("   " + juce::String(helpFamilyLabel(cls)), fSub,
                          Palette::familyAccent(familyOf(cls)));
            chunks_.push_back(std::move(c));
        }
        using K = help_detail::Block;
        bool related = false;
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& blk = blocks[i];
            if (blk.kind == K::Image) continue;
            if (related && onOrganism_) {
                const auto text = blk.kind == K::Def ? blk.a + "  " + blk.b : blk.a;
                auto toks = help_detail::linkTokens(text);
                bool any = false;
                for (const auto& t : toks) any = any || t.link;
                if (any) {
                    Chunk c;
                    c.tokens = std::move(toks);
                    if (i > 0 && blocks[i - 1].kind == K::Def) c.gap = 3.0f;
                    chunks_.push_back(std::move(c));
                    continue;
                }
            }
            Chunk c;
            if (blk.kind == K::Header) {
                related = help_detail::isRelatedHeader(blk.a);
                help_detail::appendRich(c.attr, blk.a, blk.rich, fHead, fHead, acc);
                c.gap = 14.0f;
            } else if (blk.kind == K::Def) {
                c.def = true;
                c.label = blk.a;
                help_detail::appendRich(c.attr, blk.b, blk.rich, fBody, fName, body);
                if (i > 0 && blocks[i - 1].kind == K::Def) c.gap = 0.0f;
            } else {
                help_detail::appendRich(
                    c.attr,
                    blk.bullet ? juce::String::fromUTF8("\xe2\x80\xa2  ") + blk.a : blk.a,
                    blk.rich, fBody, fName, blk.level < 0 ? dim : body);
                if (blk.bullet && i > 0 && blocks[i - 1].bullet) c.gap = 3.0f;
            }
            chunks_.push_back(std::move(c));
        }
        if (chunks_.empty()) {
            Chunk c;
            c.attr.append(doc.text, fBody, body);
            chunks_.push_back(std::move(c));
        }
    }

    void layoutTo(int width) {
        const float w = (float) juce::jmax(80, width - 2 * kPad);
        const float lineH = linkFont_.getHeight() + 4.0f;
        assignLabelColumns(w);
        float y = 0.0f;
        for (auto& c : chunks_) {
            if (y > 0.0f) y += c.gap;
            c.y = y;
            if (c.def) {
                c.layout = juce::TextLayout();
                c.layout.createLayout(c.attr, w - c.labelW);
                c.rowH = juce::jmax(c.layout.getHeight(), labelFont_.getHeight()) + 2.0f * kRowPad;
                y += c.rowH;
                continue;
            }
            if (!c.tokens.empty()) {
                c.rects.clear();
                float x = 0.0f, ly = 0.0f;
                for (const auto& t : c.tokens) {
                    const float tw = std::ceil(
                        juce::GlyphArrangement::getStringWidth(linkFont_, t.text)) + 2.0f;
                    if (x > 0.0f && x + tw > w) { x = 0.0f; ly += lineH; }
                    c.rects.push_back({(float) kPad + x, (float) kPad + y + ly, tw, lineH});
                    x += tw;
                }
                y += ly + lineH;
                continue;
            }
            c.layout = juce::TextLayout();
            c.layout.createLayout(c.attr, w);
            y += c.layout.getHeight();
        }
        setSize(width, (int) std::ceil(y) + 2 * kPad);
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced((float) kPad);
        for (auto& c : chunks_) {
            if (c.def) {
                const float top = r.getY() + c.y;
                g.setColour(Palette::border);
                g.drawHorizontalLine((int) top, r.getX(), r.getRight());
                g.setColour(Palette::accent);
                g.setFont(labelFont_);
                g.drawText(c.label, juce::Rectangle<float>(r.getX(), top + kRowPad, c.labelW - 8.0f,
                                                            labelFont_.getHeight() + 2.0f),
                           juce::Justification::topLeft);
                c.layout.draw(g, juce::Rectangle<float>(r.getX() + c.labelW, top + kRowPad,
                                                        r.getWidth() - c.labelW,
                                                        c.layout.getHeight()));
                continue;
            }
            if (!c.tokens.empty()) {
                g.setFont(linkFont_);
                for (size_t k = 0; k < c.tokens.size(); ++k) {
                    const auto& t = c.tokens[k];
                    const auto& b = c.rects[k];
                    g.setColour(t.link ? Palette::accent : Palette::text);
                    g.drawText(t.text, b.toNearestInt(), juce::Justification::centredLeft);
                    if (t.link) {
                        const bool hot = (int) k == hotToken_ && &c == hotChunk_;
                        g.setColour(Palette::accent.withAlpha(hot ? 0.9f : 0.4f));
                        g.drawLine(b.getX(), b.getBottom() - 3.0f, b.getRight(),
                                   b.getBottom() - 3.0f, 1.0f);
                    }
                }
                continue;
            }
            c.layout.draw(g, juce::Rectangle<float>(r.getX(), r.getY() + c.y, r.getWidth(),
                                                    c.layout.getHeight()));
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const Chunk* ch = nullptr;
        const int tok = tokenAt(e.position, ch);
        if (tok != hotToken_ || ch != hotChunk_) {
            hotToken_ = tok;
            hotChunk_ = ch;
            setMouseCursor(tok >= 0 ? juce::MouseCursor::PointingHandCursor
                                    : juce::MouseCursor::NormalCursor);
            repaint();
        }
    }
    void mouseExit(const juce::MouseEvent&) override {
        hotToken_ = -1;
        hotChunk_ = nullptr;
        repaint();
    }
    void mouseUp(const juce::MouseEvent& e) override {
        const Chunk* ch = nullptr;
        const int tok = tokenAt(e.position, ch);
        if (tok >= 0 && onOrganism_)
            onOrganism_(ch->tokens[(size_t) tok].text.toStdString());
    }

private:
    static constexpr int kPad = 12;
    static constexpr float kRowPad = 5.0f;
    static constexpr float kLabelMin = 90.0f, kLabelShare = 0.38f;
    struct Chunk {
        juce::AttributedString attr;
        juce::TextLayout layout;
        std::vector<help_detail::LinkToken> tokens;
        std::vector<juce::Rectangle<float>> rects;
        juce::String label;
        bool def = false;
        float labelW = 0.0f;
        float rowH = 0.0f;
        float y = 0.0f;
        float gap = 8.0f;
    };

    void assignLabelColumns(float w) {
        size_t i = 0;
        while (i < chunks_.size()) {
            if (!chunks_[i].def) { ++i; continue; }
            size_t j = i;
            float widest = 0.0f;
            for (; j < chunks_.size() && chunks_[j].def; ++j)
                widest = juce::jmax(widest, (float) std::ceil(juce::GlyphArrangement::getStringWidth(
                                                labelFont_, chunks_[j].label)));
            const float col = juce::jlimit(kLabelMin, juce::jmax(kLabelMin, w * kLabelShare),
                                           widest + 16.0f);
            for (size_t k = i; k < j; ++k) chunks_[k].labelW = col;
            i = j;
        }
    }

    int tokenAt(juce::Point<float> p, const Chunk*& chunk) const {
        for (const auto& c : chunks_)
            for (size_t k = 0; k < c.rects.size(); ++k)
                if (c.tokens[k].link && c.rects[k].contains(p)) {
                    chunk = &c;
                    return (int) k;
                }
        chunk = nullptr;
        return -1;
    }

    std::function<void(const std::string&)> onOrganism_;
    juce::Font linkFont_{juce::FontOptions(13.0f)};
    juce::Font labelFont_{juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f,
                                            juce::Font::plain)};
    std::vector<Chunk> chunks_;
    int hotToken_ = -1;
    const Chunk* hotChunk_ = nullptr;
};

}
