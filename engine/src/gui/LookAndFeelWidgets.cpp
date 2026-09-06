#include "gui/LookAndFeel.h"

namespace hum {

namespace {
juce::Path pillPath(juce::Rectangle<float> r, float rad, int connectedEdges) {
    const bool l = (connectedEdges & juce::Button::ConnectedOnLeft) != 0;
    const bool rr = (connectedEdges & juce::Button::ConnectedOnRight) != 0;
    const bool t = (connectedEdges & juce::Button::ConnectedOnTop) != 0;
    const bool b = (connectedEdges & juce::Button::ConnectedOnBottom) != 0;
    juce::Path p;
    p.addRoundedRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight(), rad, rad,
                          !(l || t), !(rr || t), !(l || b), !(rr || b));
    return p;
}
}

void fillOrganicPill(juce::Graphics& g, juce::Rectangle<float> r, float rad,
                     juce::Colour fill, juce::Colour outline, bool raised) {
    if (raised) {
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.fillRoundedRectangle(r.translated(0.0f, 1.0f), rad);
    }
    juce::ColourGradient grad(fill.brighter(raised ? 0.10f : 0.02f), 0.0f, r.getY(),
                              fill.darker(raised ? 0.12f : 0.04f), 0.0f, r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, rad);
    g.setColour(outline);
    g.drawRoundedRectangle(r.reduced(0.5f), rad, 1.0f);
}

void HumLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                          const juce::Colour& backgroundColour,
                                          bool highlighted, bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const float rad = juce::jmin(r.getHeight() * 0.45f, 9.0f);
    juce::Colour fill = backgroundColour;
    if (down) fill = fill.darker(0.15f);
    else if (highlighted) fill = fill.brighter(0.06f);
    juce::Colour outline = b.getToggleState() ? Palette::accent : Palette::border;

    if (b.getProperties()["flatPill"]) {
        if (b.getToggleState()) {
            foxfire::bloom(g, r, Palette::accent);
            return;
        }
        outline = juce::Colours::transparentBlack;
        if (!down && !highlighted) fill = juce::Colours::transparentBlack;
    }

    const int edges = b.getConnectedEdgeFlags();
    if (edges == 0) {
        fillOrganicPill(g, r, rad, fill, outline, !down);
        return;
    }
    const juce::Path p = pillPath(r, rad, edges);
    juce::ColourGradient grad(fill.brighter(down ? 0.02f : 0.10f), 0.0f, r.getY(),
                              fill.darker(down ? 0.04f : 0.12f), 0.0f, r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillPath(p);
    g.setColour(outline);
    g.strokePath(p, juce::PathStrokeType(1.0f));
}

void HumLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component&, float x, float y,
                                 float w, float h, bool ticked, bool enabled,
                                 bool highlighted, bool down) {
    const float side = juce::jmin(w, h);
    juce::Rectangle<float> r(x + (w - side) * 0.5f, y + (h - side) * 0.5f, side, side);
    r = r.reduced(1.0f);
    const float rad = r.getWidth() * 0.40f;

    g.setColour(juce::Colours::black.withAlpha(0.20f));
    g.fillRoundedRectangle(r.translated(0.0f, 1.0f), rad);
    juce::Colour base = ticked ? Palette::accent : Palette::panelLight;
    if (!enabled) base = base.withAlpha(0.5f);
    else if (down) base = base.darker(0.12f);
    else if (highlighted) base = base.brighter(0.08f);
    juce::ColourGradient grad(base.brighter(0.18f), 0.0f, r.getY(),
                              base.darker(0.22f), 0.0f, r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, rad);
    g.setColour(ticked ? Palette::accent.darker(0.2f) : Palette::border);
    g.drawRoundedRectangle(r.reduced(0.5f), rad, 1.0f);

    if (!ticked) return;
    const auto stem = r.getRelativePoint(0.26f, 0.74f);
    const auto tip = r.getRelativePoint(0.78f, 0.22f);
    juce::Path leaf;
    leaf.startNewSubPath(stem);
    leaf.quadraticTo(r.getRelativePoint(0.72f, 0.68f), tip);
    leaf.quadraticTo(r.getRelativePoint(0.32f, 0.28f), stem);
    leaf.closeSubPath();
    g.setColour(Palette::background);
    g.fillPath(leaf);
    g.setColour(Palette::accent.withAlpha(0.7f));
    g.drawLine({stem, tip}, 1.0f);
}

void HumLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isDown,
                                  int, int, int, int, juce::ComboBox& box) {
    auto r = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(0.5f);
    const float rad = juce::jmin(r.getHeight() * 0.45f, 9.0f);
    fillOrganicPill(g, r, rad,
                    box.findColour(juce::ComboBox::backgroundColourId).darker(isDown ? 0.10f : 0.0f),
                    Palette::border, !isDown);
    const float cx = r.getRight() - r.getHeight() * 0.55f, cy = r.getCentreY();
    const float s = juce::jmin(5.0f, r.getHeight() * 0.22f);
    juce::Path v;
    v.startNewSubPath(cx - s, cy - s * 0.5f);
    v.lineTo(cx, cy + s * 0.5f);
    v.lineTo(cx + s, cy - s * 0.5f);
    g.setColour(Palette::textDim);
    g.strokePath(v, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
}

void HumLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
    const int inset = juce::jmax(6, (int) (box.getHeight() * 0.30f));
    label.setBounds(inset, 1, box.getWidth() - inset - (int) (box.getHeight() * 0.9f),
                    box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
}

void HumLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    if (!juce::Desktop::canUseSemiTransparentWindows()) g.fillAll(Palette::background);
    const juce::Rectangle<float> r(0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f);
    g.setColour(Palette::panel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r, 6.0f, 1.0f);
}

void HumLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics& g,
                                              const juce::Path& path, juce::Image&) {
    g.setColour(Palette::panel);
    g.fillPath(path);
    g.setColour(Palette::border);
    g.strokePath(path, juce::PathStrokeType(1.2f));
}

void HumLookAndFeel::drawProgressBar(juce::Graphics& g, juce::ProgressBar& bar, int width,
                                     int height, double progress,
                                     const juce::String& textToShow) {
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height);
    const float radius = (float) height * 0.5f;
    g.setColour(bar.findColour(juce::ProgressBar::backgroundColourId));
    g.fillRoundedRectangle(bounds, radius);

    auto filled = bounds.withWidth(0.0f);
    if (progress >= 0.0 && progress <= 1.0) {
        filled = bounds.withWidth(bounds.getWidth() * (float) progress);
        juce::Graphics::ScopedSaveState clip(g);
        juce::Path rounded;
        rounded.addRoundedRectangle(bounds, radius);
        g.reduceClipRegion(rounded);
        g.setColour(bar.findColour(juce::ProgressBar::foregroundColourId));
        g.fillRoundedRectangle(filled, radius);
    } else {
        LookAndFeel_V4::drawProgressBar(g, bar, width, height, progress, {});
    }

    if (textToShow.isEmpty()) return;
    g.setFont(juce::FontOptions((float) height * 0.62f, juce::Font::bold));
    const auto whole = bounds.toNearestInt();
    {
        juce::Graphics::ScopedSaveState overFill(g);
        g.reduceClipRegion(filled.toNearestInt());
        g.setColour(Palette::background);
        g.drawText(textToShow, whole, juce::Justification::centred, false);
    }
    juce::Graphics::ScopedSaveState overTrack(g);
    g.excludeClipRegion(filled.toNearestInt());
    g.setColour(Palette::text);
    g.drawText(textToShow, whole, juce::Justification::centred, false);
}

void HumLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& bar, int x, int y,
                                   int w, int h, bool vertical, int thumbStart, int thumbSize,
                                   bool over, bool down) {
    if (thumbSize <= 0) return;
    const juce::Rectangle<float> thumb = vertical
        ? juce::Rectangle<float>((float) x + 2.0f, (float) thumbStart, (float) w - 4.0f, (float) thumbSize)
        : juce::Rectangle<float>((float) thumbStart, (float) y + 2.0f, (float) thumbSize, (float) h - 4.0f);
    auto c = bar.findColour(juce::ScrollBar::thumbColourId);
    if (over || down) c = c.brighter(0.2f);
    g.setColour(c);
    g.fillRoundedRectangle(thumb, juce::jmin(thumb.getWidth(), thumb.getHeight()) * 0.5f);
}

}
