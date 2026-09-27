// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/help/HelpMarkdown.h"
#include "gui/style/Colours.h"

namespace hum::transcript {

enum class Voice { You, Assistant, Tool, Error };

inline Voice voiceOf(const juce::String& role) {
    if (role == "you") return Voice::You;
    if (role == "tool") return Voice::Tool;
    if (role == "error") return Voice::Error;
    return Voice::Assistant;
}

struct Line {
    juce::AttributedString attr;
    juce::TextLayout layout;
    juce::Point<float> at;
    bool code = false;
    float gap = 0.0f;
    float height = 0.0f;
};

struct Say {
    Voice voice = Voice::Assistant;
    juce::String raw;
    std::vector<Line> lines;
    float height = 0.0f;
    float top = 0.0f;
};

inline juce::String spoken(const Say& s) {
    switch (s.voice) {
        case Voice::You:   return tr("assistant-transcript.you", "You") + ": " + s.raw;
        case Voice::Tool:  return "  - " + s.raw;
        case Voice::Error: return tr("assistant-transcript.note", "Note") + ": " + s.raw;
        default:           return tr("assistant-transcript.assistant", "Assistant") + ": " + s.raw;
    }
}

inline juce::Colour ink(Voice v) {
    return v == Voice::Error ? Palette::warnAmber()
         : v == Voice::Tool  ? Palette::textDim
                             : Palette::text;
}

inline std::vector<Line> build(Voice voice, const juce::String& text) {
    const juce::Font body(juce::FontOptions(voice == Voice::Tool ? 11.0f : 13.0f));
    const juce::Font bold(juce::FontOptions(voice == Voice::Tool ? 11.0f : 13.0f)
                              .withStyle("Bold"));
    const juce::Font head(juce::FontOptions(14.0f).withStyle("Bold"));
    const juce::Font mono(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f,
                                            juce::Font::plain));
    const auto colour = ink(voice);
    std::vector<Line> out;
    if (voice == Voice::Tool) {
        Line line;
        line.attr.append(juce::String::fromUTF8("\xc2\xb7  ") + text, body, colour);
        out.push_back(std::move(line));
        return out;
    }
    using K = help_detail::Block;
    const auto blocks = help_detail::parseMarkdown(text);
    for (size_t i = 0; i < blocks.size(); ++i) {
        const auto& blk = blocks[i];
        Line line;
        if (i > 0) line.gap = blk.kind == K::Header ? 8.0f : 5.0f;
        if (blk.kind == K::Code) {
            line.code = true;
            line.attr.append(blk.a, mono, colour);
        } else if (blk.kind == K::Header) {
            help_detail::appendRich(line.attr, blk.a, blk.rich, head, head, Palette::accent,
                                    mono, colour);
        } else if (blk.kind == K::Def) {
            help_detail::appendRich(line.attr, blk.a + "  ", true, bold, bold, colour, mono,
                                    colour);
            help_detail::appendRich(line.attr, blk.b, blk.rich, body, bold, colour, mono,
                                    colour);
        } else if (blk.kind == K::Image) {
            continue;
        } else {
            help_detail::appendRich(
                line.attr,
                blk.bullet ? juce::String::fromUTF8("\xe2\x80\xa2  ") + blk.a : blk.a,
                blk.rich, body, bold, colour, mono, colour);
        }
        out.push_back(std::move(line));
    }
    if (out.empty()) {
        Line line;
        line.attr.append(text, body, colour);
        out.push_back(std::move(line));
    }
    return out;
}

}
