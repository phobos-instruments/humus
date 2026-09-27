// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

#include <juce_graphics/juce_graphics.h>

namespace hum::oklch {

struct Lch {
    double lightness = 0.0, chroma = 0.0, hue = 0.0;
};

inline double toLinear(double channel) {
    return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

inline double toGamma(double channel) {
    return channel <= 0.0031308 ? 12.92 * channel : 1.055 * std::pow(channel, 1.0 / 2.4) - 0.055;
}

inline bool inGamut(double r, double g, double b) {
    const double slack = 1.0e-4;
    return r >= -slack && g >= -slack && b >= -slack && r <= 1.0 + slack && g <= 1.0 + slack && b <= 1.0 + slack;
}

inline void toLinearRgb(const Lch& c, double& r, double& g, double& b) {
    const double radians = c.hue * juce::MathConstants<double>::twoPi;
    const double aa = c.chroma * std::cos(radians), bb = c.chroma * std::sin(radians);
    const double lr = c.lightness + 0.3963377774 * aa + 0.2158037573 * bb;
    const double mr = c.lightness - 0.1055613458 * aa - 0.0638541728 * bb;
    const double sr = c.lightness - 0.0894841775 * aa - 1.2914855480 * bb;
    const double l = lr * lr * lr, m = mr * mr * mr, s = sr * sr * sr;
    r = 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    b = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;
}

inline Lch fitted(Lch wanted) {
    double r = 0.0, g = 0.0, b = 0.0;
    toLinearRgb(wanted, r, g, b);
    if (inGamut(r, g, b)) return wanted;
    double tooMuch = wanted.chroma, enough = 0.0;
    for (int step = 0; step < 20; ++step) {
        const double middle = 0.5 * (enough + tooMuch);
        Lch probe{wanted.lightness, middle, wanted.hue};
        toLinearRgb(probe, r, g, b);
        (inGamut(r, g, b) ? enough : tooMuch) = middle;
    }
    return {wanted.lightness, enough, wanted.hue};
}

inline juce::Colour colourOf(const Lch& wanted) {
    double r = 0.0, g = 0.0, b = 0.0;
    toLinearRgb(fitted(wanted), r, g, b);
    const auto byte = [](double linear) {
        return (juce::uint8) std::lround(std::clamp(toGamma(std::clamp(linear, 0.0, 1.0)), 0.0, 1.0) * 255.0);
    };
    return juce::Colour(byte(r), byte(g), byte(b));
}

inline Lch of(juce::Colour c) {
    const double r = toLinear(c.getFloatRed()), g = toLinear(c.getFloatGreen()), b = toLinear(c.getFloatBlue());
    const double l = std::cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    const double m = std::cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    const double s = std::cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
    const double lightness = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
    const double aa = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s;
    const double bb = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s;
    double hue = std::atan2(bb, aa) / juce::MathConstants<double>::twoPi;
    if (hue < 0.0) hue += 1.0;
    return {lightness, std::sqrt(aa * aa + bb * bb), hue};
}

inline double maxChroma(double lightness, double hue) { return fitted({lightness, 0.4, hue}).chroma; }

}
