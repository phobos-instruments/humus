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

    for (int i = 0; i < lines.size(); ++i) {
        const auto t = lines[i].trim();
        if (t.startsWith("```")) {
            flush();
            juce::StringArray body;
            for (++i; i < lines.size() && !lines[i].trim().startsWith("```"); ++i)
                body.add(lines[i].trimEnd());
            blocks.push_back({Block::Code, body.joinIntoString("\n"), {}});
            continue;
        }
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

struct TextLink {
    juce::Range<int> chars;
    juce::String url;
};

struct LinkSpan {
    int open = -1, mid = -1, close = -1;
    bool found() const { return close >= 0; }
};

inline bool isWebUrl(const juce::String& url) {
    return url.startsWith("https://") || url.startsWith("http://");
}

inline LinkSpan nextLink(const juce::String& text, int from = 0) {
    for (int open = text.indexOfChar(from, '['); open >= 0; open = text.indexOfChar(open + 1, '[')) {
        const int mid = text.indexOf(open, "](");
        if (mid < 0) break;
        const int close = text.indexOfChar(mid + 2, ')');
        if (close < 0) break;
        if (!text.substring(open + 1, mid).containsChar('[') && isWebUrl(text.substring(mid + 2, close)))
            return {open, mid, close};
    }
    return {};
}

inline juce::String withoutLinks(const juce::String& text) {
    juce::String out, rest = text;
    for (auto span = nextLink(rest); span.found(); span = nextLink(rest)) {
        out += rest.substring(0, span.open) + rest.substring(span.open + 1, span.mid);
        rest = rest.substring(span.close + 1);
    }
    return out + rest;
}

inline void appendStyled(juce::AttributedString& out, const juce::String& text, bool rich,
                         const juce::Font& normal, const juce::Font& bold, juce::Colour colour,
                         const juce::Font& mono, juce::Colour codeColour) {
    if (!rich || (!text.contains("**") && !text.containsChar('`'))) {
        out.append(text, normal, colour);
        return;
    }
    juce::String rest = text;
    bool strong = false;
    while (rest.isNotEmpty()) {
        const auto upToBold = rest.indexOf("**");
        const auto upToCode = rest.indexOfChar('`');
        const bool codeFirst = upToCode >= 0 && (upToBold < 0 || upToCode < upToBold);
        const auto marker = codeFirst ? upToCode : upToBold;
        if (marker < 0) { out.append(rest, strong ? bold : normal, colour); break; }
        if (marker > 0) out.append(rest.substring(0, marker), strong ? bold : normal, colour);
        if (codeFirst) {
            const auto after = rest.substring(marker + 1);
            const auto span = after.upToFirstOccurrenceOf("`", false, false);
            out.append(span, mono, codeColour);
            rest = after.fromFirstOccurrenceOf("`", false, false);
            continue;
        }
        rest = rest.substring(marker + 2);
        strong = !strong;
    }
}

inline void appendRich(juce::AttributedString& out, const juce::String& text, bool rich,
                       const juce::Font& normal, const juce::Font& bold, juce::Colour colour,
                       const juce::Font& mono, juce::Colour codeColour,
                       std::vector<TextLink>* links = nullptr, juce::Colour linkColour = {}) {
    juce::String rest = text;
    for (auto span = nextLink(rest); span.found(); span = nextLink(rest)) {
        appendStyled(out, rest.substring(0, span.open), rich, normal, bold, colour, mono, codeColour);
        const auto label = rest.substring(span.open + 1, span.mid);
        if (links == nullptr) {
            out.append(label, normal, colour);
        } else {
            const int from = out.getText().length();
            out.append(label, normal.withStyle(normal.getStyleFlags() | juce::Font::underlined), linkColour);
            links->push_back({{from, out.getText().length()}, rest.substring(span.mid + 2, span.close)});
        }
        rest = rest.substring(span.close + 1);
    }
    appendStyled(out, rest, rich, normal, bold, colour, mono, codeColour);
}

inline void appendRich(juce::AttributedString& out, const juce::String& text, bool rich,
                       const juce::Font& normal, const juce::Font& bold, juce::Colour colour,
                       std::vector<TextLink>* links = nullptr, juce::Colour linkColour = {}) {
    appendRich(out, text, rich, normal, bold, colour, normal, colour, links, linkColour);
}

}
}
