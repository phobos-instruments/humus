// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <optional>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/app/AppSettings.h"
#include "gui/settings/HexColour.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

inline juce::Colour ThemeColours::* const kThemeRolePtrs[9] = {
    &ThemeColours::background, &ThemeColours::panel, &ThemeColours::panelLight,
    &ThemeColours::border, &ThemeColours::accent, &ThemeColours::accentDim,
    &ThemeColours::text, &ThemeColours::textDim, &ThemeColours::cord};

inline constexpr const char* kThemeRoleKeys[9] = {
    "background", "panel", "panelLight", "border", "accent",
    "accentDim", "text", "textDim", "cord"};

inline juce::var themeColoursVar(const ThemeColours& c) {
    auto* o = new juce::DynamicObject();
    for (int i = 0; i < 9; ++i) o->setProperty(kThemeRoleKeys[i], hexText(c.*kThemeRolePtrs[i]));
    return juce::var(o);
}

inline std::optional<ThemeColours> themeColoursFromLegacy(const juce::String& s) {
    const auto a = juce::StringArray::fromTokens(s, ",", "");
    if (a.size() < 9) return std::nullopt;
    ThemeColours c;
    for (int i = 0; i < 9; ++i) c.*kThemeRolePtrs[i] = juce::Colour::fromString(a[i]);
    return c;
}

inline std::optional<ThemeColours> themeColoursFromVar(const juce::var& v, const ThemeColours& base) {
    if (v.isString()) return themeColoursFromLegacy(v.toString());
    const auto* o = v.getDynamicObject();
    if (o == nullptr) return std::nullopt;
    ThemeColours c = base;
    bool any = false;
    for (int i = 0; i < 9; ++i) {
        if (!o->hasProperty(kThemeRoleKeys[i])) continue;
        const auto parsed = parseHexColour(o->getProperty(kThemeRoleKeys[i]).toString());
        if (!parsed) return std::nullopt;
        c.*kThemeRolePtrs[i] = *parsed;
        any = true;
    }
    if (!any) return std::nullopt;
    return c;
}

inline juce::String themeColoursToString(const ThemeColours& c) {
    return juce::JSON::toString(themeColoursVar(c), true);
}

inline ThemeColours themeColoursFromString(const juce::String& s, const ThemeColours& fallback) {
    const auto text = s.trim();
    const auto parsed = text.startsWithChar('{')
                            ? themeColoursFromVar(juce::JSON::parse(text), fallback)
                            : themeColoursFromLegacy(text);
    return parsed.value_or(fallback);
}

namespace themestore {

struct UserTheme {
    juce::String name;
    ThemeColours colours;
};

inline std::vector<UserTheme> list() {
    std::vector<UserTheme> out;
    const auto v = juce::JSON::parse(AppSettings::instance().getString("userThemes"));
    if (const auto* arr = v.getArray())
        for (const auto& e : *arr) {
            UserTheme t;
            t.name = e["name"].toString().trim();
            if (t.name.isEmpty()) continue;
            t.colours = themeColoursFromVar(e["colours"], presetColours(0)).value_or(presetColours(0));
            out.push_back(std::move(t));
        }
    return out;
}

inline void persist(const std::vector<UserTheme>& themes) {
    juce::Array<juce::var> arr;
    for (const auto& t : themes) {
        auto* o = new juce::DynamicObject();
        o->setProperty("name", t.name);
        o->setProperty("colours", themeColoursVar(t.colours));
        arr.add(juce::var(o));
    }
    AppSettings::instance().set("userThemes", juce::JSON::toString(juce::var(arr), true));
}

inline void save(const juce::String& name, const ThemeColours& c) {
    auto all = list();
    for (auto& t : all)
        if (t.name == name) { t.colours = c; persist(all); return; }
    all.push_back({name, c});
    persist(all);
}

inline void remove(const juce::String& name) {
    auto all = list();
    all.erase(std::remove_if(all.begin(), all.end(),
                             [&](const UserTheme& t) { return t.name == name; }),
              all.end());
    persist(all);
}

inline bool find(const juce::String& name, ThemeColours& out) {
    for (const auto& t : list())
        if (t.name == name) { out = t.colours; return true; }
    return false;
}

inline juce::var themeFileVar(const UserTheme& t) {
    auto* o = new juce::DynamicObject();
    o->setProperty("name", t.name);
    o->setProperty("colours", themeColoursVar(t.colours));
    return juce::var(o);
}

inline bool parseThemeFile(const juce::var& v, UserTheme& out, const ThemeColours& base,
                           const juce::String& fallbackName = {}) {
    if (v.hasProperty("humus") && v["humus"].toString() != "theme") return false;
    out.name = v["name"].toString().trim();
    if (out.name.isEmpty()) out.name = fallbackName.trim();
    if (out.name.isEmpty()) return false;
    const auto colours = themeColoursFromVar(v["colours"], base);
    if (!colours) return false;
    out.colours = *colours;
    return true;
}

inline void upgradeStoredThemes() {
    auto& s = AppSettings::instance();
    bool legacy = false;
    const auto stored = juce::JSON::parse(s.getString("userThemes"));
    if (const auto* arr = stored.getArray())
        for (const auto& e : *arr) legacy = legacy || e["colours"].isString();
    if (legacy) persist(list());
    const auto custom = s.getString("customColours").trim();
    if (custom.isNotEmpty() && !custom.startsWithChar('{'))
        if (const auto c = themeColoursFromLegacy(custom))
            s.set("customColours", themeColoursToString(*c));
}

}
}
