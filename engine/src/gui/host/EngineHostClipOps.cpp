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

namespace {
int reelCount(const Pattern& p) {
    int n = 0;
    for (const auto& ch : p.channels) if (clipops::isCompoundClip(ch)) ++n;
    return n;
}


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

int ClipEditor::moveToNode(const std::string& node, int clip, const std::string& toNode) {
    if (node == toNode) return clip;
    auto* src = doc_.mutableByName(node);
    if (!src) return -1;
    clipops::upgradeLegacyClip(src->pattern);
    const auto* ch = clipops::clipChannel(src->pattern, clip);
    if (!ch) return -1;
    const PatternChannel moved = *ch;

    if (!accepts(toNode, moved)) return -1;

    if (clipops::isMediaClip(moved)) host_.patterns().ensureAudio(toNode);
    else                             host_.patterns().ensureNote(toNode);
    auto* dst = doc_.mutableByName(toNode);
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
    src = doc_.mutableByName(node);
    if (src) clipops::removeClip(src->pattern, clip);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    patternSync_.syncPattern(toNode);
    return out;
}

PatternChannel ClipEditor::copyClip(const std::string& node, int clip) const {
    if (const auto* cm = doc_.document().byName(node)) {
        auto pat = cm->pattern;
        clipops::upgradeLegacyClip(pat);
        if (const auto* ch = clipops::clipChannel(pat, clip)) return *ch;
    }
    return {};
}

int ClipEditor::pasteClip(const std::string& node, const PatternChannel& data, int atTick) {
    if (!clipops::isClipType(data)) return -1;
    if (!accepts(node, data)) return -1;
    if (clipops::isMediaClip(data)) host_.patterns().ensureAudio(node);
    else                            host_.patterns().ensureNote(node);
    auto* cm = doc_.mutableByName(node);
    if (!cm) return -1;
    PatternChannel ch = data;
    ch.startTick = std::max(0, atTick);
    if (const auto seeds = clipops::noteChannels(cm->pattern); seeds.size() == 1) {
        auto& seed = cm->pattern.channels[(size_t) seeds[0]];
        if (seed.startTick < 0 && decodeNoteEvents(seed.matrix).empty()) {
            ch.id = clipops::nextClipId(cm->pattern);
            seed = std::move(ch);
            doc_.flagDirty();
            patternSync_.syncPattern(node);
            return 0;
        }
    }
    clipops::upgradeLegacyClip(cm->pattern);
    ch.id = clipops::nextClipId(cm->pattern);
    cm->pattern.channels.push_back(std::move(ch));
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return clipops::clipCount(cm->pattern) - 1;
}

int ClipEditor::duplicate(const std::string& node, int clip, int newStartTick) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return -1;
    clipops::upgradeLegacyClip(cm->pattern);
    const int out = clipops::duplicateClip(cm->pattern, clip, newStartTick);
    if (out >= 0) { doc_.flagDirty(); patternSync_.syncPattern(node); }
    return out;
}

int ClipEditor::split(const std::string& node, int clip, int atTick) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return -1;
    clipops::upgradeLegacyClip(cm->pattern);
    const int out = clipops::splitClip(cm->pattern, clip, atTick, samplesPerTick());
    if (out >= 0) { doc_.flagDirty(); patternSync_.syncPattern(node); }
    return out;
}

int ClipEditor::join(const std::string& node, int a, int b) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return -1;
    clipops::upgradeLegacyClip(cm->pattern);
    const int out = clipops::joinClips(cm->pattern, a, b, samplesPerTick());
    if (out >= 0) { doc_.flagDirty(); patternSync_.syncPattern(node); }
    return out;
}

std::string ClipEditor::compoundReel(const std::string& node, int clip) const {
    const auto* cm = host_.model().byName(node);
    if (cm == nullptr) return {};
    const auto* ch = clipops::clipChannel(const_cast<Pattern&>(cm->pattern), clip);
    return ch != nullptr && clipops::isCompoundClip(*ch) ? ch->audioFile : std::string{};
}

int ClipEditor::makeCompound(const std::string& node, const std::vector<int>& ordinals) {
    auto* cm = doc_.mutableByName(node);
    if (cm == nullptr || ordinals.size() < 2) return -1;
    clipops::upgradeLegacyClip(cm->pattern);

    const double spt = samplesPerTick();
    struct Piece {
        PatternChannel ch;
        int atTick = 0;
    };
    std::vector<Piece> flat;
    std::vector<std::string> absorbed;
    std::vector<int> ids;
    int start = INT_MAX, end = 0;
    for (int o : ordinals) {
        const auto* ch = clipops::clipChannel(cm->pattern, o);
        if (ch == nullptr) continue;
        ids.push_back(ch->id);
        start = std::min(start, ch->startTick);
        end = std::max(end, ch->startTick + ch->lengthTicks);
        if (!clipops::isCompoundClip(*ch)) {
            flat.push_back({*ch, ch->startTick});
            continue;
        }
        const auto* reel = host_.model().byName(ch->audioFile);
        if (reel == nullptr) continue;
        int span = 0;
        for (const auto& inner : reel->pattern.channels)
            if (clipops::isClipType(inner))
                span = std::max(span, inner.startTick + inner.lengthTicks);
        const bool shaped = ch->fadeInTicks > 0 || ch->fadeOutTicks > 0 || ch->loopClip
                         || ch->audioReverse || ch->warpMode != 0 || ch->audioOffset != 0
                         || std::abs(ch->audioGain - 1.0) > 1.0e-9
                         || ch->lengthTicks != span;
        if (shaped) {
            flat.push_back({*ch, ch->startTick});
            continue;
        }
        absorbed.push_back(ch->audioFile);
        const int into = spt > 0.0 ? (int) std::llround((double) ch->audioOffset / spt) : 0;
        for (const auto& inner : reel->pattern.channels)
            if (clipops::isClipType(inner))
                flat.push_back({inner, ch->startTick + inner.startTick - into});
    }
    if (flat.size() < 2 || end <= start) return -1;

    const auto reelNode = nodes_.addOrganism(classWithRole(role::kVideoTrack), {0, 0});
    if (reelNode.empty()) return -1;
    nodes_.setNodeInternal(reelNode, true);
    cm = doc_.mutableByName(node);
    auto* inner = doc_.mutableByName(reelNode);
    if (cm == nullptr || inner == nullptr) return -1;
    inner->pattern = Pattern{};
    int nextId = 1;
    for (auto piece : flat) {
        piece.ch.id = nextId++;
        piece.ch.startTick = piece.atTick - start;
        inner->pattern.channels.push_back(std::move(piece.ch));
    }
    patternSync_.syncPattern(reelNode);

    for (int id : ids)
        if (const int at = clipops::clipIndexOfId(cm->pattern, id); at >= 0)
            clipops::removeClip(cm->pattern, at);
    const int made = clipops::addCompoundClip(cm->pattern, start, end - start, reelNode, 0);
    if (auto* ch = clipops::clipChannel(cm->pattern, made))
        ch->name = "Reel " + std::to_string(reelCount(cm->pattern));
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    for (const auto& gone : absorbed) nodes_.removeOrganism(gone);
    return made;
}

int ClipEditor::mergeAudio(const std::string& node, const std::vector<int>& ordinals) {
    auto* cm = doc_.mutableByName(node);
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
    const auto out = renderParts(parts, start, end, samplesPerTick(), audio_.sampleRate());
    const auto path = writeTake(recording_.recorder().recordingsDir(), node + "-merged", out, audio_.sampleRate());
    if (path.empty()) return -1;
    std::vector<int> ids;
    for (const auto& ch : parts) ids.push_back(ch.id);
    for (int id : ids)
        if (const int at = clipops::clipIndexOfId(cm->pattern, id); at >= 0)
            clipops::removeClip(cm->pattern, at);
    const int made = clipops::addAudioClip(cm->pattern, start, end - start, path, 0);
    doc_.flagDirty();
    patternSync_.syncPattern(node);
    return made;
}

std::string ClipEditor::exportFile(const std::string& node, int clip) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return {};
    clipops::upgradeLegacyClip(cm->pattern);
    const auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (!ch || !clipops::isAudioClip(*ch) || ch->audioFile.empty()) return {};
    const double spt = samplesPerTick();
    const double sr = audio_.sampleRate();
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
    return writeTake(recording_.recorder().recordingsDir(), node + "-clip", out, sr);
}

std::string ClipEditor::stretchAudioFile(const std::string& node, int clip, double factor) {
    auto* cm = doc_.mutableByName(node);
    if (!cm) return {};
    clipops::upgradeLegacyClip(cm->pattern);
    const auto* ch = clipops::clipChannel(cm->pattern, clip);
    if (!ch || !clipops::isAudioClip(*ch)) return {};
    factor = std::clamp(factor, 1.0, 100.0);
    const double spt = samplesPerTick();
    const double sr = audio_.sampleRate();
    auto src = renderParts({*ch}, ch->startTick, ch->startTick + ch->lengthTicks, spt, sr);
    const int maxSource = (int) (1200.0 / factor * sr);
    if (src.getNumSamples() > maxSource)
        src.setSize(src.getNumChannels(), maxSource, true, true, false);
    if (src.getNumSamples() < 1) return {};
    const auto stretched = paulstretchRender(src, sr, factor);
    if (stretched.getNumSamples() < 1) return {};
    return writeTake(recording_.recorder().recordingsDir(), node + "-stretched", stretched, sr);
}

}
