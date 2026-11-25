// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHostPattern.h"

#include <string>
#include <vector>

#include "hum/PatternMatrix.h"

namespace hum {

namespace {
constexpr int kFreshBankTicks = 4 * 4 * Pattern::kTicksPerBeat;

PatternChannel* bankChannelMut(Pattern& p, int bank, int lengthTicks) {
    if (bank < 1) return nullptr;
    while (matrixChannelCount(p, kNoteBankChannel) < bank) {
        PatternChannel ch;
        ch.type = kNoteBankChannel;
        ch.startTick = 0;
        ch.lengthTicks = lengthTicks > 0 ? lengthTicks : kFreshBankTicks;
        ch.loopClip = true;
        p.channels.push_back(ch);
    }
    return const_cast<PatternChannel*>(matrixChannel(p, kNoteBankChannel, bank - 1));
}
}

std::vector<NoteEvent> PatternHost::bankNotes(const std::string& name, int bank) const {
    if (auto* cm = doc_.document().byName(name))
        if (auto* ch = noteBankChannel(cm->pattern, bank)) return decodeNoteEvents(ch->matrix);
    return {};
}

std::vector<CCEvent> PatternHost::bankCCs(const std::string& name, int bank) const {
    if (auto* cm = doc_.document().byName(name))
        if (auto* ch = noteBankChannel(cm->pattern, bank)) return decodeCCEvents(ch->matrix);
    return {};
}

int PatternHost::bankLength(const std::string& name, int bank) const {
    if (auto* cm = doc_.document().byName(name))
        if (auto* ch = noteBankChannel(cm->pattern, bank)) return ch->lengthTicks;
    return 0;
}

void PatternHost::setBankNotes(const std::string& name, int bank, const std::vector<NoteEvent>& notes,
                               int lengthTicks) {
    patternSync_.recordPatternRevert(name);
    ensureNote(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm) return;
    auto* ch = bankChannelMut(cm->pattern, bank, lengthTicks);
    if (!ch) return;
    ch->matrix = replaceNoteEvents(ch->matrix, notes);
    if (lengthTicks > 0) ch->lengthTicks = lengthTicks;
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

void PatternHost::setBankCCs(const std::string& name, int bank, const std::vector<CCEvent>& ccs) {
    patternSync_.recordPatternRevert(name);
    ensureNote(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm) return;
    auto* ch = bankChannelMut(cm->pattern, bank, 0);
    if (!ch) return;
    ch->matrix = replaceCCEvents(ch->matrix, ccs);
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

}
