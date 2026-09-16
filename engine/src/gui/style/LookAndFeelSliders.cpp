// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <cmath>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

void pebbleBody(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour base, bool active) {
    const float w = b.getWidth(), h = b.getHeight() * 0.96f;
    juce::Rectangle<float> body(b.getX(), b.getY() + (b.getHeight() - h) * 0.7f, w, h);
    g.setColour(juce::Colours::black.withAlpha(alpha::scrim));
    g.fillEllipse(body.translated(0.0f, 1.5f));
    if (active) base = base.brighter(0.12f);
    const auto c = body.getCentre();
    const float r = juce::jmax(w, h) * 0.5f;
    juce::ColourGradient grad(base.brighter(0.28f), c.x - r * 0.35f, c.y - r * 0.45f,
                              base.darker(0.35f), c.x + r * 0.9f, c.y + r * 0.9f, true);
    g.setGradientFill(grad);
    g.fillEllipse(body);
    g.setColour(Palette::border);
    g.drawEllipse(body, 1.0f);
    if (r > 7.0f) {
        juce::Path hl;
        hl.addCentredArc(c.x, c.y, body.getWidth() * 0.5f - 1.8f, body.getHeight() * 0.5f - 1.8f,
                         0.0f, -1.9f, -0.4f, true);
        g.setColour(juce::Colours::white.withAlpha(alpha::mist));
        g.strokePath(hl, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }
}

void paintFaderGroove(juce::Graphics& g, juce::Rectangle<float> b,
                      float fillLo, float fillHi, bool muted, bool vertical) {
    const float trackW = vertical ? 8.0f
                                  : juce::jlimit(8.0f, 14.0f, b.getHeight() * 0.28f);
    const juce::Rectangle<float> track = vertical
        ? juce::Rectangle<float>(b.getCentreX() - trackW * 0.5f, b.getY(), trackW, b.getHeight())
        : juce::Rectangle<float>(b.getX(), b.getCentreY() - trackW * 0.5f, b.getWidth(), trackW);
    g.setColour(Palette::background.darker(0.25f));
    g.fillRoundedRectangle(track, trackW * 0.5f);
    {
        juce::ColourGradient shadow(juce::Colours::black.withAlpha(alpha::dim),
                                    track.getX(), track.getY(),
                                    juce::Colours::transparentBlack,
                                    vertical ? track.getX() + 3.0f : track.getX(),
                                    vertical ? track.getY() : track.getY() + 3.0f, false);
        g.setGradientFill(shadow);
        g.fillRoundedRectangle(track, trackW * 0.5f);
    }
    if (fillHi - fillLo > 0.5f) {
        const juce::Rectangle<float> fill = vertical
            ? juce::Rectangle<float>(track.getX(), fillLo, trackW, fillHi - fillLo)
            : juce::Rectangle<float>(fillLo, track.getY(), fillHi - fillLo, trackW);
        const juce::Colour base = muted ? Palette::textDim : Palette::accent;
        juce::ColourGradient grad(base.brighter(0.20f), fill.getX(), fill.getY(),
                                  base.darker(0.15f),
                                  vertical ? fill.getRight() : fill.getX(),
                                  vertical ? fill.getY() : fill.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(fill, trackW * 0.5f);
        g.setColour(juce::Colours::white.withAlpha(alpha::mist));
        if (vertical) g.fillRect(fill.getX() + 1.0f, fill.getY(), 1.0f, fill.getHeight());
        else          g.fillRect(fill.getX(), fill.getY() + 1.0f, fill.getWidth(), 1.0f);
    }
}

void drawFaderPot(juce::Graphics& g, juce::Rectangle<float> body,
                  float gradTop, float gradBottom, float lineY, bool active,
                  bool roundTop, bool roundBottom, juce::Colour lineColour) {
    const float rad = juce::jmin(5.0f, body.getHeight() * 0.38f);
    juce::Path p;
    p.addRoundedRectangle(body.getX(), body.getY(), body.getWidth(), body.getHeight(),
                          rad, rad, roundTop, roundTop, roundBottom, roundBottom);
    {
        juce::Path s = p;
        s.applyTransform(juce::AffineTransform::translation(0.0f, 1.5f));
        g.setColour(juce::Colours::black.withAlpha(alpha::scrim));
        g.fillPath(s);
    }
    if (gradBottom <= gradTop) gradBottom = gradTop + 1.0f;
    juce::ColourGradient grad(Palette::panelLight.brighter(active ? 0.35f : 0.20f), 0.0f, gradTop,
                              Palette::panel.darker(0.28f), 0.0f, gradBottom, false);
    g.setGradientFill(grad);
    g.fillPath(p);
    if (roundTop) {
        g.setColour(juce::Colours::white.withAlpha(active ? 0.22f : 0.15f));
        g.drawLine(body.getX() + rad, body.getY() + 1.0f,
                   body.getRight() - rad, body.getY() + 1.0f, 1.0f);
    }
    g.setColour(active ? Palette::accent : Palette::border.brighter(0.18f));
    g.strokePath(p, juce::PathStrokeType(1.2f));
    g.setColour(juce::Colours::black.withAlpha(alpha::dim));
    g.fillRoundedRectangle(body.getX() + 3.0f, lineY - 1.5f, body.getWidth() - 6.0f, 1.5f, 0.75f);
    g.setColour(lineColour);
    g.fillRoundedRectangle(body.getX() + 3.0f, lineY, body.getWidth() - 6.0f, 1.5f, 0.75f);
}

void paintVerticalFader(juce::Graphics& g, juce::Rectangle<float> b, float thumbY, bool muted) {
    thumbY = juce::jlimit(b.getY(), b.getBottom(), thumbY);
    paintFaderGroove(g, b, thumbY, b.getBottom(), muted);
    const float cx = b.getCentreX();
    const float tw = juce::jlimit(12.0f, 30.0f, b.getWidth() - 6.0f), th = 14.0f;
    juce::Rectangle<float> thumb(cx - tw * 0.5f, thumbY - th * 0.5f, tw, th);
    drawFaderPot(g, thumb, thumb.getY(), thumb.getBottom(), thumbY, false, true, true,
                 muted ? Palette::textDim : Palette::text);
}

void paintHorizontalFader(juce::Graphics& g, juce::Rectangle<float> b, float thumbX, bool muted) {
    thumbX = juce::jlimit(b.getX(), b.getRight(), thumbX);
    paintFaderGroove(g, b, b.getX(), thumbX, muted, false);
    const float cy = b.getCentreY();
    const float th = juce::jlimit(12.0f, 30.0f, b.getHeight() - 6.0f);
    const float tw = juce::jlimit(14.0f, 22.0f, b.getHeight() * 0.45f);
    const juce::Rectangle<float> thumb(thumbX - tw * 0.5f, cy - th * 0.5f, tw, th);
    const float rad = juce::jmin(5.0f, thumb.getWidth() * 0.38f);
    g.setColour(juce::Colours::black.withAlpha(alpha::scrim));
    g.fillRoundedRectangle(thumb.translated(0.0f, 1.5f), rad);
    juce::ColourGradient grad(Palette::panelLight.brighter(0.20f), 0.0f, thumb.getY(),
                              Palette::panel.darker(0.28f), 0.0f, thumb.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(thumb, rad);
    g.setColour(juce::Colours::white.withAlpha(alpha::mist));
    g.drawLine(thumb.getX() + rad, thumb.getY() + 1.0f,
               thumb.getRight() - rad, thumb.getY() + 1.0f, 1.0f);
    g.setColour(Palette::border.brighter(0.18f));
    g.drawRoundedRectangle(thumb.reduced(0.5f), rad, 1.2f);
    g.setColour(juce::Colours::black.withAlpha(alpha::dim));
    g.fillRoundedRectangle(thumbX - 1.5f, thumb.getY() + 3.0f, 1.5f, thumb.getHeight() - 6.0f, 0.75f);
    g.setColour(muted ? Palette::textDim : Palette::text);
    g.fillRoundedRectangle(thumbX, thumb.getY() + 3.0f, 1.5f, thumb.getHeight() - 6.0f, 0.75f);
}

void HumLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      juce::Slider::SliderStyle style, juce::Slider& s) {
    if (style == juce::Slider::LinearVertical) {
        paintVerticalFader(g, juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h)
                                  .reduced(0.0f, 4.0f),
                           sliderPos, false);
        return;
    }
    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar) {
        paintHorizontalFader(g, juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h)
                                    .reduced(4.0f, 0.0f),
                             sliderPos, false);
        return;
    }
    juce::LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minSliderPos, maxSliderPos,
                                           style, s);
}

void HumLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float pos, float startAngle, float endAngle,
                                      juce::Slider& s) {
    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float lineW = juce::jmax(2.0f, radius * 0.16f);
    const float arcR = radius - lineW * 0.5f;

    const auto fam = (Family) (int) s.getProperties().getWithDefault("family", (int) Family::Voice);
    const auto famBright = Palette::familyAccent(fam);

    juce::Path track;
    track.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour(Palette::panelLight.withAlpha(alpha::nearOpaque));
    g.strokePath(track, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroAngle = bipolar
        ? startAngle + (float) s.valueToProportionOfLength(0.0) * (endAngle - startAngle)
        : startAngle;
    const float sweepFrom = juce::jmin(zeroAngle, angle);
    const float sweepTo = juce::jmax(zeroAngle, angle);
    if (sweepTo - sweepFrom > 0.01f) {
        const auto root = famBright.darker(0.32f);
        const int segs = juce::jmax(3, (int) ((sweepTo - sweepFrom) / 0.12f));
        const bool growsRight = angle >= zeroAngle;
        for (int k = 0; k < segs; ++k) {
            const int i = growsRight ? k : segs - 1 - k;
            const float f0 = (float) i / (float) segs;
            const float f1 = (float) (i + 1) / (float) segs;
            const float a0 = sweepFrom + (sweepTo - sweepFrom) * f0
                           - (growsRight && i > 0 ? 0.03f : 0.0f);
            const float a1 = sweepFrom + (sweepTo - sweepFrom) * f1
                           + (!growsRight && i + 1 < segs ? 0.03f : 0.0f);
            float t = (f0 + f1) * 0.5f;
            if (!growsRight) t = 1.0f - t;
            juce::Path seg;
            seg.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, a0, a1, true);
            g.setColour(root.interpolatedWith(famBright, t));
            g.strokePath(seg, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }
        if (radius > 14.0f) {
            juce::Path echo;
            const float echoR = arcR - lineW * 1.1f;
            echo.addCentredArc(centre.x, centre.y, echoR, echoR, 0.0f, sweepFrom, sweepTo, true);
            g.setColour(famBright.withAlpha(alpha::scrim));
            g.strokePath(echo, juce::PathStrokeType(1.0f));
        }
    }
    if (bipolar && radius > 10.0f) {
        const float halfPi = juce::MathConstants<float>::halfPi;
        const juce::Point<float> z(centre.x + arcR * std::cos(zeroAngle - halfPi),
                                   centre.y + arcR * std::sin(zeroAngle - halfPi));
        g.setColour(Palette::textDim.withAlpha(alpha::mid));
        g.fillEllipse(z.x - 1.0f, z.y - 1.0f, 2.0f, 2.0f);
    }

    const float knobR = arcR - lineW * 1.3f;
    if (knobR <= 3.0f) return;
    pebbleBody(g, {centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f},
               Palette::panelLight, false);

    const double meter = (double) s.getProperties().getWithDefault("meter", -1.0);
    if (meter >= 0.0 && knobR > 6.0f) {
        const float raw = (float) meter;
        const float db = 20.0f * std::log10(juce::jmax(raw, 1.0e-5f));
        const float lvl = juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 48.0f);
        const float ringR = knobR * 0.80f;
        const float ringW = juce::jmax(2.0f, knobR * 0.16f);
        juce::Path bed;
        bed.addCentredArc(centre.x, centre.y, ringR, ringR, 0.0f, startAngle, endAngle, true);
        g.setColour(Palette::background.withAlpha(alpha::strong));
        g.strokePath(bed, juce::PathStrokeType(ringW, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        if (lvl > 0.001f) {
            const auto lit = Palette::textDim;
            juce::Path on;
            on.addCentredArc(centre.x, centre.y, ringR, ringR, 0.0f, startAngle,
                             startAngle + lvl * (endAngle - startAngle), true);
            g.setColour(lit);
            g.strokePath(on, juce::PathStrokeType(ringW, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        }
    }

    const float halfPi = juce::MathConstants<float>::halfPi;
    const juce::Point<float> p(centre.x + knobR * 0.60f * std::cos(angle - halfPi),
                               centre.y + knobR * 0.60f * std::sin(angle - halfPi));
    const float dr = juce::jmax(2.0f, knobR * 0.18f);
    if (knobR >= 8.0f) {
        juce::ColourGradient dim(juce::Colours::black.withAlpha(alpha::dim), p.x, p.y,
                                 juce::Colours::transparentBlack, p.x + dr, p.y + dr, true);
        g.setGradientFill(dim);
        g.fillEllipse(p.x - dr, p.y - dr, dr * 2.0f, dr * 2.0f);
        juce::Path lip;
        lip.addCentredArc(p.x, p.y, dr - 0.5f, dr - 0.5f, 0.0f, 1.1f, 2.8f, true);
        g.setColour(juce::Colours::white.withAlpha(alpha::scrim));
        g.strokePath(lip, juce::PathStrokeType(1.0f));
    }
    g.setColour(famBright);
    const float sd = dr * 0.55f;
    g.fillEllipse(p.x - sd, p.y - sd, sd * 2.0f, sd * 2.0f);

    {
        const float ember = juce::jlimit(0.0f, 1.0f,
            (float) (double) s.getProperties().getWithDefault("ember", 0.0));
        const juce::Point<float> tip(centre.x + arcR * std::cos(angle - halfPi),
                                     centre.y + arcR * std::sin(angle - halfPi));
        const float tr = juce::jmax(2.2f, lineW * 0.72f) * (1.0f + 0.25f * ember);
        const float haloR = tr * (2.6f + 1.6f * ember);
        juce::ColourGradient halo(famBright.withAlpha(0.34f + 0.30f * ember), tip.x, tip.y,
                                  famBright.withAlpha(alpha::none), tip.x + haloR, tip.y, true);
        g.setGradientFill(halo);
        g.fillEllipse(tip.x - haloR, tip.y - haloR, haloR * 2.0f, haloR * 2.0f);
        g.setColour(famBright.brighter(0.35f + 0.4f * ember));
        g.fillEllipse(tip.x - tr, tip.y - tr, tr * 2.0f, tr * 2.0f);
    }
}

}
