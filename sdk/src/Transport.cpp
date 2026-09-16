// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/Transport.h"
#include <cstdlib>

namespace hum {

double Transport::rhythmicUnitToSamples(const std::string& unit) const {
    double num = 1.0, den = 16.0;
    auto slash = unit.find('/');
    if (slash != std::string::npos) {
        num = std::atof(unit.substr(0, slash).c_str());
        den = std::atof(unit.substr(slash + 1).c_str());
    }
    if (den <= 0.0) den = 16.0;
    double wholeNoteBeats = 4.0;
    double beats = wholeNoteBeats * (num / den);
    return beats * samplesPerBeat();
}

}
