// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/LayoutLoader.h"

#include <algorithm>
#include <set>

#include "core/json/Json.h"

namespace hum {

namespace {

std::string getString(const json::Value& obj, const char* key, const std::string& fallback = {}) {
    const auto& v = obj[key];
    return v.isString() ? v.text() : fallback;
}

int getInt(const json::Value& obj, const char* key, int fallback = 0) {
    const auto& v = obj[key];
    return v.isNumber() ? v.integer() : fallback;
}

bool getBool(const json::Value& obj, const char* key, bool fallback = false) {
    const auto& v = obj[key];
    return v.isNull() ? fallback : v.truthy();
}

constexpr const char* kSwingType = "swing";
constexpr int kSwingW = 180, kSwingH = 24, kSwingPadW = 58, kSwingGap = 6, kSwingRowMinW = 124;

void addSwing(LayoutSpec& spec, const json::Value& cv) {
    using CT = LayoutSpec::ControlType;
    const auto follow = getString(cv, "follow", "SwingFollow");
    const auto amount = getString(cv, "swing", "Swing");
    const int x = getInt(cv, "x", 0), y = getInt(cv, "y", 0), h = getInt(cv, "h", kSwingH);
    const int w = getInt(cv, "w", kSwingW);
    const bool stacked = w < kSwingRowMinW;
    LayoutSpec::Control pad;
    pad.type = CT::LitButton;
    pad.param = follow;
    pad.label = "Follow";
    pad.x = x;
    pad.y = y;
    pad.w = stacked ? w : kSwingPadW;
    pad.h = h;
    pad.extra = {{"icon", "none"}, {"tooltip", "Take the swing from the transport."}};
    spec.controls.push_back(pad);
    LayoutSpec::Control slider;
    slider.type = CT::HSlider;
    slider.param = amount;
    slider.label = stacked ? "" : "Swing";
    slider.x = stacked ? x : x + kSwingPadW + kSwingGap;
    slider.y = stacked ? y + h + kSwingGap : y;
    slider.w = stacked ? w : w - kSwingPadW - kSwingGap;
    slider.h = h;
    slider.extra = {{"dim-when", follow}, {"tooltip", "Swing"}};
    if (!stacked) slider.extra["show-value"] = "1";
    spec.controls.push_back(slider);
}

}

LayoutSpec loadLayoutSpec(const std::string& jsonText) {
    LayoutSpec spec;
    const auto root = json::parse(jsonText);
    if (!root.isObject()) return spec;

    spec.width = getInt(root, "width", 280);
    spec.height = getInt(root, "height", 220);
    spec.scrollFrom = getInt(root, "scroll-from", -1);
    if (const auto& skin = root["skin"]; skin.isObject()) spec.skin = json::write(skin);
    for (const auto& [name, value] : root["bind"].members()) spec.bind[name] = value.text();
    if (getString(root, "resize") == "stretch") spec.resize = LayoutSpec::Resize::Stretch;
    if (getString(root, "resize") == "grow") spec.resize = LayoutSpec::Resize::Grow;

    static const std::set<std::string> known = {"type", "param", "param2", "label", "x", "y", "w", "h",
                                                "decimals", "log", "logarithmic", "options"};
    for (const auto& cv : root["controls"].items()) {
        if (getString(cv, "type") == kSwingType) {
            addSwing(spec, cv);
            continue;
        }
        LayoutSpec::Control c;
        c.type = LayoutSpec::ControlType::Knob;
        controlTypeFromName(getString(cv, "type", "knob"), c.type);
        c.param = getString(cv, "param");
        c.param2 = getString(cv, "param2");
        c.label = getString(cv, "label", c.param);
        c.x = getInt(cv, "x", 0);
        c.y = getInt(cv, "y", 0);
        c.w = getInt(cv, "w", 60);
        c.h = getInt(cv, "h", 80);
        c.decimalPlaces = getInt(cv, "decimals", 2);
        c.logarithmic = getBool(cv, "log", getBool(cv, "logarithmic", false));
        for (const auto& o : cv["options"].items()) c.options.push_back(o.text());
        for (const auto& [name, value] : cv.members())
            if (known.count(name) == 0 && !value.isArray()) c.extra[name] = value.text();
        spec.controls.push_back(std::move(c));
    }
    return spec;
}

LayoutSpec loadLayoutSpecFromFile(const std::string& path) {
    std::string text;
    if (!json::readTextFile(path, text)) return {};
    auto spec = loadLayoutSpec(text);
    if (const auto slash = path.find_last_of("/\\"); slash != std::string::npos) spec.dir = path.substr(0, slash);
    return spec;
}

bool knownLayoutControlType(const std::string& type) {
    LayoutSpec::ControlType t;
    return type == kSwingType || controlTypeFromName(type, t);
}

}
