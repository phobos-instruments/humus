// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <cstdio>
#include <cstdlib>
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <cmath>

#include "core/graph/AudioGraph.h"

namespace hum {

void PatternHost::ensure(const std::string& name, int laneCount) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || cm->pattern.present) return;
    Pattern& p = cm->pattern;
    p.present = true;
    p.duration = 4 * Pattern::kTicksPerBeat;
    p.matrixResolution = "1/16";
    PatternChannel ts; ts.type = "time-signatures"; ts.timeSignatures = {{0, 4, 4}};
    p.channels.push_back(ts);
    for (int i = 0; i < laneCount; ++i) p.channels.push_back(PatternChannel{});
    bool hasProp = false;
    for (auto& pr : cm->properties) if (pr.name == "Pattern") { hasProp = true; break; }
    if (!hasProp) {
        Parameter pr; pr.name = "Pattern"; pr.type = "pattern"; pr.userEdited = true;
        cm->properties.push_back(pr);
    }
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

namespace {
PatternChannel* triggerChannel(Pattern& p, int channel) {
    int idx = 0;
    for (auto& ch : p.channels) {
        if (ch.type == "time-signatures") continue;
        if (idx == channel) return &ch;
        ++idx;
    }
    return nullptr;
}

int rowCountOf(const std::string& classRaw) {
    int rows = 0;
    for (const auto& d : schemaFor(classRaw))
        if (d.name.rfind("Note_", 0) == 0) ++rows;
    return rows;
}
}

void PatternHost::ensureBanks(const std::string& name, int laneCount, int banks) {
    ensure(name, laneCount);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return;
    const int want = laneCount * banks;
    if ((int) cm->pattern.triggerChannels().size() >= want) return;
    patternSync_.recordPatternRevert(name);
    while ((int) cm->pattern.triggerChannels().size() < want)
        cm->pattern.channels.push_back(PatternChannel{});
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

int PatternHost::laneOffset(const std::string& name) const {
    const int b = bank(name);
    if (b == 0) return 0;
    const auto* cm = doc_.document().byName(name);
    return cm ? b * rowCountOf(cm->classRaw) : 0;
}

std::vector<const PatternChannel*> PatternHost::triggerLanes(const std::string& name) const {
    const auto* cm = doc_.document().byName(name);
    if (!cm) return {};
    auto all = cm->pattern.triggerChannels();
    const int rows = rowCountOf(cm->classRaw);
    const int off = laneOffset(name);
    if (rows <= 0) return all;
    std::vector<const PatternChannel*> out;
    for (int r = 0; r < rows && off + r < (int) all.size(); ++r) out.push_back(all[(size_t) (off + r)]);
    return out;
}

void PatternHost::setLaneTriggers(const std::string& name, int bank, int lane,
                                  const std::vector<int>& triggers) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present || lane < 0) return;
    auto* ch = triggerChannel(cm->pattern, bank * rowCountOf(cm->classRaw) + lane);
    if (!ch) return;
    ch->triggers = triggers;
    std::sort(ch->triggers.begin(), ch->triggers.end());
    ch->triggers.erase(std::unique(ch->triggers.begin(), ch->triggers.end()), ch->triggers.end());
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

void PatternHost::addTrigger(const std::string& name, int channel, int tick) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present || tick < 0) return;
    auto* ch = triggerChannel(cm->pattern, laneOffset(name) + channel);
    if (!ch) return;
    auto& t = ch->triggers;
    if (std::find(t.begin(), t.end(), tick) != t.end()) return;
    t.push_back(tick);
    std::sort(t.begin(), t.end());
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

void PatternHost::removeTrigger(const std::string& name, int channel, int tick, int tol) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return;
    auto* ch = triggerChannel(cm->pattern, laneOffset(name) + channel);
    if (!ch) return;
    auto& t = ch->triggers;
    auto it = std::find_if(t.begin(), t.end(),
                           [&](int x) { return std::abs(x - tick) <= tol; });
    if (it == t.end()) return;
    t.erase(it);
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

int PatternHost::moveTrigger(const std::string& name, int channel, int oldTick, int newTick) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return -1;
    auto* ch = triggerChannel(cm->pattern, laneOffset(name) + channel);
    if (!ch) return -1;
    auto& t = ch->triggers;
    auto it = std::find(t.begin(), t.end(), oldTick);
    if (it == t.end()) return -1;
    newTick = std::max(0, newTick);
    if (newTick != oldTick && std::find(t.begin(), t.end(), newTick) != t.end()) {
        t.erase(it);
    } else {
        *it = newTick;
    }
    std::sort(t.begin(), t.end());
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
    auto f = std::find(t.begin(), t.end(), newTick);
    return f == t.end() ? -1 : (int) std::distance(t.begin(), f);
}

void PatternHost::clearChannel(const std::string& name, int channel) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return;
    if (auto* ch = triggerChannel(cm->pattern, laneOffset(name) + channel)) {
        ch->triggers.clear();
        patternSync_.markPatternEdited();
        patternSync_.syncPattern(name);
    }
}

void PatternHost::setDuration(const std::string& name, int ticks) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present || ticks <= 0) return;
    cm->pattern.duration = ticks;
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

void PatternHost::setResolution(const std::string& name, const std::string& res) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return;
    cm->pattern.matrixResolution = res;
    patternSync_.markPatternEdited();
}

void PatternHost::nudgeChannel(const std::string& name, int channel, int deltaTicks) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return;
    const int dur = cm->pattern.duration;
    if (dur <= 0) return;
    if (auto* ch = triggerChannel(cm->pattern, laneOffset(name) + channel)) {
        for (int& t : ch->triggers) t = ((t + deltaTicks) % dur + dur) % dur;
        std::sort(ch->triggers.begin(), ch->triggers.end());
        patternSync_.markPatternEdited();
        patternSync_.syncPattern(name);
    }
}

void PatternHost::setChannelSnap(const std::string& name, int channel, const std::string& snap) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return;
    if (auto* ch = triggerChannel(cm->pattern, laneOffset(name) + channel)) { ch->snap = snap; patternSync_.markPatternEdited(); }
}

void PatternHost::reframe(const std::string& name, int deltaTicks) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || !cm->pattern.present) return;
    const int dur = cm->pattern.duration;
    if (dur <= 0) return;
    for (auto& ch : cm->pattern.channels) {
        if (ch.type == "time-signatures") continue;
        for (int& t : ch.triggers) t = ((t + deltaTicks) % dur + dur) % dur;
        std::sort(ch.triggers.begin(), ch.triggers.end());
    }
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

namespace {
PatternChannel* matrixChannelMut(Pattern& p, const std::string& type, int ordinal = 0) {
    return const_cast<PatternChannel*>(matrixChannel(p, type, ordinal));
}
}

void PatternHost::ensureMatrix(const std::string& name, const std::string& type, int steps,
                               const std::string& seed) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || steps <= 0) return;
    Pattern& p = cm->pattern;
    if (!p.present) {
        p.present = true;
        p.matrixResolution = "1/16";
        p.duration = steps * stepTicksFor(p.matrixResolution);
        PatternChannel ts; ts.type = "time-signatures"; ts.timeSignatures = {{0, 4, 4}};
        p.channels.push_back(ts);
    }
    if (type == "bassline-pattern-matrix") {
        while (matrixChannelCount(p, type) < kPatternBanks) {
            PatternChannel ch; ch.type = type;
            ch.matrix = encodeBassline(std::vector<BasslineStep>((size_t) steps));
            p.channels.push_back(ch);
        }
    } else if (!matrixChannelMut(p, type)) {
        PatternChannel ch; ch.type = type;
        std::vector<ArpStep> st((size_t) steps);
        for (size_t i = 0; !seed.empty() && i < st.size(); ++i) {
            const char c = seed[i % seed.size()];
            st[i].trigger = c == '1' || c == 'x';
        }
        ch.matrix = encodeArp(st);
        p.channels.push_back(ch);
    }
    bool hasProp = false;
    for (auto& pr : cm->properties) if (pr.name == "Pattern") { hasProp = true; break; }
    if (!hasProp) {
        Parameter pr; pr.name = "Pattern"; pr.type = "pattern"; pr.userEdited = true;
        cm->properties.push_back(pr);
    }
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

int PatternHost::bank(const std::string& name) const {
    const double v = host_.liveParamValue(name, "Bank");
    return std::clamp((int) std::lround(v), 0, kPatternBanks - 1);
}

std::vector<BasslineStep> PatternHost::basslineSteps(const std::string& name) const {
    return basslineSteps(name, bank(name));
}

std::vector<BasslineStep> PatternHost::basslineSteps(const std::string& name, int bank) const {
    if (auto* cm = doc_.document().byName(name))
        if (auto* ch = matrixChannel(cm->pattern, "bassline-pattern-matrix", bank))
            return decodeBassline(ch->matrix);
    return {};
}

void PatternHost::setBasslineStep(const std::string& name, int index, const BasslineStep& s) {
    if (index < 0) return;
    auto steps = basslineSteps(name);
    if (index >= (int) steps.size()) steps.resize((size_t) index + 1);
    steps[(size_t) index] = s;
    setBasslineSteps(name, bank(name), steps);
}

void PatternHost::setBasslineSteps(const std::string& name, int bank,
                                   const std::vector<BasslineStep>& steps) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm) return;
    auto* ch = matrixChannelMut(cm->pattern, "bassline-pattern-matrix", bank);
    if (!ch) return;
    ch->matrix = encodeBassline(steps);
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

std::vector<ArpStep> PatternHost::arpSteps(const std::string& name) const {
    if (auto* cm = doc_.document().byName(name))
        if (auto* ch = matrixChannel(cm->pattern, "trigger-tie-matrix"))
            return decodeArp(ch->matrix);
    return {};
}

void PatternHost::setArpStep(const std::string& name, int index, const ArpStep& s) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || index < 0) return;
    auto* ch = matrixChannelMut(cm->pattern, "trigger-tie-matrix");
    if (!ch) return;
    auto steps = decodeArp(ch->matrix);
    if (index >= (int) steps.size()) steps.resize((size_t) index + 1);
    steps[(size_t) index] = s;
    ch->matrix = encodeArp(steps);
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

std::vector<bool> PatternHost::arpUps(const std::string& name) const {
    if (auto* cm = doc_.document().byName(name)) {
        if (auto* ch = matrixChannel(cm->pattern, "octave-row"))
            return decodeOctaveRow(ch->matrix);
        if (auto* tt = matrixChannel(cm->pattern, "trigger-tie-matrix"))
            return std::vector<bool>(decodeArp(tt->matrix).size(), false);
    }
    return {};
}

void PatternHost::setArpUp(const std::string& name, int index, bool up) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || index < 0) return;
    auto* ch = matrixChannelMut(cm->pattern, "octave-row");
    if (!ch) {
        auto* tt = matrixChannelMut(cm->pattern, "trigger-tie-matrix");
        if (!tt) return;
        PatternChannel oc; oc.type = "octave-row";
        oc.matrix = encodeOctaveRow(std::vector<bool>(decodeArp(tt->matrix).size(), false));
        cm->pattern.channels.push_back(oc);
        ch = matrixChannelMut(cm->pattern, "octave-row");
    }
    auto ups = decodeOctaveRow(ch->matrix);
    if (index >= (int) ups.size()) ups.resize((size_t) index + 1, false);
    ups[(size_t) index] = up;
    ch->matrix = encodeOctaveRow(ups);
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

void PatternHost::ensureNote(const std::string& name) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm || matrixChannel(cm->pattern, "note-events")) return;
    Pattern& p = cm->pattern;
    if (!p.present) {
        p.present = true;
        p.duration = 4 * 4 * Pattern::kTicksPerBeat;
        p.matrixResolution = "1/16";
        PatternChannel ts; ts.type = "time-signatures"; ts.timeSignatures = {{0, 4, 4}};
        p.channels.push_back(ts);
    }
    PatternChannel notes; notes.type = "note-events";
    p.channels.push_back(std::move(notes));
    bool hasProp = false;
    for (auto& pr : cm->properties) if (pr.name == "Pattern") { hasProp = true; break; }
    if (!hasProp) {
        Parameter pr; pr.name = "Pattern"; pr.type = "pattern"; pr.userEdited = true;
        cm->properties.push_back(pr);
    }
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

void PatternHost::ensureAudio(const std::string& name) {
    patternSync_.recordPatternRevert(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm) return;
    Pattern& p = cm->pattern;
    if (!p.present) {
        p.present = true;
        p.duration = 4 * 4 * Pattern::kTicksPerBeat;
        p.matrixResolution = "1/16";
        PatternChannel ts; ts.type = "time-signatures"; ts.timeSignatures = {{0, 4, 4}};
        p.channels.push_back(ts);
    }
    bool hasProp = false;
    for (auto& pr : cm->properties) if (pr.name == "Pattern") { hasProp = true; break; }
    if (!hasProp) {
        Parameter pr; pr.name = "Pattern"; pr.type = "pattern"; pr.userEdited = true;
        cm->properties.push_back(pr);
    }
    patternSync_.markPatternEdited();
}

std::vector<NoteEvent> PatternHost::noteEvents(const std::string& name) const {
    if (auto* cm = doc_.document().byName(name))
        if (auto* ch = matrixChannel(cm->pattern, "note-events"))
            return decodeNoteEvents(ch->matrix);
    return {};
}

void PatternHost::setNoteEvents(const std::string& name, const std::vector<NoteEvent>& notes,
                                  int durationTicks) {
    patternSync_.recordPatternRevert(name);
    ensureNote(name);
    auto* cm = doc_.mutableByName(name);
    if (!cm) return;
    auto* ch = matrixChannelMut(cm->pattern, "note-events");
    if (!ch) return;
    ch->matrix = replaceNoteEvents(ch->matrix, notes);
    if (durationTicks > 0) cm->pattern.duration = durationTicks;
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

}
