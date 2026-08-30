#include "gui/TracksPane.h"

#include <algorithm>
#include <cmath>

#include "gui/LookAndFeel.h"
#include "gui/WaveformCache.h"

namespace hum {

namespace {
constexpr int kEdgeGrab = 6;
constexpr int kClipRibbonH = 18;
}

void TracksPane::enterClipMode(const std::string& node, int clipId) {
    if (node.empty() || clipId <= 0) return;
    if (mode_ == Mode::Song) { songPpb_ = ppb_; songScroll_ = scrollBeats_; }
    mode_ = Mode::Clip;
    clipNode_ = node;
    clipId_ = clipId;
    clipDrag_ = ClipDrag::None;
    hasSel_ = false;
    clearClipSel();
    clearSelection();
    clearPointSelection();
    syncLiveTarget();
    zoomToClip();
}

void TracksPane::leaveClipMode() {
    if (mode_ != Mode::Clip) return;
    mode_ = Mode::Song;
    clipNode_.clear();
    clipId_ = -1;
    hasSel_ = false;
    ppb_ = songPpb_;
    scrollBeats_ = songScroll_;
    syncLiveTarget();
    rebuildSlots();
    repaint();
}

int TracksPane::clipOrdinal() const {
    return mode_ == Mode::Clip ? clipIndexOfId(clipNode_, clipId_) : -1;
}

bool TracksPane::clipInfo(ClipEditor::ClipInfo& ci) const {
    const int c = clipOrdinal();
    if (c < 0) return false;
    const auto clips = host_.clips().list(clipNode_);
    if (c >= (int) clips.size()) return false;
    ci = clips[(size_t) c];
    return true;
}

double TracksPane::samplesPerBeat() const {
    return (host_.tempo() > 0.0 ? 60.0 / host_.tempo() : 0.5) * host_.sampleRate();
}

juce::Rectangle<int> TracksPane::clipField() const {
    const int top = headerH() + kChipH;
    return {kStripW, top, std::max(0, getWidth() - kStripW),
            std::max(0, fieldBottom() - kClipRibbonH - top)};
}

TracksPane::ClipHit TracksPane::clipEditorHit(juce::Point<int> p) const {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return ClipHit::None;
    const auto box = clipBox();
    if (!clipField().contains(p)) return ClipHit::None;
    using timelinechrome::FadeGrip;
    if (box.contains(p)) {
        const auto fg = timelinechrome::fadeGripAt(box, p, kFadeGrip * 2, ci.fadeInTicks,
                                                   ci.fadeOutTicks, ci.lengthTicks);
        if (fg == FadeGrip::Left) return ClipHit::FadeL;
        if (fg == FadeGrip::Right) return ClipHit::FadeR;
        const auto cg = timelinechrome::fadeCurveGripAt(box, p, kFadeGrip, ci.fadeInTicks,
                                                        ci.fadeOutTicks, ci.lengthTicks,
                                                        ci.fadeInCurve, ci.fadeOutCurve);
        if (cg == FadeGrip::Left) return ClipHit::CurveL;
        if (cg == FadeGrip::Right) return ClipHit::CurveR;
    }
    if (std::abs(p.x - box.getX()) <= kEdgeGrab) return ClipHit::EdgeL;
    if (std::abs(p.x - box.getRight()) <= kEdgeGrab) return ClipHit::EdgeR;
    return box.contains(p) ? ClipHit::Body : ClipHit::None;
}

juce::MouseCursor TracksPane::clipEditorCursor(juce::Point<int> p) const {
    if (effectiveTool() != Tool::Pointer) return timelinechrome::toolCursor(effectiveTool());
    switch (clipEditorHit(p)) {
        case ClipHit::EdgeL:
        case ClipHit::EdgeR:  return juce::MouseCursor::LeftRightResizeCursor;
        case ClipHit::FadeL:  return juce::MouseCursor::TopLeftCornerResizeCursor;
        case ClipHit::FadeR:  return juce::MouseCursor::TopRightCornerResizeCursor;
        case ClipHit::CurveL:
        case ClipHit::CurveR: return juce::MouseCursor::UpDownResizeCursor;
        case ClipHit::Body:   return juce::MouseCursor::IBeamCursor;
        case ClipHit::None:   break;
    }
    return juce::MouseCursor::NormalCursor;
}

TracksPane::ClipHit TracksPane::rowClipHit(int row, juce::Point<int> p) const {
    bool l = false, r = false;
    const int c = clipAt(row, p, l, r);
    if (c < 0) return ClipHit::None;
    const auto clips = host_.clips().list(rows_[(size_t) row]);
    if (c >= (int) clips.size()) return ClipHit::None;
    const auto& ci = clips[(size_t) c];
    const auto b = clipBounds(row, ci);
    using timelinechrome::FadeGrip;
    const auto fg = timelinechrome::fadeGripAt(b, p, kFadeGrip, ci.fadeInTicks, ci.fadeOutTicks,
                                               ci.lengthTicks);
    if (fg == FadeGrip::Left) return ClipHit::FadeL;
    if (fg == FadeGrip::Right) return ClipHit::FadeR;
    const auto cg = timelinechrome::fadeCurveGripAt(b, p, kFadeGrip, ci.fadeInTicks,
                                                    ci.fadeOutTicks, ci.lengthTicks,
                                                    ci.fadeInCurve, ci.fadeOutCurve);
    if (cg == FadeGrip::Left) return ClipHit::CurveL;
    if (cg == FadeGrip::Right) return ClipHit::CurveR;
    if (l) return ClipHit::EdgeL;
    if (r) return ClipHit::EdgeR;
    return ClipHit::Body;
}

juce::Rectangle<int> TracksPane::clipBox() const {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return {};
    const auto f = clipField();
    const int x0 = (int) std::floor(tickToX(ci.startTick));
    const int x1 = (int) std::ceil(tickToX(ci.startTick + ci.lengthTicks));
    return {x0, f.getY(), std::max(1, x1 - x0), f.getHeight()};
}

bool TracksPane::clipSelectionTicks(int& from, int& to) const {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci) || !hasSel_ || selTo_ <= selFrom_) return false;
    from = std::max(ci.startTick, (int) std::llround(selFrom_ * Pattern::kTicksPerBeat));
    to = std::min(ci.startTick + ci.lengthTicks,
                  (int) std::llround(selTo_ * Pattern::kTicksPerBeat));
    return to > from;
}

void TracksPane::zoomToClip() {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    const double beats = std::max(0.25, ci.lengthTicks / (double) Pattern::kTicksPerBeat);
    const double w = std::max(60, getWidth() - kStripW) * 0.9;
    ppb_ = juce::jlimit(2.0, kClipMaxPpb, w / beats);
    scrollBeats_ = std::max(0.0, ci.startTick / (double) Pattern::kTicksPerBeat
                                     - (w / 0.9 - w) * 0.5 / ppb_);
    repaint();
}

void TracksPane::zoomToSelection() {
    int from = 0, to = 0;
    if (!clipSelectionTicks(from, to)) { zoomToClip(); return; }
    const double beats = (to - from) / (double) Pattern::kTicksPerBeat;
    const double w = std::max(60, getWidth() - kStripW) * 0.9;
    ppb_ = juce::jlimit(2.0, kClipMaxPpb, w / beats);
    scrollBeats_ = std::max(0.0, from / (double) Pattern::kTicksPerBeat
                                     - (w / 0.9 - w) * 0.5 / ppb_);
    repaint();
}

void TracksPane::splitClipAt(int tick) {
    const int c = clipOrdinal();
    if (c < 0) return;
    host_.pushUndo();
    host_.clips().split(clipNode_, c, tick);
    repaint();
}

void TracksPane::splitClipSelection() {
    int from = 0, to = 0;
    if (clipSelectionTicks(from, to)) {
        const int c = clipOrdinal();
        if (c < 0) return;
        host_.beginTransaction();
        host_.pushUndo();
        const int right = host_.clips().split(clipNode_, c, from);
        host_.clips().split(clipNode_, right >= 0 ? right : clipOrdinal(), to);
        host_.endTransaction();
        hasSel_ = false;
        repaint();
        return;
    }
    splitClipAt((int) std::llround(host_.positionBeats() * Pattern::kTicksPerBeat));
}

bool TracksPane::deleteClipSelection(bool ripple) {
    int from = 0, to = 0;
    const int c = clipOrdinal();
    if (c < 0 || !clipSelectionTicks(from, to)) return false;
    host_.pushUndo();
    const bool ok = host_.clips().removeRange(clipNode_, c, from, to, ripple);
    hasSel_ = false;
    if (clipOrdinal() < 0) leaveClipMode();
    else repaint();
    if (ok && onPatchChanged) onPatchChanged();
    return ok;
}

void TracksPane::trimClipToSelection() {
    int from = 0, to = 0;
    const int c = clipOrdinal();
    if (c < 0 || !clipSelectionTicks(from, to)) return;
    host_.pushUndo();
    host_.clips().trimTo(clipNode_, c, from, to);
    hasSel_ = false;
    repaint();
}

void TracksPane::copyClipSelection() {
    int from = 0, to = 0;
    const int c = clipOrdinal();
    if (c < 0) return;
    ClipEditor::ClipInfo ci;
    if (!clipSelectionTicks(from, to)) {
        if (!clipInfo(ci)) return;
        from = ci.startTick; to = ci.startTick + ci.lengthTicks;
    }
    auto data = host_.clips().copyRange(clipNode_, c, from, to);
    if (data.audioFile.empty()) return;
    data.startTick = 0;
    clipboard_ = {{0, 0, std::move(data)}};
    clipboardSpan_ = to - from;
}

bool TracksPane::pasteClipSelection(int atTick) {
    const auto it = std::find(rows_.begin(), rows_.end(), clipNode_);
    if (it == rows_.end()) return false;
    return pasteClips(atTick, (int) (it - rows_.begin()));
}

void TracksPane::straightenFade(bool in) {
    ClipEditor::ClipInfo ci;
    const int c = clipOrdinal();
    if (c < 0 || !clipInfo(ci)) return;
    host_.pushUndo();
    host_.clips().setFadeCurves(clipNode_, c, in ? 0.0 : ci.fadeInCurve,
                                in ? ci.fadeOutCurve : 0.0);
    repaint();
}

void TracksPane::fadeClipToPlayhead(bool in) {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    const int at = (int) std::llround(host_.positionBeats() * Pattern::kTicksPerBeat);
    int fi = ci.fadeInTicks, fo = ci.fadeOutTicks;
    if (in) fi = juce::jlimit(0, ci.lengthTicks, at - ci.startTick);
    else fo = juce::jlimit(0, ci.lengthTicks, ci.startTick + ci.lengthTicks - at);
    host_.pushUndo();
    host_.clips().setFades(clipNode_, ci.index, fi, fo);
    repaint();
}

void TracksPane::setClipGainDb(double db) {
    const int c = clipOrdinal();
    if (c < 0) return;
    host_.pushUndo();
    host_.clips().setGain(clipNode_, c, std::pow(10.0, db / 20.0));
    repaint();
}

void TracksPane::loopClipSelection() {
    int from = 0, to = 0;
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    if (!clipSelectionTicks(from, to)) { from = ci.startTick; to = from + ci.lengthTicks; }
    host_.automation().setLoop(from / (double) Pattern::kTicksPerBeat,
                               to / (double) Pattern::kTicksPerBeat, true);
    repaint();
}

void TracksPane::toggleClipReverse() {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    host_.pushUndo();
    host_.clips().setReverse(clipNode_, ci.index, !ci.audioReverse);
    repaint();
}

void TracksPane::normalizeClip() {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (!peaks || !peaks->ready || peaks->binSamples <= 0) return;
    const double toFile = (peaks->fileSampleRate > 0.0 ? peaks->fileSampleRate : host_.sampleRate())
                          / host_.sampleRate();
    const auto len = (long long) std::llround(ci.lengthTicks * samplesPerBeat() / Pattern::kTicksPerBeat);
    const int b0 = (int) ((double) ci.audioOffset * toFile / peaks->binSamples);
    const int b1 = (int) ((double) (ci.audioOffset + len) * toFile / peaks->binSamples) + 1;
    float peak = 0.0f;
    for (int b = std::max(0, b0); b < std::min(b1, (int) peaks->hi.size()); ++b)
        peak = std::max({peak, peaks->hi[(size_t) b], -peaks->lo[(size_t) b]});
    if (peak <= 1e-6f) return;
    host_.pushUndo();
    host_.clips().setGain(clipNode_, ci.index, 0.98 / peak);
    repaint();
}

void TracksPane::stretchClipBy(double factor) {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci) || factor <= 0.0) return;
    host_.pushUndo();
    host_.clips().stretch(clipNode_, ci.index, std::max(1, (int) std::llround(ci.lengthTicks * factor)));
    repaint();
}

void TracksPane::setClipPitch(double semitones) {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    host_.pushUndo();
    host_.clips().setPitch(clipNode_, ci.index, semitones);
    repaint();
}

long long TracksPane::sampleToTick(const ClipEditor::ClipInfo& ci, long long s) const {
    const double spt = samplesPerBeat() / Pattern::kTicksPerBeat;
    return ci.startTick + (long long) std::llround((double) (s - ci.audioOffset) / spt);
}

std::vector<long long> TracksPane::clipTransients(const ClipEditor::ClipInfo& ci) {
    std::vector<long long> out;
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (!peaks || !peaks->ready) return out;
    const double toFile = (peaks->fileSampleRate > 0.0 ? peaks->fileSampleRate : host_.sampleRate())
                          / host_.sampleRate();
    const auto clipEnd = ci.audioOffset
                         + (long long) std::llround(ci.lengthTicks * samplesPerBeat() / Pattern::kTicksPerBeat);
    const float thresh = (float) (1.0 - transientSense_);
    for (const auto& o : peaks->onsets) {
        if (o.pos == 0 || o.strength < thresh) continue;
        const auto& win = clipWindow_.read(ci.audioFile, o.pos, o.pos + 2 * kOnsetHop);
        const long long filePos = win.empty() ? o.pos
                                              : clipWindow_.from() + refineOnset(win.data(), 0, (int) win.size());
        const auto s = (long long) std::llround((double) filePos / toFile);
        if (s > ci.audioOffset && s < clipEnd) out.push_back(s);
    }
    return out;
}

bool TracksPane::tabToTransient(int dir) {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return false;
    const auto ts = clipTransients(ci);
    const double spt = samplesPerBeat() / Pattern::kTicksPerBeat;
    const auto cur = ci.audioOffset
                     + (long long) std::llround((host_.positionBeats() * Pattern::kTicksPerBeat - ci.startTick) * spt);
    const long long slack = (long long) (spt * 0.5);
    long long target = -1;
    if (dir > 0) { for (auto s : ts) if (s > cur + slack) { target = s; break; } }
    else { for (auto s : ts) if (s < cur - slack) target = s; }
    if (target < 0) {
        const auto edge = dir > 0 ? ci.startTick + ci.lengthTicks : ci.startTick;
        if (std::llabs((long long) std::llround(host_.positionBeats() * Pattern::kTicksPerBeat) - edge) < 1) return false;
        host_.setPositionBeats(edge / (double) Pattern::kTicksPerBeat);
        repaint();
        return true;
    }
    host_.setPositionBeats(sampleToTick(ci, target) / (double) Pattern::kTicksPerBeat);
    repaint();
    return true;
}

void TracksPane::splitAtTransients() {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    int from = ci.startTick, to = ci.startTick + ci.lengthTicks;
    clipSelectionTicks(from, to);
    std::vector<int> ticks;
    for (auto s : clipTransients(ci)) {
        const auto t = sampleToTick(ci, s);
        if (t > from && t < to && (ticks.empty() || t != ticks.back())) ticks.push_back((int) t);
    }
    if (ticks.empty()) return;
    host_.beginTransaction();
    host_.pushUndo();
    for (auto it = ticks.rbegin(); it != ticks.rend(); ++it)
        if (const int c = clipOrdinal(); c >= 0) host_.clips().split(clipNode_, c, *it);
    host_.endTransaction();
    hasSel_ = false;
    repaint();
}

bool TracksPane::mouseDownClip(const juce::MouseEvent& e, juce::Point<int> p) {
    if (mode_ != Mode::Clip) return false;
    if (p.y < kTopH) {
        if (crumbBackBox().contains(p)) { leaveClipMode(); return true; }
        return false;
    }
    const auto f = clipField();
    if (!f.contains(p)) return p.y >= headerH();
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return true;
    if (e.mods.isPopupMenu()) { showClipDetailMenu(e.getScreenPosition()); return true; }

    const auto box = clipBox();
    const double beat = std::max(0.0, xToBeat((float) p.x));
    clipStart0_ = ci.startTick;
    clipLen0_ = ci.lengthTicks;
    clipOffset0_ = ci.audioOffset;
    clipAnchorBeat_ = beat;

    if (effectiveTool() == Tool::Scissors) {
        splitClipAt((int) std::llround(snapBeats(beat, e.mods.isAltDown()) * Pattern::kTicksPerBeat));
        return true;
    }
    const bool nearL = std::abs(p.x - box.getX()) <= kEdgeGrab;
    const bool nearR = std::abs(p.x - box.getRight()) <= kEdgeGrab;
    const auto fg = box.contains(p)
                        ? timelinechrome::fadeGripAt(box, p, kFadeGrip * 2, ci.fadeInTicks,
                                                     ci.fadeOutTicks, ci.lengthTicks)
                        : timelinechrome::FadeGrip::None;
    const auto cg = box.contains(p)
                        ? timelinechrome::fadeCurveGripAt(box, p, kFadeGrip, ci.fadeInTicks,
                                                          ci.fadeOutTicks, ci.lengthTicks,
                                                          ci.fadeInCurve, ci.fadeOutCurve)
                        : timelinechrome::FadeGrip::None;
    if (fg == timelinechrome::FadeGrip::Left) clipDrag_ = ClipDrag::FadeL;
    else if (fg == timelinechrome::FadeGrip::Right) clipDrag_ = ClipDrag::FadeR;
    else if (cg != timelinechrome::FadeGrip::None) {
        clipDrag_ = cg == timelinechrome::FadeGrip::Left ? ClipDrag::CurveL : ClipDrag::CurveR;
        dragCurveY0_ = p.y;
        dragCurve0_ = cg == timelinechrome::FadeGrip::Left ? ci.fadeInCurve : ci.fadeOutCurve;
    }
    else if (nearL) clipDrag_ = e.mods.isCommandDown() ? ClipDrag::StretchL : ClipDrag::TrimL;
    else if (nearR) clipDrag_ = e.mods.isCommandDown() ? ClipDrag::StretchR : ClipDrag::TrimR;
    else if (e.mods.isAltDown() && box.contains(p)) clipDrag_ = ClipDrag::Slip;
    else clipDrag_ = ClipDrag::Pending;
    if (clipDrag_ != ClipDrag::Pending) host_.pushUndo();
    return true;
}

void TracksPane::mouseDragClip(const juce::MouseEvent& e) {
    if (clipDrag_ == ClipDrag::None) return;
    const double beat = std::max(0.0, xToBeat((float) e.x));
    const double snapped = snapBeats(beat, e.mods.isAltDown());
    const int c = clipOrdinal();
    if (c < 0) return;
    switch (clipDrag_) {
        case ClipDrag::Pending:
            if (std::abs(e.getDistanceFromDragStartX()) < 3) return;
            clipDrag_ = ClipDrag::Select;
            selAnchor_ = snapBeats(clipAnchorBeat_, e.mods.isAltDown());
            hasSel_ = true;
            [[fallthrough]];
        case ClipDrag::Select:
            selFrom_ = std::min(selAnchor_, snapped);
            selTo_ = std::max(selAnchor_, snapped);
            break;
        case ClipDrag::TrimL: {
            const auto ci = host_.clips().list(clipNode_)[(size_t) c];
            const int floorTick = ci.isAudio && !ci.audioReverse
                ? clipStart0_ - (int) std::llround((double) clipOffset0_ * Pattern::kTicksPerBeat / samplesPerBeat())
                : 0;
            const int end = clipStart0_ + clipLen0_;
            const int t = juce::jlimit(std::max(0, floorTick), end - 1,
                                       (int) std::llround(snapped * Pattern::kTicksPerBeat));
            if (t != ci.startTick) host_.clips().resize(clipNode_, c, end - t, true);
            break;
        }
        case ClipDrag::TrimR: {
            const int t = std::max(clipStart0_ + 1, (int) std::llround(snapped * Pattern::kTicksPerBeat));
            host_.clips().resize(clipNode_, c, t - clipStart0_, false);
            break;
        }
        case ClipDrag::StretchR: {
            const int t = std::max(clipStart0_ + 1, (int) std::llround(snapped * Pattern::kTicksPerBeat));
            const auto ci = host_.clips().list(clipNode_)[(size_t) c];
            if (t - clipStart0_ != ci.lengthTicks) host_.clips().stretch(clipNode_, c, t - clipStart0_);
            break;
        }
        case ClipDrag::StretchL: {
            const int end = clipStart0_ + clipLen0_;
            const int t = juce::jlimit(0, end - 1, (int) std::llround(snapped * Pattern::kTicksPerBeat));
            const auto ci = host_.clips().list(clipNode_)[(size_t) c];
            if (t != ci.startTick) {
                host_.clips().stretch(clipNode_, c, end - t);
                host_.clips().move(clipNode_, c, t);
            }
            break;
        }
        case ClipDrag::Slip: {
            const auto delta = (long long) std::llround((clipAnchorBeat_ - beat) * samplesPerBeat());
            const auto cur = host_.clips().list(clipNode_)[(size_t) c].audioOffset;
            host_.clips().slip(clipNode_, c, clipOffset0_ + delta - cur);
            break;
        }
        case ClipDrag::FadeL:
        case ClipDrag::FadeR: {
            const auto ci = host_.clips().list(clipNode_)[(size_t) c];
            const int t = (int) std::llround(snapped * Pattern::kTicksPerBeat);
            if (clipDrag_ == ClipDrag::FadeL)
                host_.clips().setFades(clipNode_, c, juce::jlimit(0, ci.lengthTicks, t - ci.startTick), ci.fadeOutTicks);
            else
                host_.clips().setFades(clipNode_, c, ci.fadeInTicks,
                                       juce::jlimit(0, ci.lengthTicks, ci.startTick + ci.lengthTicks - t));
            break;
        }
        case ClipDrag::CurveL:
        case ClipDrag::CurveR: {
            const auto ci = host_.clips().list(clipNode_)[(size_t) c];
            const double v = timelinechrome::fadeCurveFromDrag(dragCurve0_, e.y - dragCurveY0_,
                                                               clipBox().getHeight());
            if (clipDrag_ == ClipDrag::CurveL)
                host_.clips().setFadeCurves(clipNode_, c, v, ci.fadeOutCurve);
            else
                host_.clips().setFadeCurves(clipNode_, c, ci.fadeInCurve, v);
            break;
        }
        case ClipDrag::None: break;
    }
    repaint();
}

void TracksPane::mouseUpClip() {
    if (clipDrag_ == ClipDrag::Pending) {
        hasSel_ = false;
        host_.setPositionBeats(snapBeats(clipAnchorBeat_, false));
    }
    clipDrag_ = ClipDrag::None;
    repaint();
}

bool TracksPane::keyPressedClip(const juce::KeyPress& k) {
    const auto cmd = juce::ModifierKeys::commandModifier;
    const auto shift = juce::ModifierKeys::shiftModifier;
    if (k.getKeyCode() == juce::KeyPress::escapeKey && hasSel_) { hasSel_ = false; repaint(); return true; }
    if (k.getKeyCode() == juce::KeyPress::escapeKey
        || k.getKeyCode() == juce::KeyPress::returnKey) { leaveClipMode(); return true; }
    if (k.getKeyCode() == juce::KeyPress::tabKey)
        return tabToTransient(k.getModifiers().testFlags(shift) ? -1 : 1);
    if (k == juce::KeyPress('s')) { splitClipSelection(); return true; }
    if (k == juce::KeyPress('s', shift, 0)) { splitAtTransients(); return true; }
    if (k == juce::KeyPress('z')) { zoomToSelection(); return true; }
    if (k == juce::KeyPress('r')) { toggleClipReverse(); return true; }
    if (k == juce::KeyPress('l')) { loopClipSelection(); return true; }
    if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey)
        return deleteClipSelection(k.getModifiers().testFlags(shift));
    if (k == juce::KeyPress('a', cmd, 0)) {
        ClipEditor::ClipInfo ci;
        if (clipInfo(ci)) selectTicksForTest(ci.startTick, ci.startTick + ci.lengthTicks);
        repaint();
        return true;
    }
    if (k == juce::KeyPress('c', cmd, 0)) { copyClipSelection(); return true; }
    if (k == juce::KeyPress('x', cmd, 0)) { copyClipSelection(); deleteClipSelection(false); return true; }
    if (k == juce::KeyPress('v', cmd, 0))
        return pasteClipSelection((int) std::llround(
            snapBeats(host_.positionBeats(), false) * Pattern::kTicksPerBeat));
    if (k == juce::KeyPress('1')) { tool_ = Tool::Pointer;  repaint(); return true; }
    if (k == juce::KeyPress('3')) { tool_ = Tool::Scissors; repaint(); return true; }
    return false;
}

void TracksPane::showClipDetailMenu(juce::Point<int> screenPos) {
    enum { kSplit = 1, kDelete, kRipple, kTrim, kFadeIn, kFadeOut, kLoop, kZoomClip, kZoomSel,
           kSplitTransients, kReverse, kNormalize, kDouble, kHalf,
           kGain0 = 100, kSense0 = 200, kPitch0 = 300 };
    int from = 0, to = 0;
    const bool sel = clipSelectionTicks(from, to);
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    juce::PopupMenu m;
    m.addItem(kSplit, sel ? "Split at Selection" : "Split at Playhead");
    m.addItem(kDelete, "Delete Selection", sel);
    m.addItem(kRipple, "Delete and Close Gap", sel);
    m.addItem(kTrim, "Trim to Selection", sel);
    m.addItem(kSplitTransients, sel ? "Split Selection at Transients" : "Split at Transients");
    juce::PopupMenu sense;
    static const double senses[] = {0.25, 0.5, 0.75, 1.0};
    static const char* senseNames[] = {"Strong hits only", "Normal", "Sensitive", "Everything"};
    for (int i = 0; i < 4; ++i)
        sense.addItem(kSense0 + i, senseNames[i], true, std::abs(transientSense_ - senses[i]) < 0.01);
    m.addSubMenu("Transients", sense);
    m.addSeparator();
    m.addItem(kFadeIn, "Fade In to Playhead");
    m.addItem(kFadeOut, "Fade Out from Playhead");
    juce::PopupMenu gain;
    static const double dbs[] = {12, 6, 3, 0, -3, -6, -12, -24};
    const double curDb = ci.audioGain > 0.0 ? 20.0 * std::log10(ci.audioGain) : -99.0;
    for (int i = 0; i < 8; ++i)
        gain.addItem(kGain0 + i, clipdetail::gainDb(std::pow(10.0, dbs[i] / 20.0)), true,
                     std::abs(curDb - dbs[i]) < 0.05);
    m.addSubMenu("Gain", gain);
    m.addItem(kNormalize, "Normalize");
    m.addItem(kReverse, "Reverse", true, ci.audioReverse);
    m.addItem(kDouble, "Stretch to Double Length");
    m.addItem(kHalf, "Stretch to Half Length");
    juce::PopupMenu pitch;
    for (int st = 12; st >= -12; --st)
        pitch.addItem(kPitch0 + st + 24, (st > 0 ? "+" : "") + juce::String(st) + " st", true,
                      std::abs(ci.audioPitch - st) < 0.01);
    m.addSubMenu("Pitch", pitch);
    m.addSeparator();
    m.addItem(kLoop, sel ? "Loop Selection" : "Loop Clip");
    m.addItem(kZoomClip, "Zoom to Clip");
    m.addItem(kZoomSel, "Zoom to Selection", sel);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                    [this](int r) {
        if (r <= 0) return;
        if (r >= kPitch0) { setClipPitch(r - kPitch0 - 24); return; }
        if (r >= kSense0) { transientSense_ = senses[r - kSense0]; repaint(); return; }
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
