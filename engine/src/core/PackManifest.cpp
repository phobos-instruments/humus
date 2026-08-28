#include "core/PackManifest.h"

#include <juce_core/juce_core.h>

namespace hum {

namespace {
std::string str(const juce::var& v, const char* key, const std::string& def = {}) {
    auto p = v.getProperty(key, {});
    return p.isVoid() ? def : p.toString().toStdString();
}
double num(const juce::var& v, const char* key, double def = 0.0) {
    auto p = v.getProperty(key, {});
    return p.isVoid() ? def : (double) p;
}
bool flag(const juce::var& v, const char* key) { return (bool) v.getProperty(key, false); }
}

bool parsePackManifest(const std::string& jsonText, PackManifest& out) {
    auto v = juce::JSON::parse(juce::String(jsonText));
    if (!v.isObject()) return false;
    out.id = str(v, "id");
    out.name = str(v, "name", out.id);
    out.version = str(v, "version", "0.0.0");
    out.description = str(v, "description");
    out.origin = str(v, "origin", out.name);
    out.license = str(v, "license");
    return !out.id.empty();
}

bool parseOrganismManifest(const std::string& jsonText, OrganismManifest& out) {
    auto v = juce::JSON::parse(juce::String(jsonText));
    if (!v.isObject()) return false;
    auto* arr = v.getProperty("classes", {}).getArray();
    if (!arr) return false;
    const bool strips = flag(v, "strips");
    for (auto& cv : *arr) {
        OrganismClassManifest c;
        c.className = str(cv, "class");
        if (c.className.empty()) continue;
        c.strips = strips;
        c.category = str(cv, "category", "Other");
        c.editor = str(cv, "editor");
        c.help = str(cv, "help");
        c.hidden = flag(cv, "hidden");
        c.canonical = str(cv, "canonical");
        if (auto* ps = cv.getProperty("params", {}).getArray()) {
            for (auto& pv : *ps) {
                ParamDesc d;
                d.name = str(pv, "name");
                d.min = num(pv, "min", 0.0);
                d.max = num(pv, "max", 1.0);
                d.def = num(pv, "def", 0.0);
                d.isBool = flag(pv, "bool");
                d.isEnum = flag(pv, "enum");
                d.isInt = flag(pv, "int");
                d.isRange = flag(pv, "range");
                d.defMax = num(pv, "defMax", 0.0);
                d.text = str(pv, "text");
                d.isText = flag(pv, "isText");
                d.isPlainText = flag(pv, "plainText");
                d.isTrigger = flag(pv, "trigger");
                d.randomize = flag(pv, "random");
                d.defRandom = flag(pv, "def-random");
                d.unit = str(pv, "unit");
                if (!d.name.empty()) c.params.push_back(std::move(d));
            }
        }
        for (auto& pd : parsePresetDefs(cv.getProperty("presets", {})))
            if (!pd.values.empty() || !pd.texts.empty()) c.presets.push_back(std::move(pd));
        out.classes.push_back(std::move(c));
    }
    return !out.classes.empty();
}

}
