// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/tracks/SongView.h"
#include "core/timeline/ClipDrag.h"
#include "core/packs/Roles.h"
#include "gui/video/VideoDeckPool.h"
#include "gui/tracks/FileDragImage.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "gui/tracks/AutoPointPopup.h"
#include "gui/app/QwertyPiano.h"
#include "gui/common/Localisation.h"
#include "gui/tracks/ClipChrome.h"

namespace hum {

void SongView::mouseDragAt(const juce::MouseEvent& e) {
    if (!dragSync_ && drag_ != Drag::None)
        dragSync_.emplace(host());
    ctx_.edgeScroll(e.getPosition());
    const auto p = e.getPosition();
    const double beat = std::max(0.0, xToBeat((float) p.x));
    const bool alt = e.mods.isAltDown();

    if ((drag_ == Drag::Pencil || drag_ == Drag::Line) && lineSlot_ >= 0
        && lineSlot_ < (int) slots_.size()) {
        const auto& sl = slots_[(size_t) lineSlot_];
        const auto [lo, hi] = laneRange(dragAutoNode_, dragAutoParam_);
        const double v = laneValueAtY(sl, p.y, lo, hi);
        if (drag_ == Drag::Pencil) pencilStep(dragAutoNode_, dragAutoParam_, beat, v);
        else { lineBeat1_ = snapBeats(beat, alt); lineVal1_ = v; }
        repaintAll();
        return;
    }

    if (dragAutoPoint_ >= 0 && dragAutoSlot_ >= 0 && dragAutoSlot_ < (int) slots_.size()) {
        const auto& sl = slots_[(size_t) dragAutoSlot_];
        const auto [lo, hi] = laneRange(dragAutoNode_, dragAutoParam_);
        const double v = laneValueAtY(sl, p.y, lo, hi);
        const double snapped = snapBeats(beat, alt);
        if (dragAutoGrip_ == AutoGrip::Value) {
            dragAutoPoint_ = host().automation().movePoint(
                dragAutoNode_, dragAutoParam_, dragAutoPoint_, snapped, v);
        } else if (dragAutoGrip_ == AutoGrip::TriggerTime) {
            dragAutoPoint_ = host().automation().movePoint(
                dragAutoNode_, dragAutoParam_, dragAutoPoint_, snapped, 0.0);
        } else {
            double curLo = v, curHi = v, curBeat = beat;
            if (const auto* cm = host().model().byName(dragAutoNode_))
                for (const auto& l : cm->automation)
                    if (l.propertyName == dragAutoParam_
                        && dragAutoPoint_ < (int) l.points.size()) {
                        const auto& pt = l.points[(size_t) dragAutoPoint_];
                        curLo = pt.value; curHi = pt.valueMax; curBeat = pt.beat;
                        break;
                    }
            if (dragAutoGrip_ == AutoGrip::RangeLo) curLo = v;
            else if (dragAutoGrip_ == AutoGrip::RangeHi) curHi = v;
            else { const double mid = (curLo + curHi) * 0.5, half = (curHi - curLo) * 0.5;
                   (void) mid; curLo = v - half; curHi = v + half; }
            if (curLo > curHi) std::swap(curLo, curHi);
            (void) curBeat;
            dragAutoPoint_ = host().automation().moveRangePoint(
                dragAutoNode_, dragAutoParam_, dragAutoPoint_, snapped, curLo, curHi);
        }
        repaintAll();
        return;
    }

    switch (drag_) {
        case Drag::BoxMove:
            boxDragDelta_ = snapBeats(beat, alt) - snapBeats(boxAnchor_, alt);
            if (dragBox_ >= 0 && dragBox_ < (int) host().automation().boxes().size())
                boxDragDelta_ = std::max(boxDragDelta_,
                    -host().automation().boxes()[(size_t) dragBox_].startBeat);
            repaintAll();
            return;
        case Drag::BoxTrimL:
            boxTrimL_ = juce::jlimit(-boxOrigS_, boxOrigE_ - 0.25 - boxOrigS_,
                                     snapBeats(beat, alt) - boxOrigS_);
            repaintAll();
            return;
        case Drag::BoxTrimR:
            boxTrimR_ = std::max(boxOrigS_ + 0.25 - boxOrigE_,
                                 snapBeats(beat, alt) - boxOrigE_);
            repaintAll();
            return;
        default: break;
    }

    if (drag_ == Drag::ClipMarquee) { updateClipMarquee(p); return; }
    if (dragRow_ < 0 || dragClip_ < 0) return;
    const auto& node = rows_[(size_t) dragRow_];
    const auto clips = host().clips().list(node);
    if (dragClip_ >= (int) clips.size()) return;
    const auto& ci = clips[(size_t) dragClip_];
    const int tick = std::max(0, xToTick((float) p.x));
    auto snapT = [&](int t) {
        return (int) std::llround(snapBeats(t / (double) Pattern::kTicksPerBeat, alt)
                                  * Pattern::kTicksPerBeat);
    };

    switch (drag_) {
        case Drag::ClipMove: {
            if (leftForGood(p)) { dragClipOut(rows_[(size_t) dragRow_]); break; }
            const int overRow = rowAt(p.y);
            moveSelection(snapT(tick - dragGrabTicks_) - dragOriginTick_,
                          overRow >= 0 ? overRow - moveGrabRow_ : 0);
            break;
        }
        case Drag::ClipCreate:
        case Drag::ClipResizeR: {
            const int end = std::max(ci.startTick + 1, snapT(tick));
            host().clips().resize(node, dragClip_, std::max(Pattern::kTicksPerBeat / 4,
                                                       end - ci.startTick), false);
            break;
        }
        case Drag::ClipRepeat:
            applyRepeatFill(snapT(tick));
            break;
        case Drag::ClipMarquee:
            updateClipMarquee(p);
            break;
        case Drag::Curve: {
            const double d = (dragCurveY0_ - e.y) / 60.0;
            host().automation().setCurve(dragAutoNode_, dragAutoParam_, dragCurveIndex_,
                                        dragCurve0_ + d);
            repaintAll();
            break;
        }
        case Drag::PointMarquee:
            updatePointMarquee(p);
            break;
        case Drag::PointGroup:
            dragSelectedPoints(e);
            break;
        case Drag::ClipFadeL:
        case Drag::ClipFadeR: {
            const auto clips = host().clips().list(dragRow_ >= 0 ? rows_[(size_t) dragRow_]
                                                                : std::string());
            if (dragClip_ < 0 || dragClip_ >= (int) clips.size()) break;
            const auto& ci = clips[(size_t) dragClip_];
            const int t = (int) std::llround(snapBeats(xToBeat((float) e.x),
                                                       e.mods.isAltDown())
                                             * Pattern::kTicksPerBeat);
            const int len = std::max(1, ci.lengthTicks);
            if (drag_ == Drag::ClipFadeL)
                host().clips().setFades(rows_[(size_t) dragRow_], dragClip_,
                                       juce::jlimit(0, len, t - ci.startTick),
                                       ci.fadeOutTicks);
            else
                host().clips().setFades(rows_[(size_t) dragRow_], dragClip_, ci.fadeInTicks,
                                       juce::jlimit(0, len, ci.startTick + len - t));
            repaintAll();
            break;
        }
        case Drag::ClipFadeCurveL:
        case Drag::ClipFadeCurveR: {
            const auto clips = host().clips().list(rows_[(size_t) dragRow_]);
            if (dragClip_ < 0 || dragClip_ >= (int) clips.size()) break;
            const auto& fc = clips[(size_t) dragClip_];
            const int h = clipBounds(dragRow_, fc).getHeight();
            const double c = timelinechrome::fadeCurveFromDrag(dragCurve0_, e.y - dragCurveY0_, h);
            if (drag_ == Drag::ClipFadeCurveL)
                host().clips().setFadeCurves(rows_[(size_t) dragRow_], dragClip_, c,
                                            fc.fadeOutCurve);
            else
                host().clips().setFadeCurves(rows_[(size_t) dragRow_], dragClip_, fc.fadeInCurve, c);
            repaintAll();
            break;
        }
        case Drag::ClipResizeL: {
            const int end = ci.startTick + ci.lengthTicks;
            const int ns = std::min(snapT(tick), end - Pattern::kTicksPerBeat / 4);
            host().clips().resize(node, dragClip_, end - std::max(0, ns), true);
            break;
        }
        default: break;
    }
    repaintAll();
}

bool SongView::leftForGood(juce::Point<int> p) const {
    const juce::Rectangle<int> b(0, 0, getWidth(), getBottom());
    return p.x > b.getRight() + kDragOutSlack
           || p.y < b.getY() - kDragOutSlack
           || p.y > b.getBottom() + kDragOutSlack;
}

juce::Rectangle<int> SongView::clipChipBounds(int row, const ClipEditor::ClipInfo& ci) const {
    const auto field = getBounds().withTrimmedLeft(kStripW);
    auto box = clipBounds(row, ci).getIntersection(field);
    if (box.isEmpty()) return {};
    return box.withWidth(std::min(box.getWidth(), 220));
}

juce::Image SongView::clipChip(int row, const ClipEditor::ClipInfo& ci) const {
    const auto box = clipChipBounds(row, ci);
    if (box.isEmpty()) return {};
    return const_cast<SongView*>(this)->createComponentSnapshot(box - getPosition(), true, kChipScale);
}

void SongView::dragClipOut(const std::string& node) {
    if (extDrag_ || dragClip_ < 0) return;
    auto clips = host().clips().list(node);
    if (dragClip_ >= (int) clips.size()) return;
    const bool audio = clips[(size_t) dragClip_].isAudio;
    const bool video = clips[(size_t) dragClip_].isVideo;
    const juce::Image chip = clipChip(dragRow_, clips[(size_t) dragClip_]);
    std::string home = node;
    int clip = dragClip_;
    if (dragDuplicated_) {
        for (const auto& m : moveBase_)
            if (const int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id); at >= 0)
                host().clips().remove(rows_[(size_t) m.curRow], at);
        moveBase_.clear();
        sel_.clear();
        clip = -1;
    } else if (!moveBase_.empty()) {
        int homeRow = dragRow_;
        for (const auto& m : moveBase_) if (m.id == moveGrabId_) homeRow = m.row;
        restoreClipMove();
        home = rows_[(size_t) homeRow];
        clip = clipIndexOfId(home, moveGrabId_);
        if (clip < 0) return;
    }
    drag_ = Drag::None;
    dragSync_.reset();
    dragRow_ = dragClip_ = -1;
    rebuild();
    repaintAll();
    if (clip < 0) return;
    clips = host().clips().list(home);
    if (clip >= (int) clips.size()) return;
    if (!audio) {
        const auto tag = video ? juce::String(clipdrag::videoClip(home, clips[(size_t) clip].id))
                               : juce::String("noteclip:")
                                     + juce::String(juce::CharPointer_UTF8(home.c_str()))
                                     + ":" + juce::String(clips[(size_t) clip].id);
        if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this))
            dnd->startDragging(tag, this, juce::ScaledImage(chip, kChipScale), true);
        return;
    }
    juce::StringArray files;
    auto addExport = [&](const std::string& n, int c) {
        const auto path = host().clips().exportFile(n, c);
        if (!path.empty()) files.add(juce::String(juce::CharPointer_UTF8(path.c_str())));
    };
    std::set<std::pair<std::string, int>> done;
    for (const auto& ref : sel_) {
        if (ref.kind != timeline::ItemRef::Kind::Clip) continue;
        const auto& rn = rows_[(size_t) ref.row];
        for (const auto& ci : host().clips().list(rn))
            if (ci.id == ref.key && ci.isAudio && done.insert({rn, ci.index}).second) addExport(rn, ci.index);
    }
    if (done.insert({home, clip}).second) addExport(home, clip);
    if (files.isEmpty()) return;
    extDrag_ = true;
    if (!dragFilesWithImage(files, this, chip, [this] { extDrag_ = false; }))
        juce::DragAndDropContainer::performExternalDragDropOfFiles(files, false, this,
                                                                   [this] { extDrag_ = false; });
}

bool SongView::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& d) {
    std::string pad;
    int index = 0;
    return d.description.toString().startsWith("print:")
           || clipdrag::parseVideoPad(d.description.toString().toStdString(), pad, index);
}

void SongView::dropPad(const std::string& pad, int index, juce::Point<int> at) {
    const auto* cm = host().model().byName(pad);
    if (cm == nullptr || !classHasRole(cm->classRaw, role::kClipPads)) return;
    const auto n = std::to_string(index + 1);
    const auto file = VideoDeckPool::resolveTape(host().documentPath(),
                                                 juce::String(host().liveParamText(pad, "File" + n)));
    if (!file.existsAsFile()) return;
    ClipEditor::MediaRange r;
    r.file = file.getFullPathName().toStdString();
    r.inSeconds = host().liveParamValue(pad, "In" + n);
    r.outSeconds = host().liveParamValue(pad, "Out" + n);
    const int tick = (int) std::llround(
        snapBeats(std::max(0.0, xToBeat((float) at.x)), false) * Pattern::kTicksPerBeat);
    host().pushUndo();
    const auto node = dropTargetNode(at.y, true);
    if (!node.empty()) host().clips().addVideoRange(node, tick, r);
    rebuild();
    repaintAll();
}

void SongView::itemDropped(const juce::DragAndDropTarget::SourceDetails& d) {
    setDropHot(false);
    std::string pad;
    int index = 0;
    if (clipdrag::parseVideoPad(d.description.toString().toStdString(), pad, index)) {
        dropPad(pad, index, d.localPosition);
        return;
    }
    const auto node = d.description.toString().fromFirstOccurrenceOf("print:", false, false).toStdString();
    const int bar = 4 * Pattern::kTicksPerBeat;
    const int at = (std::max(0, xToTick((float) d.localPosition.x)) / bar) * bar;
    std::string err;
    host().printToTimeline(node, err, at);
    if (!err.empty())
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, tr("tracks-pane-input.midi-to-track", "MIDI to Track"),
                                               juce::String(juce::CharPointer_UTF8(err.c_str())));
    rebuild();
    repaintAll();
}

}
