// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "gui/help/HelpMarkdown.h"
#include "gui/help/SelectableText.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class GuideBody : public SelectableText {
public:
    explicit GuideBody(const juce::String& raw) {
        const auto blocks = help_detail::parseMarkdown(raw);
        const juce::Font fBody(juce::FontOptions(14.5f));
        const juce::Font fName(juce::FontOptions(14.5f).withStyle("Bold"));
        const juce::Font fH[3] = {juce::Font(juce::FontOptions(19.0f).withStyle("Bold")),
                                  juce::Font(juce::FontOptions(15.5f).withStyle("Bold")),
                                  juce::Font(juce::FontOptions(14.5f).withStyle("Bold"))};
        using K = help_detail::Block;
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& blk = blocks[i];
            Chunk c;
            if (blk.kind == K::Image) {
                c.img = juce::ImageFileFormat::loadFrom(resolveAssetRef(blk.a));
                if (c.img.isNull()) continue;
            } else if (blk.kind == K::Header) {
                const auto& f = fH[juce::jlimit(1, 3, blk.level) - 1];
                help_detail::appendRich(c.attr, blk.a, blk.rich, f, f, Palette::accent);
            } else if (blk.kind == K::Def) {
                c.attr.append(blk.a, fName, Palette::text);
                help_detail::appendRich(c.attr, "  " + blk.b, blk.rich, fBody, fName,
                                        Palette::text);
                c.tight = i > 0 && blocks[i - 1].kind == K::Def;
            } else {
                const auto dim = blk.level < 0;
                help_detail::appendRich(c.attr, blk.bullet ? juce::String::fromUTF8(
                                                    "\xe2\x80\xa2  ") + blk.a : blk.a,
                                        blk.rich, fBody, fName,
                                        dim ? Palette::textDim : Palette::text);
                c.tight = blk.bullet && i > 0 && blocks[i - 1].bullet;
            }
            chunks_.push_back(std::move(c));
        }
    }

    void layoutTo(int width, int minHeight) {
        colW_ = (float) juce::jmin(width - 24, 560);
        float y = 0.0f;
        for (auto& c : chunks_) {
            if (y > 0.0f) y += c.tight ? 3.0f : 12.0f;
            c.y = y;
            if (c.img.isValid()) {
                const float w = juce::jmin(colW_, (float) c.img.getWidth());
                c.imgH = w * (float) c.img.getHeight() / (float) c.img.getWidth();
                y += c.imgH;
                continue;
            }
            c.layout = juce::TextLayout();
            c.layout.createLayout(c.attr, colW_);
            y += c.layout.getHeight();
        }
        textH_ = (float) std::ceil(y);
        setSize(width, juce::jmax((int) textH_ + 16, minHeight));
    }
    float textHeight() const { return textH_; }

    void paint(juce::Graphics& g) override {
        paintSelection(g);
        const float x = ((float) getWidth() - colW_) * 0.5f;
        const float top = ((float) getHeight() - textH_) * 0.5f;
        for (auto& c : chunks_) {
            if (c.img.isValid()) {
                const float w = juce::jmin(colW_, (float) c.img.getWidth());
                g.drawImage(c.img, juce::Rectangle<float>(x + (colW_ - w) * 0.5f,
                                                          top + c.y, w, c.imgH),
                            juce::RectanglePlacement::centred);
                continue;
            }
            c.layout.draw(g, juce::Rectangle<float>(x, top + c.y, colW_,
                                                    c.layout.getHeight()));
        }
    }

protected:
    std::vector<TextSpan> textSpans() const override {
        const float x = ((float) getWidth() - colW_) * 0.5f;
        const float top = ((float) getHeight() - textH_) * 0.5f;
        std::vector<TextSpan> out;
        for (const auto& c : chunks_) {
            if (c.img.isValid()) continue;
            const juce::Point<float> at(x, top + c.y);
            out.push_back({&c.layout, at, c.attr.getText(), {}, {x, at.y, colW_, c.layout.getHeight()}});
        }
        return out;
    }

private:
    struct Chunk {
        juce::AttributedString attr;
        juce::TextLayout layout;
        juce::Image img;
        float y = 0.0f, imgH = 0.0f;
        bool tight = false;
    };
    std::vector<Chunk> chunks_;
    float colW_ = 560.0f, textH_ = 0.0f;
};

}
