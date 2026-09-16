// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/dsp/MelodyTrace.h"
#include "gui/tracks/MediaInfo.h"
#include "gui/video/VideoProbe.h"
#include "gui/tracks/SongView.h"
#include <cmath>
#include <juce_audio_formats/juce_audio_formats.h>
#include "gui/common/Localisation.h"
#include "hum/dsp/DspMath.h"

namespace hum {

class MelodyTraceJob : public juce::ThreadWithProgressWindow {
public:
    MelodyTraceJob(std::unique_ptr<juce::AudioFormatReader> reader, juce::int64 from, int want,
                   double ticksPerSecond, juce::Component::SafePointer<SongView> pane,
                   ClipEditor::ClipInfo clip)
        : juce::ThreadWithProgressWindow(tr("tracks-pane-menu.convert-to-midi-track-2", "Convert to MIDI Track"), true, true),
          reader_(std::move(reader)), from_(from), want_(want),
          ticksPerSecond_(ticksPerSecond), pane_(pane), clip_(std::move(clip)) {
        setStatusMessage(tr("tracks-pane-menu.reading-the-take", "Reading the take..."));
    }

    void run() override {
        std::vector<float> mono((size_t) want_, 0.0f);
        const int channels = std::max(1, (int) reader_->numChannels);
        juce::AudioBuffer<float> block(channels, std::min(want_, 1 << 16));
        for (int at = 0; at < want_ && !threadShouldExit();) {
            const int take = std::min(block.getNumSamples(), want_ - at);
            if (!reader_->read(&block, 0, take, from_ + at, true, true)) return;
            for (int c = 0; c < channels; ++c) {
                const float* src = block.getReadPointer(std::min(c, block.getNumChannels() - 1));
                for (int i = 0; i < take; ++i) mono[(size_t) (at + i)] += src[i];
            }
            at += take;
            setProgress(0.2 * (double) at / (double) want_);
        }
        if (threadShouldExit()) return;
        const float norm = 1.0f / (float) channels;
        for (auto& v : mono) v *= norm;

        setStatusMessage(tr("tracks-pane-menu.listening-for-the-melody", "Listening for the melody..."));
        notes_ = melodytrace::trace(mono.data(), want_, reader_->sampleRate, ticksPerSecond_,
                                    [this](double at) {
            setProgress(0.2 + 0.8 * at);
            return !threadShouldExit();
        });
    }

    void threadComplete(bool userPressedCancel) override {
        if (!userPressedCancel && pane_ != nullptr && !notes_.empty())
            pane_->placeTracedMelody(clip_, notes_);
        else if (!userPressedCancel && notes_.empty())
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon, "Convert to MIDI Track",
                "No melody came out of that clip - it may be percussive, noisy or silent.");
        delete this;
    }

private:
    std::unique_ptr<juce::AudioFormatReader> reader_;
    juce::int64 from_ = 0;
    int want_ = 0;
    double ticksPerSecond_ = 1.0;
    juce::Component::SafePointer<SongView> pane_;
    ClipEditor::ClipInfo clip_;
    std::vector<NoteEvent> notes_;
};

bool SongView::consolidateRange(int row, double& from, double& to) const {
    if (view_.sel.active && view_.sel.to > view_.sel.from) {
        from = view_.sel.from;
        to = view_.sel.to;
        return true;
    }
    from = to = 0.0;
    bool any = false;
    for (const auto& ci : host().clips().list(rows_[(size_t) row])) {
        const double s = ci.startTick / (double) Pattern::kTicksPerBeat;
        const double e = (ci.startTick + std::max(1, ci.lengthTicks))
                             / (double) Pattern::kTicksPerBeat;
        from = any ? std::min(from, s) : s;
        to = any ? std::max(to, e) : e;
        any = true;
    }
    if (!any) { from = 0.0; to = host().songEndBeat(); }
    return to > from;
}

void SongView::consolidateRow(int row) {
    if (row < 0 || row >= (int) rows_.size()) return;
    const auto node = rows_[(size_t) row];
    double from = 0.0, to = 0.0;
    if (!consolidateRange(row, from, to)) return;

    juce::MouseCursor::showWaitCursor();
    std::string error;
    const auto track = host().consolidate(node, from, to, error);
    juce::MouseCursor::hideWaitCursor();

    if (track.empty()) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Consolidate", juce::String(error));
        return;
    }
    rebuild();
    repaintAll();
    ctx_.patchChanged();
}

void SongView::convertClipToMidi(const std::string&,
                                   const ClipEditor::ClipInfo& ci) {
    std::string uri = ci.audioFile;
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(f));
    if (!rd || rd->lengthInSamples <= 0 || rd->sampleRate <= 0.0) return;

    const double bpm = host().tempo() > 0.0 ? host().tempo() : 120.0;
    const double ticksPerSecond = bpm / kSecondsPerMinute * Pattern::kTicksPerBeat;
    const double hostSr = host().sampleRate() > 0.0 ? host().sampleRate() : kDefaultSampleRate;
    const double seconds = std::min(600.0, (double) ci.lengthTicks / ticksPerSecond);
    const auto from = (juce::int64) ((double) ci.audioOffset * rd->sampleRate / hostSr);
    const int want = (int) std::min<juce::int64>(
        (juce::int64) std::llround(seconds * rd->sampleRate),
        rd->lengthInSamples - from);
    if (want <= 0) return;

    (new MelodyTraceJob(std::move(rd), from, want, ticksPerSecond, this, ci))->launchThread();
}

void SongView::stretchClip(const std::string& node, int clip,
                             const ClipEditor::ClipInfo& ci, double factor) {
    const auto path = host().clips().stretchAudioFile(node, clip, factor);
    if (path.empty()) return;
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(
        juce::File(juce::String(juce::CharPointer_UTF8(path.c_str())))));
    if (!rd || rd->sampleRate <= 0.0) return;
    const double bpm = host().tempo() > 0.0 ? host().tempo() : 120.0;
    const int ticks = std::max(1, (int) std::llround((double) rd->lengthInSamples
                                                     / rd->sampleRate * bpm / kSecondsPerMinute
                                                     * Pattern::kTicksPerBeat));
    host().pushUndo();
    const auto dest = addTrack(true, {});
    if (dest.empty()) return;
    const int made = host().clips().addAudio(dest, ci.startTick, ticks, path);
    if (made >= 0)
        host().clips().rename(dest, made,
                             (ci.name.empty() ? std::string("stretched") : ci.name)
                                 + " x" + std::to_string((int) factor));
    rebuild();
    repaintAll();
}

int SongView::clipsUnderPlayhead() const {
    const int at = (int) std::llround(view_.playBeat * Pattern::kTicksPerBeat);
    int found = 0;
    for (const auto& node : rows_)
        for (const auto& ci : host().clips().list(node))
            if (!ci.looped && at > ci.startTick && at < ci.startTick + ci.lengthTicks) ++found;
    return found;
}

int SongView::cutAtPlayhead() {
    const int at = (int) std::llround(view_.playBeat * Pattern::kTicksPerBeat);
    std::vector<std::pair<std::string, int>> victims;
    for (const auto& node : rows_)
        for (const auto& ci : host().clips().list(node))
            if (!ci.looped && at > ci.startTick && at < ci.startTick + ci.lengthTicks)
                victims.emplace_back(node, ci.id);
    if (victims.empty()) return 0;
    host().beginTransaction();
    host().pushUndo();
    int cuts = 0;
    for (const auto& [node, id] : victims) {
        const int idx = clipIndexOfId(node, id);
        if (idx >= 0 && host().clips().split(node, idx, at) >= 0) ++cuts;
    }
    host().endTransaction();
    clearClipSel();
    rebuild();
    repaintAll();
    return cuts;
}

}
