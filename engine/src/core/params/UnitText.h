// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

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
    Kilohertz,
    Frames,
    OutOfMax,
};

Unit unitFromName(const std::string& name);
const char* unitName(Unit u);
const char* unitSuffix(Unit u);

std::string unitText(Unit u, double value, double min, double max);

Unit unitInferred(const std::string& paramName, double min, double max);

Unit unitResolve(const std::string& paramName, const std::string& declared,
                 double min, double max);

}
