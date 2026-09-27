// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>
#include <vector>

#include "gui/host/ClipEdits.h"
#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {


class ClipEditor : public ClipEdits {
public:
    ClipEditor(BrickHost& host, HostCore& core) : host_(host), doc_(core), patternSync_(core), audio_(core), nodes_(core), recording_(core) {}

    std::vector<ClipInfo> list(const std::string& node) const override;
    void upgradeLegacy(const std::string& node) override;
    int  add(const std::string& node, int startTick, int lengthTicks) override;
    int  addAudio(const std::string& node, int startTick, int lengthTicks,
                  const std::string& file, long long offsetSamples = 0) override;
    int  makeCompound(const std::string& node, const std::vector<int>& ordinals) override;
    std::string compoundReel(const std::string& node, int clip) const override;
    int  addVideo(const std::string& node, int startTick, int lengthTicks,
                  const std::string& file, long long offsetSamples = 0) override;
    void setWarp(const std::string& node, int clip, int mode) override;
    void setSourceBpm(const std::string& node, int clip, double bpm) override;
    void setFades(const std::string& node, int clip, int inTicks, int outTicks) override;
    void setFadeCurves(const std::string& node, int clip, double in, double out) override;
    void slip(const std::string& node, int clip, long long deltaSamples) override;
    void setGain(const std::string& node, int clip, double gain) override;
    void setReverse(const std::string& node, int clip, bool reverse) override;
    void setPitch(const std::string& node, int clip, double semitones) override;
    bool stretch(const std::string& node, int clip, int newLengthTicks) override;
    bool trimTo(const std::string& node, int clip, int fromTick, int toTick) override;
    bool removeRange(const std::string& node, int clip, int fromTick, int toTick, bool ripple) override;
    PatternChannel copyRange(const std::string& node, int clip, int fromTick, int toTick) const override;
    double samplesPerTick() const override;
    void remove(const std::string& node, int clip) override;
    void move(const std::string& node, int clip, int newStartTick) override;
    void resize(const std::string& node, int clip, int newLengthTicks, bool fromLeft) override;
    Tape tape(const std::string& file) const override;
    int tailTicks(const PatternChannel& clip) const override;
    MediaRange rangeOf(const std::string& node, int clipId) const override;
    int addVideoRange(const std::string& node, int atTick, const MediaRange& range) override;
    void setLooped(const std::string& node, int clip, bool looped) override;
    void rename(const std::string& node, int clip, const std::string& name) override;
    void setColor(const std::string& node, int clip, int color) override;
    int  duplicate(const std::string& node, int clip, int newStartTick) override;
    int  split(const std::string& node, int clip, int atTick) override;
    int  join(const std::string& node, int a, int b) override;
    int  mergeAudio(const std::string& node, const std::vector<int>& clips) override;
    std::string exportFile(const std::string& node, int clip) override;
    std::string stretchAudioFile(const std::string& node, int clip, double factor) override;
    int  raise(const std::string& node, int clip) override;
    PatternChannel copyClip(const std::string& node, int clip) const override;
    int  pasteClip(const std::string& node, const PatternChannel& data, int atTick) override;
    int  moveToNode(const std::string& node, int clip, const std::string& toNode) override;
    bool accepts(const std::string& node, bool audio) override;
    bool accepts(const std::string& node, const ClipInfo& clip) override;
    bool accepts(const std::string& node, const PatternChannel& clip) override;
    void transpose(const std::string& node, int clip, int steps) override;
    void quantise(const std::string& node, int clip, int gridTicks) override;
    void nudgeVelocity(const std::string& node, int clip, int delta) override;
    std::vector<NoteEvent> notes(const std::string& node, int clip) const override;
    void removeTrack(const std::string& node) override;
    void setNotes(const std::string& node, int clip,
                  const std::vector<NoteEvent>& notes, int lengthTicks) override;
    bool adoptNotes(const std::string& src, int clipId, const std::string& dst) override;
    bool ownsPattern(const std::string& node) const override;
    bool gridTarget(const std::string& node) const override;
    std::vector<CCEvent> ccs(const std::string& node, int clip) const override;
    void setCCs(const std::string& node, int clip, const std::vector<CCEvent>& ccs) override;

private:
    BrickHost& host_;
    HostDocument& doc_;
    HostPatterns& patternSync_;
    HostGraph& audio_;
    HostNodes& nodes_;
    HostRecording& recording_;
    mutable std::map<std::string, Tape> tapes_;
};

}
