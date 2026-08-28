#pragma once
#include <algorithm>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/AppSettings.h"
#include "gui/LookAndFeel.h"

namespace hum {

inline juce::Colour ThemeColours::* const kThemeRolePtrs[9] = {
    &ThemeColours::background, &ThemeColours::panel, &ThemeColours::panelLight,
    &ThemeColours::border, &ThemeColours::accent, &ThemeColours::accentDim,
    &ThemeColours::text, &ThemeColours::textDim, &ThemeColours::cord};

inline juce::String themeColoursToString(const ThemeColours& c) {
    juce::StringArray a;
    for (int i = 0; i < 9; ++i) a.add((c.*kThemeRolePtrs[i]).toString());
    return a.joinIntoString(",");
}

inline ThemeColours themeColoursFromString(const juce::String& s, const ThemeColours& fallback) {
    auto a = juce::StringArray::fromTokens(s, ",", "");
    if (a.size() < 9) return fallback;
    ThemeColours c;
    for (int i = 0; i < 9; ++i) c.*kThemeRolePtrs[i] = juce::Colour::fromString(a[i]);
    return c;
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
            t.colours = themeColoursFromString(e["colours"].toString(), presetColours(0));
            out.push_back(std::move(t));
        }
    return out;
}

inline void persist(const std::vector<UserTheme>& themes) {
    juce::Array<juce::var> arr;
    for (const auto& t : themes) {
        auto* o = new juce::DynamicObject();
        o->setProperty("name", t.name);
        o->setProperty("colours", themeColoursToString(t.colours));
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
    o->setProperty("humus", "theme");
    o->setProperty("name", t.name);
    o->setProperty("colours", themeColoursToString(t.colours));
    return juce::var(o);
}

inline bool parseThemeFile(const juce::var& v, UserTheme& out) {
    if (v["humus"].toString() != "theme") return false;
    out.name = v["name"].toString().trim();
    if (out.name.isEmpty()) return false;
    const auto s = v["colours"].toString();
    if (juce::StringArray::fromTokens(s, ",", "").size() < 9) return false;
    out.colours = themeColoursFromString(s, {});
    return true;
}

}
}
