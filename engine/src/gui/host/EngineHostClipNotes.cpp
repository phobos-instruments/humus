// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/timeline/ClipOps.h"
#include "core/timeline/RecordTake.h"
#include "hum/dsp/PaulstretchCore.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "core/timeline/ClipRangeOps.h"
#include "hum/PatternMatrix.h"
#include <climits>
#include <cmath>
#include "core/params/ParamSchema.h"
#include "core/packs/Roles.h"
#include "gui/video/VideoProbe.h"
#include <memory>
#include <juce_audio_formats/juce_audio_formats.h>
#include "hum/dsp/DspMath.h"

namespace hum {

std::vector<NoteEvent> ClipEditor::notes(const std::string& node, int clip) const {
    if (const auto* cm = doc_.document().byName(node))
        if (const auto* ch = clipops::clipChannel(cm->pattern, clip))
            return decodeNoteEvents(ch->matrix);
    return {};
}

void ClipEditor::transpose(const std::string& node, int clip, int steps) {
    auto* cm = doc_.mutableByName(node);
    if (cm == nullptr || !clipops::transposeClip(cm->pattern, clip, steps)) return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::quantise(const std::string& node, int clip, int gridTicks) {
    auto* cm = doc_.mutableByName(node);
    if (cm == nullptr || !clipops::quantiseClip(cm->pattern, clip, gridTicks)) return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::nudgeVelocity(const std::string& node, int clip, int delta) {
    auto* cm = doc_.mutableByName(node);
    if (cm == nullptr || !clipops::nudgeVelocity(cm->pattern, clip, delta)) return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

int ClipEditor::raise(const std::string& node, int clip) {
    auto* cm = doc_.mutableByName(node);
    if (cm == nullptr) return -1;
    const int n = clipops::raiseClip(cm->pattern, clip);
    if (n < 0) return -1;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return n;
}

void ClipEditor::setNotes(const std::string& node, int clip,
                          const std::vector<NoteEvent>& notes, int lengthTicks) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return;
    auto* ch = clipops::writableClipChannel(cm->pattern, clip);
    if (!ch && clip == 0) {
        host_.patterns().ensureNote(node);
        cm = doc_.mutableByName(node);
        if (!cm) return;
        ch = clipops::writableClipChannel(cm->pattern, clip);
    }
    if (!ch) return;
    ch->matrix = replaceNoteEvents(ch->matrix, notes);
    if (lengthTicks > 0) {
        if (ch->startTick >= 0) ch->lengthTicks = lengthTicks;
        else cm->pattern.duration = lengthTicks;
    }
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

std::vector<CCEvent> ClipEditor::ccs(const std::string& node, int clip) const {
    if (const auto* cm = doc_.document().byName(node))
        if (const auto* ch = clipops::clipChannel(cm->pattern, clip))
            return decodeCCEvents(ch->matrix);
    return {};
}

void ClipEditor::setCCs(const std::string& node, int clip, const std::vector<CCEvent>& ccs) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (!ch) return;
    auto sorted = ccs;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const CCEvent& a, const CCEvent& b) { return a.tick < b.tick; });
    ch->matrix = replaceCCEvents(ch->matrix, sorted);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

bool ClipEditor::ownsPattern(const std::string& node) const {
    return audio_.graph() != nullptr
           && dynamic_cast<ClipArrangement*>(audio_.graph()->find(node)) != nullptr;
}

bool ClipEditor::gridTarget(const std::string& node) const {
    const auto* cm = doc_.document().byName(node);
    return cm != nullptr && !ownsPattern(node) && classHasRole(cm->classRaw, role::kNoteLanes);
}

bool ClipEditor::adoptNotes(const std::string& src, int clipId, const std::string& dst) {
    const auto* sm = doc_.document().byName(src);
    if (!sm) return false;
    const int ord = clipops::clipIndexOfId(sm->pattern, clipId);
    if (ord < 0) return false;
    const auto from = list(src)[(size_t) ord];
    if (from.isAudio) return false;
    const auto ne = notes(src, ord);
    const int len = std::max(1, from.lengthTicks);

    if (ownsPattern(dst)) {
        setNotes(dst, 0, ne, len);
        return true;
    }
    if (!gridTarget(dst)) return false;

    int lanes = 0;
    for (const auto& d : schemaFor(doc_.document().byName(dst)->classRaw))
        if (d.name.rfind("Note_", 0) == 0) ++lanes;
    if (lanes <= 0) return false;
    host_.patterns().ensure(dst, lanes);
    for (int k = 0; k < lanes; ++k) host_.patterns().clearChannel(dst, k);
    host_.patterns().setDuration(dst, len);
    const auto* cm = doc_.document().byName(dst);
    const int step = std::max(1, stepTicksFor(cm ? cm->pattern.matrixResolution : std::string("1/16")));
    std::vector<double> laneNote((size_t) lanes);
    for (int k = 0; k < lanes; ++k)
        laneNote[(size_t) k] = host_.liveParamValue(dst, "Note_" + std::to_string(k + 1));
    for (const auto& n : ne) {
        int lane = 0;
        double best = 1e9;
        for (int k = 0; k < lanes; ++k)
            if (const double dpitch = std::abs(laneNote[(size_t) k] - n.pitch); dpitch < best) { best = dpitch; lane = k; }
        const int at = std::min(len - 1, (n.tick + step / 2) / step * step);
        host_.patterns().addTrigger(dst, lane, at);
    }
    return true;
}

}
