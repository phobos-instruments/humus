#include "core/ParamUnit.h"

#include "core/MidiFormat.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

namespace hum {
namespace {

struct Named { Unit u; const char* name; };
const Named kNames[] = {
    {Unit::None, ""},          {Unit::Percent, "%"},      {Unit::Signed, "signed%"},
    {Unit::Pan, "pan"},        {Unit::Hertz, "Hz"},       {Unit::Millis, "ms"},
    {Unit::Seconds, "s"},      {Unit::Semitones, "st"},   {Unit::Cents, "cent"},
    {Unit::Bpm, "BPM"},        {Unit::Degrees, "deg"},    {Unit::Beats, "beats"},
    {Unit::MidiNote, "note"},  {Unit::RatioTo1, ":1"},    {Unit::PerSecond, "/s"},
    {Unit::PercentRaw, "pct"}, {Unit::Bits, "bit"},
    {Unit::Decibels, "gain"},  {Unit::Db, "dB"},
};

juce::String trim(double v, int decimals) {
    if (decimals > 0) return juce::String(v, decimals);
    return juce::String((juce::int64) std::llround(v));
}

const char* plus(double v, double min) { return (min < 0.0 && v > 0.0) ? "+" : ""; }

std::vector<std::string> words(const std::string& n) {
    std::vector<std::string> out;
    std::string cur;
    auto flush = [&] { if (!cur.empty()) { out.push_back(cur); cur.clear(); } };
    for (size_t i = 0; i < n.size(); ++i) {
        const unsigned char c = (unsigned char) n[i];
        if (c == '_' || c == '-' || c == ' ' || std::isdigit(c)) { flush(); continue; }
        const bool starts = std::isupper(c) && i > 0
                            && (std::islower((unsigned char) n[i - 1])
                                || (i + 1 < n.size() && std::islower((unsigned char) n[i + 1])));
        if (starts) flush();
        cur += (char) std::tolower(c);
    }
    flush();
    return out;
}

bool isGridRow(const std::string& n) {
    const auto us = n.find('_');
    if (us == std::string::npos || us == 0) return false;
    return std::isdigit((unsigned char) n[us - 1]) != 0;
}

bool isOneOf(const std::string& s, std::initializer_list<const char*> names) {
    for (const char* n : names) if (s == n) return true;
    return false;
}

}

Unit unitFromName(const std::string& name) {
    for (const auto& n : kNames) if (name == n.name) return n.u;
    return Unit::None;
}

const char* unitName(Unit u) {
    for (const auto& n : kNames) if (n.u == u) return n.name;
    return "";
}

juce::String unitFormat(Unit u, double v, double min, double max) {
    switch (u) {
        case Unit::Percent:
            return trim(v * 100.0, std::abs(v) < 0.1 ? 1 : 0) + "%";
        case Unit::Signed: {
            const auto pct = v * 100.0;
            return (pct > 0.0 ? "+" : "") + trim(pct, std::abs(pct) < 10.0 ? 1 : 0) + "%";
        }
        case Unit::PercentRaw:
            return plus(v, min) + trim(v, std::abs(v) < 10.0 ? 1 : 0) + "%";
        case Unit::Pan: {
            const auto mid = min < 0.0 ? 0.0 : (min + max) * 0.5;
            const auto off = v - mid;
            const auto span = off < 0.0 ? mid - min : max - mid;
            const auto pct = span > 0.0 ? std::abs(off) / span * 100.0 : 0.0;
            if (pct < 0.5) return "C";
            return (off < 0.0 ? "L" : "R") + trim(pct, 0);
        }
        case Unit::Decibels:
            if (v <= 0.0) return juce::String::fromUTF8("-\xe2\x88\x9e dB");
            else {
                const auto db = 20.0 * std::log10(v);
                return (db > 0.0 ? "+" : "") + trim(db, 1) + " dB";
            }
        case Unit::Db:         return (v > 0.0 ? "+" : "") + trim(v, 1) + " dB";
        case Unit::MidiNote:   return juce::String(midiNoteName((int) std::lround(v)));
        case Unit::RatioTo1:   return trim(v, 1) + ":1";
        case Unit::PerSecond:  return trim(v, std::abs(v) < 10.0 ? 1 : 0) + "/s";
        case Unit::Bits:       return trim(v, 0) + (std::llround(v) == 1 ? " bit" : " bits");
        case Unit::Hertz:
            if (std::abs(v) >= 1000.0)
                return trim(v / 1000.0, std::abs(v) < 10000.0 ? 2 : 1) + " kHz";
            return trim(v, std::abs(v) < 100.0 ? 1 : 0) + " Hz";
        case Unit::Millis:
            if (std::abs(v) >= 1000.0)
                return trim(v / 1000.0, std::abs(v) < 10000.0 ? 2 : std::abs(v) < 100000.0 ? 1 : 0)
                       + " s";
            return trim(v, std::abs(v) < 10.0 ? 1 : 0) + " ms";
        case Unit::Seconds:
            return trim(v, std::abs(v) < 10.0 ? 2 : std::abs(v) < 100.0 ? 1 : 0) + " s";
        case Unit::Semitones:  return plus(v, min) + trim(v, 0) + " st";
        case Unit::Cents:      return plus(v, min) + trim(v, 0) + " ct";
        case Unit::Bpm:        return trim(v, std::abs(v) < 100.0 ? 1 : 0) + " BPM";
        case Unit::Degrees:    return trim(v, 0) + juce::String::fromUTF8("\xc2\xb0");
        case Unit::Beats:      return trim(v, 2) + " beats";
        case Unit::None:       break;
    }
    return {};
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

const char* unitSuffix(Unit u) {
    switch (u) {
        case Unit::Percent: case Unit::Signed: case Unit::PercentRaw: return "%";
        case Unit::Decibels: case Unit::Db: return "dB";
        case Unit::Hertz:     return "Hz";
        case Unit::Millis:    return "ms";
        case Unit::Seconds:   return "s";
        case Unit::Semitones: return "st";
        case Unit::Cents:     return "ct";
        case Unit::Bpm:       return "BPM";
        case Unit::Degrees:   return "deg";
        case Unit::PerSecond: return "/s";
        case Unit::Bits:      return "bits";
        case Unit::RatioTo1:  return ":1";
        case Unit::Beats:     return "beats";
        case Unit::Pan: case Unit::MidiNote: case Unit::None: break;
    }
    return "";
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
        case Unit::Beats:
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

Unit unitInferred(const std::string& paramName, double min, double max) {
    if (isGridRow(paramName)) return Unit::None;

    const auto w = words(paramName);
    if (w.empty()) return Unit::None;
    std::string joined;
    for (const auto& part : w) joined += part;
    auto is = [&](std::initializer_list<const char*> names) {
        return isOneOf(joined, names) || isOneOf(w.back(), names)
            || isOneOf(w.front(), names);
    };

    const bool bipolar = min < 0.0 && max > 0.0;
    const bool fraction = min >= -1.0 && max <= 1.0;

    if (is({"attack", "decay", "release", "hold", "time", "duration", "delay",
            "predelay", "lookahead", "glide", "portamento", "rise", "fall", "smooth"}))
        if (max > 1.0) return Unit::Millis;

    if (is({"frequency", "freq", "hz", "cutoff", "lowcut", "highcut", "hipass",
            "lowpass", "bandwidth", "centre", "center", "range"})) {
        if (min >= 20.0 || max >= 1000.0) return Unit::Hertz;
        if (max >= 100.0 && is({"frequency", "freq", "hz"})) return Unit::Hertz;
    }

    if (is({"rate", "speed", "lfospeed"}))
        if (min > 0.0 && min < 1.0 && max >= 5.0 && max <= 500.0) return Unit::Hertz;

    if (is({"density"}) && min >= 0.0 && max > 1.0) return Unit::PerSecond;

    if (is({"gain", "mastergain", "inputgain", "outputgain", "makeupgain", "level",
            "volume", "amplitude", "amp", "trim", "output", "input", "harm",
            "threshold", "knee", "kneewidth", "makeup", "ceiling", "floor"})) {
        if (min < 0.0) return Unit::Db;
        if (max >= 1.0 && is({"gain", "mastergain", "inputgain", "outputgain",
                              "makeupgain", "level", "volume", "amplitude", "amp",
                              "trim", "output", "input", "harm"}))
            return Unit::Decibels;
    }

    if (is({"pan", "panning", "balance", "spread", "position", "azimuth"})) {
        if (bipolar && fraction) return Unit::Pan;
        if (min == 0.0 && max == 1.0 && is({"pan", "panning", "position"}))
            return Unit::Pan;
    }

    if (is({"tempo", "bpm"})) return Unit::Bpm;
    if (is({"percent", "pitchpercent"})) return Unit::PercentRaw;
    if (is({"bitdepth", "bits"}) && max > 1.0) return Unit::Bits;
    if (is({"note", "root", "lownote", "highnote", "basenote"}))
        if (min >= 0.0 && max <= 127.0 && max >= 24.0) return Unit::MidiNote;
    if (is({"ratio", "compressionratio"}) && min >= 1.0 && max >= 4.0)
        return Unit::RatioTo1;
    if (is({"tune"}) && min >= 20.0) return Unit::Hertz;
    if (is({"transpose", "semitones", "interval", "tune", "shift", "pitch"}))
        if (max >= 7.0 && max <= 48.0) return Unit::Semitones;
    if (is({"fine", "cents", "detune"})) if (std::abs(max) >= 50.0) return Unit::Cents;
    if (is({"phase", "angle", "rotation"})) if (max >= 30.0) return Unit::Degrees;

    if (fraction && max > min
        && !is({"ratio", "attdecratio", "q", "filterq", "unit", "syncunit", "seed",
                "index", "mode", "scaler", "pitchscaler", "multiplier", "slot",
                "channel", "port", "curve", "x", "y", "z", "w"}))
        return bipolar ? Unit::Signed : Unit::Percent;
    return Unit::None;
}

Unit unitResolve(const std::string& paramName, const std::string& declared,
                 double min, double max) {
    if (declared == "none") return Unit::None;
    if (!declared.empty()) return unitFromName(declared);
    return unitInferred(paramName, min, max);
}

}
