// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/params/Dice.h"
#include "core/params/ParamSchema.h"
#include "core/params/PatternRolls.h"
#include "hum/PatternMatrix.h"
#include "hum/dsp/WaveTable.h"

namespace hum {

class JuceDice : public Dice {
public:
    explicit JuceDice(juce::Random& random) : random_(random) {}
    double nextDouble() override { return random_.nextDouble(); }
    int nextInt(int below) override { return random_.nextInt(below); }

private:
    juce::Random& random_;
};

namespace roll {
inline constexpr std::string_view kFormula = "formula";
inline constexpr std::string_view kWaveTable = "wave-table";
inline constexpr std::string_view kBassline = "bassline";
inline constexpr std::string_view kTriggerSteps = "trigger-steps";
inline constexpr std::string_view kDrumRows = "drum-rows";

inline bool isTextRoll(std::string_view k) { return k == kFormula || k == kWaveTable; }
inline bool isPatternRoll(std::string_view k) { return k == kBassline || k == kTriggerSteps || k == kDrumRows; }
}

inline bool hasRandomParams(const std::vector<ParamDesc>& schema) {
    for (const auto& d : schema)
        if (d.randomize && !d.isText) return true;
    return false;
}

inline std::vector<std::pair<std::string, double>> randomParamValues(
    const std::vector<ParamDesc>& schema, juce::Random& r) {
    std::vector<std::pair<std::string, double>> out;
    for (const auto& d : schema) {
        if (!d.randomize || d.isText || d.isRange) continue;
        double v = d.rollLow() + r.nextDouble() * (d.rollHigh() - d.rollLow());
        if (d.isBool || d.isEnum || d.isInt) v = std::round(v);
        out.emplace_back(d.name, juce::jlimit(d.rollLow(), d.rollHigh(), v));
    }
    return out;
}

inline constexpr double kRollSpanWidest = 0.5;

struct RolledSpan {
    std::string name;
    double low = 0.0;
    double high = 0.0;
};

inline RolledSpan rollSpan(const ParamDesc& d, double lo, double hi, juce::Random& r) {
    RolledSpan span{d.name, lo, lo};
    const double room = hi - lo;
    if (room <= 0.0) return span;
    const double width = r.nextDouble() * kRollSpanWidest * room;
    span.low = lo + r.nextDouble() * (room - width);
    span.high = span.low + width;
    if (d.isBool || d.isEnum || d.isInt) {
        span.low = std::round(span.low);
        span.high = std::round(span.high);
    }
    span.low = juce::jlimit(lo, hi, span.low);
    span.high = juce::jlimit(span.low, hi, span.high);
    return span;
}

inline std::vector<RolledSpan> randomSpanValues(const std::vector<ParamDesc>& schema,
                                                juce::Random& r) {
    std::vector<RolledSpan> out;
    for (const auto& d : schema) {
        if (!d.randomize || d.isText || !d.isRange) continue;
        out.push_back(rollSpan(d, d.rollLow(), d.rollHigh(), r));
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

inline std::string rolledText(std::string_view kind, juce::Random& r) {
    if (kind == roll::kFormula) return randomFormula(r);
    if (kind == roll::kWaveTable) {
        float t[kWaveTableLen];
        waveTableRandom((uint32_t) r.nextInt(), t, kWaveTableLen);
        return encodeWaveTable(t, kWaveTableLen);
    }
    return {};
}

}
