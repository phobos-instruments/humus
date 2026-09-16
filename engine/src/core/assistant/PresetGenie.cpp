// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/assistant/PresetGenie.h"

#include <algorithm>
#include <cmath>

namespace hum {

juce::String genieSystemPrompt(const std::string& className,
                               const std::vector<ParamDesc>& schema) {
    juce::String s;
    s << "You set parameters for a device in a modular music application.\n"
      << "Device class: " << juce::String(className) << "\n"
      << "Parameters (name | min..max | default";
    s << "):\n";
    for (const auto& d : schema) {
        if (d.isText) continue;
        s << "- " << juce::String(d.name) << " | " << d.min << ".." << d.max
          << " | " << d.def;
        if (d.isBool) s << " | boolean (0 or 1)";
        else if (d.isEnum) s << " | enum index (integer)";
        s << "\n";
    }
    s << "\nReply with ONLY a JSON object mapping parameter names to numeric "
         "values, e.g. {\"Frequency\": 440.0}. Set every parameter that "
         "matters for the requested sound; omit ones you would leave at their "
         "current value. No prose, no markdown fences.";
    return s;
}

std::vector<std::pair<std::string, double>> parseGenieReply(
        const juce::String& reply, const std::vector<ParamDesc>& schema) {
    std::vector<std::pair<std::string, double>> out;
    const int open = reply.indexOfChar('{');
    const int close = reply.lastIndexOfChar('}');
    if (open < 0 || close <= open) return out;
    const juce::var parsed = juce::JSON::parse(reply.substring(open, close + 1));
    auto* obj = parsed.getDynamicObject();
    if (!obj) return out;

    for (const auto& prop : obj->getProperties()) {
        if (!prop.value.isDouble() && !prop.value.isInt() && !prop.value.isInt64()
            && !prop.value.isBool())
            continue;
        const std::string name = prop.name.toString().toStdString();
        const ParamDesc* d = nullptr;
        for (const auto& sd : schema)
            if (sd.name == name) { d = &sd; break; }
        if (!d || d->isText) continue;
        double v = prop.value.isBool() ? (bool(prop.value) ? 1.0 : 0.0)
                                       : (double) prop.value;
        v = std::clamp(v, d->min, d->max);
        if (d->isBool || d->isEnum) v = std::round(v);
        out.emplace_back(name, v);
    }
    return out;
}

}
