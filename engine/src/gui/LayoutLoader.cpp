#include "gui/LayoutLoader.h"

#include <set>

#include <juce_core/juce_core.h>

namespace hum {

namespace {

LayoutSpec::ControlType typeFromString(const juce::String& s) {
    LayoutSpec::ControlType t = LayoutSpec::ControlType::Knob;
    controlTypeFromName(s.toStdString(), t);
    return t;
}

bool typeKnown(const juce::String& s) {
    LayoutSpec::ControlType t;
    return controlTypeFromName(s.toStdString(), t);
}

std::string getString(const juce::var& obj, const char* key, const std::string& fallback = {}) {
    if (!obj.isObject()) return fallback;
    auto* d = obj.getDynamicObject();
    if (!d) return fallback;
    auto v = d->getProperty(key);
    return v.isString() ? v.toString().toStdString() : fallback;
}

double getDouble(const juce::var& obj, const char* key, double fallback = 0.0) {
    if (!obj.isObject()) return fallback;
    auto* d = obj.getDynamicObject();
    if (!d) return fallback;
    auto v = d->getProperty(key);
    return v.isDouble() ? (double) v : fallback;
}

int getInt(const juce::var& obj, const char* key, int fallback = 0) {
    if (!obj.isObject()) return fallback;
    auto* d = obj.getDynamicObject();
    if (!d) return fallback;
    auto v = d->getProperty(key);
    return v.isInt() || v.isDouble() ? (int) v : fallback;
}

bool getBool(const juce::var& obj, const char* key, bool fallback = false) {
    if (!obj.isObject()) return fallback;
    auto* d = obj.getDynamicObject();
    if (!d) return fallback;
    const auto v = d->getProperty(key);
    return v.isVoid() ? fallback : (bool) v;
}

}

LayoutSpec loadLayoutSpec(const std::string& jsonText) {
    LayoutSpec spec;
    juce::var parsed = juce::JSON::parse(juce::String(jsonText));
    if (!parsed.isObject()) return spec;

    auto* root = parsed.getDynamicObject();
    if (!root) return spec;

    spec.width = getInt(parsed, "width", 280);
    spec.height = getInt(parsed, "height", 220);
    if (getString(parsed, "resize") == "stretch") spec.resize = LayoutSpec::Resize::Stretch;
    if (getString(parsed, "resize") == "grow") spec.resize = LayoutSpec::Resize::Grow;

    auto controlsVar = root->getProperty("controls");
    if (auto* arr = controlsVar.getArray()) {
        for (auto& cv : *arr) {
            LayoutSpec::Control c;
            c.type = typeFromString(juce::String(getString(cv, "type", "knob")));
            c.param = getString(cv, "param");
            c.param2 = getString(cv, "param2");
            c.label = getString(cv, "label", c.param);
            c.x = getInt(cv, "x", 0);
            c.y = getInt(cv, "y", 0);
            c.w = getInt(cv, "w", 60);
            c.h = getInt(cv, "h", 80);
            c.decimalPlaces = getInt(cv, "decimals", 2);
            c.logarithmic = getBool(cv, "log", getBool(cv, "logarithmic", false));
            if (auto* co = cv.getDynamicObject()) {
                if (auto* opts = co->getProperty("options").getArray())
                    for (auto& o : *opts) c.options.push_back(o.toString().toStdString());
                static const std::set<juce::String> known = {
                    "type", "param", "param2", "label", "x", "y", "w", "h",
                    "decimals", "log", "logarithmic", "options"};
                for (auto& prop : co->getProperties())
                    if (known.count(prop.name.toString()) == 0 && !prop.value.isArray())
                        c.extra[prop.name.toString().toStdString()] =
                            prop.value.toString().toStdString();
            }
            spec.controls.push_back(std::move(c));
        }
    }
    return spec;
}

LayoutSpec loadLayoutSpecFromFile(const std::string& path) {
    juce::File f { juce::String(path) };
    if (!f.existsAsFile()) return {};
    return loadLayoutSpec(f.loadFileAsString().toStdString());
}

bool knownLayoutControlType(const std::string& type) {
    return typeKnown(juce::String(type));
}

}
