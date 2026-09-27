// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/params/ParamUnit.h"

#include <cmath>

namespace hum {

juce::String unitFormat(Unit u, double v, double min, double max) {
    return juce::String::fromUTF8(unitText(u, v, min, max).c_str());
}

double unitParse(Unit u, const juce::String& text, double min, double max) {
    auto t = text.trim();
    const bool left = t.startsWithIgnoreCase("L");
    const bool right = t.startsWithIgnoreCase("R");
    const bool kilo = t.containsIgnoreCase("k");
    const bool secs = u == Unit::Millis && t.containsIgnoreCase("s")
                      && !t.containsIgnoreCase("ms");
    if (u == Unit::Pan && t.equalsIgnoreCase("C")) return 0.0;
    const double n = t.retainCharacters("0123456789.,-+").replace(",", ".").getDoubleValue();

    switch (u) {
        case Unit::Percent:
        case Unit::Signed:    return n / 100.0;
        case Unit::Pan: {
            const auto mid = min < 0.0 ? 0.0 : (min + max) * 0.5;
            if (!left && !right) return mid + n / 100.0 * (max - mid);
            const auto span = left ? mid - min : max - mid;
            return mid + (left ? -1.0 : 1.0) * std::abs(n) / 100.0 * span;
        }
        case Unit::Decibels:  return n <= -120.0 ? 0.0 : std::pow(10.0, n / 20.0);
        case Unit::Hertz:     return kilo ? n * 1000.0 : n;
        case Unit::Millis:    return secs ? n * 1000.0 : n;
        default:              return n;
    }
}

bool isRelativeEntry(const juce::String& text) {
    const auto t = text.trim();
    if (t.isEmpty()) return false;
    const auto lead = t[0];
    return lead == '*' || lead == '/' || lead == 'x' || lead == 'X';
}

double applyRelativeEntry(const juce::String& text, double current) {
    const auto t = text.trim();
    if (!isRelativeEntry(t)) return current;
    const double by = t.substring(1).retainCharacters("0123456789.,-+")
                       .replace(",", ".").getDoubleValue();
    if (by == 0.0) return current;
    return t[0] == '/' ? current / by : current * by;
}

namespace {
juce::String plain(double v) {
    juce::String s(v, 3);
    if (s.contains(".")) s = s.trimCharactersAtEnd("0").trimCharactersAtEnd(".");
    return s;
}
}

juce::String unitPlain(Unit u, double v, double min, double max) {
    switch (u) {
        case Unit::Percent: case Unit::Signed: return plain(v * 100.0);
        case Unit::Decibels:
            return v <= 0.0 ? juce::String::fromUTF8("-\xe2\x88\x9e")
                            : plain(20.0 * std::log10(v));
        case Unit::Hertz: case Unit::Millis: case Unit::Db: case Unit::PercentRaw:
        case Unit::Seconds: case Unit::Semitones: case Unit::Cents: case Unit::Bpm:
        case Unit::Degrees: case Unit::PerSecond: case Unit::Bits: case Unit::RatioTo1:
        case Unit::Beats: case Unit::Kilohertz: case Unit::Frames: case Unit::OutOfMax:
            return plain(v);
        case Unit::Pan: case Unit::MidiNote: return unitFormat(u, v, min, max);
        case Unit::None: break;
    }
    return plain(v);
}

double unitPlainParse(Unit u, const juce::String& text, double min, double max) {
    switch (u) {
        case Unit::Pan: case Unit::MidiNote: return unitParse(u, text, min, max);
        case Unit::Percent: case Unit::Signed:
            return text.retainCharacters("0123456789.,-").replace(",", ".").getDoubleValue()
                   / 100.0;
        case Unit::Decibels: {
            if (text.containsChar(juce::String::fromUTF8("\xe2\x88\x9e")[0])) return 0.0;
            const double db = text.retainCharacters("0123456789.,-").replace(",", ".")
                                  .getDoubleValue();
            return db <= -120.0 ? 0.0 : std::pow(10.0, db / 20.0);
        }
        default:
            return text.retainCharacters("0123456789.,-").replace(",", ".").getDoubleValue();
    }
}

}
