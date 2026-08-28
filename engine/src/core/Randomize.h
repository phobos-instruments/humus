#pragma once
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/ParamSchema.h"
#include "hum/PatternMatrix.h"

namespace hum {

inline bool hasRandomParams(const std::vector<ParamDesc>& schema) {
    for (const auto& d : schema)
        if (d.randomize && !d.isText && !d.isRange) return true;
    return false;
}

inline std::vector<std::pair<std::string, double>> randomParamValues(
    const std::vector<ParamDesc>& schema, juce::Random& r) {
    std::vector<std::pair<std::string, double>> out;
    for (const auto& d : schema) {
        if (!d.randomize || d.isText || d.isRange) continue;
        double v = d.min + r.nextDouble() * (d.max - d.min);
        if (d.isBool || d.isEnum || d.isInt) v = std::round(v);
        out.emplace_back(d.name, juce::jlimit(d.min, d.max, v));
    }
    return out;
}

inline double rollDefaultValue(const ParamDesc& d, juce::Random& r) {
    if (!d.defRandom || d.isText || d.isRange) return d.def;
    if (d.isBool || d.isEnum || d.isInt)
        return d.min + (double) r.nextInt((int) std::round(d.max - d.min) + 1);
    return d.min + r.nextDouble() * (d.max - d.min);
}

inline std::string randomFormula(juce::Random& r) {
    auto pick = [&r](std::initializer_list<const char*> c) {
        return juce::String(c.begin()[r.nextInt((int) c.size())]);
    };
    const juce::String k1 = pick({"x", "y", "z", "w"});
    const juce::String k2 = pick({"x", "y", "z", "w"});
    const juce::String rate = pick({"0.25", "0.5", "1", "2", "4"});
    const juce::String rate2 = pick({"0.5", "1", "3", "8"});
    auto shape = [&] {
        const juce::String p = "beat*" + rate;
        switch (r.nextInt(4)) {
            case 0:  return "sin(tau*" + p + ")";
            case 1:  return "tri(" + p + ")";
            case 2:  return "saw(" + p + ")";
            default: return "sqr(" + p + ")";
        }
    };
    juce::String e;
    switch (r.nextInt(6)) {
        case 0:
            e = shape() + "*" + k1;
            break;
        case 1:
            e = r.nextBool() ? "a*(1 - " + k1 + "*(0.5 + 0.5*" + shape() + "))"
                             : "a*(pulse(beat*" + rate + ", 0.2 + " + k1 + "*0.6) > 0)";
            break;
        case 2:
            e = r.nextBool() ? "tanh(a*(1 + " + k1 + "*15))*(1 - " + k2 + "*0.5)"
                             : "floor(a*(2 + " + k1 + "*30))/(2 + " + k1 + "*30)";
            break;
        case 3:
            e = pick({"sin(tau*", "0.4*sqr(", "0.4*saw("})
                + "ph(55 + " + k1 + "*440))*(0.2 + 0.8*" + k2 + ")";
            break;
        case 4:
            e = r.nextBool()
                    ? "mix(prev, noise(), 0.002 + " + k1 + "*0.05)"
                    : "mix(prev, noise(), step(0.97, fract(beat*(1 + floor(" + k1 + "*7)))))";
            break;
        default:
            e = "sin(tau*beat*" + rate + " + sin(tau*beat*" + rate2 + ")*(1 + "
                + k1 + "*8))*" + k2;
            break;
    }
    if (r.nextInt(3) == 0) e = "(" + e + ")*(0.5 + 0.5*sin(tau*beat*0.25))";
    return e.toStdString();
}

inline std::vector<BasslineStep> randomBassline(int steps, int rootNote, juce::Random& r) {
    static const int kScale[] = {0, 0, 3, 5, 7, 10, 12};
    std::vector<BasslineStep> out((size_t) juce::jmax(1, steps));
    for (size_t i = 0; i < out.size(); ++i) {
        auto& s = out[i];
        s.gate = i == 0 || r.nextDouble() < 0.65;
        int n = rootNote + kScale[r.nextInt(juce::numElementsInArray(kScale))];
        if (r.nextDouble() < 0.12) n -= 12;
        while (n < 36) n += 12;
        while (n > 84) n -= 12;
        s.note = n;
        s.accent = s.gate && r.nextDouble() < 0.25;
        s.slide = s.gate && r.nextDouble() < 0.20;
    }
    return out;
}

inline std::vector<bool> randomTriggerRow(int steps, double density, juce::Random& r) {
    std::vector<bool> out((size_t) juce::jmax(0, steps), false);
    if (out.empty() || density <= 0.0) return out;
    bool any = false;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = r.nextDouble() < density;
        any = any || out[i];
    }
    if (!any) out[(size_t) r.nextInt((int) out.size())] = true;
    return out;
}

}
