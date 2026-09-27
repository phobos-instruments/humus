// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "gui/style/Contrast.h"
#include "gui/style/LookAndFeel.h"
#include "gui/style/Oklch.h"

namespace hum::themeroll {

inline constexpr double kTextOnPanel = 6.0;
inline constexpr double kDimOnPanel = 3.2;
inline constexpr double kAccentOnPanel = 3.4;
inline constexpr double kCordOnPanel = 2.6;
inline constexpr double kPanelOnBack = 1.12;

enum class Harmony { Monochrome, Analogous, Complementary, SplitComplementary, Triadic, Tetradic, Count };

struct Turns {
    double accent = 0.0, cord = 0.0;
};

inline const char* harmonyName(Harmony h) {
    static const char* kNames[(std::size_t) Harmony::Count] = {
        "Monochrome", "Analogous", "Complementary", "Split complementary", "Triadic", "Tetradic"};
    return kNames[(std::size_t) h];
}

inline Turns turnsOf(Harmony h) {
    switch (h) {
        case Harmony::Analogous:          return {1.0 / 12.0, -1.0 / 12.0};
        case Harmony::Complementary:      return {0.5, 0.5 - 1.0 / 20.0};
        case Harmony::SplitComplementary: return {0.5 - 1.0 / 12.0, 0.5 + 1.0 / 12.0};
        case Harmony::Triadic:            return {1.0 / 3.0, 2.0 / 3.0};
        case Harmony::Tetradic:           return {0.25, 0.5};
        case Harmony::Monochrome:
        default:                          return {0.0, 0.0};
    }
}

class Dice {
public:
    explicit Dice(std::uint32_t seed) : state_(scrambled(seed)) {}

    static std::uint32_t scrambled(std::uint32_t seed) {
        std::uint32_t x = seed + 0x9e3779b9u;
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return x == 0 ? 0x9e3779b9u : x;
    }

    std::uint32_t next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }
    double unit() { return (double) (next() >> 8) / (double) (1u << 24); }
    double between(double lo, double hi) { return lo + unit() * (hi - lo); }
    bool chance(double odds) { return unit() < odds; }
    int upTo(int count) { return (int) (next() % (std::uint32_t) count); }

private:
    std::uint32_t state_;
};

inline double wrapTurn(double hue) { return hue - std::floor(hue); }

struct Room {
    bool light = false;
    double hue = 0.0;
    double ground = 0.0, step = 0.0, tint = 0.0;
};

inline juce::Colour surfaceAt(const Room& room, int depth) {
    const double away = room.light ? -room.step * depth : room.step * depth;
    return oklch::colourOf({room.ground + away, room.tint * (1.0 + 0.25 * depth), room.hue});
}

inline juce::Colour toned(oklch::Lch base, double lightness, double chromaShare) {
    const double ceiling = oklch::maxChroma(lightness, base.hue);
    return oklch::colourOf({lightness, std::min(base.chroma, ceiling * chromaShare), base.hue});
}

struct Rolled {
    ThemeColours colours;
    Harmony harmony = Harmony::Monochrome;
    double roomHue = 0.0, accentHue = 0.0, cordHue = 0.0;
    bool light = false;
};

inline Rolled rolled(std::uint32_t seed) {
    Dice dice(seed);
    Room room;
    room.light = dice.chance(0.3);
    room.hue = dice.unit();
    room.ground = room.light ? dice.between(0.94, 0.97) : dice.between(0.16, 0.22);
    room.step = dice.between(0.035, 0.055);
    room.tint = dice.between(0.004, 0.016);

    const auto harmony = (Harmony) dice.upTo((int) Harmony::Count);
    const auto turns = turnsOf(harmony);
    const double accentHue = wrapTurn(room.hue + turns.accent + dice.between(-0.015, 0.015));
    const double cordHue = wrapTurn(room.hue + turns.cord + dice.between(-0.015, 0.015));
    const double accentLight = room.light ? dice.between(0.5, 0.6) : dice.between(0.74, 0.84);
    const double chromaShare = dice.between(0.68, 0.95);

    ThemeColours t;
    t.background = surfaceAt(room, 0);
    t.panel = surfaceAt(room, 1);
    t.panelLight = surfaceAt(room, 2);
    t.border = surfaceAt(room, room.light ? 3 : 4);

    t.accent = oklch::colourOf({accentLight, oklch::maxChroma(accentLight, accentHue) * chromaShare, accentHue});
    t.cord = oklch::colourOf({harmony == Harmony::Monochrome ? accentLight - (room.light ? -0.1 : 0.12) : accentLight,
                              oklch::maxChroma(accentLight, cordHue) * chromaShare * 0.78, cordHue});
    t.text = oklch::colourOf({room.light ? dice.between(0.22, 0.3) : dice.between(0.92, 0.96),
                              room.tint * 1.5, room.hue});
    t.textDim = oklch::colourOf({room.light ? dice.between(0.46, 0.54) : dice.between(0.66, 0.72),
                                 room.tint * 2.0, room.hue});

    t.panel = contrast::lifted(t.background, t.panel, kPanelOnBack);
    t.panelLight = contrast::lifted(t.panel, t.panelLight, kPanelOnBack);
    t.border = contrast::lifted(t.panel, t.border, 1.3);
    t.text = contrast::lifted(t.panel, t.text, kTextOnPanel);
    t.textDim = contrast::lifted(t.panel, t.textDim, kDimOnPanel);
    t.accent = contrast::lifted(t.panel, t.accent, kAccentOnPanel);
    t.cord = contrast::lifted(t.panel, t.cord, kCordOnPanel);

    const auto litAccent = oklch::of(t.accent);
    t.accentDim = toned(litAccent, room.light ? std::min(0.72, litAccent.lightness + 0.16)
                                              : std::max(0.42, litAccent.lightness - 0.24), 0.62);
    return {t, harmony, room.hue, accentHue, cordHue, room.light};
}

inline ThemeColours roll(std::uint32_t seed) { return rolled(seed).colours; }

}
