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

std::vector<ClipEditor::ClipInfo> ClipEditor::list(const std::string& node) const {
    std::vector<ClipInfo> out;
    const auto* cm = doc_.document().byName(node);
    if (!cm) return out;
    const auto& p = cm->pattern;
    for (int i = 0; i < clipops::clipCount(p); ++i) {
        const auto* ch = clipops::clipChannel(p, i);
        ClipInfo ci;
        ci.index = i;
        ci.name = ch->name;
        ci.startTick = clipops::clipStart(p, i);
        ci.lengthTicks = clipops::clipLength(p, i);
        ci.looped = clipops::clipLooped(p, i);
        ci.legacy = clipops::clipIsLegacy(p, i);
        ci.color = ch->color;
        ci.isAudio = clipops::isAudioClip(*ch);
        ci.isVideo = clipops::isVideoClip(*ch);
        ci.isCompound = clipops::isCompoundClip(*ch);
        ci.audioFile = ch->audioFile;
        ci.audioOffset = ch->audioOffset;
        ci.id = ch->id;
        ci.sourceBpm = ch->sourceBpm;
        ci.warpMode = ch->warpMode;
        ci.fadeInTicks = ch->fadeInTicks;
        ci.fadeOutTicks = ch->fadeOutTicks;
        ci.fadeInCurve = ch->fadeInCurve;
        ci.fadeOutCurve = ch->fadeOutCurve;
        ci.audioGain = ch->audioGain;
        ci.audioReverse = ch->audioReverse;
        ci.audioPitch = ch->audioPitch;
        out.push_back(std::move(ci));
    }
    return out;
}

void ClipEditor::slip(const std::string& node, int clip, long long deltaSamples) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !clipops::slipClip(cm->pattern, clip, deltaSamples)) return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::setGain(const std::string& node, int clip, double gain) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !clipops::setClipGain(cm->pattern, clip, gain)) return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::setReverse(const std::string& node, int clip, bool reverse) {
    auto* cm = doc_.mutableByName(node);
    auto* ch = cm ? clipops::clipChannel(cm->pattern, clip) : nullptr;
    if (!ch || !clipops::isMediaClip(*ch) || ch->audioReverse == reverse) return;
    ch->audioReverse = reverse;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::setPitch(const std::string& node, int clip, double semitones) {
    auto* cm = doc_.mutableByName(node);
    auto* ch = cm ? clipops::clipChannel(cm->pattern, clip) : nullptr;
    if (!ch || !clipops::isAudioClip(*ch)) return;
    ch->audioPitch = juce::jlimit(-24.0, 24.0, semitones);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

bool ClipEditor::stretch(const std::string& node, int clip, int newLengthTicks) {
    auto* cm = doc_.mutableByName(node);
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    if (!cm || !clipops::stretchClip(cm->pattern, clip, newLengthTicks, bpm)) return false;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return true;
}

bool ClipEditor::trimTo(const std::string& node, int clip, int fromTick, int toTick) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !clipops::trimClipTo(cm->pattern, clip, fromTick, toTick, samplesPerTick()))
        return false;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return true;
}

bool ClipEditor::removeRange(const std::string& node, int clip, int fromTick, int toTick,
                             bool ripple) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !clipops::removeClipRange(cm->pattern, clip, fromTick, toTick, ripple,
                                         samplesPerTick()))
        return false;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return true;
}

PatternChannel ClipEditor::copyRange(const std::string& node, int clip, int fromTick,
                                     int toTick) const {
    if (const auto* cm = doc_.document().byName(node))
        return clipops::clipRangeCopy(cm->pattern, clip, fromTick, toTick, samplesPerTick());
    return {};
}

void ClipEditor::setWarp(const std::string& node, int clip, int mode) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (ch == nullptr || ch->warpMode == mode) return;
    patternSync_.recordPatternRevert(node);
    ch->warpMode = mode;
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(node);
}

void ClipEditor::setSourceBpm(const std::string& node, int clip, double bpm) {
    auto* cm = doc_.mutableByName(node);
    if (cm == nullptr) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    const double v = bpm > 0.0 ? juce::jlimit(20.0, 999.0, bpm) : 0.0;
    if (ch == nullptr || std::abs(ch->sourceBpm - v) < 1e-9) return;
    patternSync_.recordPatternRevert(node);
    ch->sourceBpm = v;
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(node);
}

void ClipEditor::setFades(const std::string& node, int clip, int inTicks, int outTicks) {
    auto* cm = doc_.mutableByName(node);
    if (cm == nullptr) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (ch == nullptr) return;
    const int len = clipops::clipLength(cm->pattern, clip);
    int fi = juce::jlimit(0, len, inTicks);
    int fo = juce::jlimit(0, len, outTicks);
    if (fi + fo > len) {
        const double scale = (double) len / (double) (fi + fo);
        fi = (int) (fi * scale);
        fo = len - fi;
    }
    if (ch->fadeInTicks == fi && ch->fadeOutTicks == fo) return;
    patternSync_.recordPatternRevert(node);
    ch->fadeInTicks = fi;
    ch->fadeOutTicks = fo;
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(node);
}

void ClipEditor::setFadeCurves(const std::string& node, int clip, double in, double out) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !cm->pattern.present) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (!ch) return;
    in = juce::jlimit(-1.0, 1.0, in);
    out = juce::jlimit(-1.0, 1.0, out);
    if (ch->fadeInCurve == in && ch->fadeOutCurve == out) return;
    patternSync_.recordPatternRevert(node);
    ch->fadeInCurve = in;
    ch->fadeOutCurve = out;
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(node);
}

void ClipEditor::upgradeLegacy(const std::string& node) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !cm->pattern.present) return;
    clipops::upgradeLegacyClip(cm->pattern);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

int ClipEditor::add(const std::string& node, int startTick, int lengthTicks) {
    host_.patterns().ensureNote(node);
    auto* cm = doc_.mutableByName(node);
    if (!cm) return -1;
    const auto idx = clipops::noteChannels(cm->pattern);
    if (idx.size() == 1) {
        auto& ch = cm->pattern.channels[(size_t) idx[0]];
        if (ch.startTick < 0 && decodeNoteEvents(ch.matrix).empty()) {
            ch.startTick = std::max(0, startTick);
            ch.lengthTicks = std::max(1, lengthTicks);
            if (ch.id <= 0) ch.id = clipops::nextClipId(cm->pattern);
            doc_.flagDirty();
            patternSync_.syncPattern(node);
            return 0;
        }
    }
    const int clip = clipops::addClip(cm->pattern, startTick, lengthTicks);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return clip;
}

int ClipEditor::addAudio(const std::string& node, int startTick, int lengthTicks,
                         const std::string& file, long long offsetSamples) {
    host_.patterns().ensureAudio(node);
    auto* cm = doc_.mutableByName(node);
    if (!cm) return -1;
    const int clip = clipops::addAudioClip(cm->pattern, startTick, lengthTicks,
                                           file, offsetSamples);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return clip;
}

int ClipEditor::addVideo(const std::string& node, int startTick, int lengthTicks,
                         const std::string& file, long long offsetSamples) {
    host_.patterns().ensureAudio(node);
    auto* cm = doc_.mutableByName(node);
    if (!cm) return -1;
    const int clip = clipops::addVideoClip(cm->pattern, startTick, lengthTicks, file, offsetSamples);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return clip;
}

void ClipEditor::removeTrack(const std::string& node) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return;
    patternSync_.recordPatternRevert(node);
    auto& chans = cm->pattern.channels;
    chans.erase(std::remove_if(chans.begin(), chans.end(),
                               [](const PatternChannel& c) { return c.type == "note-events"; }),
                chans.end());
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    nodes_.syncNodeTrack(node);
}

void ClipEditor::remove(const std::string& node, int clip) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !clipops::removeClip(cm->pattern, clip)) return;
    if (clipops::noteChannels(cm->pattern).empty() && cm->pattern.present) {
        bool audio = false;
        for (const auto& ch : cm->pattern.channels)
            if (clipops::isMediaClip(ch)) audio = true;
        if (!audio) {
            PatternChannel seed;
            seed.type = "note-events";
            cm->pattern.channels.push_back(std::move(seed));
        }
    }
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::move(const std::string& node, int clip, int newStartTick) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return;
    clipops::upgradeLegacyClip(cm->pattern);
    if (!clipops::moveClip(cm->pattern, clip, newStartTick)) return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::resize(const std::string& node, int clip, int newLengthTicks, bool fromLeft) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return;
    clipops::upgradeLegacyClip(cm->pattern);
    const auto* ch = clipops::clipChannel(cm->pattern, clip);
    const int tail = ch != nullptr && clipops::isMediaClip(*ch) ? tailTicks(*ch) : 0;
    if (!clipops::resizeClip(cm->pattern, clip, newLengthTicks, fromLeft, samplesPerTick(), tail))
        return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

ClipEditor::Tape ClipEditor::tape(const std::string& file) const {
    if (file.empty()) return {};
    if (const auto it = tapes_.find(file); it != tapes_.end()) return it->second;
    Tape t;
    const auto uri = file.rfind("file://", 0) == 0 ? file.substr(7) : file;
    const juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    if (!f.existsAsFile()) return t;
    if (isVideoFile(f)) {
        t.seconds = probeVideoSeconds(f);
        t.sampleRate = audio_.sampleRate();
    } else {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(f));
        if (rd != nullptr && rd->sampleRate > 0.0) {
            t.seconds = (double) rd->lengthInSamples / rd->sampleRate;
            t.sampleRate = rd->sampleRate;
        }
    }
    if (t.seconds > 0.0) tapes_[file] = t;
    return t;
}

ClipEditor::MediaRange ClipEditor::rangeOf(const std::string& node, int clipId) const {
    MediaRange r;
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    for (const auto& ci : list(node)) {
        if (ci.id != clipId || !ci.hasMedia()) continue;
        const double rate = ci.warpMode != 0 && ci.sourceBpm > 0.0 ? bpm / ci.sourceBpm : 1.0;
        const auto t = ci.isAudio ? tape(ci.audioFile) : Tape{};
        const double offsetRate = t.sampleRate > 0.0 ? t.sampleRate : audio_.sampleRate();
        r.file = ci.audioFile;
        r.inSeconds = (double) ci.audioOffset / offsetRate;
        r.outSeconds = r.inSeconds
                       + ci.lengthTicks / (double) Pattern::kTicksPerBeat * (kSecondsPerMinute / bpm) * rate;
        r.looped = ci.looped;
    }
    return r;
}

int ClipEditor::addVideoRange(const std::string& node, int atTick, const MediaRange& range) {
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    double span = range.outSeconds - range.inSeconds;
    if (span <= 0.0) span = tape(range.file).seconds - range.inSeconds;
    if (span <= 0.0) return -1;
    const int ticks = std::max(1, (int) std::llround(span * (bpm / kSecondsPerMinute) * Pattern::kTicksPerBeat));
    return addVideo(node, atTick, ticks, range.file,
                    (long long) std::llround(range.inSeconds * audio_.sampleRate()));
}

int ClipEditor::tailTicks(const PatternChannel& clip) const {
    const auto t = tape(clip.audioFile);
    if (t.seconds <= 0.0 || t.sampleRate <= 0.0) return 0;
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    const double rate = clip.warpMode != 0 && clip.sourceBpm > 0.0 ? bpm / clip.sourceBpm : 1.0;
    const double left = t.seconds - (double) clip.audioOffset / t.sampleRate;
    return (int) std::floor(left / rate * (bpm / kSecondsPerMinute) * Pattern::kTicksPerBeat);
}

double ClipEditor::samplesPerTick() const {
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    return (kSecondsPerMinute / bpm) * audio_.sampleRate() / Pattern::kTicksPerBeat;
}

void ClipEditor::setLooped(const std::string& node, int clip, bool looped) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return;
    clipops::upgradeLegacyClip(cm->pattern);
    if (!clipops::setClipLooped(cm->pattern, clip, looped)) return;
    doc_.flagDirty();
    patternSync_.syncPattern(node);
}

void ClipEditor::rename(const std::string& node, int clip, const std::string& name) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !clipops::renameClip(cm->pattern, clip, name)) return;
    doc_.flagDirty();
}

void ClipEditor::setColor(const std::string& node, int clip, int color) {
    auto* cm = doc_.mutableByName(node);
    if (!cm || !clipops::setClipColor(cm->pattern, clip, color)) return;
    doc_.flagDirty();
}

bool ClipEditor::accepts(const std::string& node, bool audio) {
    return nodes_.nodeRecordsAudio(node) == audio && !nodes_.nodeArrangesVideo(node);
}

bool ClipEditor::accepts(const std::string& node, const ClipInfo& clip) {
    if (clip.isVideo) return nodes_.nodeArrangesVideo(node);
    return accepts(node, clip.isAudio);
}

bool ClipEditor::accepts(const std::string& node, const PatternChannel& clip) {
    if (clipops::isVideoClip(clip)) return nodes_.nodeArrangesVideo(node);
    return accepts(node, clipops::isAudioClip(clip));
}

}
