// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace hum {

class EmbedFit {
public:
    struct Size {
        int w = 0, h = 0;
    };

    static constexpr int kSettleTicks = 4;

    void reset(Size editor);
    bool observe(Size editor, float appliedScale, float outerScale, bool buttonDown, float shownScale);
    void freezeAt(float shownScale);
    bool letGo(bool buttonDown, int boxWidth);

    float scale(int boxWidth) const;
    int heightFor(int boxWidth) const;
    bool held() const { return held_ > 0.0f; }
    bool ignoresZoom() const { return ignoresZoom_; }
    int naturalW() const { return natural_.w; }
    int naturalH() const { return natural_.h; }

private:
    bool hostChanged(float appliedScale, float outerScale);
    bool learn(Size natural, bool buttonDown, float shownScale);
    bool closeToNatural(Size seen, float toPlugin) const;

    Size natural_;
    Size reference_;
    bool rebaseline_ = false;
    float held_ = 0.0f;
    bool ignoresZoom_ = false;
    float appliedAtRead_ = 1.0f;
    float outerAtRead_ = 1.0f;
    int settle_ = 0;
};

}
