// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/MediaInfo.h"
#include "gui/video/VideoProbe.h"
#include "gui/tracks/SongView.h"
#include "core/analysis/BeatDetector.h"
#include "core/packs/Roles.h"
#include <cmath>
#include <cstdint>
#include <juce_audio_formats/juce_audio_formats.h>
#include "core/midi/MidiClipImport.h"
#include "gui/common/Localisation.h"
#include "hum/dsp/DspMath.h"

namespace hum {

static const juce::String& audioWildcard() {
    static const juce::String w = [] {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        return fm.getWildcardForAllFormats();
    }();
    return w;
}

int SongView::placeAudioFile(const std::string& node, int atTick, const juce::File& f) {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
    if (!reader || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0) return -1;
    const double seconds = (double) reader->lengthInSamples / reader->sampleRate;
    const double bpm = host().tempo() > 0.0 ? host().tempo() : 120.0;
    const int ticks = std::max(1, (int) std::llround(
        seconds * (bpm / kSecondsPerMinute) * Pattern::kTicksPerBeat));
    const int clip = host().clips().addAudio(node, atTick, ticks, f.getFullPathName().toStdString());

    if (clip >= 0 && seconds >= 2.0) {
        juce::AudioBuffer<float> buf((int) reader->numChannels,
                                     (int) juce::jmin(reader->lengthInSamples,
                                                      (juce::int64) (reader->sampleRate * 30.0)));
        reader->read(&buf, 0, buf.getNumSamples(), 0, true, true);
        const auto est = detectBeat(buf, reader->sampleRate);
        if (est.confidence > 0.0) {
            host().clips().setSourceBpm(node, clip, est.bpm);
            host().clips().setWarp(node, clip, (int) PatternChannel::Warp::Tone);
        }
    }
    return clip;
}

void SongView::placeTracedMelody(const ClipEditor::ClipInfo& ci,
                                   const std::vector<NoteEvent>& notes) {
    host().pushUndo();
    const auto midiNode = addMidiTrack();
    if (midiNode.empty()) return;
    const int clip = host().clips().add(midiNode, ci.startTick, ci.lengthTicks);
    if (clip < 0) return;
    host().clips().setNotes(midiNode, clip, notes, 0);
    if (!ci.name.empty()) host().clips().rename(midiNode, clip, ci.name + " melody");
    rebuild();
    repaintAll();
    ctx_.patchChanged();
}

static bool isMidiFile(const juce::File& f) {
    return f.hasFileExtension("mid;midi");
}

bool SongView::isInterestedInFileDrag(const juce::StringArray& files) {
    static const juce::String exts = audioWildcard().replace("*.", "").replace(";*", ";");
    for (const auto& f : files)
        if (juce::File(f).hasFileExtension(exts) || isVideoFile(juce::File(f))
            || isMidiFile(juce::File(f))) return true;
    return false;
}

std::string SongView::dropTargetMidiNode(int y) {
    if (const int row = rowAt(y); row >= 0) {
        const auto& node = rows_[(size_t) row];
        if (const auto* cm = host().model().byName(node);
            cm != nullptr && classHasRole(cm->classRaw, role::kMidiTrack)) return node;
    }
    return addMidiTrack();
}

int SongView::placeMidiFile(const std::string& node, int atTick, const juce::File& f) {
    juce::MemoryBlock raw;
    if (!f.loadFileAsData(raw) || raw.getSize() == 0) return -1;
    const auto song = midiclip::importMidi(static_cast<const std::uint8_t*>(raw.getData()),
                                           raw.getSize());
    if (song.notes.empty()) return -1;
    const int clip = host().clips().add(node, atTick, song.lengthTicks);
    if (clip < 0) return -1;
    host().clips().setNotes(node, clip, song.notes, song.lengthTicks);
    host().clips().rename(node, clip, f.getFileNameWithoutExtension().toStdString());
    return clip;
}

std::string SongView::dropTargetNode(int y, bool video) {
    if (const int row = rowAt(y); row >= 0) {
        const auto& node = rows_[(size_t) row];
        if (video ? host().nodeArrangesVideo(node) : host().nodeRecordsAudio(node)) return node;
    }
    return host().addOrganism(classWithRole(video ? role::kVideoTrack : role::kAudioTrack), host().spotBelowPatch());
}

int SongView::placeVideoFile(const std::string& node, int atTick, const juce::File& f) {
    const double seconds = probeVideoSeconds(f);
    if (seconds <= 0.0) return -1;
    const double bpm = host().tempo() > 0.0 ? host().tempo() : 120.0;
    const int ticks = std::max(1, (int) std::llround(
        seconds * (bpm / kSecondsPerMinute) * Pattern::kTicksPerBeat));
    return host().clips().addVideo(node, atTick, ticks, f.getFullPathName().toStdString());
}

void SongView::filesDropped(const juce::StringArray& files, int x, int y) {
    setDropHot(false);
    const int atTick = (int) std::llround(
        snapBeats(std::max(0.0, xToBeat((float) x)), false) * Pattern::kTicksPerBeat);
    host().beginTransaction();
    host().pushUndo();
    int placed = 0, tick = atTick;
    for (const auto& path : files) {
        const juce::File f(path);
        if (!f.existsAsFile() || !isInterestedInFileDrag({path})) continue;
        const bool video = isVideoFile(f);
        const bool midi = isMidiFile(f);
        const auto node = midi ? dropTargetMidiNode(y) : dropTargetNode(y, video);
        if (node.empty()) continue;
        const int clip = midi    ? placeMidiFile(node, tick, f)
                         : video ? placeVideoFile(node, tick, f)
                                 : placeAudioFile(node, tick, f);
        if (clip < 0) continue;
        ++placed;
        for (const auto& ci : host().clips().list(node))
            if (ci.index == clip) tick = ci.startTick + ci.lengthTicks;
    }
    host().endTransaction();
    if (placed > 0) { rebuild(); repaintAll(); return; }
    const juce::File first(files[0]);
    const auto kind = isMidiFile(first)  ? tr("tracks-pane-menu.as-midi", "as MIDI")
                      : isVideoFile(first) ? tr("tracks-pane-menu.as-video", "as video")
                                           : tr("tracks-pane-menu.as-audio", "as audio");
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, tr("tracks-pane-menu.import", "Import"),
        files.size() == 1
            ? first.getFileName() + " " + tr("tracks-pane-menu.could-not-be-read", "could not be read")
                  + " " + kind + "."
            : tr("tracks-pane-menu.none-readable",
                 "None of those files could be read as audio or video."));
}

void SongView::importAudioInto(const std::string& node, int atTick) {
    chooser_ = std::make_unique<juce::FileChooser>(
        tr("tracks-pane-menu.import-audio-file", "Import Audio File"),
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        audioWildcard());
    chooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [this, node, atTick](const juce::FileChooser& fc) {
        const auto f = fc.getResult();
        if (!f.existsAsFile()) return;
        host().pushUndo();
        placeAudioFile(node, atTick, f);
        rebuild();
        repaintAll();
    });
}

}
