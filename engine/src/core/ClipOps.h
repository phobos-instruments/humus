#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum::clipops {

inline bool isClipType(const PatternChannel& ch) {
    return ch.type == "note-events" || ch.type == "audio-clip";
}
inline bool isAudioClip(const PatternChannel& ch) { return ch.type == "audio-clip"; }

inline bool isSeedChannel(const PatternChannel& ch) {
    return ch.type == "note-events" && ch.startTick < 0 && ch.matrix.empty();
}

inline std::vector<int> clipChannels(const Pattern& p) {
    std::vector<int> out;
    for (int i = 0; i < (int) p.channels.size(); ++i)
        if (isClipType(p.channels[(size_t) i]) && !isSeedChannel(p.channels[(size_t) i]))
            out.push_back(i);
    return out;
}

inline std::vector<int> noteChannels(const Pattern& p) {
    std::vector<int> out;
    for (int i = 0; i < (int) p.channels.size(); ++i)
        if (p.channels[(size_t) i].type == "note-events") out.push_back(i);
    return out;
}

inline PatternChannel* clipChannel(Pattern& p, int clip) {
    const auto idx = clipChannels(p);
    if (clip < 0 || clip >= (int) idx.size()) return nullptr;
    return &p.channels[(size_t) idx[(size_t) clip]];
}
inline const PatternChannel* clipChannel(const Pattern& p, int clip) {
    return clipChannel(const_cast<Pattern&>(p), clip);
}

inline int clipCount(const Pattern& p) { return (int) clipChannels(p).size(); }

inline PatternChannel* writableClipChannel(Pattern& p, int clip) {
    if (auto* ch = clipChannel(p, clip)) return ch;
    if (clip != 0) return nullptr;
    for (auto& ch : p.channels)
        if (isSeedChannel(ch)) return &ch;
    return nullptr;
}

inline int clipStart(const Pattern& p, int clip) {
    const auto* ch = clipChannel(p, clip);
    return ch && ch->startTick >= 0 ? ch->startTick : 0;
}

inline int clipLength(const Pattern& p, int clip) {
    const auto* ch = clipChannel(p, clip);
    if (!ch) return 0;
    if (ch->startTick >= 0 && ch->lengthTicks > 0) return ch->lengthTicks;
    return p.duration > 0 ? p.duration : 4 * 4 * Pattern::kTicksPerBeat;
}

inline bool clipLooped(const Pattern& p, int clip) {
    const auto* ch = clipChannel(p, clip);
    return ch && (ch->startTick < 0 || ch->loopClip);
}

inline bool clipIsLegacy(const Pattern& p, int clip) {
    const auto* ch = clipChannel(p, clip);
    return ch && ch->startTick < 0;
}

inline int nextClipId(const Pattern& p) {
    int top = 0;
    for (const auto& ch : p.channels) top = std::max(top, ch.id);
    return top + 1;
}

inline bool ensureClipIds(Pattern& p) {
    bool changed = false;
    for (const auto& i : clipChannels(p)) {
        auto& ch = p.channels[(size_t) i];
        if (ch.id > 0) continue;
        ch.id = nextClipId(p);
        changed = true;
    }
    return changed;
}

inline int clipIndexOfId(const Pattern& p, int id) {
    if (id <= 0) return -1;
    const auto idx = clipChannels(p);
    for (int i = 0; i < (int) idx.size(); ++i)
        if (p.channels[(size_t) idx[(size_t) i]].id == id) return i;
    return -1;
}

inline int clipId(const Pattern& p, int clip) {
    const auto* ch = clipChannel(p, clip);
    return ch != nullptr ? ch->id : 0;
}

inline void upgradeLegacyClip(Pattern& p) {
    for (auto& ch : p.channels) {
        if (ch.type != "note-events" || ch.startTick >= 0) continue;
        if (isSeedChannel(ch)) continue;
        ch.startTick = 0;
        ch.lengthTicks = p.duration > 0 ? p.duration : 4 * 4 * Pattern::kTicksPerBeat;
        ch.loopClip = true;
        if (ch.id <= 0) ch.id = nextClipId(p);
    }
}

inline int addClip(Pattern& p, int startTick, int lengthTicks) {
    p.present = true;
    if (p.duration <= 0) p.duration = 4 * 4 * Pattern::kTicksPerBeat;
    PatternChannel ch;
    ch.id = nextClipId(p);
    ch.type = "note-events";
    ch.startTick = std::max(0, startTick);
    ch.lengthTicks = std::max(1, lengthTicks);
    p.channels.push_back(std::move(ch));
    return clipCount(p) - 1;
}

inline int addAudioClip(Pattern& p, int startTick, int lengthTicks,
                        const std::string& file, std::int64_t offsetFileSamples = 0,
                        double gain = 1.0) {
    p.present = true;
    if (p.duration <= 0) p.duration = 4 * 4 * Pattern::kTicksPerBeat;
    PatternChannel ch;
    ch.id = nextClipId(p);
    ch.type = "audio-clip";
    ch.startTick = std::max(0, startTick);
    ch.lengthTicks = std::max(1, lengthTicks);
    ch.audioFile = file;
    ch.audioOffset = std::max<std::int64_t>(0, offsetFileSamples);
    ch.audioGain = gain;
    p.channels.push_back(std::move(ch));
    return clipCount(p) - 1;
}

inline bool removeClip(Pattern& p, int clip) {
    const auto idx = clipChannels(p);
    if (clip < 0 || clip >= (int) idx.size()) return false;
    p.channels.erase(p.channels.begin() + idx[(size_t) clip]);
    return true;
}

inline bool moveClip(Pattern& p, int clip, int newStartTick) {
    auto* ch = clipChannel(p, clip);
    if (!ch) return false;
    ch->startTick = std::max(0, newStartTick);
    return true;
}

inline bool resizeClip(Pattern& p, int clip, int newLengthTicks, bool fromLeft,
                       double samplesPerTick = 0.0) {
    auto* ch = clipChannel(p, clip);
    if (!ch) return false;
    const int oldLen = clipLength(p, clip);
    const int len = std::max(1, newLengthTicks);
    if (fromLeft) {
        const int shift = oldLen - len;
        ch->startTick = std::max(0, clipStart(p, clip) + shift);
        if (isAudioClip(*ch)) {
            if (!ch->audioReverse)
                ch->audioOffset = std::max<std::int64_t>(
                    0, ch->audioOffset + (std::int64_t) ((double) shift * samplesPerTick));
        } else {
            auto notes = decodeNoteEvents(ch->matrix);
            std::vector<NoteEvent> kept;
            for (auto n : notes) {
                n.tick -= shift;
                if (n.tick >= 0) kept.push_back(n);
            }
            auto ccs = decodeCCEvents(ch->matrix);
            std::vector<CCEvent> keptCC;
            for (auto c : ccs) {
                c.tick -= shift;
                if (c.tick >= 0) keptCC.push_back(c);
            }
            ch->matrix = encodeNoteEvents(kept) + encodeCCEvents(keptCC);
        }
    }
    if (!fromLeft && isAudioClip(*ch) && ch->audioReverse)
        ch->audioOffset = std::max<std::int64_t>(
            0, ch->audioOffset + (std::int64_t) ((double) (oldLen - len) * samplesPerTick));
    ch->lengthTicks = len;
    return true;
}

inline bool setClipLooped(Pattern& p, int clip, bool looped) {
    auto* ch = clipChannel(p, clip);
    if (!ch) return false;
    ch->loopClip = looped;
    return true;
}

inline bool renameClip(Pattern& p, int clip, const std::string& name) {
    auto* ch = clipChannel(p, clip);
    if (!ch) return false;
    ch->name = name;
    return true;
}

inline bool setClipColor(Pattern& p, int clip, int color) {
    auto* ch = clipChannel(p, clip);
    if (!ch) return false;
    ch->color = std::max(0, color);
    return true;
}

inline int splitClip(Pattern& p, int clip, int atAbsTick, double samplesPerTick = 0.0) {
    auto* ch = clipChannel(p, clip);
    if (!ch) return -1;
    const int start = clipStart(p, clip);
    const int len = clipLength(p, clip);
    const int cut = atAbsTick - start;
    if (cut <= 0 || cut >= len) return -1;

    PatternChannel rch;
    rch.type = ch->type;
    rch.id = nextClipId(p);
    rch.startTick = start + cut;
    rch.lengthTicks = len - cut;
    rch.name = ch->name;
    rch.color = ch->color;
    rch.fadeOutTicks = ch->fadeOutTicks;
    ch->fadeOutTicks = 0;
    if (isAudioClip(*ch)) {
        rch.audioFile = ch->audioFile;
        rch.audioGain = ch->audioGain;
        rch.sourceBpm = ch->sourceBpm;
        rch.warpMode = ch->warpMode;
        rch.audioReverse = ch->audioReverse;
        rch.audioPitch = ch->audioPitch;
        if (ch->audioReverse) {
            rch.audioOffset = ch->audioOffset;
            ch->audioOffset += (std::int64_t) ((double) (len - cut) * samplesPerTick);
        } else {
            rch.audioOffset = ch->audioOffset + (std::int64_t) ((double) cut * samplesPerTick);
        }
    } else {
        auto notes = decodeNoteEvents(ch->matrix);
        std::vector<NoteEvent> left, right;
        for (auto n : notes) {
            if (n.tick < cut) {
                n.lengthTicks = std::min(n.lengthTicks, cut - n.tick);
                left.push_back(n);
            } else {
                n.tick -= cut;
                right.push_back(n);
            }
        }
        auto ccs = decodeCCEvents(ch->matrix);
        std::vector<CCEvent> leftCC, rightCC;
        for (auto c : ccs) {
            if (c.tick < cut) leftCC.push_back(c);
            else { c.tick -= cut; rightCC.push_back(c); }
        }
        ch->matrix = encodeNoteEvents(left) + encodeCCEvents(leftCC);
        rch.matrix = encodeNoteEvents(right) + encodeCCEvents(rightCC);
    }
    ch->startTick = start;
    ch->lengthTicks = cut;
    ch->loopClip = false;
    p.channels.push_back(std::move(rch));
    return clipCount(p) - 1;
}

inline int joinClips(Pattern& p, int a, int b, double samplesPerTick = 0.0) {
    if (a == b) return -1;
    auto* ca = clipChannel(p, a);
    auto* cb = clipChannel(p, b);
    if (!ca || !cb || ca->type != cb->type) return -1;
    if (cb->startTick < ca->startTick) std::swap(ca, cb), std::swap(a, b);
    if (isAudioClip(*ca) && (ca->loopClip || cb->loopClip)) return -1;
    if (isAudioClip(*ca) && ca->startTick + ca->lengthTicks != cb->startTick) return -1;
    auto unroll = [](PatternChannel& ch, int reach) {
        if (!ch.loopClip || ch.lengthTicks <= 0) return;
        const auto cycle = decodeNoteEvents(ch.matrix);
        const auto ccCycle = decodeCCEvents(ch.matrix);
        std::vector<NoteEvent> notes;
        std::vector<CCEvent> ccs;
        for (int off = 0; off < reach; off += ch.lengthTicks) {
            for (auto n : cycle) {
                if (n.tick + off >= reach) continue;
                n.tick += off;
                n.lengthTicks = std::min(n.lengthTicks, reach - n.tick);
                notes.push_back(n);
            }
            for (auto c : ccCycle) if (c.tick + off < reach) { c.tick += off; ccs.push_back(c); }
        }
        ch.matrix = replaceCCEvents(encodeNoteEvents(notes), ccs);
        ch.lengthTicks = reach;
        ch.loopClip = false;
    };
    if (ca->loopClip) unroll(*ca, std::max(1, cb->startTick - ca->startTick));
    if (cb->loopClip) unroll(*cb, cb->lengthTicks);
    if (isAudioClip(*ca)) {
        if (ca->audioFile != cb->audioFile || ca->audioReverse != cb->audioReverse
            || std::abs(ca->audioPitch - cb->audioPitch) > 1e-9)
            return -1;
        const auto seam = ca->audioReverse
            ? cb->audioOffset + (std::int64_t) ((double) cb->lengthTicks * samplesPerTick)
            : ca->audioOffset + (std::int64_t) ((double) ca->lengthTicks * samplesPerTick);
        const auto want = ca->audioReverse ? ca->audioOffset : cb->audioOffset;
        if (std::llabs(seam - want) > (std::int64_t) std::max(2.0, samplesPerTick)) return -1;
        if (ca->audioReverse) ca->audioOffset = cb->audioOffset;
        ca->lengthTicks += cb->lengthTicks;
    } else {
        const int shift = cb->startTick - ca->startTick;
        auto notes = decodeNoteEvents(ca->matrix);
        for (auto n : decodeNoteEvents(cb->matrix)) { n.tick += shift; notes.push_back(n); }
        auto ccs = decodeCCEvents(ca->matrix);
        for (auto c : decodeCCEvents(cb->matrix)) { c.tick += shift; ccs.push_back(c); }
        ca->matrix = replaceCCEvents(encodeNoteEvents(notes), ccs);
        ca->lengthTicks = std::max(ca->lengthTicks, shift + cb->lengthTicks);
    }
    ca->fadeOutTicks = cb->fadeOutTicks;
    const int idA = ca->id;
    removeClip(p, b);
    return clipIndexOfId(p, idA);
}

inline int duplicateClip(Pattern& p, int clip, int newStartTick) {
    const auto* src = clipChannel(p, clip);
    if (!src) return -1;
    PatternChannel ch;
    ch.type = src->type;
    ch.id = nextClipId(p);
    ch.startTick = std::max(0, newStartTick);
    ch.lengthTicks = clipLength(p, clip);
    ch.loopClip = src->startTick >= 0 && src->loopClip;
    ch.name = src->name;
    ch.color = src->color;
    ch.matrix = src->matrix;
    ch.audioFile = src->audioFile;
    ch.audioOffset = src->audioOffset;
    ch.audioGain = src->audioGain;
    ch.sourceBpm = src->sourceBpm;
    ch.warpMode = src->warpMode;
    ch.audioReverse = src->audioReverse;
    ch.audioPitch = src->audioPitch;
    ch.fadeInTicks = src->fadeInTicks;
    ch.fadeOutTicks = src->fadeOutTicks;
    p.channels.push_back(std::move(ch));
    return clipCount(p) - 1;
}

inline int raiseClip(Pattern& p, int clip) {
    const auto idx = clipChannels(p);
    if (clip < 0 || clip >= (int) idx.size()) return -1;
    auto ch = std::move(p.channels[(size_t) idx[(size_t) clip]]);
    p.channels.erase(p.channels.begin() + idx[(size_t) clip]);
    p.channels.push_back(std::move(ch));
    return clipCount(p) - 1;
}

inline bool transposeClip(Pattern& p, int clip, int steps) {
    auto* ch = clipChannel(p, clip);
    if (ch == nullptr || isAudioClip(*ch) || steps == 0) return false;
    auto notes = decodeNoteEvents(ch->matrix);
    if (notes.empty()) return false;
    for (auto& n : notes) n.pitch = std::max(0, std::min(127, n.pitch + steps));
    ch->matrix = replaceNoteEvents(ch->matrix, notes);
    return true;
}

inline bool quantiseClip(Pattern& p, int clip, int gridTicks) {
    auto* ch = clipChannel(p, clip);
    if (ch == nullptr || isAudioClip(*ch) || gridTicks <= 0) return false;
    auto notes = decodeNoteEvents(ch->matrix);
    if (notes.empty()) return false;
    for (auto& n : notes)
        n.tick = (int) std::llround((double) n.tick / gridTicks) * gridTicks;
    ch->matrix = replaceNoteEvents(ch->matrix, notes);
    return true;
}

inline bool nudgeVelocity(Pattern& p, int clip, int delta) {
    auto* ch = clipChannel(p, clip);
    if (ch == nullptr || isAudioClip(*ch) || delta == 0) return false;
    auto notes = decodeNoteEvents(ch->matrix);
    if (notes.empty()) return false;
    for (auto& n : notes) n.velocity = std::max(1, std::min(127, n.velocity + delta));
    ch->matrix = replaceNoteEvents(ch->matrix, notes);
    return true;
}

}
