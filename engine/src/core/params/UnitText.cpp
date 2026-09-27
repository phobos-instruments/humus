// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/params/UnitText.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

#include "core/midi/MidiFormat.h"
#include "core/params/ValueText.h"
#include "hum/dsp/DspMath.h"

namespace hum {
namespace {

struct Named { Unit u; const char* name; };
const Named kNames[] = {
    {Unit::None, ""},          {Unit::Percent, "%"},      {Unit::Signed, "signed%"},
    {Unit::Pan, "pan"},        {Unit::Hertz, "Hz"},       {Unit::Millis, "ms"},
    {Unit::Seconds, "s"},      {Unit::Semitones, "st"},   {Unit::Cents, "cent"},
    {Unit::Bpm, "BPM"},        {Unit::Degrees, "deg"},    {Unit::Beats, "beats"},
    {Unit::MidiNote, "note"},  {Unit::RatioTo1, ":1"},    {Unit::PerSecond, "/s"},
    {Unit::PercentRaw, "pct"}, {Unit::Bits, "bit"},   {Unit::Kilohertz, "kHz"},
    {Unit::Decibels, "gain"},  {Unit::Db, "dB"},
    {Unit::Frames, "frames"},  {Unit::OutOfMax, "of-max"},
};

std::string plus(double v, double min) { return (min < 0.0 && v > 0.0) ? "+" : ""; }

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

std::string unitText(Unit u, double v, double min, double max) {
    switch (u) {
        case Unit::Percent:
            return decimalText(v * 100.0, std::abs(v) < 0.1 ? 1 : 0) + "%";
        case Unit::Signed: {
            const auto pct = v * 100.0;
            return std::string(pct > 0.0 ? "+" : "") + decimalText(pct, std::abs(pct) < 10.0 ? 1 : 0) + "%";
        }
        case Unit::PercentRaw:
            return plus(v, min) + decimalText(v, std::abs(v) < 10.0 ? 1 : 0) + "%";
        case Unit::Pan: {
            const auto mid = min < 0.0 ? 0.0 : (min + max) * 0.5;
            const auto off = v - mid;
            const auto span = off < 0.0 ? mid - min : max - mid;
            const auto pct = span > 0.0 ? std::abs(off) / span * 100.0 : 0.0;
            if (pct < 0.5) return "C";
            return std::string(off < 0.0 ? "L" : "R") + decimalText(pct, 0);
        }
        case Unit::Decibels:
            if (v <= 0.0) return "-\xe2\x88\x9e dB";  // utf8-ok
            else {
                const auto db = 20.0 * std::log10(v);
                return std::string(db > 0.0 ? "+" : "") + decimalText(db, 1) + " dB";
            }
        case Unit::Db:         return std::string(v > 0.0 ? "+" : "") + decimalText(v, 1) + " dB";
        case Unit::MidiNote:   return midiNoteName((int) std::lround(v));
        case Unit::RatioTo1:   return decimalText(v, 1) + ":1";
        case Unit::PerSecond:  return decimalText(v, std::abs(v) < 10.0 ? 1 : 0) + "/s";
        case Unit::Bits:       return decimalText(v, 0) + (std::llround(v) == 1 ? " bit" : " bits");
        case Unit::Kilohertz:  return decimalText(v, std::abs(v) < 10.0 ? 1 : 0) + " kHz";
        case Unit::Hertz:
            if (std::abs(v) >= 1000.0)
                return decimalText(v / 1000.0, std::abs(v) < 10000.0 ? 2 : 1) + " kHz";
            return decimalText(v, std::abs(v) < 100.0 ? 1 : 0) + " Hz";
        case Unit::Millis:
            if (std::abs(v) >= 1000.0)
                return decimalText(v / 1000.0, std::abs(v) < 10000.0 ? 2 : std::abs(v) < 100000.0 ? 1 : 0)
                       + " s";
            return decimalText(v, std::abs(v) < 10.0 ? 1 : 0) + " ms";
        case Unit::Seconds:
            return decimalText(v, std::abs(v) < 10.0 ? 2 : std::abs(v) < 100.0 ? 1 : 0) + " s";
        case Unit::Semitones:  return plus(v, min) + decimalText(v, 0) + " st";
        case Unit::Cents:      return plus(v, min) + decimalText(v, 0) + " ct";
        case Unit::Bpm:        return decimalText(v, std::abs(v) < 100.0 ? 1 : 0) + " BPM";
        case Unit::Degrees:    return decimalText(v, 0) + "\xc2\xb0";  // utf8-ok
        case Unit::Beats:      return decimalText(v, 2) + " beats";
        case Unit::Frames:     return decimalText(v, 0) + " fr";
        case Unit::OutOfMax:   return decimalText(v, 0) + " / " + decimalText(max, 0);
        case Unit::None:       break;
    }
    return {};
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
        case Unit::Kilohertz: return "kHz";
        case Unit::RatioTo1:  return ":1";
        case Unit::Beats:     return "beats";
        case Unit::Frames:    return "fr";
        case Unit::Pan: case Unit::MidiNote: case Unit::OutOfMax: case Unit::None: break;
    }
    return "";
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
        if (min >= 0.0 && max <= kMidiMaxD && max >= 24.0) return Unit::MidiNote;
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
