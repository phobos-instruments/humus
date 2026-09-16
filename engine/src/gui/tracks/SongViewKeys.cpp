// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "gui/tracks/AutoPointPopup.h"
#include "gui/app/QwertyPiano.h"

namespace hum {

bool SongView::selectedClip(std::string& node, ClipEditor::ClipInfo& ci) const {
    if (selClipRow_ < 0 || selClipRow_ >= (int) rows_.size() || selClip_ < 0) return false;
    node = rows_[(size_t) selClipRow_];
    const auto clips = host().clips().list(node);
    if (selClip_ >= (int) clips.size()) return false;
    ci = clips[(size_t) selClip_];
    return true;
}

bool SongView::keyPressed(const juce::KeyPress& k) {
    if (inBox() && selPts_.empty()
        && (k.getKeyCode() == juce::KeyPress::escapeKey
            || k.getKeyCode() == juce::KeyPress::returnKey)) {
        leaveBox();
        return true;
    }
    if (k.getKeyCode() == juce::KeyPress::returnKey) {
        if (selClipRow_ >= 0 && selClipRow_ < (int) rows_.size()
            && !host().nodeRecordsAudio(rows_[(size_t) selClipRow_])) {
            ctx_.enterTrack(rows_[(size_t) selClipRow_]);
            return true;
        }
    }

    if (!selPts_.empty()) {
        if (k == juce::KeyPress::deleteKey || k == juce::KeyPress::backspaceKey)
            return deleteSelectedPoints();
        if (k == juce::KeyPress::escapeKey) { clearPointSelection(); return true; }
    }

    const auto cmd = juce::ModifierKeys::commandModifier;
    if (!selPts_.empty()) {
        if (k == juce::KeyPress('c', cmd, 0)) { copySelectedPoints(); return true; }
        if (k == juce::KeyPress('x', cmd, 0)) { copySelectedPoints(); return deleteSelectedPoints(); }
    }
    if (k == juce::KeyPress('v', cmd, 0) && !pointClipboard_.empty()
        && pastePoints(snapBeats(host().positionBeats(), false)))
        return true;
    std::string node;
    ClipEditor::ClipInfo ci;
    const bool clip = selectedClip(node, ci);
    const bool box = selBox_ >= 0 && selBox_ < (int) host().automation().boxes().size();
    const int pasteTick = (int) std::llround(
        snapBeats(host().positionBeats(), false) * Pattern::kTicksPerBeat);

    if (k == juce::KeyPress('a', cmd, 0) && !rows_.empty()) {
        selectAll();
        return true;
    }
    if (!sel_.empty()) {
        if (k == juce::KeyPress('c', cmd, 0)) { copySelectedClips(); return true; }
        if (k == juce::KeyPress('x', cmd, 0)) { copySelectedClips(); return deleteSelection(); }
        if (k == juce::KeyPress('d', cmd, 0)) return duplicateSelection();
        if (k.getKeyCode() == juce::KeyPress::deleteKey
            || k.getKeyCode() == juce::KeyPress::backspaceKey)
            return deleteSelection();
        if (k.getKeyCode() == juce::KeyPress::escapeKey) { clearSelection(); repaintAll(); return true; }
    }
    if (k == juce::KeyPress('v', cmd, 0) && !clipboard_.empty()) {
        const int row = selClipRow_ >= 0 ? selClipRow_
                      : clipboardRow_ >= 0 ? clipboardRow_ : 0;
        if (pasteClips(pasteTickFor(pasteTick, row), row)) return true;
    }

    if (k == juce::KeyPress('d', cmd, 0)) {
        if (clip) {
            host().pushUndo();
            selClip_ = host().clips().duplicate(node, selClip_, ci.startTick + ci.lengthTicks);
            rebuild();
            return true;
        }
        if (box) {
            selBox_ = host().automation().duplicateBox(
                selBox_, host().automation().boxes()[(size_t) selBox_].endBeat);
            rebuild();
            return true;
        }
        return false;
    }
    if (k == juce::KeyPress('c', cmd, 0)) {
        if (clip) copySelectedClips();
        return clip || box;
    }
    if (k == juce::KeyPress('x', cmd, 0)) {
        if (clip) {
            copySelectedClips();
            host().pushUndo();
            host().clips().remove(node, selClip_);
            clearClipSel();
            rebuild();
        }
        return clip || box;
    }
    if (k == juce::KeyPress('v', cmd, 0)) return box;
    if (k.getKeyCode() == juce::KeyPress::deleteKey
        || k.getKeyCode() == juce::KeyPress::backspaceKey) {
        if (clip) {
            host().pushUndo();
            host().clips().remove(node, selClip_);
            clearClipSel();
            rebuild();
            return true;
        }
        if (box) {
            host().automation().deleteBox(selBox_);
            selBox_ = -1;
            rebuild();
            return true;
        }
        return deleteSelectedTracks();
    }
    return false;
}

bool SongView::applyToolAt(int row, juce::Point<int> p, bool alt) {
    const Tool t = effectiveTool();
    if (t == Tool::Pointer || t == Tool::Draw) return false;
    const auto& node = rows_[(size_t) row];
    bool l = false, r = false;
    const int clip = clipAt(row, p, l, r);
    const int bx = clip < 0 ? boxAt(row, p) : -1;
    if (clip < 0 && bx < 0) return true;
    if (t == Tool::Scissors) {
        const double beat = snapBeats(std::max(0.0, xToBeat((float) p.x)), alt);
        if (clip >= 0) {
            host().pushUndo();
            host().clips().split(node, clip, (int) std::llround(beat * Pattern::kTicksPerBeat));
        } else {
            host().automation().splitBox(bx, beat);
        }
    } else if (t == Tool::Eraser) {
        if (clip >= 0) {
            host().pushUndo();
            host().clips().remove(node, clip);
        } else {
            host().automation().deleteBox(bx);
            selBox_ = -1;
        }
    }
    rebuild();
    return true;
}

void SongView::traceSel(const char* what, const juce::MouseEvent& e) const {
    static const bool on = std::getenv("HUMUS_SEL_DEBUG") != nullptr;
    if (!on) return;
    std::fprintf(stderr, "[sel] %s at (%d,%d) shift=%d cmd=%d popup=%d btn=%s sel=%d single=(%d,%d) drag=%d mode=%d\n",
                 what, e.x, e.y, (int) e.mods.isShiftDown(), (int) e.mods.isCommandDown(),
                 (int) e.mods.isPopupMenu(), e.mods.isRightButtonDown() ? "R" : e.mods.isLeftButtonDown() ? "L" : "-",
                 (int) sel_.size(), selClipRow_, selClip_, (int) drag_, (int) inBox());
}

}
