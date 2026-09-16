// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {


class ClipEditor {
public:
    ClipEditor(BrickHost& host, HostCore& core) : host_(host), doc_(core), patternSync_(core), audio_(core), nodes_(core), recording_(core) {}

    struct ClipInfo {
        int index = 0;
        std::string name;
        int startTick = 0;
        int lengthTicks = 0;
        bool looped = false;
        bool legacy = false;
        int color = 0;
        bool isAudio = false;
        bool isVideo = false;
        bool isCompound = false;
        bool hasMedia() const { return isAudio || isVideo || isCompound; }
        std::string audioFile;
        long long audioOffset = 0;
        int id = 0;
        double sourceBpm = 0.0;
        int warpMode = 0;
        int fadeInTicks = 0, fadeOutTicks = 0;
        double fadeInCurve = 0.0, fadeOutCurve = 0.0;
        double audioGain = 1.0;
        bool audioReverse = false;
        double audioPitch = 0.0;
    };
    std::vector<ClipInfo> list(const std::string& node) const;
    void upgradeLegacy(const std::string& node);
    int  add(const std::string& node, int startTick, int lengthTicks);
    int  addAudio(const std::string& node, int startTick, int lengthTicks,
                  const std::string& file, long long offsetSamples = 0);
    int  makeCompound(const std::string& node, const std::vector<int>& ordinals);
    std::string compoundReel(const std::string& node, int clip) const;
    int  addVideo(const std::string& node, int startTick, int lengthTicks,
                  const std::string& file, long long offsetSamples = 0);
    void setWarp(const std::string& node, int clip, int mode);
    void setSourceBpm(const std::string& node, int clip, double bpm);
    void setFades(const std::string& node, int clip, int inTicks, int outTicks);
    void setFadeCurves(const std::string& node, int clip, double in, double out);
    void slip(const std::string& node, int clip, long long deltaSamples);
    void setGain(const std::string& node, int clip, double gain);
    void setReverse(const std::string& node, int clip, bool reverse);
    void setPitch(const std::string& node, int clip, double semitones);
    bool stretch(const std::string& node, int clip, int newLengthTicks);
    bool trimTo(const std::string& node, int clip, int fromTick, int toTick);
    bool removeRange(const std::string& node, int clip, int fromTick, int toTick, bool ripple);
    PatternChannel copyRange(const std::string& node, int clip, int fromTick, int toTick) const;
    double samplesPerTick() const;
    void remove(const std::string& node, int clip);
    void move(const std::string& node, int clip, int newStartTick);
    void resize(const std::string& node, int clip, int newLengthTicks, bool fromLeft);
    struct Tape {
        double seconds = 0.0;
        double sampleRate = 0.0;
    };
    Tape tape(const std::string& file) const;
    int tailTicks(const PatternChannel& clip) const;
    struct MediaRange {
        std::string file;
        double inSeconds = 0.0;
        double outSeconds = 0.0;
        bool looped = false;
    };
    MediaRange rangeOf(const std::string& node, int clipId) const;
    int addVideoRange(const std::string& node, int atTick, const MediaRange& range);
    void setLooped(const std::string& node, int clip, bool looped);
    void rename(const std::string& node, int clip, const std::string& name);
    void setColor(const std::string& node, int clip, int color);
    int  duplicate(const std::string& node, int clip, int newStartTick);
    int  split(const std::string& node, int clip, int atTick);
    int  join(const std::string& node, int a, int b);
    int  mergeAudio(const std::string& node, const std::vector<int>& clips);
    std::string exportFile(const std::string& node, int clip);
    std::string stretchAudioFile(const std::string& node, int clip, double factor);
    int  raise(const std::string& node, int clip);
    PatternChannel copyClip(const std::string& node, int clip) const;
    int  pasteClip(const std::string& node, const PatternChannel& data, int atTick);
    int  moveToNode(const std::string& node, int clip, const std::string& toNode);
    bool accepts(const std::string& node, bool audio);
    bool accepts(const std::string& node, const ClipInfo& clip);
    bool accepts(const std::string& node, const PatternChannel& clip);
    void transpose(const std::string& node, int clip, int steps);
    void quantise(const std::string& node, int clip, int gridTicks);
    void nudgeVelocity(const std::string& node, int clip, int delta);
    std::vector<NoteEvent> notes(const std::string& node, int clip) const;
    void removeTrack(const std::string& node);
    void setNotes(const std::string& node, int clip,
                  const std::vector<NoteEvent>& notes, int lengthTicks);
    bool adoptNotes(const std::string& src, int clipId, const std::string& dst);
    bool ownsPattern(const std::string& node) const;
    bool gridTarget(const std::string& node) const;
    std::vector<CCEvent> ccs(const std::string& node, int clip) const;
    void setCCs(const std::string& node, int clip, const std::vector<CCEvent>& ccs);

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
