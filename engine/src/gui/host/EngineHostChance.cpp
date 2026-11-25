// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHostPattern.h"

#include <string>
#include <vector>

#include "hum/PatternMatrix.h"
#include "hum/StepChance.h"

namespace hum {

std::vector<int> PatternHost::stepChances(const std::string& name, int ordinal) const {
    if (auto* cm = doc_.document().byName(name))
        if (auto* ch = matrixChannel(cm->pattern, kChanceRowChannel, ordinal)) return decodeChanceRow(ch->matrix);
    return {};
}

void PatternHost::setStepChances(const std::string& name, int ordinal, const std::vector<int>& chances) {
    if (ordinal < 0) return;
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm) return;
    while (matrixChannelCount(cm->pattern, kChanceRowChannel) <= ordinal) {
        PatternChannel ch;
        ch.type = kChanceRowChannel;
        cm->pattern.channels.push_back(ch);
    }
    auto* ch = const_cast<PatternChannel*>(matrixChannel(cm->pattern, kChanceRowChannel, ordinal));
    ch->matrix = allSure(chances) ? std::string() : encodeChanceRow(chances);
    ensurePatternProperty(*cm);
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

}
