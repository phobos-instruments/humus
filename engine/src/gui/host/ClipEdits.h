// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "hum/PatternMatrix.h"

namespace hum {

class ClipEdits {
public:
    virtual ~ClipEdits() = default;

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

    struct Tape {
        double seconds = 0.0;
        double sampleRate = 0.0;
    };

    struct MediaRange {
        std::string file;
        double inSeconds = 0.0;
        double outSeconds = 0.0;
        bool looped = false;
    };

    virtual std::vector<ClipInfo> list(const std::string& node) const = 0;
    virtual void upgradeLegacy(const std::string& node) = 0;
    virtual int add(const std::string& node, int startTick, int lengthTicks) = 0;
    virtual int addAudio(const std::string& node, int startTick, int lengthTicks, const std::string& file,
                         long long offsetSamples = 0) = 0;
    virtual int makeCompound(const std::string& node, const std::vector<int>& ordinals) = 0;
    virtual std::string compoundReel(const std::string& node, int clip) const = 0;
    virtual int addVideo(const std::string& node, int startTick, int lengthTicks, const std::string& file,
                         long long offsetSamples = 0) = 0;
    virtual void setWarp(const std::string& node, int clip, int mode) = 0;
    virtual void setSourceBpm(const std::string& node, int clip, double bpm) = 0;
    virtual void setFades(const std::string& node, int clip, int inTicks, int outTicks) = 0;
    virtual void setFadeCurves(const std::string& node, int clip, double in, double out) = 0;
    virtual void slip(const std::string& node, int clip, long long deltaSamples) = 0;
    virtual void setGain(const std::string& node, int clip, double gain) = 0;
    virtual void setReverse(const std::string& node, int clip, bool reverse) = 0;
    virtual void setPitch(const std::string& node, int clip, double semitones) = 0;
    virtual bool stretch(const std::string& node, int clip, int newLengthTicks) = 0;
    virtual bool trimTo(const std::string& node, int clip, int fromTick, int toTick) = 0;
    virtual bool removeRange(const std::string& node, int clip, int fromTick, int toTick, bool ripple) = 0;
    virtual PatternChannel copyRange(const std::string& node, int clip, int fromTick, int toTick) const = 0;
    virtual double samplesPerTick() const = 0;
    virtual void remove(const std::string& node, int clip) = 0;
    virtual void move(const std::string& node, int clip, int newStartTick) = 0;
    virtual void resize(const std::string& node, int clip, int newLengthTicks, bool fromLeft) = 0;
    virtual Tape tape(const std::string& file) const = 0;
    virtual int tailTicks(const PatternChannel& clip) const = 0;
    virtual MediaRange rangeOf(const std::string& node, int clipId) const = 0;
    virtual int addVideoRange(const std::string& node, int atTick, const MediaRange& range) = 0;
    virtual void setLooped(const std::string& node, int clip, bool looped) = 0;
    virtual void rename(const std::string& node, int clip, const std::string& name) = 0;
    virtual void setColor(const std::string& node, int clip, int color) = 0;
    virtual int duplicate(const std::string& node, int clip, int newStartTick) = 0;
    virtual int split(const std::string& node, int clip, int atTick) = 0;
    virtual int join(const std::string& node, int a, int b) = 0;
    virtual int mergeAudio(const std::string& node, const std::vector<int>& clips) = 0;
    virtual std::string exportFile(const std::string& node, int clip) = 0;
    virtual std::string stretchAudioFile(const std::string& node, int clip, double factor) = 0;
    virtual int raise(const std::string& node, int clip) = 0;
    virtual PatternChannel copyClip(const std::string& node, int clip) const = 0;
    virtual int pasteClip(const std::string& node, const PatternChannel& data, int atTick) = 0;
    virtual int moveToNode(const std::string& node, int clip, const std::string& toNode) = 0;
    virtual bool accepts(const std::string& node, bool audio) = 0;
    virtual bool accepts(const std::string& node, const ClipInfo& clip) = 0;
    virtual bool accepts(const std::string& node, const PatternChannel& clip) = 0;
    virtual void transpose(const std::string& node, int clip, int steps) = 0;
    virtual void quantise(const std::string& node, int clip, int gridTicks) = 0;
    virtual void nudgeVelocity(const std::string& node, int clip, int delta) = 0;
    virtual std::vector<NoteEvent> notes(const std::string& node, int clip) const = 0;
    virtual void removeTrack(const std::string& node) = 0;
    virtual void setNotes(const std::string& node, int clip, const std::vector<NoteEvent>& notes, int lengthTicks) = 0;
    virtual bool adoptNotes(const std::string& src, int clipId, const std::string& dst) = 0;
    virtual bool ownsPattern(const std::string& node) const = 0;
    virtual bool gridTarget(const std::string& node) const = 0;
    virtual std::vector<CCEvent> ccs(const std::string& node, int clip) const = 0;
    virtual void setCCs(const std::string& node, int clip, const std::vector<CCEvent>& ccs) = 0;
};

}
