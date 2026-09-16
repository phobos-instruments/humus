// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/Skin.h"

#include <algorithm>
#include <set>
#include <utility>

#include "gui/style/LookAndFeel.h"
#include "hum/LayoutSpec.h"

namespace hum {

namespace {

constexpr float kScrewSlotWidth = 0.9f;
constexpr float kScrewShadowDrop = 0.6f;

struct Reader {
    const juce::var& layer;
    std::vector<std::string>* problems;
    std::string where;
    const Skin::Anchors* anchors = nullptr;

    void problem(const std::string& what) const {
        if (problems != nullptr) problems->push_back(where + ": " + what);
    }

    float number(const char* key, float fallback) const {
        const auto v = layer[key];
        if (v.isVoid()) return fallback;
        if (!v.isDouble() && !v.isInt() && !v.isInt64()) {
            problem(std::string("'") + key + "' is not a number");
            return fallback;
        }
        return (float) (double) v;
    }

    bool colour(const juce::var& v, juce::Colour& out, const char* key) const {
        auto t = v.toString().trim();
        if (t.startsWithChar('#')) t = t.substring(1);
        if ((t.length() != 6 && t.length() != 8) || !t.containsOnly("0123456789abcdefABCDEF")) {
            problem(std::string("'") + key + "' is not a #RRGGBB or #AARRGGBB colour");
            return false;
        }
        auto argb = (juce::uint32) t.getHexValue32();
        if (t.length() == 6) argb |= 255u << 24;
        out = juce::Colour(argb);
        return true;
    }

    void colourKey(const char* key, juce::Colour& out) const {
        if (const auto v = layer[key]; !v.isVoid()) colour(v, out, key);
    }
};

const std::set<std::string>& knownKeys() {
    static const std::set<std::string> keys{
        "shape", "theme", "x", "y", "w", "h", "fill", "stroke", "stroke-width", "radius",
        "pitch", "offset", "margin", "inset", "size", "spread", "mark", "text", "font-size",
        "bold", "justify", "shadow", "around", "pad"};
    return keys;
}

bool shapeFrom(const juce::String& s, SkinLayer::Shape& out) {
    using S = SkinLayer::Shape;
    static const std::pair<const char*, S> names[] = {
        {"rect", S::Rect}, {"lines", S::Lines}, {"screws", S::Screws}, {"lamp", S::Lamp}, {"text", S::Text}};
    for (const auto& [n, v] : names)
        if (s == n) { out = v; return true; }
    return false;
}

bool justifyFrom(const juce::String& s, juce::Justification& out) {
    static const std::pair<const char*, int> names[] = {
        {"left", juce::Justification::centredLeft}, {"centre", juce::Justification::centred},
        {"right", juce::Justification::centredRight}, {"top-left", juce::Justification::topLeft},
        {"bottom-left", juce::Justification::bottomLeft}};
    for (const auto& [n, v] : names)
        if (s == n) { out = juce::Justification(v); return true; }
    return false;
}

juce::Point<float> padOf(const Reader& in) {
    const auto pad = in.layer["pad"];
    if (!pad.isArray()) {
        const float all = in.number("pad", 0.0f);
        return {all, all};
    }
    if (pad.size() == 2) return {(float) (double) pad[0], (float) (double) pad[1]};
    in.problem("a pad takes one number, or two: across then down");
    return {};
}

void anchor(const Reader& in, const juce::String& param, SkinLayer& l) {
    juce::Rectangle<float> area;
    if (in.anchors == nullptr || !*in.anchors) {
        in.problem("only a face layer can sit around a control");
        return;
    }
    if (!(*in.anchors)(param.toStdString(), area)) {
        in.problem("no single control for '" + param.toStdString() + "' to sit around");
        return;
    }
    if (!in.layer["x"].isVoid() || !in.layer["y"].isVoid())
        in.problem("'around' places the layer, so 'x' and 'y' have nothing to say");
    const auto pad = padOf(in);
    area = area.expanded(pad.x, pad.y);
    l.x = area.getX();
    l.y = area.getY();
    if (l.w < 0.0f) l.w = area.getWidth();
    if (l.h < 0.0f) l.h = area.getHeight();
}

SkinLayer readLayer(const Reader& in) {
    SkinLayer l;
    if (!in.layer.isObject()) {
        in.problem("a layer is not an object");
        return l;
    }
    for (const auto& prop : in.layer.getDynamicObject()->getProperties())
        if (knownKeys().count(prop.name.toString().toStdString()) == 0)
            in.problem("unknown key '" + prop.name.toString().toStdString() + "'");
    if (!shapeFrom(in.layer["shape"].toString(), l.shape))
        in.problem("unknown shape '" + in.layer["shape"].toString().toStdString() + "'");
    const auto theme = in.layer["theme"].toString();
    if (theme == "dark") l.theme = SkinLayer::Theme::Dark;
    else if (theme == "light") l.theme = SkinLayer::Theme::Light;
    else if (theme.isNotEmpty()) in.problem("theme must be dark or light");
    l.x = in.number("x", 0.0f);
    l.y = in.number("y", 0.0f);
    l.w = in.number("w", -1.0f);
    l.h = in.number("h", -1.0f);
    if (const auto around = in.layer["around"].toString(); around.isNotEmpty()) anchor(in, around, l);
    else if (!in.layer["pad"].isVoid()) in.problem("a pad only means something around a control");
    if (const auto fill = in.layer["fill"]; fill.isArray()) {
        if (fill.size() == 2 && in.colour(fill[0], l.fill, "fill") && in.colour(fill[1], l.fillTo, "fill"))
            l.gradient = true;
        else if (fill.size() != 2)
            in.problem("a fill gradient takes two colours, top then bottom");
    } else {
        in.colourKey("fill", l.fill);
    }
    in.colourKey("stroke", l.stroke);
    in.colourKey("mark", l.mark);
    in.colourKey("shadow", l.shadow);
    l.strokeWidth = in.number("stroke-width", l.stroke.isTransparent() ? 0.0f : 1.0f);
    l.radius = in.number("radius", 0.0f);
    l.pitch = std::max(1.0f, in.number("pitch", l.pitch));
    l.offset = in.number("offset", l.offset);
    l.margin = in.number("margin", 0.0f);
    l.inset = in.number("inset", l.inset);
    l.size = in.number("size", l.size);
    l.spread = in.number("spread", 0.0f);
    l.text = in.layer["text"].toString();
    l.fontSize = in.number("font-size", l.fontSize);
    l.bold = (bool) in.layer.getProperty("bold", false);
    if (const auto j = in.layer["justify"].toString(); j.isNotEmpty() && !justifyFrom(j, l.justify))
        in.problem("unknown justify '" + j.toStdString() + "'");
    return l;
}

void paintRect(juce::Graphics& g, const SkinLayer& l, juce::Rectangle<float> r) {
    if (l.gradient) g.setGradientFill({l.fill, r.getX(), r.getY(), l.fillTo, r.getX(), r.getBottom(), false});
    else g.setColour(l.fill);
    if (l.gradient || !l.fill.isTransparent()) {
        if (l.radius > 0.0f) g.fillRoundedRectangle(r, l.radius);
        else g.fillRect(r);
    }
    if (l.strokeWidth > 0.0f && !l.stroke.isTransparent()) {
        g.setColour(l.stroke);
        if (l.radius > 0.0f) g.drawRoundedRectangle(r, l.radius, l.strokeWidth);
        else g.drawRect(r, l.strokeWidth);
    }
}

void paintLines(juce::Graphics& g, const SkinLayer& l, juce::Rectangle<float> r) {
    g.setColour(l.fill);
    for (float y = r.getY() + l.offset; y < r.getBottom(); y += l.pitch)
        g.drawHorizontalLine((int) y, r.getX() + l.margin, r.getRight() - l.margin);
}

void paintScrews(juce::Graphics& g, const SkinLayer& l, juce::Rectangle<float> r) {
    const auto in = r.reduced(l.inset);
    for (const auto at : {in.getTopLeft(), in.getTopRight(), in.getBottomLeft(), in.getBottomRight()}) {
        const juce::Rectangle<float> head(at.x - l.size, at.y - l.size, l.size * 2.0f, l.size * 2.0f);
        if (!l.shadow.isTransparent()) {
            g.setColour(l.shadow);
            g.fillEllipse(head.translated(0.0f, kScrewShadowDrop));
        }
        g.setColour(l.fill);
        g.fillEllipse(head);
        g.setColour(l.mark);
        g.drawLine(head.getX() + 1.0f, head.getCentreY(), head.getRight() - 1.0f, head.getCentreY(),
                   kScrewSlotWidth);
    }
}

void paintLamp(juce::Graphics& g, const SkinLayer& l, juce::Point<float> centre) {
    if (l.spread > 0.0f && !l.mark.isTransparent()) {
        const float glow = l.size + l.spread;
        g.setColour(l.mark);
        g.fillEllipse(centre.x - glow, centre.y - glow, glow * 2.0f, glow * 2.0f);
    }
    g.setColour(l.fill);
    g.fillEllipse(centre.x - l.size, centre.y - l.size, l.size * 2.0f, l.size * 2.0f);
}

void paintText(juce::Graphics& g, const SkinLayer& l, juce::Rectangle<float> r) {
    g.setFont(juce::FontOptions(l.fontSize, l.bold ? juce::Font::bold : juce::Font::plain));
    if (!l.shadow.isTransparent()) {
        g.setColour(l.shadow);
        g.drawText(l.text, r.translated(0.0f, 1.0f), l.justify, false);
    }
    g.setColour(l.fill);
    g.drawText(l.text, r, l.justify, false);
}

}

Skin Skin::forBlueprint(const LayoutSpec& spec, std::vector<std::string>* problems) {
    if (spec.skin.empty()) return {};
    return parse(spec.skin, problems, [&spec](const std::string& param, juce::Rectangle<float>& area) {
        int found = 0;
        for (const auto& c : spec.controls)
            if (c.param == param && found++ == 0)
                area = {(float) c.x, (float) c.y, (float) c.w, (float) c.h};
        return found == 1;
    });
}

Skin Skin::parse(const std::string& json, std::vector<std::string>* problems, const Anchors& anchors) {
    Skin skin;
    const auto root = juce::JSON::parse(juce::String::fromUTF8(json.c_str()));
    if (!root.isObject()) {
        if (problems != nullptr) problems->push_back("the skin is not a JSON object");
        return skin;
    }
    for (const auto& prop : root.getDynamicObject()->getProperties())
        if (prop.name.toString() != "panel" && prop.name.toString() != "face" && problems != nullptr)
            problems->push_back("unknown skin key '" + prop.name.toString().toStdString() + "'");
    for (auto* part : {"panel", "face"}) {
        const auto list = root[part];
        if (list.isVoid()) continue;
        if (!list.isArray()) {
            if (problems != nullptr) problems->push_back(std::string(part) + " is not a list of layers");
            continue;
        }
        const bool face = std::string(part) == "face";
        auto& into = face ? skin.face_ : skin.panel_;
        for (int i = 0; i < list.size(); ++i)
            into.push_back(readLayer({list[i], problems, std::string(part) + "[" + std::to_string(i) + "]",
                                      face ? &anchors : nullptr}));
    }
    return skin;
}

bool Skin::lightTheme() { return Palette::text.getPerceivedBrightness() < 0.5f; }

void Skin::paint(juce::Graphics& g, const std::vector<SkinLayer>& layers, juce::Rectangle<float> area) {
    const auto want = lightTheme() ? SkinLayer::Theme::Light : SkinLayer::Theme::Dark;
    for (const auto& l : layers) {
        if (l.theme != SkinLayer::Theme::Any && l.theme != want) continue;
        const juce::Rectangle<float> r(area.getX() + l.x, area.getY() + l.y,
                                       l.w < 0.0f ? area.getWidth() - l.x : l.w,
                                       l.h < 0.0f ? area.getHeight() - l.y : l.h);
        switch (l.shape) {
            case SkinLayer::Shape::Rect: paintRect(g, l, r); break;
            case SkinLayer::Shape::Lines: paintLines(g, l, r); break;
            case SkinLayer::Shape::Screws: paintScrews(g, l, r); break;
            case SkinLayer::Shape::Lamp: paintLamp(g, l, r.getPosition()); break;
            case SkinLayer::Shape::Text: paintText(g, l, r); break;
        }
    }
}

}
