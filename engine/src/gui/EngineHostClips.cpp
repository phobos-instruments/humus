#include "gui/EngineHost.h"

#include "core/ClipOps.h"
#include "core/RecordTake.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "core/ClipRangeOps.h"
#include "hum/PatternMatrix.h"
#include <climits>
#include <cmath>
#include "core/ParamSchema.h"

namespace hum {

std::vector<ClipEditor::ClipInfo> ClipEditor::list(const std::string& node) const {
    std::vector<ClipInfo> out;
    const auto* cm = host_.model_.byName(node);
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
    auto* cm = host_.mutableByName(node);
    if (!cm || !clipops::slipClip(cm->pattern, clip, deltaSamples)) return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::setGain(const std::string& node, int clip, double gain) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !clipops::setClipGain(cm->pattern, clip, gain)) return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::setReverse(const std::string& node, int clip, bool reverse) {
    auto* cm = host_.mutableByName(node);
    auto* ch = cm ? clipops::clipChannel(cm->pattern, clip) : nullptr;
    if (!ch || !clipops::isAudioClip(*ch) || ch->audioReverse == reverse) return;
    ch->audioReverse = reverse;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::setPitch(const std::string& node, int clip, double semitones) {
    auto* cm = host_.mutableByName(node);
    auto* ch = cm ? clipops::clipChannel(cm->pattern, clip) : nullptr;
    if (!ch || !clipops::isAudioClip(*ch)) return;
    ch->audioPitch = juce::jlimit(-24.0, 24.0, semitones);
    host_.dirty_ = true;
    host_.syncPattern(node);
}

bool ClipEditor::stretch(const std::string& node, int clip, int newLengthTicks) {
    auto* cm = host_.mutableByName(node);
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    if (!cm || !clipops::stretchClip(cm->pattern, clip, newLengthTicks, bpm)) return false;
    host_.dirty_ = true;
    host_.syncPattern(node);
    return true;
}

bool ClipEditor::trimTo(const std::string& node, int clip, int fromTick, int toTick) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !clipops::trimClipTo(cm->pattern, clip, fromTick, toTick, samplesPerTick()))
        return false;
    host_.dirty_ = true;
    host_.syncPattern(node);
    return true;
}

bool ClipEditor::removeRange(const std::string& node, int clip, int fromTick, int toTick,
                             bool ripple) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !clipops::removeClipRange(cm->pattern, clip, fromTick, toTick, ripple,
                                         samplesPerTick()))
        return false;
    host_.dirty_ = true;
    host_.syncPattern(node);
    return true;
}

PatternChannel ClipEditor::copyRange(const std::string& node, int clip, int fromTick,
                                     int toTick) const {
    if (const auto* cm = host_.model_.byName(node))
        return clipops::clipRangeCopy(cm->pattern, clip, fromTick, toTick, samplesPerTick());
    return {};
}

void ClipEditor::setWarp(const std::string& node, int clip, int mode) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (ch == nullptr || ch->warpMode == mode) return;
    host_.recordPatternRevert(node);
    ch->warpMode = mode;
    host_.markPatternEdited();
    host_.syncPattern(node);
}

void ClipEditor::setSourceBpm(const std::string& node, int clip, double bpm) {
    auto* cm = host_.mutableByName(node);
    if (cm == nullptr) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    const double v = bpm > 0.0 ? juce::jlimit(20.0, 999.0, bpm) : 0.0;
    if (ch == nullptr || std::abs(ch->sourceBpm - v) < 1e-9) return;
    host_.recordPatternRevert(node);
    ch->sourceBpm = v;
    host_.markPatternEdited();
    host_.syncPattern(node);
}

void ClipEditor::setFades(const std::string& node, int clip, int inTicks, int outTicks) {
    auto* cm = host_.mutableByName(node);
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
    host_.recordPatternRevert(node);
    ch->fadeInTicks = fi;
    ch->fadeOutTicks = fo;
    host_.markPatternEdited();
    host_.syncPattern(node);
}

void ClipEditor::setFadeCurves(const std::string& node, int clip, double in, double out) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !cm->pattern.present) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (!ch) return;
    in = juce::jlimit(-1.0, 1.0, in);
    out = juce::jlimit(-1.0, 1.0, out);
    if (ch->fadeInCurve == in && ch->fadeOutCurve == out) return;
    host_.recordPatternRevert(node);
    ch->fadeInCurve = in;
    ch->fadeOutCurve = out;
    host_.markPatternEdited();
    host_.syncPattern(node);
}

void ClipEditor::upgradeLegacy(const std::string& node) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !cm->pattern.present) return;
    clipops::upgradeLegacyClip(cm->pattern);
    host_.dirty_ = true;
    host_.syncPattern(node);
}

int ClipEditor::add(const std::string& node, int startTick, int lengthTicks) {
    host_.patterns().ensureNote(node);
    auto* cm = host_.mutableByName(node);
    if (!cm) return -1;
    const auto idx = clipops::noteChannels(cm->pattern);
    if (idx.size() == 1) {
        auto& ch = cm->pattern.channels[(size_t) idx[0]];
        if (ch.startTick < 0 && decodeNoteEvents(ch.matrix).empty()) {
            ch.startTick = std::max(0, startTick);
            ch.lengthTicks = std::max(1, lengthTicks);
            if (ch.id <= 0) ch.id = clipops::nextClipId(cm->pattern);
            host_.dirty_ = true;
            host_.syncPattern(node);
            return 0;
        }
    }
    const int clip = clipops::addClip(cm->pattern, startTick, lengthTicks);
    host_.dirty_ = true;
    host_.syncPattern(node);
    return clip;
}

int ClipEditor::addAudio(const std::string& node, int startTick, int lengthTicks,
                         const std::string& file, long long offsetSamples) {
    host_.patterns().ensureAudio(node);
    auto* cm = host_.mutableByName(node);
    if (!cm) return -1;
    const int clip = clipops::addAudioClip(cm->pattern, startTick, lengthTicks,
                                           file, offsetSamples);
    host_.dirty_ = true;
    host_.syncPattern(node);
    return clip;
}

void ClipEditor::removeTrack(const std::string& node) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return;
    host_.recordPatternRevert(node);
    auto& chans = cm->pattern.channels;
    chans.erase(std::remove_if(chans.begin(), chans.end(),
                               [](const PatternChannel& c) { return c.type == "note-events"; }),
                chans.end());
    host_.dirty_ = true;
    host_.syncPattern(node);
    host_.syncNodeTrack(node);
}

void ClipEditor::remove(const std::string& node, int clip) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !clipops::removeClip(cm->pattern, clip)) return;
    if (clipops::noteChannels(cm->pattern).empty() && cm->pattern.present) {
        bool audio = false;
        for (const auto& ch : cm->pattern.channels)
            if (ch.type == "audio-clip") audio = true;
        if (!audio) {
            PatternChannel seed;
            seed.type = "note-events";
            cm->pattern.channels.push_back(std::move(seed));
        }
    }
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::move(const std::string& node, int clip, int newStartTick) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return;
    clipops::upgradeLegacyClip(cm->pattern);
    if (!clipops::moveClip(cm->pattern, clip, newStartTick)) return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::resize(const std::string& node, int clip, int newLengthTicks, bool fromLeft) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return;
    clipops::upgradeLegacyClip(cm->pattern);
    if (!clipops::resizeClip(cm->pattern, clip, newLengthTicks, fromLeft, samplesPerTick()))
        return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

double ClipEditor::samplesPerTick() const {
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    return (60.0 / bpm) * host_.sampleRate_ / Pattern::kTicksPerBeat;
}

void ClipEditor::setLooped(const std::string& node, int clip, bool looped) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return;
    clipops::upgradeLegacyClip(cm->pattern);
    if (!clipops::setClipLooped(cm->pattern, clip, looped)) return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::rename(const std::string& node, int clip, const std::string& name) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !clipops::renameClip(cm->pattern, clip, name)) return;
    host_.dirty_ = true;
}

void ClipEditor::setColor(const std::string& node, int clip, int color) {
    auto* cm = host_.mutableByName(node);
    if (!cm || !clipops::setClipColor(cm->pattern, clip, color)) return;
    host_.dirty_ = true;
}

bool ClipEditor::accepts(const std::string& node, bool audio) {
    return host_.nodeRecordsAudio(node) == audio;
}

int ClipEditor::moveToNode(const std::string& node, int clip, const std::string& toNode) {
    if (node == toNode) return clip;
    auto* src = host_.mutableByName(node);
    if (!src) return -1;
    clipops::upgradeLegacyClip(src->pattern);
    const auto* ch = clipops::clipChannel(src->pattern, clip);
    if (!ch) return -1;
    const PatternChannel moved = *ch;

    if (!accepts(toNode, clipops::isAudioClip(moved))) return -1;

    if (clipops::isAudioClip(moved)) host_.patterns().ensureAudio(toNode);
    else                             host_.patterns().ensureNote(toNode);
    auto* dst = host_.mutableByName(toNode);
    if (!dst) return -1;
    int out;
    const auto idx = clipops::noteChannels(dst->pattern);
    if (idx.size() == 1 && dst->pattern.channels[(size_t) idx[0]].startTick < 0
        && decodeNoteEvents(dst->pattern.channels[(size_t) idx[0]].matrix).empty()) {
        dst->pattern.channels[(size_t) idx[0]] = moved;
        out = 0;
    } else {
        dst->pattern.channels.push_back(moved);
        out = clipops::clipCount(dst->pattern) - 1;
    }
    src = host_.mutableByName(node);
    if (src) clipops::removeClip(src->pattern, clip);
    host_.dirty_ = true;
    host_.syncPattern(node);
    host_.syncPattern(toNode);
    return out;
}

PatternChannel ClipEditor::copyClip(const std::string& node, int clip) const {
    if (const auto* cm = host_.model_.byName(node)) {
        auto pat = cm->pattern;
        clipops::upgradeLegacyClip(pat);
        if (const auto* ch = clipops::clipChannel(pat, clip)) return *ch;
    }
    return {};
}

int ClipEditor::pasteClip(const std::string& node, const PatternChannel& data, int atTick) {
    if (!clipops::isClipType(data)) return -1;
    if (!accepts(node, clipops::isAudioClip(data))) return -1;
    if (clipops::isAudioClip(data)) host_.patterns().ensureAudio(node);
    else                            host_.patterns().ensureNote(node);
    auto* cm = host_.mutableByName(node);
    if (!cm) return -1;
    PatternChannel ch = data;
    ch.startTick = std::max(0, atTick);
    if (const auto seeds = clipops::noteChannels(cm->pattern); seeds.size() == 1) {
        auto& seed = cm->pattern.channels[(size_t) seeds[0]];
        if (seed.startTick < 0 && decodeNoteEvents(seed.matrix).empty()) {
            ch.id = clipops::nextClipId(cm->pattern);
            seed = std::move(ch);
            host_.dirty_ = true;
            host_.syncPattern(node);
            return 0;
        }
    }
    clipops::upgradeLegacyClip(cm->pattern);
    ch.id = clipops::nextClipId(cm->pattern);
    cm->pattern.channels.push_back(std::move(ch));
    host_.dirty_ = true;
    host_.syncPattern(node);
    return clipops::clipCount(cm->pattern) - 1;
}

int ClipEditor::duplicate(const std::string& node, int clip, int newStartTick) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return -1;
    clipops::upgradeLegacyClip(cm->pattern);
    const int out = clipops::duplicateClip(cm->pattern, clip, newStartTick);
    if (out >= 0) { host_.dirty_ = true; host_.syncPattern(node); }
    return out;
}

int ClipEditor::split(const std::string& node, int clip, int atTick) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return -1;
    clipops::upgradeLegacyClip(cm->pattern);
    const int out = clipops::splitClip(cm->pattern, clip, atTick, samplesPerTick());
    if (out >= 0) { host_.dirty_ = true; host_.syncPattern(node); }
    return out;
}

int ClipEditor::join(const std::string& node, int a, int b) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return -1;
    clipops::upgradeLegacyClip(cm->pattern);
    const int out = clipops::joinClips(cm->pattern, a, b, samplesPerTick());
    if (out >= 0) { host_.dirty_ = true; host_.syncPattern(node); }
    return out;
}

namespace {

juce::AudioBuffer<float> renderParts(const std::vector<PatternChannel>& parts, int start, int end,
                                     double spt, double sr) {
    const int total = std::max(1, (int) std::llround((end - start) * spt));
    juce::AudioBuffer<float> out(2, total);
    out.clear();
    for (const auto& ch : parts) {
        juce::AudioBuffer<float> src;
        double fsr = 0.0;
        if (!loadSoundFile(ch.audioFile, src, fsr) || src.getNumSamples() == 0) continue;
        const double ratio = fsr > 0.0 ? fsr / sr : 1.0;
        const int dst0 = (int) std::llround((ch.startTick - start) * spt);
        const int n = (int) std::llround(ch.lengthTicks * spt);
        const int fadeIn = (int) std::llround(ch.fadeInTicks * spt);
        const int fadeOut = (int) std::llround(ch.fadeOutTicks * spt);
        for (int i = 0; i < n && dst0 + i < total; ++i) {
            const double pos = (double) ch.audioOffset + (ch.audioReverse ? (n - 1 - i) : i) * ratio;
            const int a = (int) pos;
            if (a < 0 || a >= src.getNumSamples()) continue;
            const int b = std::min(a + 1, src.getNumSamples() - 1);
            const float fr = (float) (pos - a);
            float g = (float) ch.audioGain;
            if (fadeIn > 0 && i < fadeIn) g *= (float) i / (float) fadeIn;
            if (fadeOut > 0 && n - 1 - i < fadeOut) g *= (float) (n - 1 - i) / (float) fadeOut;
            for (int c = 0; c < 2; ++c) {
                const float* sp = src.getReadPointer(std::min(c, src.getNumChannels() - 1));
                out.addSample(c, dst0 + i, (sp[a] * (1.0f - fr) + sp[b] * fr) * g);
            }
        }
    }
    return out;
}

std::string writeTake(const juce::File& dir, const std::string& base,
                      const juce::AudioBuffer<float>& buf, double sr) {
    dir.createDirectory();
    std::vector<std::string> existing;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        existing.push_back(f.getFileName().toStdString());
    const auto path = dir.getChildFile(juce::String(rec::nextTakeName(existing, base)))
                          .getFullPathName().toStdString();
    return writeSoundFile(path, buf, sr) ? path : std::string();
}

}

int ClipEditor::mergeAudio(const std::string& node, const std::vector<int>& ordinals) {
    auto* cm = host_.mutableByName(node);
    if (!cm || ordinals.size() < 2) return -1;
    clipops::upgradeLegacyClip(cm->pattern);
    std::vector<PatternChannel> parts;
    int start = INT_MAX, end = 0;
    for (int o : ordinals)
        if (const auto* ch = clipops::clipChannel(cm->pattern, o); ch && clipops::isAudioClip(*ch)) {
            parts.push_back(*ch);
            start = std::min(start, ch->startTick);
            end = std::max(end, ch->startTick + ch->lengthTicks);
        }
    if (parts.size() < 2 || end <= start) return -1;
    const auto out = renderParts(parts, start, end, samplesPerTick(), host_.sampleRate_);
    const auto path = writeTake(host_.record_.recordingsDir(), node + "-merged", out, host_.sampleRate_);
    if (path.empty()) return -1;
    std::vector<int> ids;
    for (const auto& ch : parts) ids.push_back(ch.id);
    for (int id : ids)
        if (const int at = clipops::clipIndexOfId(cm->pattern, id); at >= 0)
            clipops::removeClip(cm->pattern, at);
    const int made = clipops::addAudioClip(cm->pattern, start, end - start, path, 0);
    host_.dirty_ = true;
    host_.syncPattern(node);
    return made;
}

std::string ClipEditor::exportFile(const std::string& node, int clip) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return {};
    clipops::upgradeLegacyClip(cm->pattern);
    const auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (!ch || !clipops::isAudioClip(*ch) || ch->audioFile.empty()) return {};
    const double spt = samplesPerTick();
    const double sr = host_.sampleRate_;
    const bool plain = ch->audioOffset == 0 && ch->audioGain == 1.0 && ch->fadeInTicks == 0
                       && ch->fadeOutTicks == 0 && !ch->audioReverse && ch->audioPitch == 0.0
                       && ch->fadeInCurve == 0.0 && ch->fadeOutCurve == 0.0;
    if (plain) {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(
            juce::File(juce::String(juce::CharPointer_UTF8(ch->audioFile.c_str())))));
        if (r && r->sampleRate > 0.0) {
            const double fileHostSamples = (double) r->lengthInSamples * sr / r->sampleRate;
            if (std::abs(fileHostSamples - ch->lengthTicks * spt) <= std::max(2.0, spt))
                return ch->audioFile;
        }
    }
    const auto out = renderParts({*ch}, ch->startTick, ch->startTick + ch->lengthTicks, spt, sr);
    return writeTake(host_.record_.recordingsDir(), node + "-clip", out, sr);
}

std::vector<NoteEvent> ClipEditor::notes(const std::string& node, int clip) const {
    if (const auto* cm = host_.model_.byName(node))
        if (const auto* ch = clipops::clipChannel(cm->pattern, clip))
            return decodeNoteEvents(ch->matrix);
    return {};
}

void ClipEditor::transpose(const std::string& node, int clip, int steps) {
    auto* cm = host_.mutableByName(node);
    if (cm == nullptr || !clipops::transposeClip(cm->pattern, clip, steps)) return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::quantise(const std::string& node, int clip, int gridTicks) {
    auto* cm = host_.mutableByName(node);
    if (cm == nullptr || !clipops::quantiseClip(cm->pattern, clip, gridTicks)) return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void ClipEditor::nudgeVelocity(const std::string& node, int clip, int delta) {
    auto* cm = host_.mutableByName(node);
    if (cm == nullptr || !clipops::nudgeVelocity(cm->pattern, clip, delta)) return;
    host_.dirty_ = true;
    host_.syncPattern(node);
}

int ClipEditor::raise(const std::string& node, int clip) {
    auto* cm = host_.mutableByName(node);
    if (cm == nullptr) return -1;
    const int n = clipops::raiseClip(cm->pattern, clip);
    if (n < 0) return -1;
    host_.dirty_ = true;
    host_.syncPattern(node);
    return n;
}

void ClipEditor::setNotes(const std::string& node, int clip,
                          const std::vector<NoteEvent>& notes, int lengthTicks) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return;
    auto* ch = clipops::writableClipChannel(cm->pattern, clip);
    if (!ch && clip == 0) {
        host_.patterns().ensureNote(node);
        cm = host_.mutableByName(node);
        if (!cm) return;
        ch = clipops::writableClipChannel(cm->pattern, clip);
    }
    if (!ch) return;
    ch->matrix = replaceNoteEvents(ch->matrix, notes);
    if (lengthTicks > 0) {
        if (ch->startTick >= 0) ch->lengthTicks = lengthTicks;
        else cm->pattern.duration = lengthTicks;
    }
    host_.dirty_ = true;
    host_.syncPattern(node);
}

std::vector<CCEvent> ClipEditor::ccs(const std::string& node, int clip) const {
    if (const auto* cm = host_.model_.byName(node))
        if (const auto* ch = clipops::clipChannel(cm->pattern, clip))
            return decodeCCEvents(ch->matrix);
    return {};
}

void ClipEditor::setCCs(const std::string& node, int clip, const std::vector<CCEvent>& ccs) {
    auto* cm = host_.mutableByName(node);
    if (!cm) return;
    auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (!ch) return;
    auto sorted = ccs;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const CCEvent& a, const CCEvent& b) { return a.tick < b.tick; });
    ch->matrix = replaceCCEvents(ch->matrix, sorted);
    host_.dirty_ = true;
    host_.syncPattern(node);
}

void EngineHost::setSongLengthBeats(double beats) {
    model_.clock.songLength = std::max(0.0, beats);
    dirty_ = true;
}

double EngineHost::songEndBeat() const {
    if (model_.clock.songLength > 0.0) return model_.clock.songLength;
    double end = 0.0;
    for (const auto& c : model_.organisms) {
        if (!c.pattern.present) continue;
        for (int i = 0; i < clipops::clipCount(c.pattern); ++i)
            end = std::max(end, (clipops::clipStart(c.pattern, i)
                                 + clipops::clipLength(c.pattern, i))
                                    / (double) Pattern::kTicksPerBeat);
    }
    if (model_.clock.loopEnabled) end = std::max(end, model_.clock.loopEnd);
    return end;
}

bool ClipEditor::ownsPattern(const std::string& node) const {
    return host_.graph_ != nullptr
           && dynamic_cast<ClipArrangement*>(host_.graph_->find(node)) != nullptr;
}

bool ClipEditor::gridTarget(const std::string& node) const {
    const auto* cm = host_.model_.byName(node);
    if (!cm || ownsPattern(node)) return false;
    for (const auto& d : schemaFor(cm->classRaw)) if (d.name == "Note_1") return true;
    return false;
}

bool ClipEditor::adoptNotes(const std::string& src, int clipId, const std::string& dst) {
    const auto* sm = host_.model_.byName(src);
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
    for (const auto& d : schemaFor(host_.model_.byName(dst)->classRaw))
        if (d.name.rfind("Note_", 0) == 0) ++lanes;
    if (lanes <= 0) return false;
    host_.patterns().ensure(dst, lanes);
    for (int k = 0; k < lanes; ++k) host_.patterns().clearChannel(dst, k);
    host_.patterns().setDuration(dst, len);
    const auto* cm = host_.model_.byName(dst);
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
