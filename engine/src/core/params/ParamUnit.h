// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_core/juce_core.h>

namespace hum {

enum class Unit {
    None,
    Percent,
    Signed,
    Pan,
    Decibels,
    Db,
    Hertz,
    Millis,
    Seconds,
    Semitones,
    Cents,
    Bpm,
    Degrees,
    Beats,
    MidiNote,
    RatioTo1,
    PerSecond,
    PercentRaw,
    Bits,
};

Unit unitFromName(const std::string& name);
const char* unitName(Unit u);

juce::String unitFormat(Unit u, double value, double min, double max);
double unitParse(Unit u, const juce::String& text, double min, double max);

const char* unitSuffix(Unit u);
juce::String unitPlain(Unit u, double value, double min, double max);
double unitPlainParse(Unit u, const juce::String& text, double min, double max);

Unit unitInferred(const std::string& paramName, double min, double max);

Unit unitResolve(const std::string& paramName, const std::string& declared,
                 double min, double max);

bool isRelativeEntry(const juce::String& text);
double applyRelativeEntry(const juce::String& text, double current);

}
