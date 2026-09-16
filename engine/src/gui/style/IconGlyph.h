// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <string>

#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

enum class IconGlyph {
    New, Open, Save, Export,
    Undo, Redo, Cut, Copy, Paste,
    EnableAudio, EnableMidi, GoToStart, PlayFromStart, Play, Stop, GoToEnd, Record, Keep, Loop,
    Patcher, Properties, Automation,
    Metapad, ParameterControl, Notes, DocumentSwitcher, Help, Library,
    QwertyPiano, Dice, Evolve, Overflow, Gear,
    DockCentre, DockRight, DockBottom,
    Pause, Power, KnobView, PluginView, RackView, FreeView, Warning, Reveal
};

inline const std::array<const char*, 44> kIconGlyphNames = {
    "New", "Open", "Save", "Export",
    "Undo", "Redo", "Cut", "Copy",
    "Paste", "EnableAudio", "EnableMidi", "GoToStart",
    "PlayFromStart", "Play", "Stop", "GoToEnd",
    "Record", "Keep", "Loop", "Patcher",
    "Properties", "Automation", "Metapad", "ParameterControl",
    "Notes", "DocumentSwitcher", "Help", "Library",
    "QwertyPiano", "Dice", "Evolve", "Overflow",
    "Gear", "DockCentre", "DockRight", "DockBottom",
    "Pause", "Power", "KnobView", "PluginView",
    "RackView", "FreeView", "Warning", "Reveal"
};

struct IconArt {
    std::unique_ptr<juce::Drawable> art;
    juce::Colour ink = juce::Colours::black;
};

inline IconArt* iconOverride(const juce::String& name) {
    static std::map<juce::String, IconArt> cache;
    const auto it = cache.find(name);
    if (it != cache.end()) return it->second.art ? &it->second : nullptr;

    std::unique_ptr<juce::Drawable> found;
    for (const char* ext : {".svg", ".png"}) {
        const auto want = name + ext;
        int size = 0;
        const char* data = nullptr;
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
            const auto* res = BinaryData::namedResourceList[i];
            if (want != BinaryData::getNamedResourceOriginalFilename(res)) continue;
            data = BinaryData::getNamedResource(res, size);
            break;
        }
        if (data == nullptr) continue;
        if (juce::String(ext) == ".svg") {
            if (auto xml = juce::XmlDocument::parse(juce::String::createStringFromData(data, size)))
                found = juce::Drawable::createFromSVG(*xml);
        } else if (const auto img = juce::ImageFileFormat::loadFrom(data, (size_t) size);
                   img.isValid()) {
            auto di = std::make_unique<juce::DrawableImage>();
            di->setImage(img);
            found = std::move(di);
        }
        if (found) break;
    }
    const bool ok = found != nullptr;
    cache[name] = IconArt{std::move(found), juce::Colours::black};
    return ok ? &cache[name] : nullptr;
}

inline void drawIconGlyph(juce::Graphics& g, IconGlyph glyph, juce::Rectangle<float> r,
                          juce::Colour fg, bool enabled, bool active = false) {
    const juce::String name(kIconGlyphNames[(size_t) glyph]);
    auto* art = active ? iconOverride(name + "-active") : nullptr;
    if (art == nullptr) art = iconOverride(name);
    if (art != nullptr) {
        if (art->ink != fg) {
            art->art->replaceColour(art->ink, fg);
            art->ink = fg;
        }
        art->art->drawWithin(g, r, juce::RectanglePlacement::centred, enabled ? 1.0f : 0.45f);
        return;
    }

    const float sw = juce::jmax(1.5f, r.getHeight() * 0.15f);
    const juce::PathStrokeType st(sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    auto stroke = [&](const juce::Path& pp) { g.strokePath(pp, st); };
    auto fillStroked = [&](const juce::Path& pp) { g.fillPath(pp); g.strokePath(pp, st); };
    const float W = r.getWidth(), H = r.getHeight();
    juce::Path p;

    const float u  = H / 14.0f;
    const float S  = juce::jmin(W, H);
    const float cx = r.getCentreX(), cy = r.getCentreY();
    const float hr = sw * 0.7f;
    juce::ignoreUnused(S, hr);
    switch (glyph) {
        case IconGlyph::New: {
            const float ear = W * 0.3f;
            p.startNewSubPath(r.getX(), r.getY());
            p.lineTo(r.getRight() - ear, r.getY());
            p.lineTo(r.getRight(), r.getY() + ear);
            p.lineTo(r.getRight(), r.getBottom());
            p.lineTo(r.getX(), r.getBottom());
            p.closeSubPath();
            p.startNewSubPath(r.getRight() - ear, r.getY());
            p.lineTo(r.getRight() - ear, r.getY() + ear);
            p.lineTo(r.getRight(), r.getY() + ear);
            stroke(p);
            break;
        }
        case IconGlyph::Reveal: {
            const float t = H * 0.26f;
            juce::Path folder;
            folder.startNewSubPath(r.getX(), r.getBottom());
            folder.lineTo(r.getX(), r.getY() + t);
            folder.lineTo(r.getX() + W * 0.36f, r.getY() + t);
            folder.lineTo(r.getX() + W * 0.46f, r.getY());
            folder.lineTo(r.getX() + W * 0.62f, r.getY());
            folder.lineTo(r.getX() + W * 0.62f, r.getBottom());
            folder.closeSubPath();
            stroke(folder);
            juce::Path arrow;
            const float ay = r.getY() + H * 0.62f;
            arrow.startNewSubPath(r.getX() + W * 0.5f, ay);
            arrow.lineTo(r.getRight(), ay);
            arrow.startNewSubPath(r.getRight() - W * 0.2f, ay - H * 0.2f);
            arrow.lineTo(r.getRight(), ay);
            arrow.lineTo(r.getRight() - W * 0.2f, ay + H * 0.2f);
            stroke(arrow);
            break;
        }
        case IconGlyph::Open: {
            const float t = H * 0.26f;
            p.startNewSubPath(r.getX(), r.getBottom());
            p.lineTo(r.getX(), r.getY() + t);
            p.lineTo(r.getX() + W * 0.4f, r.getY() + t);
            p.lineTo(r.getX() + W * 0.52f, r.getY());
            p.lineTo(r.getRight(), r.getY());
            p.lineTo(r.getRight(), r.getY() + t * 1.4f);
            stroke(p);
            juce::Path front;
            front.startNewSubPath(r.getX(), r.getBottom());
            front.lineTo(r.getX() + W * 0.22f, r.getY() + t * 1.5f);
            front.lineTo(r.getRight(), r.getY() + t * 1.5f);
            front.lineTo(r.getRight() - W * 0.22f, r.getBottom());
            front.closeSubPath();
            stroke(front);
            break;
        }
        case IconGlyph::Save: {
            const float c = W * 0.22f;
            p.startNewSubPath(r.getX(), r.getY());
            p.lineTo(r.getRight() - c, r.getY());
            p.lineTo(r.getRight(), r.getY() + c);
            p.lineTo(r.getRight(), r.getBottom());
            p.lineTo(r.getX(), r.getBottom());
            p.closeSubPath();
            stroke(p);
            g.fillRect(r.getX() + W * 0.2f, r.getCentreY() + H * 0.04f, W * 0.6f, H * 0.34f);
            g.fillRect(r.getX() + W * 0.56f, r.getY() + H * 0.07f, W * 0.16f, H * 0.22f);
            break;
        }
        case IconGlyph::Export: {
            const float cx = r.getCentreX();
            p.startNewSubPath(cx, r.getY());
            p.lineTo(cx, r.getY() + H * 0.55f);
            p.startNewSubPath(cx - W * 0.22f, r.getY() + H * 0.33f);
            p.lineTo(cx, r.getY() + H * 0.6f);
            p.lineTo(cx + W * 0.22f, r.getY() + H * 0.33f);
            p.startNewSubPath(r.getX(), r.getBottom());
            p.lineTo(r.getRight(), r.getBottom());
            stroke(p);
            break;
        }
        case IconGlyph::Undo:
        case IconGlyph::Redo: {
            const bool redo = glyph == IconGlyph::Redo;
            auto c = r.getCentre();
            const float rad = 5.6f * u;
            p.addCentredArc(c.x, c.y, rad, rad, 0.0f,
                            redo ?  juce::MathConstants<float>::pi * 0.4f : -juce::MathConstants<float>::pi * 0.4f,
                            redo ? -juce::MathConstants<float>::pi * 1.1f :  juce::MathConstants<float>::pi * 1.1f,
                            true);
            stroke(p);
            const float ax = redo ? c.x + rad * 0.5f : c.x - rad * 0.5f, ay = c.y - rad;
            juce::Path head;
            head.addTriangle(ax, ay - sw * 1.6f, ax, ay + sw * 1.6f, ax + (redo ? -sw * 2.6f : sw * 2.6f), ay);
            fillStroked(head);
            break;
        }
        case IconGlyph::Cut: {
            const float ringR = W * 0.17f;
            g.drawEllipse(r.getX(), r.getBottom() - ringR * 2, ringR * 2, ringR * 2, sw);
            g.drawEllipse(r.getRight() - ringR * 2, r.getBottom() - ringR * 2, ringR * 2, ringR * 2, sw);
            p.startNewSubPath(r.getX() + ringR, r.getBottom() - ringR);
            p.lineTo(r.getRight() - ringR, r.getY());
            p.startNewSubPath(r.getRight() - ringR, r.getBottom() - ringR);
            p.lineTo(r.getX() + ringR, r.getY());
            stroke(p);
            break;
        }
        case IconGlyph::Copy: {
            juce::Path back, front;
            back.addRoundedRectangle(r.getX() + W * 0.28f, r.getY(), W * 0.62f, H * 0.62f, 2.0f);
            front.addRoundedRectangle(r.getX(), r.getY() + H * 0.3f, W * 0.62f, H * 0.7f, 2.0f);
            stroke(back);
            g.setColour(Palette::panel);
            g.fillPath(front);
            g.setColour(fg);
            stroke(front);
            break;
        }
        case IconGlyph::Paste: {
            juce::Path board;
            board.addRoundedRectangle(r.getX(), r.getY() + H * 0.12f, W, H * 0.88f, 2.0f);
            stroke(board);
            g.fillRect(r.getCentreX() - W * 0.17f, r.getY(), W * 0.34f, H * 0.2f);
            break;
        }
        case IconGlyph::EnableAudio: {
            p.startNewSubPath(cx - 6.4f * u, cy - 2.5f * u);
            p.lineTo(cx - 2.9f * u, cy - 2.5f * u);
            p.lineTo(cx - 0.9f * u, cy - 5.5f * u);
            p.lineTo(cx - 0.9f * u, cy + 5.5f * u);
            p.lineTo(cx - 2.9f * u, cy + 2.5f * u);
            p.lineTo(cx - 6.4f * u, cy + 2.5f * u);
            p.closeSubPath();
            g.fillPath(p.createPathWithRoundedCorners(1.2f * u));
            const float pi = juce::MathConstants<float>::pi;
            juce::Path arc;
            arc.addCentredArc(cx - 1.0f * u, cy, 6.0f * u, 6.0f * u, 0.0f,
                              pi * 0.22f, pi * 0.78f, true);
            stroke(arc);
            break;
        }
        case IconGlyph::EnableMidi: {
            const float pi = juce::MathConstants<float>::pi;
            juce::Path shell;
            shell.addCentredArc(cx, cy, 6.265f * u, 6.265f * u, 0.0f,
                                pi * 1.15f, pi * 2.85f, true);
            g.strokePath(shell, juce::PathStrokeType(hr, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            const float pin = 1.15f * u, rho = 4.15f * u;
            for (int i = 0; i < 5; ++i) {
                const float a = pi * 1.5f + (float) i * pi * 0.25f;
                g.fillEllipse(cx + std::sin(a) * rho - pin, cy - std::cos(a) * rho - pin,
                              pin * 2.0f, pin * 2.0f);
            }
            break;
        }
        case IconGlyph::GoToStart: {
            g.fillRoundedRectangle(cx - 7.9f * u, cy - 7.0f * u, 2.5f * u, 14.0f * u, 0.6f * u);
            p.addTriangle(cx + 7.1f * u, cy - 6.6f * u,
                          cx + 7.1f * u, cy + 6.6f * u,
                          cx - 3.8f * u, cy);
            g.fillPath(p.createPathWithRoundedCorners(1.2f * u));
            break;
        }
        case IconGlyph::PlayFromStart: {
            g.fillRoundedRectangle(cx - 7.9f * u, cy - 7.0f * u, 2.5f * u, 14.0f * u, 0.6f * u);
            p.addTriangle(cx - 2.4f * u, cy - 6.6f * u,
                          cx - 2.4f * u, cy + 6.6f * u,
                          cx + 7.9f * u, cy);
            g.fillPath(p.createPathWithRoundedCorners(1.2f * u));
            break;
        }
        case IconGlyph::Play: {
            p.addTriangle(cx - 3.6f * u, cy - 6.6f * u,
                          cx - 3.6f * u, cy + 6.6f * u,
                          cx + 7.2f * u, cy);
            g.fillPath(p.createPathWithRoundedCorners(1.2f * u));
            break;
        }
        case IconGlyph::Stop:
            g.fillRoundedRectangle(cx - 5.5f * u, cy - 5.5f * u, 11.0f * u, 11.0f * u, 2.0f * u);
            break;
        case IconGlyph::GoToEnd: {
            g.fillRoundedRectangle(cx + 5.4f * u, cy - 7.0f * u, 2.5f * u, 14.0f * u, 0.6f * u);
            p.addTriangle(cx - 7.1f * u, cy - 6.6f * u,
                          cx - 7.1f * u, cy + 6.6f * u,
                          cx + 3.8f * u, cy);
            g.fillPath(p.createPathWithRoundedCorners(1.2f * u));
            break;
        }
        case IconGlyph::Record: {
            if (active && enabled) {
                g.setColour(Palette::recordRed());
                g.fillEllipse(cx - 6.3f * u, cy - 6.3f * u, 12.6f * u, 12.6f * u);
            } else {
                g.setColour(fg);
                g.drawEllipse(cx - 5.45f * u, cy - 5.45f * u, 10.9f * u, 10.9f * u, sw);
            }
            break;
        }
        case IconGlyph::Keep: {
            g.drawRoundedRectangle(cx - 0.45f * u, cy - 4.95f * u,
                                   7.4f * u, 9.9f * u, 1.0f * u, sw);
            juce::Path head;
            head.addTriangle(cx - 4.5f * u, cy,
                             cx - 8.3f * u, cy - 2.6f * u,
                             cx - 8.3f * u, cy + 2.6f * u);
            g.fillPath(head);
            break;
        }
        case IconGlyph::Loop: {
            const float pi = juce::MathConstants<float>::pi;
            p.addCentredArc(cx, cy, 5.45f * u, 5.45f * u, 0.0f, pi * 0.22f, pi * 1.78f, true);
            stroke(p);
            juce::Path head;
            head.addTriangle(cx - 1.54f * u, cy - 5.79f * u,
                             cx - 3.42f * u, cy - 0.22f * u,
                             cx - 7.38f * u, cy - 5.00f * u);
            g.fillPath(head);
            break;
        }
        case IconGlyph::Patcher: {
            const float boxW = W * 0.34f, boxH = H * 0.3f;
            juce::Rectangle<float> n1(r.getX(), r.getY(), boxW, boxH);
            juce::Rectangle<float> n2(r.getRight() - boxW, r.getBottom() - boxH, boxW, boxH);
            juce::Path boxes;
            boxes.addRoundedRectangle(n1, 1.5f);
            boxes.addRoundedRectangle(n2, 1.5f);
            stroke(boxes);
            p.startNewSubPath(n1.getRight(), n1.getCentreY());
            p.lineTo(r.getCentreX(), n1.getCentreY());
            p.lineTo(r.getCentreX(), n2.getCentreY());
            p.lineTo(n2.getX(), n2.getCentreY());
            stroke(p);
            break;
        }
        case IconGlyph::Properties: {
            for (int i = 0; i < 3; ++i) {
                const float y = r.getY() + H * (0.16f + 0.34f * (float) i);
                p.startNewSubPath(r.getX(), y);
                p.lineTo(r.getRight(), y);
                const float kx = r.getX() + W * (i == 1 ? 0.66f : 0.33f);
                g.fillEllipse(kx - sw, y - sw, sw * 2.0f, sw * 2.0f);
            }
            stroke(p);
            break;
        }
        case IconGlyph::Automation: {
            const float xs[4] = {r.getX(), r.getX() + W * 0.34f, r.getX() + W * 0.66f, r.getRight()};
            const float ys[4] = {r.getBottom(), r.getY() + H * 0.2f, r.getBottom() - H * 0.15f, r.getY()};
            p.startNewSubPath(xs[0], ys[0]);
            for (int i = 1; i < 4; ++i) p.lineTo(xs[i], ys[i]);
            stroke(p);
            for (int i = 0; i < 4; ++i) g.fillEllipse(xs[i] - sw, ys[i] - sw, sw * 2.0f, sw * 2.0f);
            break;
        }
        case IconGlyph::Metapad: {
            g.drawRoundedRectangle(cx - 5.95f * u, cy - 5.95f * u,
                                   11.9f * u, 11.9f * u, 1.0f * u, sw);
            g.setColour(fg.withMultipliedAlpha(0.5f));
            g.fillRect(cx - hr * 0.5f, cy - 3.4f * u, hr, 6.8f * u);
            g.fillRect(cx - 3.4f * u, cy - hr * 0.5f, 6.8f * u, hr);
            g.setColour(fg);
            const float pip = 1.7f * u;
            g.fillEllipse(cx + 2.6f * u - pip, cy - 2.6f * u - pip, pip * 2.0f, pip * 2.0f);
            break;
        }
        case IconGlyph::ParameterControl: {
            g.drawEllipse(cx - 5.45f * u, cy - 5.45f * u, 10.9f * u, 10.9f * u, sw);
            juce::Path nd;
            nd.startNewSubPath(cx, cy);
            nd.lineTo(cx + 3.26f * u, cy - 2.37f * u);
            stroke(nd);
            break;
        }
        case IconGlyph::Notes: {
            g.drawRoundedRectangle(cx - 4.95f * u, cy - 5.45f * u,
                                   9.9f * u, 10.9f * u, 1.0f * u, sw);
            g.fillRect(cx - 2.2f * u - hr * 0.5f, cy - 4.4f * u, hr, 8.8f * u);
            for (float dy : {-2.0f, 2.6f})
                g.fillRect(cx - 0.5f * u, cy + dy * u - hr * 0.5f, 4.4f * u, hr);
            break;
        }
        case IconGlyph::Library: {
            g.fillRoundedRectangle(cx - 6.4f * u, cy - 5.4f * u, 2.6f * u, 10.8f * u, 0.6f * u);
            g.fillRoundedRectangle(cx - 2.6f * u, cy - 5.4f * u, 2.6f * u, 10.8f * u, 0.6f * u);
            {
                juce::Path lean;
                lean.addRoundedRectangle(-1.3f * u, -5.4f * u, 2.6f * u, 10.8f * u, 0.6f * u);
                lean.applyTransform(juce::AffineTransform::rotation(0.32f)
                                        .translated(cx + 3.8f * u, cy + 0.4f * u));
                g.fillPath(lean);
            }
            break;
        }
        case IconGlyph::DocumentSwitcher: {
            g.fillRoundedRectangle(cx - 7.0f * u, cy - 6.5f * u, 6.0f * u, 5.2f * u, 1.0f * u);
            g.fillRoundedRectangle(cx + 2.0f * u, cy - 5.0f * u, 5.0f * u, 3.7f * u, 1.0f * u);
            g.drawRoundedRectangle(cx - 6.95f * u, cy - 0.95f * u,
                                   13.9f * u, 6.4f * u, 1.0f * u, sw);
            break;
        }
        case IconGlyph::Help: {
            g.setFont(juce::Font(juce::FontOptions(13.6f * u)).boldened());
            g.drawText("?", r, juce::Justification::centred, false);
            break;
        }
        case IconGlyph::QwertyPiano: {
            g.drawRoundedRectangle(cx - 6.95f * u, cy - 5.45f * u,
                                   13.9f * u, 10.9f * u, 1.0f * u, sw);
            for (float dx : {-2.67f, 2.67f})
                g.fillRect(cx + dx * u - hr * 0.5f, cy + 1.6f * u, hr, 3.85f * u);
            for (float dx : {-4.07f, 1.27f})
                g.fillRoundedRectangle(cx + dx * u, cy - 4.4f * u, 2.8f * u, 6.0f * u, 0.5f * u);
            break;
        }
        case IconGlyph::Dice: {
            g.drawRoundedRectangle(cx - 5.95f * u, cy - 5.95f * u,
                                   11.9f * u, 11.9f * u, 3.2f * u, sw);
            const float pip = 1.6f * u;
            auto dot = [&](float dx, float dy) {
                g.fillEllipse(cx + dx - pip, cy + dy - pip, pip * 2.0f, pip * 2.0f);
            };
            dot(-2.8f * u, -2.8f * u);
            dot( 2.8f * u,  2.8f * u);
            if (S >= 20.0f) dot(0.0f, 0.0f);
            break;
        }
        case IconGlyph::Evolve: {
            const float cellR = 3.6f * u;
            g.drawEllipse(cx - 2.4f * u - cellR, cy + 2.4f * u - cellR, cellR * 2.0f, cellR * 2.0f, sw);
            const float budR = 1.7f * u;
            g.fillEllipse(cx + 4.2f * u - budR, cy - 4.2f * u - budR, budR * 2.0f, budR * 2.0f);
            break;
        }
        case IconGlyph::Pause: {
            const float bw = 3.6f * u;
            g.fillRoundedRectangle(cx - 5.4f * u, cy - 6.0f * u, bw, 12.0f * u, 0.8f * u);
            g.fillRoundedRectangle(cx + 1.8f * u, cy - 6.0f * u, bw, 12.0f * u, 0.8f * u);
            break;
        }
        case IconGlyph::Power: {
            const float rad = 5.2f * u;
            p.addCentredArc(cx, cy + rad * 0.12f, rad, rad, 0.0f,
                            juce::degreesToRadians(28.0f), juce::degreesToRadians(332.0f), true);
            stroke(p);
            g.drawLine(cx, cy - 7.0f * u, cx, cy + rad * 0.05f, sw);
            break;
        }
        case IconGlyph::KnobView: {
            g.drawEllipse(cx - 5.5f * u, cy - 5.5f * u, 11.0f * u, 11.0f * u, sw);
            g.drawLine(cx, cy, cx + 3.9f * u, cy - 3.9f * u, sw);
            break;
        }
        case IconGlyph::PluginView: {
            g.drawRoundedRectangle(cx - 6.0f * u, cy - 5.0f * u, 12.0f * u, 10.0f * u, 1.5f * u, sw);
            g.drawLine(cx - 5.0f * u, cy - 2.2f * u, cx + 5.0f * u, cy - 2.2f * u, sw);
            break;
        }
        case IconGlyph::RackView:
            for (int i = -1; i <= 1; ++i)
                g.fillRoundedRectangle(cx - 6.0f * u, cy + (float) i * 4.0f * u - 1.2f * u,
                                       12.0f * u, 2.4f * u, 0.6f * u);
            break;
        case IconGlyph::FreeView:
            g.fillRoundedRectangle(cx - 6.0f * u, cy - 5.5f * u, 6.5f * u, 5.0f * u, 0.8f * u);
            g.fillRoundedRectangle(cx - 1.5f * u, cy + 1.0f * u, 6.5f * u, 5.0f * u, 0.8f * u);
            break;
        case IconGlyph::Warning: {
            p.addTriangle(cx, cy - 6.2f * u, cx + 6.6f * u, cy + 5.4f * u, cx - 6.6f * u, cy + 5.4f * u);
            stroke(p.createPathWithRoundedCorners(1.6f * u));
            g.fillRoundedRectangle(cx - 0.8f * u, cy - 2.6f * u, 1.6f * u, 4.4f * u, 0.8f * u);
            g.fillEllipse(cx - 0.95f * u, cy + 2.6f * u, 1.9f * u, 1.9f * u);
            break;
        }
        case IconGlyph::DockCentre:
        case IconGlyph::DockRight:
        case IconGlyph::DockBottom: {
            juce::Rectangle<float> slot;
            if (glyph == IconGlyph::DockCentre)
                slot = { cx - 5.9f * u, cy - 4.4f * u, 7.82f * u, 5.96f * u };
            else if (glyph == IconGlyph::DockRight)
                slot = { cx + 1.92f * u, cy - 4.4f * u, 3.98f * u, 5.96f * u };
            else
                slot = { cx - 5.9f * u, cy + 1.56f * u, 11.8f * u, 3.89f * u };
            g.setColour(fg.withMultipliedAlpha(0.42f));
            g.fillRect(slot);
            g.setColour(fg);
            g.drawRoundedRectangle(cx - 6.95f * u, cy - 5.45f * u,
                                   13.9f * u, 10.9f * u, 1.0f * u, sw);
            if (glyph == IconGlyph::DockBottom)
                g.fillRect(cx - 5.9f * u, cy + 1.56f * u - hr * 0.5f, 11.8f * u, hr);
            else
                g.fillRect(cx + 1.92f * u - hr * 0.5f, cy - 4.4f * u, hr, 5.96f * u);
            break;
        }
        case IconGlyph::Gear: {
            const float pi = juce::MathConstants<float>::pi;
            juce::Path teeth;
            for (int i = 0; i < 6; ++i) {
                const float a = pi * (float) i / 3.0f;
                const float s = std::sin(a), c = std::cos(a);
                teeth.startNewSubPath(cx + s * 3.4f * u, cy - c * 3.4f * u);
                teeth.lineTo(cx + s * 6.2f * u, cy - c * 6.2f * u);
            }
            g.strokePath(teeth, juce::PathStrokeType(2.2f * u, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            g.drawEllipse(cx - 3.6f * u, cy - 3.6f * u, 7.2f * u, 7.2f * u, sw);
            break;
        }
        case IconGlyph::Overflow: {
            const float pip = 1.3f * u;
            for (float dx : {-5.6f, 0.0f, 5.6f})
                g.fillEllipse(cx + dx * u - pip, cy - pip, pip * 2.0f, pip * 2.0f);
            break;
        }

    }
}

enum class WaveGlyph { Saw, Square, Pulse, Tri, Sine, SawDown, Rand, Fat };

inline const std::array<const char*, 8> kWaveGlyphNames = {
    "Saw", "Square", "Pulse", "Tri", "Sine", "SawDown", "Rand", "Fat"
};

inline juce::String waveGlyphDescription(WaveGlyph g) {
    switch (g) {
        case WaveGlyph::Square:  return "Square";
        case WaveGlyph::Pulse:   return tr("icon-glyph.thin-pulse", "Thin pulse");
        case WaveGlyph::Tri:     return "Triangle";
        case WaveGlyph::Sine:    return "Sine";
        case WaveGlyph::SawDown: return tr("icon-glyph.saw-down", "Saw down");
        case WaveGlyph::Rand:    return "Sample & hold";
        case WaveGlyph::Fat:     return tr("icon-glyph.fat-saw-pair-detuned", "Fat saw pair (detuned)");
        default:                 return "Sawtooth";
    }
}

inline WaveGlyph waveGlyphFor(const juce::String& label) {
    const auto l = label.toLowerCase();
    if (l.contains("fat")) return WaveGlyph::Fat;
    if (l.contains("sin")) return WaveGlyph::Sine;
    if (l.contains("s&h") || l.contains("rnd") || l.contains("rand")) return WaveGlyph::Rand;
    if (l.contains("tri")) return WaveGlyph::Tri;
    if (l.contains("pls") || l.contains("pul")) return WaveGlyph::Pulse;
    if (l.contains("sq")) return WaveGlyph::Square;
    if (l.contains("dsaw") || l.contains("down")) return WaveGlyph::SawDown;
    return WaveGlyph::Saw;
}

inline void drawWaveGlyph(juce::Graphics& g, WaveGlyph glyph_, juce::Rectangle<float> r,
                          juce::Colour ink) {
    const auto b = r.reduced(r.getWidth() * 0.18f, r.getHeight() * 0.28f);
    auto pt = [&](float fx, float fy) {
        return juce::Point<float>(b.getX() + fx * b.getWidth(),
                                  b.getY() + fy * b.getHeight());
    };
    juce::Path p;
    switch (glyph_) {
        case WaveGlyph::Saw:
            p.startNewSubPath(pt(0, 1)); p.lineTo(pt(0.5f, 0)); p.lineTo(pt(0.5f, 1));
            p.lineTo(pt(1, 0));
            break;
        case WaveGlyph::Square:
            p.startNewSubPath(pt(0, 1)); p.lineTo(pt(0, 0)); p.lineTo(pt(0.5f, 0));
            p.lineTo(pt(0.5f, 1)); p.lineTo(pt(1, 1)); p.lineTo(pt(1, 0));
            break;
        case WaveGlyph::Pulse:
            p.startNewSubPath(pt(0, 1)); p.lineTo(pt(0, 0)); p.lineTo(pt(0.25f, 0));
            p.lineTo(pt(0.25f, 1)); p.lineTo(pt(1, 1)); p.lineTo(pt(1, 0));
            break;
        case WaveGlyph::Tri:
            p.startNewSubPath(pt(0, 1)); p.lineTo(pt(0.25f, 0)); p.lineTo(pt(0.75f, 1));
            p.lineTo(pt(1, 0));
            break;
        case WaveGlyph::Sine:
            p.startNewSubPath(pt(0, 0.5f));
            p.cubicTo(pt(0.17f, -0.1f), pt(0.33f, -0.1f), pt(0.5f, 0.5f));
            p.cubicTo(pt(0.67f, 1.1f), pt(0.83f, 1.1f), pt(1, 0.5f));
            break;
        case WaveGlyph::SawDown:
            p.startNewSubPath(pt(0, 0)); p.lineTo(pt(0.5f, 1)); p.lineTo(pt(0.5f, 0));
            p.lineTo(pt(1, 1));
            break;
        case WaveGlyph::Rand:
            p.startNewSubPath(pt(0, 0.6f)); p.lineTo(pt(0.25f, 0.6f));
            p.lineTo(pt(0.25f, 0.05f)); p.lineTo(pt(0.5f, 0.05f));
            p.lineTo(pt(0.5f, 0.85f)); p.lineTo(pt(0.75f, 0.85f));
            p.lineTo(pt(0.75f, 0.35f)); p.lineTo(pt(1, 0.35f));
            break;
        case WaveGlyph::Fat:
            p.startNewSubPath(pt(0, 0.85f)); p.lineTo(pt(0.5f, -0.15f));
            p.lineTo(pt(0.5f, 0.85f)); p.lineTo(pt(1, -0.15f));
            p.startNewSubPath(pt(0, 1.15f)); p.lineTo(pt(0.5f, 0.15f));
            p.lineTo(pt(0.5f, 1.15f)); p.lineTo(pt(1, 0.15f));
            break;
    }
    g.setColour(ink);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::mitered,
                                         juce::PathStrokeType::rounded));
}

}
