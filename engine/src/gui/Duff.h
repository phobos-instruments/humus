#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

namespace hum::duff {

inline juce::Image hyphaImage(int rowH, int parity) {
    juce::Image img(juce::Image::ARGB, 6, juce::jmax(1, rowH), true);
    juce::Graphics g(img);
    const float H = (float) rowH;
    const float y0 = 0.17f * H, y1 = 0.87f * H;
    const float cx = 3.0f;
    const float side = parity ? -1.0f : 1.0f;
    const float pi = juce::MathConstants<float>::pi;

    auto halfW = [pi](float t) { return 0.65f * std::pow(std::sin(pi * t), 0.6f) + 0.16f; };
    auto spineX = [pi, cx, side](float t) { return cx + side * 0.9f * std::sin(pi * t); };

    juce::Path fil;
    constexpr int kN = 20;
    for (int i = 0; i <= kN; ++i) {
        const float t = (float) i / (float) kN;
        const float y = y0 + (y1 - y0) * t;
        const float x = spineX(t) + halfW(t);
        if (i == 0) fil.startNewSubPath(x, y); else fil.lineTo(x, y);
    }
    for (int i = kN; i >= 0; --i) {
        const float t = (float) i / (float) kN;
        fil.lineTo(spineX(t) - halfW(t), y0 + (y1 - y0) * t);
    }
    fil.closeSubPath();

    juce::ColourGradient grad(Palette::border.withAlpha(0.28f), cx, y0,
                              Palette::border.withAlpha(0.28f), cx, y1, false);
    grad.addColour(0.5, Palette::border.brighter(0.10f));
    g.setGradientFill(grad);
    g.fillPath(fil);

    const float tn = 0.62f;
    juce::Path nub;
    const float nx = spineX(tn), ny = y0 + (y1 - y0) * tn;
    nub.startNewSubPath(nx, ny + 0.5f);
    nub.quadraticTo(nx + side * 2.0f, ny - 0.7f, nx + side * 3.4f, ny - 2.1f);
    g.setColour(Palette::border.withAlpha(0.8f));
    g.strokePath(nub, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
    return img;
}

}
