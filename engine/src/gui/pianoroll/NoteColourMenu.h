// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/tracks/ClipColors.h"
#include "gui/pianoroll/NoteRules.h"
#include "hum/PatternMatrix.h"

namespace hum::notecolour {

inline constexpr int kDefaultItem = 61000;
inline constexpr int kCustomItem = kDefaultItem + kNumClipColors + 1;

inline int rgbOf(juce::Colour c) { return (int) (c.getARGB() & 0xFFFFFFu); }

inline juce::Colour colourOf(int rgb) {
    return juce::Colour((juce::uint8) ((rgb >> 16) & 0xFF), (juce::uint8) ((rgb >> 8) & 0xFF),
                        (juce::uint8) (rgb & 0xFF));
}

inline juce::Colour fill(int colour, juce::Colour fallback) {
    return colour >= 0 ? colourOf(colour) : fallback;
}

inline int swatch(int index) { return rgbOf(clipColour(index)); }

inline bool isSwatch(int colour) {
    for (int i = 1; i <= kNumClipColors; ++i)
        if (swatch(i) == colour) return true;
    return false;
}

inline juce::PopupMenu menu(int current) {
    juce::PopupMenu m;
    juce::PopupMenu::Item none(tr("note-color.default", "Default"));
    none.itemID = kDefaultItem;
    none.colour = Palette::text;
    none.isTicked = current == kNoColour;
    m.addItem(std::move(none));
    for (int i = 1; i <= kNumClipColors; ++i) {
        juce::PopupMenu::Item it(clipColourName(i));
        it.itemID = kDefaultItem + i;
        it.colour = clipColour(i);
        it.isTicked = current == swatch(i);
        m.addItem(std::move(it));
    }
    m.addSeparator();
    m.addItem(kCustomItem, tr("note-color.custom", "Custom..."), true,
              current >= 0 && !isSwatch(current));
    return m;
}

enum class Pick { None, Colour, Custom };

struct Choice {
    Pick pick = Pick::None;
    int colour = kNoColour;
};

inline Choice choiceFor(int menuResult) {
    if (menuResult == kDefaultItem) return {Pick::Colour, kNoColour};
    if (menuResult > kDefaultItem && menuResult <= kDefaultItem + kNumClipColors)
        return {Pick::Colour, swatch(menuResult - kDefaultItem)};
    if (menuResult == kCustomItem) return {Pick::Custom, kNoColour};
    return {};
}

inline void openPicker(juce::Rectangle<int> screenArea, juce::Colour start,
                       std::function<void(int)> onChange) {
    struct Picker : juce::ColourSelector, juce::ChangeListener {
        explicit Picker(std::function<void(int)> f)
            : juce::ColourSelector(juce::ColourSelector::showColourspace
                                   | juce::ColourSelector::showColourAtTop),
              apply(std::move(f)) {
            addChangeListener(this);
        }
        ~Picker() override { removeChangeListener(this); }
        void changeListenerCallback(juce::ChangeBroadcaster*) override {
            if (apply) apply(rgbOf(getCurrentColour()));
        }
        std::function<void(int)> apply;
    };
    auto picker = std::make_unique<Picker>(std::move(onChange));
    picker->setCurrentColour(start, juce::dontSendNotification);
    picker->setSize(220, 200);
    juce::CallOutBox::launchAsynchronously(std::move(picker), screenArea, nullptr);
}

}
