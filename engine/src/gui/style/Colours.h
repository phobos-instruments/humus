// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_graphics/juce_graphics.h>

namespace hum::alpha {
inline constexpr float none = 0.0f;
inline constexpr float wash = 0.06f;
inline constexpr float mist = 0.12f;
inline constexpr float veil = 0.18f;
inline constexpr float scrim = 0.25f;
inline constexpr float muted = 0.35f;
inline constexpr float dim = 0.45f;
inline constexpr float mid = 0.55f;
inline constexpr float strong = 0.70f;
inline constexpr float heavy = 0.85f;
inline constexpr float nearOpaque = 0.95f;
}

namespace hum::ink {

inline const juce::Colour unset{0xff000000};

namespace brand {
inline const juce::Colour ground{0xfff4ecdc};
inline const juce::Colour line{0xff33402a};
inline const juce::Colour glow{0xff7e8f3c};
inline const juce::Colour recovered{0xffb07818};
}

namespace state {
inline const juce::Colour ok{0xff37c86a};
inline const juce::Colour caution{0xffe6c83c};
inline const juce::Colour danger{0xffe1463c};
inline const juce::Colour warning{0xffd9a03c};
inline const juce::Colour recording{0xffd6553f};
inline const juce::Colour armed{0xffb04040};
inline const juce::Colour overdubbing{0xffb07030};
inline const juce::Colour stepOn{0xff7ec44a};
inline const juce::Colour controlCord{0xffc9a24a};
}

namespace strand {
inline const juce::Colour mute{0xffe23b3b};
inline const juce::Colour solo{0xffe0a03c};
}

namespace skeleton {
inline const juce::Colour bone{0xff21c063};
inline const juce::Colour joint{0xffe23d2e};
}

namespace keyboard {
inline const juce::Colour whiteKey{0xfff2f2f2};
inline const juce::Colour blackKey{0xff141414};
}

namespace vu {
inline const juce::Colour halo{0xffff9640};
inline const juce::Colour paperTop{0xfff0e4c4};
inline const juce::Colour paperBottom{0xffdccb9f};
inline const juce::Colour lampWash{0xffffa63a};
inline const juce::Colour needleShadow{0xff3c2814};
inline const juce::Colour needle{0xff1c160e};
inline const juce::Colour peakLampLit{0xffff5038};
inline const juce::Colour peakLampDark{0xff3f1510};
inline const juce::Colour bezel{0xff15110d};
inline const juce::Colour print{0xff2a2216};
inline const juce::Colour printHot{0xffb23a28};
}

namespace spectrum {
inline const juce::Colour ground{0xff0a100c};
inline const juce::Colour grid{0xff223129};
inline const juce::Colour label{0xff5e7a6c};
inline const juce::Colour trace{0xff46d98a};
inline const juce::Colour hold{0xffb8e6cd};
inline const juce::Colour frame{0xff31443a};
inline const juce::Colour heatSea{0xff10473a};
inline const juce::Colour heatMint{0xff9df2c0};
inline const juce::Colour heatPeak{0xffeafff3};
}

namespace field {
inline const juce::Colour ground{0xff12100c};
inline const juce::Colour edge{0xff0c0a08};
inline const juce::Colour strip{0xff0e0c09};
}

namespace clip {
inline constexpr int kCount = 8;
inline const juce::Colour wheel[kCount] = {
    juce::Colour(0xffc75450), juce::Colour(0xffcf8a3c), juce::Colour(0xffc9b458),
    juce::Colour(0xff6aa84f), juce::Colour(0xff45a5a0), juce::Colour(0xff5b8dd9),
    juce::Colour(0xff9a6fd0), juce::Colour(0xffc06fa8),
};
}

namespace slice {
inline const juce::Colour start{0xffe0a83c};
}

namespace fx {
inline const juce::Colour ground{0xff17181c};
inline const juce::Colour panel{0xff1d1f24};
inline const juce::Colour box{0xff262a31};
inline const juce::Colour line{0xff3a404b};
inline const juce::Colour text{0xffd8dbe0};
inline const juce::Colour dim{0xff8a8f99};
inline const juce::Colour accent{0xffa6d608};
inline const juce::Colour warning{0xffd6a608};
}

namespace testPattern {
inline const juce::Colour bars[] = {
    juce::Colour(0xffc0c0c0), juce::Colour(0xffc0c000), juce::Colour(0xff00c0c0),
    juce::Colour(0xff00c000), juce::Colour(0xffc000c0), juce::Colour(0xffc00000),
    juce::Colour(0xff0000c0),
};
inline const juce::Colour ground{0xff101010};
}

}
