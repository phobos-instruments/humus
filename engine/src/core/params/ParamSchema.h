// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace hum {

struct ParamDesc {
    std::string name;
    double min = 0.0;
    double max = 1.0;
    double def = 0.0;
    bool isBool = false;
    bool isEnum = false;
    bool isInt = false;
    bool isRange = false;
    double defMax = 0.0;
    std::string text;
    bool isText = false;
    bool isPlainText = false;
    bool isTrigger = false;
    bool socket = false;
    bool namedInlet = false;
    bool carry = false;
    bool randomize = false;
    std::string randomText;
    bool defRandom = false;
    bool rollSpanned = false;
    double rollMin = 0.0;
    double rollMax = 0.0;

    double rollLow() const { return rollSpanned ? std::clamp(rollMin, min, max) : min; }
    double rollHigh() const { return rollSpanned ? std::clamp(rollMax, rollLow(), max) : max; }
    std::string unit;
};

const std::vector<ParamDesc>& schemaFor(const std::string& className);

}
