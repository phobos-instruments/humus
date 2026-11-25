// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHostSamples.h"

#include <algorithm>
#include <cstdint>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

#include "core/app/AppPaths.h"
#include "core/timeline/ClipOps.h"
#include "core/timeline/SampleRepair.h"
#include "gui/host/EngineHostRecord.h"
#include "gui/tracks/WaveformCache.h"
#include "hum/Organism.h"
#include "hum/caps/Files.h"
#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

namespace {

bool isAudioClip(const PatternChannel& ch) { return ch.type == "audio-clip" && !ch.audioFile.empty(); }

std::string localPath(std::string path) {
    if (path.rfind("file://", 0) == 0) path = path.substr(7);
    return path;
}

}

const PatternChannel* SampleEditor::clipOf(const std::string& node, int clip) const {
    const auto* cm = host_.model().byName(node);
    const auto* ch = cm != nullptr ? clipops::clipChannel(cm->pattern, clip) : nullptr;
    return ch != nullptr && isAudioClip(*ch) ? ch : nullptr;
}

SampleEditor::Working* SampleEditor::load(const std::string& file) {
    if (const auto it = working_.find(file); it != working_.end()) return &it->second;
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(fileAt(localPath(file))));
    if (!reader || reader->lengthInSamples <= 0 || reader->numChannels < 1) return nullptr;
    juce::AudioBuffer<float> buf((int) reader->numChannels, (int) reader->lengthInSamples);
    if (!reader->read(&buf, 0, buf.getNumSamples(), 0, true, true)) return nullptr;
    Working w;
    w.rate = reader->sampleRate;
    w.bits = reader->usesFloatingPointData ? 32 : (reader->bitsPerSample <= 16 ? 16 : 24);
    for (int c = 0; c < buf.getNumChannels(); ++c)
        w.pcm.emplace_back(buf.getReadPointer(c), buf.getReadPointer(c) + buf.getNumSamples());
    const auto& revisions = host_.model().audioRevisions;
    if (const auto r = revisions.find(file); r != revisions.end()) w.applied = r->second;
    return &working_.emplace(file, std::move(w)).first->second;
}

std::string SampleEditor::makeCopy(const std::string& original, const Working& source) {
    const auto name = fileAt(localPath(original)).getFileNameWithoutExtension() + "-edit";
    const auto dir = recording_.recorder().recordingsDir();
    dir.createDirectory();
    const auto copy = pathOf(dir.getNonexistentChildFile(name, ".wav", false));
    Working fresh;
    fresh.pcm = source.pcm;
    fresh.rate = source.rate;
    fresh.bits = source.bits;
    if (!save(copy, fresh)) return {};
    working_.emplace(copy, std::move(fresh));
    return copy;
}

std::string SampleEditor::prepare(std::string file, Working*& out) {
    auto& doc = doc_.document();
    const bool isCopy = doc.editedAudio.count(file) > 0;
    auto* source = load(file);
    if (source == nullptr) return {};
    const auto copy = isCopy ? file : makeCopy(file, *source);
    if (copy.empty()) return {};
    if (!isCopy) working_.erase(file);
    out = &working_.at(copy);
    doc.audioRevisions.emplace(copy, out->applied);
    host_.pushUndo();
    if (!isCopy) {
        doc.editedAudio[copy] = file;
        repoint(file, copy);
    }
    return copy;
}

bool SampleEditor::drawSamples(const std::string& node, int clip, std::int64_t start,
                               const std::vector<float>& mono) {
    const auto* ch = clipOf(node, clip);
    if (ch == nullptr || mono.empty()) return false;
    Working* w = nullptr;
    const auto copy = prepare(ch->audioFile, w);
    if (copy.empty()) return false;
    const auto length = (std::int64_t) w->pcm.front().size();
    const auto from = std::clamp<std::int64_t>(start, 0, length);
    const auto to = std::clamp<std::int64_t>(start + (std::int64_t) mono.size(), 0, length);
    if (to <= from) return false;
    Edit e;
    e.start = from;
    for (const auto& c : w->pcm) e.before.emplace_back(c.begin() + from, c.begin() + to);
    e.after = e.before;
    const float share = 1.0f / (float) w->pcm.size();
    for (auto s = from; s < to; ++s) {
        float now = 0.0f;
        for (const auto& c : e.before) now += c[(size_t) (s - from)] * share;
        const float delta = mono[(size_t) (s - start)] - now;
        for (auto& c : e.after) c[(size_t) (s - from)] += delta;
    }
    return commit(copy, *w, std::move(e));
}

bool SampleEditor::repairSamples(const std::string& node, int clip, std::int64_t start, int length) {
    const auto* ch = clipOf(node, clip);
    if (ch == nullptr || length <= 0) return false;
    Working* w = nullptr;
    const auto copy = prepare(ch->audioFile, w);
    if (copy.empty()) return false;
    Edit e;
    for (const auto& c : w->pcm) {
        auto rebuilt = samplefix::repairGap(c.data(), (std::int64_t) c.size(), start, length);
        if (rebuilt.empty()) return false;
        const auto from = std::clamp<std::int64_t>(start, 0, (std::int64_t) c.size());
        e.start = from;
        e.before.emplace_back(c.begin() + from, c.begin() + from + (std::int64_t) rebuilt.size());
        e.after.push_back(std::move(rebuilt));
    }
    return commit(copy, *w, std::move(e));
}

bool SampleEditor::commit(const std::string& copy, Working& w, Edit edit) {
    if (w.applied < (int) w.edits.size()) w.edits.resize((size_t) w.applied);
    put(w, edit, true);
    w.edits.push_back(std::move(edit));
    w.applied = (int) w.edits.size();
    doc_.document().audioRevisions[copy] = w.applied;
    doc_.flagDirty();
    return save(copy, w);
}

void SampleEditor::put(Working& w, const Edit& e, bool forward) {
    const auto& values = forward ? e.after : e.before;
    for (size_t c = 0; c < w.pcm.size() && c < values.size(); ++c)
        std::copy(values[c].begin(), values[c].end(), w.pcm[c].begin() + e.start);
}

bool SampleEditor::save(const std::string& copy, const Working& w) {
    juce::AudioBuffer<float> buf((int) w.pcm.size(), (int) w.pcm.front().size());
    for (int c = 0; c < buf.getNumChannels(); ++c)
        std::copy(w.pcm[(size_t) c].begin(), w.pcm[(size_t) c].end(), buf.getWritePointer(c));
    if (!writeSoundFile(localPath(copy), buf, w.rate, -1, w.bits)) return false;
    ++stamps_[copy];
    WaveformCache::instance().forget(copy);
    for (const auto& cm : host_.model().organisms) {
        auto* live = host_.liveOrganism(cm.name);
        if (auto* watcher = dynamic_cast<AudioFileWatcher*>(live)) watcher->audioFileChanged(copy);
        if (auto* clips = dynamic_cast<ClipRecorder*>(live)) clips->ensureClipsLoaded();
    }
    return true;
}

void SampleEditor::repoint(const std::string& from, const std::string& to) {
    for (auto& cm : doc_.document().organisms) {
        bool changed = false;
        for (auto& ch : cm.pattern.channels)
            if (isAudioClip(ch) && ch.audioFile == from) {
                ch.audioFile = to;
                changed = true;
            }
        if (changed) patternSync_.syncPattern(cm.name);
    }
    doc_.flagDirty();
}

bool SampleEditor::restoreOriginal(const std::string& node, int clip) {
    const auto* ch = clipOf(node, clip);
    if (ch == nullptr) return false;
    const auto& edited = host_.model().editedAudio;
    const auto it = edited.find(ch->audioFile);
    if (it == edited.end() || it->second.empty()) return false;
    const auto copy = it->first, original = it->second;
    host_.pushUndo();
    repoint(copy, original);
    return true;
}

bool SampleEditor::canRestore(const std::string& node, int clip) const {
    const auto* ch = clipOf(node, clip);
    return ch != nullptr && host_.model().editedAudio.count(ch->audioFile) > 0;
}

unsigned SampleEditor::contentStamp(const std::string& file) const {
    const auto it = stamps_.find(file);
    return it != stamps_.end() ? it->second : 0u;
}

void SampleEditor::syncRevisions() {
    const auto& revisions = host_.model().audioRevisions;
    for (auto& [copy, w] : working_) {
        const auto it = revisions.find(copy);
        if (it == revisions.end() || it->second == w.applied) continue;
        const int want = std::clamp(it->second, 0, (int) w.edits.size());
        while (w.applied > want) put(w, w.edits[(size_t) --w.applied], false);
        while (w.applied < want) put(w, w.edits[(size_t) w.applied++], true);
        save(copy, w);
    }
}

}
