#pragma once

namespace hum::toolbar {

enum Shed {
    ShedNone = 0,
    ShedSetup,
    ShedClick,
    ShedLink,
    ShedDsp,
    ShedLocate,
    ShedMax
};

inline constexpr int kIcon = 31;
inline constexpr int kSep  = 16;
inline constexpr int kTempoW = 156;
inline constexpr int kTsigW = 40;
inline constexpr int kPlayW = 34;
inline constexpr int kNavW  = 24;

inline constexpr int kPinnedWidth =
      3 * (28 + 3) + (kPlayW + 3) + kSep
    + 4 * kIcon + kSep
    + 128 + kSep
    + kTempoW + 3 + kTsigW + 4
    + kSep + 2 * kIcon
    + 72 + 6
    + 30 + 30 + 8;

inline constexpr int widthFor(int shed) {
    int w = kPinnedWidth;
    if (shed < ShedSetup)  w += kIcon + 38 + 3 + 24 + 3;
    if (shed < ShedClick)  w += 44 + 3;
    if (shed < ShedLink)   w += 48;
    if (shed < ShedDsp)    w += 58 + 2;
    if (shed < ShedLocate) w += 3 * (kNavW + 3) + kSep;
    if (shed > ShedNone)   w += kIcon;
    return w;
}

inline constexpr int kInsets = 12;

inline constexpr int fullWidth() { return widthFor(ShedNone) + kInsets; }

inline constexpr int shedFor(int avail) {
    int shed = ShedNone;
    while (shed < ShedLocate && widthFor(shed) > avail) ++shed;
    return shed;
}

}
