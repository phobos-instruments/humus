// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/PackManifest.h"

#include <algorithm>

#include "core/json/Json.h"

namespace hum {

namespace {
std::string str(const json::Value& v, const char* key, const std::string& def = {}) {
    const auto& p = v[key];
    return p.isNull() ? def : p.text();
}
double num(const json::Value& v, const char* key, double def = 0.0) {
    const auto& p = v[key];
    return p.isNull() ? def : p.number();
}
bool flag(const json::Value& v, const char* key) { return v[key].truthy(); }
}

bool parsePackManifest(const std::string& jsonText, PackManifest& out) {
    const auto v = json::parse(jsonText);
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
    const auto v = json::parse(jsonText);
    if (!v.isObject()) return false;
    const auto& classes = v["classes"];
    if (!classes.isArray()) return false;
    const bool strips = flag(v, "strips");
    for (const auto& cv : classes.items()) {
        OrganismClassManifest c;
        c.className = str(cv, "class");
        if (c.className.empty()) continue;
        c.strips = strips;
        c.category = str(cv, "category", "Other");
        c.editor = str(cv, "editor");
        c.help = str(cv, "help");
        c.blurb = str(cv, "blurb");
        c.caution = str(cv, "caution");
        c.cautionIcon = str(cv, "caution-icon", "Warning");
        c.hidden = flag(cv, "hidden");
        c.canonical = str(cv, "canonical");
        c.roll = str(cv, "roll");
        c.fixedTailOutlets = std::max(0, (int) num(cv, "fixed-tail-outlets", 0.0));
        for (const auto& r : cv["roles"].items()) c.roles.push_back(r.text());
        for (const auto& pv : cv["params"].items()) {
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
            d.socket = flag(pv, "socket");
            d.namedInlet = flag(pv, "named-inlet");
            d.carry = flag(pv, "carry");
            if (const auto& random = pv["random"]; random.isString())
                d.randomText = random.text();
            else
                d.randomize = random.truthy();
            d.defRandom = flag(pv, "def-random");
            d.rollSpanned = pv["random-min"].isNumber() && pv["random-max"].isNumber();
            d.rollMin = num(pv, "random-min", d.min);
            d.rollMax = num(pv, "random-max", d.max);
            d.unit = str(pv, "unit");
            if (!d.name.empty()) c.params.push_back(std::move(d));
        }
        for (auto& pd : parsePresetDefs(cv["presets"]))
            if (!pd.values.empty() || !pd.texts.empty()) c.presets.push_back(std::move(pd));
        out.classes.push_back(std::move(c));
    }
    return !out.classes.empty();
}

}
