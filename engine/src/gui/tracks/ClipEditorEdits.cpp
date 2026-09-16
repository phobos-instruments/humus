// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/ClipEditorView.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "gui/common/Localisation.h"
#include "gui/host/EngineHostAutomation.h"
#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/tracks/TracksGeometry.h"
#include "gui/tracks/WaveformCache.h"
#include "io/PatchDocument.h"

namespace hum {

void ClipEditorView::zoomToClip() {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    const double beats = std::max(0.25, ci.lengthTicks / (double) Pattern::kTicksPerBeat);
    const double w = std::max(60, getWidth() - tracksgeo::kStripW) * 0.9;
    view_.ppb = juce::jlimit(2.0, tracksgeo::kClipMaxPpb, w / beats);
    view_.scrollBeats = std::max(0.0, ci.startTick / (double) Pattern::kTicksPerBeat
                                     - (w / 0.9 - w) * 0.5 / view_.ppb);
    ctx_.viewChanged();
}

void ClipEditorView::zoomToSelection() {
    int from = 0, to = 0;
    if (!clipSelectionTicks(from, to)) { zoomToClip(); return; }
    const double beats = (to - from) / (double) Pattern::kTicksPerBeat;
    const double w = std::max(60, getWidth() - tracksgeo::kStripW) * 0.9;
    view_.ppb = juce::jlimit(2.0, tracksgeo::kClipMaxPpb, w / beats);
    view_.scrollBeats = std::max(0.0, from / (double) Pattern::kTicksPerBeat
                                     - (w / 0.9 - w) * 0.5 / view_.ppb);
    ctx_.viewChanged();
}

void ClipEditorView::splitClipAt(int tick) {
    const int c = clipOrdinal();
    if (c < 0) return;
    host().pushUndo();
    host().clips().split(node_, c, tick);
    repaint();
}

void ClipEditorView::splitClipSelection() {
    SelectionWatch watch(*this);
    int from = 0, to = 0;
    if (clipSelectionTicks(from, to)) {
        const int c = clipOrdinal();
        if (c < 0) return;
        host().beginTransaction();
        host().pushUndo();
        const int right = host().clips().split(node_, c, from);
        host().clips().split(node_, right >= 0 ? right : clipOrdinal(), to);
        host().endTransaction();
        view_.sel.active = false;
        repaint();
        return;
    }
    splitClipAt((int) std::llround(host().positionBeats() * Pattern::kTicksPerBeat));
}

bool ClipEditorView::deleteClipSelection(bool ripple) {
    SelectionWatch watch(*this);
    int from = 0, to = 0;
    const int c = clipOrdinal();
    if (c < 0 || !clipSelectionTicks(from, to)) return false;
    host().pushUndo();
    const bool ok = host().clips().removeRange(node_, c, from, to, ripple);
    view_.sel.active = false;
    if (clipOrdinal() < 0) ctx_.leaveClip();
    else repaint();
    if (ok) ctx_.patchChanged();
    return ok;
}

void ClipEditorView::trimClipToSelection() {
    SelectionWatch watch(*this);
    int from = 0, to = 0;
    const int c = clipOrdinal();
    if (c < 0 || !clipSelectionTicks(from, to)) return;
    host().pushUndo();
    host().clips().trimTo(node_, c, from, to);
    view_.sel.active = false;
    repaint();
}

void ClipEditorView::copyClipSelection() {
    int from = 0, to = 0;
    const int c = clipOrdinal();
    if (c < 0) return;
    ClipEditor::ClipInfo ci;
    if (!clipSelectionTicks(from, to)) {
        if (!clipInfo(ci)) return;
        from = ci.startTick; to = ci.startTick + ci.lengthTicks;
    }
    auto data = host().clips().copyRange(node_, c, from, to);
    if (data.audioFile.empty()) return;
    data.startTick = 0;
    ctx_.setClipboard(std::move(data), to - from);
}

bool ClipEditorView::pasteClipSelection(int atTick) {
    return ctx_.pasteClipboardInto(node_, atTick);
}

void ClipEditorView::straightenFade(bool in) {
    ClipEditor::ClipInfo ci;
    const int c = clipOrdinal();
    if (c < 0 || !clipInfo(ci)) return;
    host().pushUndo();
    host().clips().setFadeCurves(node_, c, in ? 0.0 : ci.fadeInCurve,
                                in ? ci.fadeOutCurve : 0.0);
    repaint();
}

void ClipEditorView::fadeClipToPlayhead(bool in) {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    const int at = (int) std::llround(host().positionBeats() * Pattern::kTicksPerBeat);
    int fi = ci.fadeInTicks, fo = ci.fadeOutTicks;
    if (in) fi = juce::jlimit(0, ci.lengthTicks, at - ci.startTick);
    else fo = juce::jlimit(0, ci.lengthTicks, ci.startTick + ci.lengthTicks - at);
    host().pushUndo();
    host().clips().setFades(node_, ci.index, fi, fo);
    repaint();
}

void ClipEditorView::setClipGainDb(double db) {
    const int c = clipOrdinal();
    if (c < 0) return;
    host().pushUndo();
    host().clips().setGain(node_, c, std::pow(10.0, db / 20.0));
    repaint();
}

void ClipEditorView::loopClipSelection() {
    int from = 0, to = 0;
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    if (!clipSelectionTicks(from, to)) { from = ci.startTick; to = from + ci.lengthTicks; }
    host().automation().setLoop(from / (double) Pattern::kTicksPerBeat,
                               to / (double) Pattern::kTicksPerBeat, true);
    ctx_.viewChanged();
}

void ClipEditorView::toggleClipReverse() {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    host().pushUndo();
    host().clips().setReverse(node_, ci.index, !ci.audioReverse);
    repaint();
}

void ClipEditorView::normalizeClip() {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (!peaks || !peaks->ready || peaks->binSamples <= 0) return;
    const double toFile = (peaks->fileSampleRate > 0.0 ? peaks->fileSampleRate : host().sampleRate())
                          / host().sampleRate();
    const auto len = (long long) std::llround(ci.lengthTicks * samplesPerBeat() / Pattern::kTicksPerBeat);
    const int b0 = (int) ((double) ci.audioOffset * toFile / peaks->binSamples);
    const int b1 = (int) ((double) (ci.audioOffset + len) * toFile / peaks->binSamples) + 1;
    float peak = 0.0f;
    for (int b = std::max(0, b0); b < std::min(b1, (int) peaks->hi.size()); ++b)
        peak = std::max({peak, peaks->hi[(size_t) b], -peaks->lo[(size_t) b]});
    if (peak <= 1e-6f) return;
    host().pushUndo();
    host().clips().setGain(node_, ci.index, 0.98 / peak);
    repaint();
}

void ClipEditorView::stretchClipBy(double factor) {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci) || factor <= 0.0) return;
    host().pushUndo();
    host().clips().stretch(node_, ci.index, std::max(1, (int) std::llround(ci.lengthTicks * factor)));
    repaint();
}

void ClipEditorView::setClipPitch(double semitones) {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    host().pushUndo();
    host().clips().setPitch(node_, ci.index, semitones);
    repaint();
}

void ClipEditorView::showClipDetailMenu(juce::Point<int> screenPos) {
    enum { kSplit = 1, kDelete, kRipple, kTrim, kFadeIn, kFadeOut, kLoop, kZoomClip, kZoomSel,
           kSplitTransients, kReverse, kNormalize, kDouble, kHalf,
           kGain0 = 100, kSense0 = 200, kPitch0 = 300 };
    int from = 0, to = 0;
    const bool sel = clipSelectionTicks(from, to);
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    juce::PopupMenu m;
    m.addItem(kSplit, sel ? tr("tracks-pane-clip.split-at-selection", "Split at Selection") : tr("tracks-pane-clip.split-at-playhead", "Split at Playhead"));
    m.addItem(kDelete, tr("tracks-pane-clip.delete-selection", "Delete Selection"), sel);
    m.addItem(kRipple, tr("tracks-pane-clip.delete-and-close-gap", "Delete and Close Gap"), sel);
    m.addItem(kTrim, tr("tracks-pane-clip.trim-to-selection", "Trim to Selection"), sel);
    m.addItem(kSplitTransients, sel ? tr("tracks-pane-clip.split-selection-at-transients", "Split Selection at Transients") : tr("tracks-pane-clip.split-at-transients", "Split at Transients"));
    juce::PopupMenu sense;
    static const double senses[] = {0.25, 0.5, 0.75, 1.0};
    struct Sense { const char* key; const char* name; };
    static const Sense senseNames[] = {{"tracks-pane-clip.sense-strong", "Strong hits only"},
                                       {"tracks-pane-clip.sense-normal", "Normal"},
                                       {"tracks-pane-clip.sense-sensitive", "Sensitive"},
                                       {"tracks-pane-clip.sense-everything", "Everything"}};
    for (int i = 0; i < 4; ++i)
        sense.addItem(kSense0 + i, tr(senseNames[i].key, senseNames[i].name), true,
                      std::abs(sense_ - senses[i]) < 0.01);
    m.addSubMenu(tr("tracks-pane-clip.transients", "Transients"), sense);
    m.addSeparator();
    m.addItem(kFadeIn, tr("tracks-pane-clip.fade-in-to-playhead", "Fade In to Playhead"));
    m.addItem(kFadeOut, tr("tracks-pane-clip.fade-out-from-playhead", "Fade Out from Playhead"));
    juce::PopupMenu gain;
    static const double dbs[] = {12, 6, 3, 0, -3, -6, -12, -24};
    const double curDb = ci.audioGain > 0.0 ? 20.0 * std::log10(ci.audioGain) : -99.0;
    for (int i = 0; i < 8; ++i)
        gain.addItem(kGain0 + i, clipdetail::gainDb(std::pow(10.0, dbs[i] / 20.0)), true,
                     std::abs(curDb - dbs[i]) < 0.05);
    m.addSubMenu(tr("tracks-pane-clip.gain", "Gain"), gain);
    m.addItem(kNormalize, tr("tracks-pane-clip.normalize", "Normalize"));
    m.addItem(kReverse, tr("tracks-pane-clip.reverse", "Reverse"), true, ci.audioReverse);
    m.addItem(kDouble, tr("tracks-pane-clip.stretch-to-double-length", "Stretch to Double Length"));
    m.addItem(kHalf, tr("tracks-pane-clip.stretch-to-half-length", "Stretch to Half Length"));
    juce::PopupMenu pitch;
    for (int st = 12; st >= -12; --st)
        pitch.addItem(kPitch0 + st + 24, (st > 0 ? "+" : "") + juce::String(st) + tr("tracks-pane-clip.st", " st"), true,
                      std::abs(ci.audioPitch - st) < 0.01);
    m.addSubMenu(tr("tracks-pane-clip.pitch", "Pitch"), pitch);
    m.addSeparator();
    m.addItem(kLoop, sel ? tr("tracks-pane-clip.loop-selection", "Loop Selection") : tr("tracks-pane-clip.loop-clip", "Loop Clip"));
    m.addItem(kZoomClip, tr("tracks-pane-clip.zoom-to-clip", "Zoom to Clip"));
    m.addItem(kZoomSel, tr("tracks-pane-clip.zoom-to-selection", "Zoom to Selection"), sel);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                    [this](int r) {
        if (r <= 0) return;
        if (r >= kPitch0) { setClipPitch(r - kPitch0 - 24); return; }
        if (r >= kSense0) { sense_ = senses[r - kSense0]; repaint(); return; }
        if (r >= kGain0) { setClipGainDb(dbs[r - kGain0]); return; }
        switch (r) {
            case kSplit: splitClipSelection(); break;
            case kSplitTransients: splitAtTransients(); break;
            case kReverse: toggleClipReverse(); break;
            case kNormalize: normalizeClip(); break;
            case kDouble: stretchClipBy(2.0); break;
            case kHalf: stretchClipBy(0.5); break;
            case kDelete: deleteClipSelection(false); break;
            case kRipple: deleteClipSelection(true); break;
            case kTrim: trimClipToSelection(); break;
            case kFadeIn: fadeClipToPlayhead(true); break;
            case kFadeOut: fadeClipToPlayhead(false); break;
            case kLoop: loopClipSelection(); break;
            case kZoomClip: zoomToClip(); break;
            case kZoomSel: zoomToSelection(); break;
            default: break;
        }
    });
}

}
