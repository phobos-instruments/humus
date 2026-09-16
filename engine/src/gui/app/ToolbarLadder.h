// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace hum::toolbar {

enum Shed {
    ShedNone = 0,
    ShedSetup,
    ShedClick,
    ShedLink,
    ShedDsp,
    ShedLocate,
    ShedLimiter,
    ShedKeep,
    ShedCaptions,
    ShedMax
};

inline constexpr int kIcon = 31;
inline constexpr int kSep  = 16;
inline constexpr int kTempoW = 156;
inline constexpr int kTsigW = 40;
inline constexpr int kPlayW = 34;
inline constexpr int kNavW  = 24;
inline constexpr int kTapW = 38;
inline constexpr int kBeatW = 24;
inline constexpr int kClickW = 44;
inline constexpr int kLinkW = 48;
inline constexpr int kGap = 3;
inline constexpr int kMeterW = 72;
inline constexpr int kOutW = 30;
inline constexpr int kOutCaptionW = 30;
inline constexpr int kLimW = 34;
inline constexpr int kGrooveW = 134;
inline constexpr int kGrooveCompactW = 64;

inline constexpr int kPinnedWidth =
      3 * (28 + 3) + (kPlayW + 3) + kSep
    + 2 * kIcon + kSep
    + 128 + kSep
    + kTempoW + 3 + kTsigW + 4
    + kSep + 2 * kIcon + 8
    + kMeterW + 6
    + kOutW + 4 + kGrooveCompactW + 8;

inline constexpr int widthFor(int shed) {
    int w = kPinnedWidth;
    if (shed < ShedSetup)  w += kIcon + kTapW + kGap + kBeatW + kGap;
    if (shed < ShedClick)  w += kClickW + kGap;
    if (shed < ShedLink)   w += kLinkW;
    if (shed < ShedDsp)    w += 58 + 2;
    if (shed < ShedLocate) w += 3 * (kNavW + 3) + kSep;
    if (shed < ShedLimiter) w += kLimW + 4;
    if (shed < ShedKeep)    w += 2 * kIcon;
    if (shed < ShedCaptions) w += kOutCaptionW + kGrooveW - kGrooveCompactW;
    if (shed > ShedNone)   w += kIcon;
    return w;
}

inline constexpr int kInsets = 12;

inline constexpr int fullWidth() { return widthFor(ShedNone) + kInsets; }

inline constexpr int shedFor(int avail) {
    int shed = ShedNone;
    while (shed < ShedMax - 1 && widthFor(shed) > avail) ++shed;
    return shed;
}

}
