// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/help/HelpMarkup.h"

namespace hum {
namespace help_detail {

inline std::vector<Block> parseMarkdown(const juce::String& raw) {
    juce::StringArray lines;
    lines.addLines(raw);

    std::vector<Block> blocks;
    juce::StringArray para;
    auto flush = [&] {
        if (para.isEmpty()) return;
        Block b{Block::Para, tidy(para.joinIntoString(" ")), {}};
        b.rich = true;
        blocks.push_back(std::move(b));
        para.clear();
    };

    for (const auto& line : lines) {
        const auto t = line.trim();
        if (t.isEmpty()) { flush(); continue; }

        if (t.startsWithChar('#')) {
            flush();
            int level = 0;
            while (level < t.length() && t[level] == '#') ++level;
            Block b{Block::Header, tidy(t.substring(level).trim()), {}};
            b.level = juce::jlimit(1, 3, level);
            b.rich = true;
            blocks.push_back(std::move(b));
            continue;
        }

        if (t.startsWith("![")) {
            flush();
            Block b{Block::Image,
                    t.fromFirstOccurrenceOf("(", false, false)
                     .upToLastOccurrenceOf(")", false, false).trim(),
                    t.fromFirstOccurrenceOf("![", false, false)
                     .upToFirstOccurrenceOf("]", false, false).trim()};
            blocks.push_back(std::move(b));
            continue;
        }

        if (t.startsWithChar('>')) {
            flush();
            Block b{Block::Para, tidy(t.substring(1).trim()), {}};
            b.rich = true;
            b.level = -1;
            blocks.push_back(std::move(b));
            continue;
        }

        const bool bullet = t.startsWith("- ") || t.startsWith("* ");
        const auto text = bullet ? t.substring(2).trim() : t;

        if (text.startsWith("**") && text.substring(2).contains("**")) {
            const auto label = text.substring(2).upToFirstOccurrenceOf("**", false, false);
            const auto rest = text.substring(2).fromFirstOccurrenceOf("**", false, false).trim();
            if (rest.isNotEmpty()) {
                flush();
                Block b{Block::Def, tidy(label), tidy(rest)};
                b.rich = true;
                b.bullet = bullet;
                blocks.push_back(std::move(b));
                continue;
            }
        }

        if (bullet) {
            flush();
            Block b{Block::Para, tidy(text), {}};
            b.rich = true;
            b.bullet = true;
            blocks.push_back(std::move(b));
            continue;
        }

        para.add(t);
    }
    flush();
    return blocks;
}

inline std::pair<juce::String, std::vector<Block>>
parseHelpDoc(const HelpDoc& page, const juce::String& cls) {
    auto blocks = parseMarkdown(page.text);
    if (!blocks.empty() && blocks.front().kind == Block::Header && blocks.front().level == 1
        && blocks.front().a == juce::String(page.docClass))
        blocks.erase(blocks.begin());
    return {juce::String(helpSubtitle(page, cls.toStdString())), blocks};
}

inline void appendRich(juce::AttributedString& out, const juce::String& text, bool rich,
                       const juce::Font& normal, const juce::Font& bold,
                       juce::Colour colour) {
    if (!rich || !text.contains("**")) { out.append(text, normal, colour); return; }
    juce::String rest = text;
    bool strong = false;
    while (rest.isNotEmpty()) {
        const auto chunk = rest.upToFirstOccurrenceOf("**", false, false);
        if (chunk.isNotEmpty()) out.append(chunk, strong ? bold : normal, colour);
        rest = rest.fromFirstOccurrenceOf("**", false, false);
        strong = !strong;
    }
}

}
}
