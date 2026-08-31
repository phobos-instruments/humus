#include "gui/TracksPane.h"

#include "gui/FileDragImage.h"

#include <cstdio>
#include <cstdlib>

#include <cmath>

#include "gui/LookAndFeel.h"
#include "gui/OrganismEditor.h"
#include "gui/QwertyPiano.h"

namespace hum {

TracksPane::Tool TracksPane::effectiveTool() const {
    if (!QwertyPiano::instance().enabled()) {
        if (juce::KeyPress::isKeyCurrentlyDown('x')) return Tool::Scissors;
        if (juce::KeyPress::isKeyCurrentlyDown('e')) return Tool::Eraser;
    }
    return tool_;
}

bool TracksPane::selectedClip(std::string& node, ClipEditor::ClipInfo& ci) const {
    if (selClipRow_ < 0 || selClipRow_ >= (int) rows_.size() || selClip_ < 0) return false;
    node = rows_[(size_t) selClipRow_];
    const auto clips = host_.clips().list(node);
    if (selClip_ >= (int) clips.size()) return false;
    ci = clips[(size_t) selClip_];
    return true;
}

bool TracksPane::keyPressed(const juce::KeyPress& k) {
    if (mode_ == Mode::Clip) return keyPressedClip(k);
    if (mode_ == Mode::Box && selPts_.empty()
        && (k.getKeyCode() == juce::KeyPress::escapeKey
            || k.getKeyCode() == juce::KeyPress::returnKey)) {
        leaveBoxMode();
        return true;
    }
    if (mode_ == Mode::Track) {
        if (k.getKeyCode() == juce::KeyPress::escapeKey && !selNotes_.empty()) {
            selNotes_.clear();
            repaint();
            return true;
        }
        if (k.getKeyCode() == juce::KeyPress::escapeKey
            || k.getKeyCode() == juce::KeyPress::returnKey) { leaveTrackMode(); return true; }
        const auto alt = juce::ModifierKeys::altModifier;
        if (k == juce::KeyPress(juce::KeyPress::upKey, alt, 0))   { stepTrack(-1); return true; }
        if (k == juce::KeyPress(juce::KeyPress::downKey, alt, 0)) { stepTrack(+1); return true; }
        const auto cmd = juce::ModifierKeys::commandModifier;
        const auto shift = juce::ModifierKeys::shiftModifier;
        const int grid = std::max(1, (int) std::llround(gridBeats() * Pattern::kTicksPerBeat));
        if (k == juce::KeyPress(juce::KeyPress::upKey, cmd, 0))    { nudgeNotes(0, 0, 10);  return true; }
        if (k == juce::KeyPress(juce::KeyPress::downKey, cmd, 0))  { nudgeNotes(0, 0, -10); return true; }
        const int oct = juce::jmax(1, noteedit::octaveSteps(host_.model()));
        if (k == juce::KeyPress(juce::KeyPress::upKey, shift, 0))  { nudgeNotes(0, oct, 0);  return true; }
        if (k == juce::KeyPress(juce::KeyPress::downKey, shift, 0)){ nudgeNotes(0, -oct, 0); return true; }
        if (k == juce::KeyPress(juce::KeyPress::upKey))            { nudgeNotes(0, 1, 0);   return true; }
        if (k == juce::KeyPress(juce::KeyPress::downKey))          { nudgeNotes(0, -1, 0);  return true; }
        if (k == juce::KeyPress(juce::KeyPress::leftKey))          { nudgeNotes(-grid, 0, 0); return true; }
        if (k == juce::KeyPress(juce::KeyPress::rightKey))         { nudgeNotes(grid, 0, 0);  return true; }
        if (k == juce::KeyPress('a', cmd, 0)) { selectAllNotes(); return true; }
        if (k == juce::KeyPress('c', cmd, 0)) { copySelectedNotes(); return true; }
        if (k == juce::KeyPress('x', cmd, 0)) {
            copySelectedNotes();
            deleteSelectedNotes();
            return true;
        }
        if (k == juce::KeyPress('d', cmd, 0)) return duplicateSelectedNotes();
        if (k == juce::KeyPress('v', cmd, 0))
            return pasteNotes((int) std::llround(
                snapBeats(host_.positionBeats(), false) * Pattern::kTicksPerBeat));
        if (k.getKeyCode() == juce::KeyPress::deleteKey
            || k.getKeyCode() == juce::KeyPress::backspaceKey) {
            if (!selPts_.empty()) return deleteSelectedPoints();
            deleteSelectedNotes();
            return true;
        }
        if (k == juce::KeyPress('1')) { tool_ = Tool::Pointer;  repaint(); return true; }
        if (k == juce::KeyPress('2')) { tool_ = Tool::Draw;     repaint(); return true; }
        if (k == juce::KeyPress('3')) { tool_ = Tool::Line;     repaint(); return true; }
        if (k == juce::KeyPress('4')) { tool_ = Tool::Scissors; repaint(); return true; }
        if (k == juce::KeyPress('5')) { tool_ = Tool::Eraser;   repaint(); return true; }
        return false;
    } else if (k.getKeyCode() == juce::KeyPress::returnKey) {
        if (selClipRow_ >= 0 && selClipRow_ < (int) rows_.size()
            && !host_.nodeRecordsAudio(rows_[(size_t) selClipRow_])) {
            enterTrackMode(rows_[(size_t) selClipRow_]);
            return true;
        }
    }
    if (k == juce::KeyPress('1')) { tool_ = Tool::Pointer;  repaint(); return true; }
    if (k == juce::KeyPress('2')) { tool_ = Tool::Draw;     repaint(); return true; }
    if (k == juce::KeyPress('3')) { tool_ = Tool::Line;     repaint(); return true; }
    if (k == juce::KeyPress('4')) { tool_ = Tool::Scissors; repaint(); return true; }
    if (k == juce::KeyPress('5')) { tool_ = Tool::Eraser;   repaint(); return true; }

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
        && pastePoints(snapBeats(host_.positionBeats(), false)))
        return true;
    std::string node;
    ClipEditor::ClipInfo ci;
    const bool clip = selectedClip(node, ci);
    const bool box = selBox_ >= 0 && selBox_ < (int) host_.automation().boxes().size();
    const int pasteTick = (int) std::llround(
        snapBeats(host_.positionBeats(), false) * Pattern::kTicksPerBeat);

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
        if (k.getKeyCode() == juce::KeyPress::escapeKey) { clearSelection(); repaint(); return true; }
    }
    if (k == juce::KeyPress('v', cmd, 0) && !clipboard_.empty()) {
        const int row = selClipRow_ >= 0 ? selClipRow_
                      : clipboardRow_ >= 0 ? clipboardRow_ : 0;
        if (pasteClips(pasteTick, row)) return true;
    }

    if (k == juce::KeyPress('d', cmd, 0)) {
        if (clip) {
            host_.pushUndo();
            selClip_ = host_.clips().duplicate(node, selClip_, ci.startTick + ci.lengthTicks);
            rebuild();
            return true;
        }
        if (box) {
            selBox_ = host_.automation().duplicateBox(
                selBox_, host_.automation().boxes()[(size_t) selBox_].endBeat);
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
            host_.pushUndo();
            host_.clips().remove(node, selClip_);
            clearClipSel();
            rebuild();
        }
        return clip || box;
    }
    if (k == juce::KeyPress('v', cmd, 0)) return box;
    if (k.getKeyCode() == juce::KeyPress::deleteKey
        || k.getKeyCode() == juce::KeyPress::backspaceKey) {
        if (clip) {
            host_.pushUndo();
            host_.clips().remove(node, selClip_);
            clearClipSel();
            rebuild();
            return true;
        }
        if (box) {
            host_.automation().deleteBox(selBox_);
            selBox_ = -1;
            rebuild();
            return true;
        }
        return false;
    }
    return false;
}

bool TracksPane::applyToolAt(int row, juce::Point<int> p, bool alt) {
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
            host_.pushUndo();
            host_.clips().split(node, clip, (int) std::llround(beat * Pattern::kTicksPerBeat));
        } else {
            host_.automation().splitBox(bx, beat);
        }
    } else if (t == Tool::Eraser) {
        if (clip >= 0) {
            host_.pushUndo();
            host_.clips().remove(node, clip);
        } else {
            host_.automation().deleteBox(bx);
            selBox_ = -1;
        }
    }
    rebuild();
    return true;
}

void TracksPane::traceSel(const char* what, const juce::MouseEvent& e) const {
    static const bool on = std::getenv("HUMUS_SEL_DEBUG") != nullptr;
    if (!on) return;
    std::fprintf(stderr, "[sel] %s at (%d,%d) shift=%d cmd=%d popup=%d btn=%s sel=%d single=(%d,%d) drag=%d mode=%d\n",
                 what, e.x, e.y, (int) e.mods.isShiftDown(), (int) e.mods.isCommandDown(),
                 (int) e.mods.isPopupMenu(), e.mods.isRightButtonDown() ? "R" : e.mods.isLeftButtonDown() ? "L" : "-",
                 (int) sel_.size(), selClipRow_, selClip_, (int) drag_, (int) mode_);
}

bool TracksPane::mouseDownZoom(juce::Point<int> p) {
    const int axes = mode_ == Mode::Box ? 1 : 2;
    for (int i = 0; i < axes * 2; ++i)
        if (zoomBox(i).contains(p)) { zoomBy(i / 2, (i % 2) ? 1.25 : 0.8); return true; }
    for (int axis = 0; axis < axes; ++axis)
        if (zoomGroove(axis).contains(p)) {
            zoomDrag_ = axis;
            const auto gr = zoomGroove(axis);
            setZoomNorm(axis, axis == 0
                            ? (p.x - gr.getX() - 8.0) / std::max(1, gr.getWidth() - 16)
                            : (gr.getBottom() - p.y - 8.0) / std::max(1, gr.getHeight() - 16));
            return true;
        }
    return false;
}

void TracksPane::mouseDown(const juce::MouseEvent& e) {
    traceSel("down", e);
    const auto p = e.getPosition();
    if (mouseDownZoom(p)) return;
    if (mouseDownRoll(e, p)) return;
    if (mouseDownClip(e, p)) return;
    if (mode_ == Mode::Box && p.y < kTopH && crumbBackBox().contains(p)) { leaveBoxMode(); return; }
    if (mouseDownToolbar(p)) return;
    if (p.y < headerH() && p.x >= kStripW) { mouseDownRuler(e, p); return; }
    if (mouseDownAutoLane(e, p)) return;
    if (mouseDownBoxRow(e, p)) return;
    const int row = rowAt(p.y);
    if (row < 0) {
        if (e.mods.isPopupMenu() && p.x < kStripW && p.y >= headerH())
            showAddTrackMenu(localPointToGlobal(p));
        return;
    }
    if (p.x < kStripW) mouseDownHeader(e, row, p);
    else mouseDownBody(e, row, p);
}

bool TracksPane::mouseDownToolbar(juce::Point<int> p) {
    if (p.x >= kStripW || p.y >= headerH()) return false;
    if (followBox().contains(p)) { follow_ = !follow_; repaint(); return true; }
    for (int i = 0; i < kToolCount; ++i)
        if (toolBox(i).contains(p)) {
            tool_ = kToolbar[i];
            repaint();
            return true;
        }
    if (snapBox().contains(p)) { showSnapMenu(localPointToGlobal(p)); return true; }
    if (addTrackBox().contains(p)) {
        showAddTrackMenu(localPointToGlobal(p));
        return true;
    }
    return false;
}

void TracksPane::mouseDownRuler(const juce::MouseEvent& e, juce::Point<int> p) {
        const double b = std::max(0.0, xToBeat((float) p.x));
        if (p.y < loopTop()) return;
        if (p.y < rulerTop()) {
            const double ls = host_.automation().loopStartBeat(), le = host_.automation().loopEndBeat();
            const float xs = beatToX(ls), xe = beatToX(le);
            if (host_.automation().loopEnabled() && std::abs(p.x - xs) <= 5) {
                drag_ = Drag::LoopL; loopOrigStart_ = ls; loopOrigEnd_ = le;
            } else if (host_.automation().loopEnabled() && std::abs(p.x - xe) <= 5) {
                drag_ = Drag::LoopR; loopOrigStart_ = ls; loopOrigEnd_ = le;
            } else if (host_.automation().loopEnabled() && p.x > xs && p.x < xe) {
                drag_ = Drag::LoopMove; loopAnchor_ = b - ls;
                loopOrigStart_ = ls; loopOrigEnd_ = le;
            } else {
                drag_ = Drag::LoopNew; loopAnchor_ = snapBeats(b, e.mods.isAltDown());
            }
        } else if (host_.songEndBeat() > 0.0
                   && std::abs(p.x - beatToX(host_.songEndBeat())) <= 7) {
            drag_ = Drag::SongEnd;
            host_.pushUndo();
        } else if (e.mods.isShiftDown()) {
            drag_ = Drag::TimeSelect;
            selAnchor_ = snapBeats(b, e.mods.isAltDown());
            selFrom_ = selTo_ = selAnchor_;
            hasSel_ = true;
            repaint();
        } else {
            drag_ = Drag::Scrub;
            hasSel_ = false;
            host_.setPositionBeats(b);
            repaint();
        }
        return;
}

bool TracksPane::timeSelection(double& from, double& to) const {
    if (!hasSel_ || selTo_ <= selFrom_) return false;
    from = selFrom_; to = selTo_;
    return true;
}

bool TracksPane::mouseDownAutoLane(const juce::MouseEvent& e, juce::Point<int> p) {
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::PodHeader) {
        const auto& pod = slots_[(size_t) s].param;
        if (p.x < kStripW) {
            if (!collapsedPods_.insert(pod).second) collapsedPods_.erase(pod);
            rebuildSlots();
            repaint();
        }
        return true;
    }
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::AutoLane) {
        const auto& sl = slots_[(size_t) s];
        const auto& lnode = rows_[(size_t) sl.track];
        if (heldLaneBox(sl).contains(p) && host_.automation().isHeld(lnode, sl.param)) {
            host_.automation().release(lnode, sl.param);
            repaint();
            return true;
        }
        if (p.x >= kStripW && !e.mods.isPopupMenu()) {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            if (tryBeginPointSelection(e, s, hit)) return true;
            if (hit < 0 && !selPts_.empty()) clearPointSelection();
        }
        bool muted = false;
        std::string kind = "double";
        if (const auto* cm = host_.model().byName(lnode))
            for (const auto& l : cm->automation)
                if (l.propertyName == sl.param)
                    { muted = l.mute; kind = l.kind; break; }
        if (e.mods.isPopupMenu()) {
            const int hit = (kind == "double" && p.x >= kStripW)
                                ? autoPointAt(sl, lnode, sl.param, p) : -1;
            if (hit >= 0) {
                host_.pushUndo();
                host_.automation().deletePoint(lnode, sl.param, hit);
                repaint();
                return true;
            }
            juce::PopupMenu m;
            m.addItem(1, "Clear Lane Points");
            m.addItem(2, "Delete Lane");
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, node = lnode, param = sl.param](int r) {
                if (r == 1) host_.automation().clearLane(node, param);
                else if (r == 2) host_.automation().remove(node, param);
                if (r > 0) rebuild();
            });
            return true;
        }
        if (p.x < kStripW) {
            const int by = sl.y + (sl.h - 12) / 2;
            if (juce::Rectangle<int>(kStripW - 54, by, 22, 12).contains(p))
                host_.automation().setLaneMute(lnode, sl.param, !muted);
            repaint();
        } else if (kind == "double") {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            const auto tool = effectiveTool();
            if (tool == Tool::Eraser) {
                if (hit >= 0) { host_.pushUndo(); host_.automation().deletePoint(lnode, sl.param, hit); }
                repaint();
                return true;
            }
            if (tool == Tool::Draw || tool == Tool::Line) {
                host_.pushUndo();
                const auto [lo, hi] = laneRange(lnode, sl.param);
                lineSlot_ = s;
                dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
                lineBeat0_ = lineBeat1_ = std::max(0.0, xToBeat((float) p.x));
                lineVal0_ = lineVal1_ = laneValueAtY(sl, p.y, lo, hi);
                if (tool == Tool::Line || e.mods.isShiftDown()) drag_ = Drag::Line;
                else {
                    drag_ = Drag::Pencil;
                    pencilLast_ = -1.0;
                    pencilStep(lnode, sl.param, lineBeat0_, lineVal0_);
                }
                repaint();
                return true;
            }
            if (e.mods.isAltDown()) {
                if (hit >= 0) { host_.pushUndo(); host_.automation().deletePoint(lnode, sl.param, hit); }
                repaint();
                return true;
            }
            if (hit < 0) {
                if (const int seg = segmentAt(sl, lnode, sl.param, p); seg >= 0) {
                    host_.pushUndo();
                    drag_ = Drag::Curve;
                    dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
                    dragCurveIndex_ = seg;
                    dragCurve0_ = host_.automation().curveAt(lnode, sl.param, seg);
                    dragCurveY0_ = p.y;
                    return true;
                }
            }
            host_.pushUndo();
            const auto [lo, hi] = laneRange(lnode, sl.param);
            const double beat = std::max(0.0, xToBeat((float) p.x));
            int idx = hit;
            if (idx < 0) {
                host_.automation().addPoint(lnode, sl.param, beat, laneValueAtY(sl, p.y, lo, hi));
                idx = autoPointAt(sl, lnode, sl.param, p);
            }
            dragAutoSlot_ = s; dragAutoPoint_ = idx;
            dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
            dragAutoGrip_ = AutoGrip::Value;
            repaint();
        } else if (kind == "trigger") {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            if (e.mods.isAltDown()) {
                if (hit >= 0) { host_.pushUndo(); host_.automation().deletePoint(lnode, sl.param, hit); }
                repaint();
                return true;
            }
            host_.pushUndo();
            const double beat = snapBeats(std::max(0.0, xToBeat((float) p.x)), e.mods.isAltDown());
            int idx = hit;
            if (idx < 0) {
                host_.automation().addTriggerPoint(lnode, sl.param, beat);
                idx = autoPointAt(sl, lnode, sl.param, p);
            }
            dragAutoSlot_ = s; dragAutoPoint_ = idx;
            dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
            dragAutoGrip_ = AutoGrip::TriggerTime;
            repaint();
        } else if (kind == "range") {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            if (e.mods.isAltDown()) {
                if (hit >= 0) { host_.pushUndo(); host_.automation().deletePoint(lnode, sl.param, hit); }
                repaint();
                return true;
            }
            host_.pushUndo();
            const auto [lo, hi] = laneRange(lnode, sl.param);
            const double beat = std::max(0.0, xToBeat((float) p.x));
            const double v = laneValueAtY(sl, p.y, lo, hi);
            int idx = hit;
            if (idx < 0) {
                host_.automation().addRangePoint(lnode, sl.param, beat, v, v);
                idx = autoPointAt(sl, lnode, sl.param, p);
            }
            dragAutoSlot_ = s; dragAutoPoint_ = idx;
            dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
            dragAutoGrip_ = e.mods.isShiftDown() ? AutoGrip::RangeBoth
                                                 : nearerRangeEdge(sl, lnode, sl.param, idx, p);
            repaint();
        }
        return true;
    }
    return false;
}

void TracksPane::mouseDownHeader(const juce::MouseEvent& e, int row, juce::Point<int> p) {
    const auto& node = rows_[(size_t) row];
        if (e.mods.isPopupMenu()) {
            const auto* cm = host_.model().byName(node);
            bool hasBoxes = false;
            for (const auto& b : host_.automation().boxes())
                if (b.organism == node) { hasBoxes = true; break; }
            const bool has = (cm != nullptr && !cm->automation.empty()) || hasBoxes;
            juce::PopupMenu m;
            m.addItem(1, "Clear Automation Points", has);
            m.addItem(2, "Delete Automation Lanes", has);
            double from = 0.0, to = 0.0;
            const int bpb = juce::jmax(1, host_.automation().timeSigNumerator());
            const bool canBounce = consolidateRange(row, from, to)
                                   && !host_.bounceSourceOf(node).empty();
            m.addSeparator();
            m.addItem(3, canBounce
                             ? "Consolidate to Audio (bars " + juce::String((int) (from / bpb) + 1)
                                   + juce::String::fromUTF8("\xe2\x80\x93")
                                   + juce::String((int) std::ceil(to / bpb)) + ")"
                             : juce::String("Consolidate to Audio"),
                      canBounce);
            const bool ownsTheRow = host_.nodeRecordsAudio(node);
            m.addSeparator();
            m.addItem(4, ownsTheRow ? "Delete Track" : "Remove Track");
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, node, row](int r) {
                if (r == 3) { consolidateRow(row); return; }
                if (r == 4) {
                    clearClipSel();
                    if (host_.nodeRecordsAudio(node)) {
                        host_.removeOrganism(node);
                    } else if (arrangeable_.count(node) != 0) {
                        host_.pushUndo();
                        host_.clips().removeTrack(node);
                    } else {
                        host_.automation().clearOrganism(node, true);
                    }
                    rebuild();
                    repaint();
                    if (onPatchChanged) onPatchChanged();
                    return;
                }
                if (r > 0) host_.automation().clearOrganism(node, r == 2);
                if (r > 0) rebuild();
            });
            return;
        }
        if (heldBox(row).contains(p) && host_.automation().anyHeld(node)) {
            if (const auto* cm = host_.model().byName(node))
                for (const auto& l : cm->automation)
                    host_.automation().release(node, l.propertyName);
            repaint();
            return;
        }
        if (p.x > 18 && p.x < kStripW - 84 && !e.mods.isPopupMenu()) {
            selectClip(row, -1);
            repaint();
            return;
        }
        if (foldBox(row).contains(p) && hasLanes(row) && !wantsBoxRow(row)) {
            if (!expanded_.insert(node).second) expanded_.erase(node);
            rebuildSlots();
            repaint();
            return;
        }
        if (e.mods.isPopupMenu()) {
            const char* target = muteBox(row).contains(p) ? kTrackMuteParam
                               : soloBox(row).contains(p) ? kSoloParam
                               : recBox(row).contains(p)  ? kArmParam : nullptr;
            if (target != nullptr) {
                showAutomateMenu(host_, node, target, localPointToGlobal(p),
                                 [this] { repaint(); }, false);
                return;
            }
        }
        if (soloBox(row).contains(p) && arrangeable_.count(node) != 0) {
            host_.setSoloed(node, !host_.soloed(node));
            repaint();
            return;
        }
        if (muteBox(row).contains(p)) {
            setNodeMuted(node, !nodeMuted(node));
            repaint();
        } else if (recBox(row).contains(p)) {
            if (host_.nodeRecordsAudio(node)) {
                bool armed = false;
                if (const auto* cm = host_.model().byName(node))
                    for (const auto& pr : cm->properties)
                        if (pr.name == "Record") { armed = pr.value >= 0.5; break; }
                host_.setParam(node, "Record", armed ? 0.0 : 1.0);
                repaint();
                return;
            }
            const bool arm = !host_.midi().isRecordTarget(node);
            host_.midi().setRecordTarget(node, arm, 0, EngineHost::kClipOnDemand, true);
            repaint();
        }
        return;
}

void TracksPane::showBoxMenu(int bx, juce::Point<int> sp) {
    selBox_ = bx;
    juce::PopupMenu m;
    m.addItem(1, "Duplicate After");
    m.addItem(2, "Delete");
    m.addItem(3, "Merge", sel_.size() > 1);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                    [this, bx](int r) {
        const auto& boxes = host_.automation().boxes();
        if (r == 3) { mergeSelection(); return; }
        if (bx >= (int) boxes.size()) return;
        if (r == 1)
            selBox_ = host_.automation().duplicateBox(bx, boxes[(size_t) bx].endBeat);
        else if (r == 2) {
            host_.automation().deleteBox(bx);
            selBox_ = -1;
        }
        rebuild();
    });
    repaint();
}

void TracksPane::beginBoxDrag(int row, int bx, bool leftEdge, bool rightEdge, juce::Point<int> p) {
    clearClipSel();
    selBox_ = bx;
    dragBox_ = bx;
    dragRow_ = row;
    boxAnchor_ = xToBeat((float) p.x);
    boxDragDelta_ = boxTrimL_ = boxTrimR_ = 0.0;
    boxOrigS_ = host_.automation().boxes()[(size_t) bx].startBeat;
    boxOrigE_ = host_.automation().boxes()[(size_t) bx].endBeat;
    drag_ = leftEdge ? Drag::BoxTrimL : rightEdge ? Drag::BoxTrimR : Drag::BoxMove;
    repaint();
}

bool TracksPane::mouseDownBoxRow(const juce::MouseEvent& e, juce::Point<int> p) {
    const int s = trackslayout::slotAt(slots_, p.y);
    if (s < 0 || slots_[(size_t) s].kind != trackslayout::Kind::BoxRow) return false;
    const auto& sl = slots_[(size_t) s];
    const int row = sl.track;
    if (p.x < kStripW) {
        const auto& node = rows_[(size_t) row];
        if (!expanded_.insert(node).second) expanded_.erase(node);
        rebuildSlots();
        repaint();
        return true;
    }
    bool bl = false, br = false;
    const int bx = boxAt(row, p, bl, br);
    if (e.mods.isPopupMenu()) {
        if (bx >= 0) showBoxMenu(bx, e.getScreenPosition());
        return true;
    }
    if (e.mods.isShiftDown()) {
        if (bx >= 0) toggleSelected(boxRef(row, bx));
        repaint();
        return true;
    }
    if (applyToolAt(row, p, e.mods.isAltDown())) return true;
    if (bx >= 0) beginBoxDrag(row, bx, bl, br, p);
    else {
        selBox_ = -1;
        clearSelection();
        marqueeAnchor_ = p;
        clipMarquee_ = juce::Rectangle<int>(p, p);
        drag_ = Drag::ClipMarquee;
        repaint();
    }
    return true;
}

void TracksPane::mouseDownBody(const juce::MouseEvent& e, int row, juce::Point<int> p) {
    const auto& node = rows_[(size_t) row];
    bool leftEdge = false, rightEdge = false;
    const int clip = clipAt(row, p, leftEdge, rightEdge);
    const int tick = std::max(0, xToTick((float) p.x));

    if (e.mods.isPopupMenu()) {
        if (const int bx = boxAt(row, p); clip < 0 && bx >= 0) {
            showBoxMenu(bx, e.getScreenPosition());
            return;
        }
        if (clip >= 0) {
            if (sel_.size() <= 1 && !selected(clipRef(row, clip))) {
                clearSelection();
                sel_.insert(clipRef(row, clip));
            }
            selectClip(row, clip);
            repaint();
            showClipMenu(row, clip, e.getScreenPosition(), tick);
        } else if (host_.nodeRecordsAudio(node)) {
            juce::PopupMenu m;
            m.addItem(1, "Import Audio File...");
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options()
                                .withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, node, tick, alt = e.mods.isAltDown()](int r) {
                if (r != 1) return;
                const int start = (int) std::llround(
                    snapBeats(tick / (double) Pattern::kTicksPerBeat, alt)
                    * Pattern::kTicksPerBeat);
                importAudioInto(node, start);
            });
        }
        return;
    }

    if (e.mods.isShiftDown()) {
        if (sel_.empty() && selClipRow_ >= 0 && selClip_ >= 0)
            sel_.insert(clipRef(selClipRow_, selClip_));
        if (clip >= 0) toggleSelected(clipRef(row, clip));
        else if (const int bx = boxAt(row, p); bx >= 0) toggleSelected(boxRef(row, bx));
        syncTimeSelection();
        repaint();
        return;
    }

    if (applyToolAt(row, p, e.mods.isAltDown())) return;

    if (bool bl = false, br = false; clip < 0)
        if (const int bx = boxAt(row, p, bl, br); bx >= 0) {
            beginBoxDrag(row, bx, bl, br, p);
            return;
        }
    selBox_ = -1;

    dragRow_ = row;
    if (clip >= 0 && !selected(clipRef(row, clip))) { clearSelection(); sel_.insert(clipRef(row, clip)); }
    if (clip >= 0 && effectiveTool() == Tool::Draw) {
        selectClip(row, clip);
        repaint();
        return;
    }
    if (clip >= 0) {
        host_.pushUndo();
        const auto clips = host_.clips().list(node);
        const auto& ci = clips[(size_t) clip];
        dragOriginTick_ = ci.startTick;
        dragOriginNode_ = node;
        dragDuplicated_ = false;
        if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
            const int made = beginClipMove(row, clip, true);
            if (made < 0) { repaint(); return; }
            dragClip_ = made;
            dragDuplicated_ = true;
            selectClip(row, made);
            drag_ = Drag::ClipMove;
            dragGrabTicks_ = tick - ci.startTick;
        } else if (const auto fg = timelinechrome::fadeGripAt(clipBounds(row, ci), p, kFadeGrip,
                                                             ci.fadeInTicks,
                                                             ci.fadeOutTicks,
                                                             ci.lengthTicks);
                   fg != timelinechrome::FadeGrip::None) {
            drag_ = fg == timelinechrome::FadeGrip::Left ? Drag::ClipFadeL : Drag::ClipFadeR;
            dragClip_ = clip;
            selectClip(row, clip);
        } else if (const auto cg = timelinechrome::fadeCurveGripAt(
                       clipBounds(row, ci), p, kFadeGrip, ci.fadeInTicks, ci.fadeOutTicks,
                       ci.lengthTicks, ci.fadeInCurve, ci.fadeOutCurve);
                   cg != timelinechrome::FadeGrip::None) {
            drag_ = cg == timelinechrome::FadeGrip::Left ? Drag::ClipFadeCurveL
                                                        : Drag::ClipFadeCurveR;
            dragClip_ = clip;
            dragCurveY0_ = p.y;
            dragCurve0_ = cg == timelinechrome::FadeGrip::Left ? ci.fadeInCurve : ci.fadeOutCurve;
            host_.pushUndo();
            selectClip(row, clip);
        } else if (!ci.looped && overRepeatGrip(clipBounds(row, ci), p)) {
            drag_ = Drag::ClipRepeat;
            dragClip_ = clip;
            repeatSrcId_ = ci.id;
            repeatStart_ = ci.startTick;
            repeatLen_ = std::max(1, ci.lengthTicks);
            repeatIds_.clear();
            selectClip(row, clip);
        } else if (leftEdge) {
            drag_ = Drag::ClipResizeL;
            dragClip_ = clip;
            selectClip(row, clip);
        } else if (rightEdge) {
            drag_ = Drag::ClipResizeR;
            dragClip_ = clip;
            selectClip(row, clip);
        } else {
            drag_ = Drag::ClipMove;
            dragClip_ = clip;
            dragGrabTicks_ = tick - ci.startTick;
            selectClip(row, clip);
            beginClipMove(row, clip, false);
        }
        syncTimeSelection();
    } else {
        clearClipSel();
        clearSelection();
        if (effectiveTool() != Tool::Draw) {
            marqueeAnchor_ = p;
            clipMarquee_ = juce::Rectangle<int>(p, p);
            drag_ = Drag::ClipMarquee;
            repaint();
            return;
        }
        if (host_.nodeRecordsAudio(node) || arrangeable_.count(node) == 0) { repaint(); return; }
        host_.pushUndo();
        const int start = (int) std::llround(
            snapBeats(tick / (double) Pattern::kTicksPerBeat, e.mods.isAltDown())
            * Pattern::kTicksPerBeat);
        dragClip_ = host_.clips().add(node, start, barTicks());
        drag_ = Drag::ClipCreate;
        selectClip(row, dragClip_);
    }
    repaint();
}

void TracksPane::mouseDrag(const juce::MouseEvent& e) {
    if (!dragSync_ && (drag_ != Drag::None || clipDrag_ != ClipDrag::None))
        dragSync_.emplace(host_);
    if (zoomDrag_ >= 0) {
        const auto gr = zoomGroove(zoomDrag_);
        const auto p = e.getPosition();
        setZoomNorm(zoomDrag_, zoomDrag_ == 0
                        ? (p.x - gr.getX() - 8.0) / std::max(1, gr.getWidth() - 16)
                        : (gr.getBottom() - p.y - 8.0) / std::max(1, gr.getHeight() - 16));
        return;
    }
    edgeScroll(e.getPosition());
    if (mode_ == Mode::Clip && drag_ == Drag::None) { mouseDragClip(e); return; }
    if (mode_ == Mode::Track && rollKeyNote_ >= 0) {
        if (const auto rp = rollPlot(); rp.usable)
            soundRollKey(rp.pitchAt((float) e.getPosition().y));
        return;
    }
    if (mode_ == Mode::Track && rollDrag_ != RollDrag::None) { mouseDragRoll(e); return; }
    const auto p = e.getPosition();
    const double beat = std::max(0.0, xToBeat((float) p.x));
    const bool alt = e.mods.isAltDown();

    if ((drag_ == Drag::Pencil || drag_ == Drag::Line) && lineSlot_ >= 0
        && lineSlot_ < (int) slots_.size()) {
        const auto& sl = slots_[(size_t) lineSlot_];
        const auto [lo, hi] = laneRange(dragAutoNode_, dragAutoParam_);
        const double v = laneValueAtY(sl, p.y, lo, hi);
        if (drag_ == Drag::Pencil) pencilStep(dragAutoNode_, dragAutoParam_, beat, v);
        else { lineBeat1_ = beat; lineVal1_ = v; }
        repaint();
        return;
    }

    if (dragAutoPoint_ >= 0 && dragAutoSlot_ >= 0 && dragAutoSlot_ < (int) slots_.size()) {
        const auto& sl = slots_[(size_t) dragAutoSlot_];
        const auto [lo, hi] = laneRange(dragAutoNode_, dragAutoParam_);
        const double v = laneValueAtY(sl, p.y, lo, hi);
        if (dragAutoGrip_ == AutoGrip::Value) {
            dragAutoPoint_ = host_.automation().movePoint(
                dragAutoNode_, dragAutoParam_, dragAutoPoint_, beat, v);
        } else if (dragAutoGrip_ == AutoGrip::TriggerTime) {
            dragAutoPoint_ = host_.automation().movePoint(
                dragAutoNode_, dragAutoParam_, dragAutoPoint_,
                snapBeats(beat, e.mods.isAltDown()), 0.0);
        } else {
            double curLo = v, curHi = v, curBeat = beat;
            if (const auto* cm = host_.model().byName(dragAutoNode_))
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
            dragAutoPoint_ = host_.automation().moveRangePoint(
                dragAutoNode_, dragAutoParam_, dragAutoPoint_, beat, curLo, curHi);
        }
        repaint();
        return;
    }

    switch (drag_) {
        case Drag::BoxMove:
            boxDragDelta_ = snapBeats(beat, alt) - snapBeats(boxAnchor_, alt);
            if (dragBox_ >= 0 && dragBox_ < (int) host_.automation().boxes().size())
                boxDragDelta_ = std::max(boxDragDelta_,
                    -host_.automation().boxes()[(size_t) dragBox_].startBeat);
            repaint();
            return;
        case Drag::BoxTrimL:
            boxTrimL_ = juce::jlimit(-boxOrigS_, boxOrigE_ - 0.25 - boxOrigS_,
                                     snapBeats(beat, alt) - boxOrigS_);
            repaint();
            return;
        case Drag::BoxTrimR:
            boxTrimR_ = std::max(boxOrigS_ + 0.25 - boxOrigE_,
                                 snapBeats(beat, alt) - boxOrigE_);
            repaint();
            return;
        case Drag::Scrub:
            host_.setPositionBeats(beat);
            repaint();
            return;
        case Drag::SongEnd:
            host_.setSongLengthBeats(snapBeats(beat, alt));
            repaint();
            return;
        case Drag::LoopNew: {
            const double b = snapBeats(beat, alt);
            host_.automation().setLoop(std::min(loopAnchor_, b), std::max(loopAnchor_, b),
                                    std::abs(b - loopAnchor_) > 1e-6);
            repaint();
            return;
        }
        case Drag::LoopL:
            host_.automation().setLoop(std::min(snapBeats(beat, alt), loopOrigEnd_ - 1e-3),
                                    loopOrigEnd_, true);
            repaint();
            return;
        case Drag::LoopR:
            host_.automation().setLoop(loopOrigStart_,
                                    std::max(snapBeats(beat, alt), loopOrigStart_ + 1e-3), true);
            repaint();
            return;
        case Drag::LoopMove: {
            const double len = loopOrigEnd_ - loopOrigStart_;
            const double s = snapBeats(beat - loopAnchor_, alt);
            host_.automation().setLoop(std::max(0.0, s), std::max(0.0, s) + len, true);
            repaint();
            return;
        }
        default: break;
    }

    if (drag_ == Drag::ClipMarquee) { updateClipMarquee(p); return; }
    if (dragRow_ < 0 || dragClip_ < 0) return;
    const auto& node = rows_[(size_t) dragRow_];
    const auto clips = host_.clips().list(node);
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
            host_.clips().resize(node, dragClip_, std::max(Pattern::kTicksPerBeat / 4,
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
            host_.automation().setCurve(dragAutoNode_, dragAutoParam_, dragCurveIndex_,
                                        dragCurve0_ + d);
            repaint();
            break;
        }
        case Drag::PointMarquee:
            updatePointMarquee(p);
            break;
        case Drag::PointGroup:
            dragSelectedPoints(e);
            break;
        case Drag::TimeSelect: {
            const double b = snapBeats(std::max(0.0, xToBeat((float) e.x)), e.mods.isAltDown());
            selFrom_ = std::min(selAnchor_, b);
            selTo_ = std::max(selAnchor_, b);
            repaint();
            break;
        }
        case Drag::ClipFadeL:
        case Drag::ClipFadeR: {
            const auto clips = host_.clips().list(dragRow_ >= 0 ? rows_[(size_t) dragRow_]
                                                                : std::string());
            if (dragClip_ < 0 || dragClip_ >= (int) clips.size()) break;
            const auto& ci = clips[(size_t) dragClip_];
            const int t = (int) std::llround(snapBeats(xToBeat((float) e.x),
                                                       e.mods.isAltDown())
                                             * Pattern::kTicksPerBeat);
            const int len = std::max(1, ci.lengthTicks);
            if (drag_ == Drag::ClipFadeL)
                host_.clips().setFades(rows_[(size_t) dragRow_], dragClip_,
                                       juce::jlimit(0, len, t - ci.startTick),
                                       ci.fadeOutTicks);
            else
                host_.clips().setFades(rows_[(size_t) dragRow_], dragClip_, ci.fadeInTicks,
                                       juce::jlimit(0, len, ci.startTick + len - t));
            repaint();
            break;
        }
        case Drag::ClipFadeCurveL:
        case Drag::ClipFadeCurveR: {
            const auto clips = host_.clips().list(rows_[(size_t) dragRow_]);
            if (dragClip_ < 0 || dragClip_ >= (int) clips.size()) break;
            const auto& fc = clips[(size_t) dragClip_];
            const int h = clipBounds(dragRow_, fc).getHeight();
            const double c = timelinechrome::fadeCurveFromDrag(dragCurve0_, e.y - dragCurveY0_, h);
            if (drag_ == Drag::ClipFadeCurveL)
                host_.clips().setFadeCurves(rows_[(size_t) dragRow_], dragClip_, c,
                                            fc.fadeOutCurve);
            else
                host_.clips().setFadeCurves(rows_[(size_t) dragRow_], dragClip_, fc.fadeInCurve, c);
            repaint();
            break;
        }
        case Drag::ClipResizeL: {
            const int end = ci.startTick + ci.lengthTicks;
            const int ns = std::min(snapT(tick), end - Pattern::kTicksPerBeat / 4);
            host_.clips().resize(node, dragClip_, end - std::max(0, ns), true);
            break;
        }
        default: break;
    }
    repaint();
}

bool TracksPane::leftForGood(juce::Point<int> p) const {
    const auto b = getLocalBounds();
    return p.x > b.getRight() + kDragOutSlack
           || p.y < b.getY() - kDragOutSlack
           || p.y > b.getBottom() + kDragOutSlack;
}

juce::Rectangle<int> TracksPane::clipChipBounds(int row, const ClipEditor::ClipInfo& ci) const {
    const auto field = getLocalBounds().withTrimmedLeft(kStripW).withTrimmedTop(headerH());
    auto box = clipBounds(row, ci).getIntersection(field);
    if (box.isEmpty()) return {};
    return box.withWidth(std::min(box.getWidth(), 220));
}

juce::Image TracksPane::clipChip(int row, const ClipEditor::ClipInfo& ci) const {
    const auto box = clipChipBounds(row, ci);
    if (box.isEmpty()) return {};
    return const_cast<TracksPane*>(this)->createComponentSnapshot(box, true, kChipScale);
}

void TracksPane::dragClipOut(const std::string& node) {
    if (extDrag_ || dragClip_ < 0) return;
    auto clips = host_.clips().list(node);
    if (dragClip_ >= (int) clips.size()) return;
    const bool audio = clips[(size_t) dragClip_].isAudio;
    const juce::Image chip = clipChip(dragRow_, clips[(size_t) dragClip_]);
    std::string home = node;
    int clip = dragClip_;
    if (dragDuplicated_) {
        for (const auto& m : moveBase_)
            if (const int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id); at >= 0)
                host_.clips().remove(rows_[(size_t) m.curRow], at);
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
    repaint();
    if (clip < 0) return;
    clips = host_.clips().list(home);
    if (clip >= (int) clips.size()) return;
    if (!audio) {
        if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this))
            dnd->startDragging(juce::String("noteclip:")
                                   + juce::String(juce::CharPointer_UTF8(home.c_str()))
                                   + ":" + juce::String(clips[(size_t) clip].id),
                               this, juce::ScaledImage(chip, kChipScale));
        return;
    }
    juce::StringArray files;
    auto addExport = [&](const std::string& n, int c) {
        const auto path = host_.clips().exportFile(n, c);
        if (!path.empty()) files.add(juce::String(juce::CharPointer_UTF8(path.c_str())));
    };
    std::set<std::pair<std::string, int>> done;
    for (const auto& ref : sel_) {
        if (ref.kind != timeline::ItemRef::Kind::Clip) continue;
        const auto& rn = rows_[(size_t) ref.row];
        for (const auto& ci : host_.clips().list(rn))
            if (ci.id == ref.key && ci.isAudio && done.insert({rn, ci.index}).second) addExport(rn, ci.index);
    }
    if (done.insert({home, clip}).second) addExport(home, clip);
    if (files.isEmpty()) return;
    extDrag_ = true;
    if (!dragFilesWithImage(files, this, chip, [this] { extDrag_ = false; }))
        juce::DragAndDropContainer::performExternalDragDropOfFiles(files, false, this,
                                                                   [this] { extDrag_ = false; });
}

bool TracksPane::isInterestedInDragSource(const SourceDetails& d) {
    return d.description.toString().startsWith("print:");
}

void TracksPane::itemDropped(const SourceDetails& d) {
    dropHot_ = false;
    const auto node = d.description.toString().fromFirstOccurrenceOf("print:", false, false).toStdString();
    const int bar = 4 * Pattern::kTicksPerBeat;
    const int at = (std::max(0, xToTick((float) d.localPosition.x)) / bar) * bar;
    std::string err;
    host_.printToTimeline(node, err, at);
    if (!err.empty())
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "MIDI to Track",
                                               juce::String(juce::CharPointer_UTF8(err.c_str())));
    rebuild();
    repaint();
}

void TracksPane::mouseUp(const juce::MouseEvent& e) {
    dragSync_.reset();
    traceSel("up", e);
    if (zoomDrag_ >= 0) { zoomDrag_ = -1; repaint(); return; }
    if (mode_ == Mode::Track) { mouseUpRoll(); return; }
    if (mode_ == Mode::Clip && drag_ == Drag::None) { mouseUpClip(); return; }
    if (drag_ == Drag::BoxMove && dragBox_ >= 0 && std::abs(boxDragDelta_) > 1e-9) {
        host_.automation().moveBox(dragBox_, boxDragDelta_);
        rebuild();
    }
    if ((drag_ == Drag::BoxTrimL || drag_ == Drag::BoxTrimR) && dragBox_ >= 0
        && std::abs(boxTrimL_) + std::abs(boxTrimR_) > 1e-9) {
        host_.automation().trimBox(dragBox_, boxOrigS_ + boxTrimL_, boxOrigE_ + boxTrimR_);
        rebuild();
    }
    if (drag_ == Drag::Line && lineSlot_ >= 0)
        commitLine(dragAutoNode_, dragAutoParam_, lineBeat0_, lineVal0_, lineBeat1_, lineVal1_);
    lineSlot_ = -1;
    pencilLast_ = -1.0;
    dragBox_ = -1;
    boxDragDelta_ = boxTrimL_ = boxTrimR_ = 0.0;
    clipMarquee_ = {};
    drag_ = Drag::None;
    moveBase_.clear();
    dragRow_ = dragClip_ = -1;
    dragAutoSlot_ = dragAutoPoint_ = -1;
    dragCurveIndex_ = -1;
    repeatSrcId_ = 0;
    repeatIds_.clear();
    dragAutoGrip_ = AutoGrip::Value;
    dragAutoNode_.clear(); dragAutoParam_.clear();
    repaint();
}

void TracksPane::mouseDoubleClick(const juce::MouseEvent& e) {
    traceSel("dbl", e);
    const auto p = e.getPosition();
    if (mode_ == Mode::Clip) {
        const auto h = clipEditorHit(p);
        if (h == ClipHit::CurveL || h == ClipHit::CurveR) straightenFade(h == ClipHit::CurveL);
        return;
    }
    if (mode_ == Mode::Track) return;
    if (p.x < kStripW && p.y >= headerH()) {
        if (const int r = rowAt(p.y); r >= 0) {
            selectClip(r, -1);
            if (onSelect) onSelect(rows_[(size_t) r]);
        }
        return;
    }
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::BoxRow) {
        if (const int bx = boxAt(slots_[(size_t) s].track, p);
            bx >= 0 && onOpenBoxDetail && mode_ != Mode::Box)
            onOpenBoxDetail(host_.automation().boxes()[(size_t) bx].organism);
        return;
    }
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::AutoLane) {
        if (slots_[(size_t) s].laneKind != "double" && onOpenAutomation)
            onOpenAutomation(rows_[(size_t) slots_[(size_t) s].track], slots_[(size_t) s].param);
        return;
    }
    const int row = rowAt(p.y);
    if (row < 0 || p.x < kStripW) return;
    bool l = false, r = false;
    const int clip = clipAt(row, p, l, r);
    if (const auto h = rowClipHit(row, p); h == ClipHit::CurveL || h == ClipHit::CurveR) {
        const auto clips = host_.clips().list(rows_[(size_t) row]);
        if (clip >= 0 && clip < (int) clips.size()) {
            const auto& ci = clips[(size_t) clip];
            host_.pushUndo();
            host_.clips().setFadeCurves(rows_[(size_t) row], clip,
                                        h == ClipHit::CurveL ? 0.0 : ci.fadeInCurve,
                                        h == ClipHit::CurveR ? 0.0 : ci.fadeOutCurve);
            repaint();
        }
        return;
    }
    if (const int bx = boxAt(row, p); clip < 0 && bx >= 0) {
        if (onOpenBoxDetail)
            onOpenBoxDetail(host_.automation().boxes()[(size_t) bx].organism);
        return;
    }
    if (clip >= 0) {
        const auto clips = host_.clips().list(rows_[(size_t) row]);
        if (clip < (int) clips.size() && clips[(size_t) clip].isAudio) {
            enterClipMode(rows_[(size_t) row], clips[(size_t) clip].id);
            return;
        }
        enterTrackMode(rows_[(size_t) row]);
    }
}

void TracksPane::mouseMove(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    const auto was = hover_;
    hover_ = p;
    if (p.y >= loopTop() && p.y < rulerTop() && host_.automation().loopEnabled()) {
        const float xs = beatToX(host_.automation().loopStartBeat());
        const float xe = beatToX(host_.automation().loopEndBeat());
        if (std::abs(p.x - xs) <= 5 || std::abs(p.x - xe) <= 5) {
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            return;
        }
    }
    if (mode_ == Mode::Clip) {
        setMouseCursor(clipEditorCursor(p));
        return;
    }
    const int row = rowAt(p.y);
    const Tool tool = effectiveTool();
    if (tool == Tool::Scissors) repaintCutGuide(was.x, p.x);
    if (mode_ == Mode::Track && rollPlot().usable && rollField().contains(p)) {
        if (tool != Tool::Pointer) { setMouseCursor(timelinechrome::toolCursor(tool)); return; }
        int clip = -1, index = -1;
        bool nl = false, nr = false;
        noteAt(p, clip, index, nl, nr);
        setMouseCursor(index < 0 ? juce::MouseCursor(juce::MouseCursor::NormalCursor)
                     : nl || nr  ? juce::MouseCursor(juce::MouseCursor::LeftRightResizeCursor)
                                 : juce::MouseCursor(juce::MouseCursor::DraggingHandCursor));
        return;
    }
    if (const int s = trackslayout::slotAt(slots_, p.y); s >= 0 && p.x >= kStripW) {
        const auto& sl = slots_[(size_t) s];
        if (sl.kind == trackslayout::Kind::AutoLane) {
            const int hit = autoPointAt(sl, rows_[(size_t) sl.track], sl.param, p);
            if (hit != hoverPt_ || s != hoverPtSlot_) {
                hoverPtSlot_ = s; hoverPt_ = hit;
                repaint(kStripW, sl.y, getWidth() - kStripW, sl.h);
            }
            if (tool != Tool::Pointer) { setMouseCursor(timelinechrome::toolCursor(tool)); return; }
            setMouseCursor(hit >= 0 ? juce::MouseCursor::DraggingHandCursor
                                    : juce::MouseCursor::NormalCursor);
            return;
        }
        if (sl.kind == trackslayout::Kind::BoxRow) {
            bool bl = false, br = false;
            boxAt(sl.track, p, bl, br);
            setMouseCursor(bl || br ? juce::MouseCursor::LeftRightResizeCursor
                                    : juce::MouseCursor::NormalCursor);
            return;
        }
    }
    if (hoverPt_ >= 0 && hoverPtSlot_ >= 0 && hoverPtSlot_ < (int) slots_.size()) {
        const auto& hs = slots_[(size_t) hoverPtSlot_];
        repaint(kStripW, hs.y, getWidth() - kStripW, hs.h);
        hoverPtSlot_ = hoverPt_ = -1;
    }
    if (tool != Tool::Pointer && row >= 0 && p.x >= kStripW) {
        setMouseCursor(timelinechrome::toolCursor(tool));
        return;
    }
    bool l = false, r = false;
    if (row >= 0 && p.x >= kStripW) {
        if (const int c = clipAt(row, p, l, r); c < 0) boxAt(row, p, l, r);
        else if (const auto clips = host_.clips().list(rows_[(size_t) row]);
                 c < (int) clips.size()) {
            const auto cb = clipBounds(row, clips[(size_t) c]);
            const auto& hc = clips[(size_t) c];
            if (const auto fg = timelinechrome::fadeGripAt(cb, p, kFadeGrip, hc.fadeInTicks,
                                                           hc.fadeOutTicks, hc.lengthTicks);
                fg != timelinechrome::FadeGrip::None) {
                setMouseCursor(fg == timelinechrome::FadeGrip::Left
                                   ? juce::MouseCursor::TopLeftCornerResizeCursor
                                   : juce::MouseCursor::TopRightCornerResizeCursor);
                return;
            }
            if (timelinechrome::fadeCurveGripAt(cb, p, kFadeGrip, hc.fadeInTicks, hc.fadeOutTicks,
                                                hc.lengthTicks, hc.fadeInCurve, hc.fadeOutCurve)
                != timelinechrome::FadeGrip::None) {
                setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
                return;
            }
            if (!clips[(size_t) c].looped && overRepeatGrip(cb, p)) {
                setMouseCursor(juce::MouseCursor::CopyingCursor);
                return;
            }
        }
    }
    setMouseCursor(l || r ? juce::MouseCursor::LeftRightResizeCursor
                          : juce::MouseCursor::NormalCursor);
}

void TracksPane::mouseWheelMove(const juce::MouseEvent& e,
                                const juce::MouseWheelDetails& wheel) {
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
        zoomAbout((float) e.getPosition().x, wheel.deltaY > 0 ? 1.15 : 1.0 / 1.15);
        return;
    }
    if (e.mods.isShiftDown()) {
        const double d = (std::abs(wheel.deltaX) > std::abs(wheel.deltaY)
                              ? wheel.deltaX : wheel.deltaY);
        scrollBeats_ = std::max(0.0, scrollBeats_ - d * 0.5 * (getWidth() - kStripW) / ppb_);
    } else if (mode_ == Mode::Clip) {
        const double d = std::abs(wheel.deltaX) > std::abs(wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
        scrollBeats_ = std::max(0.0, scrollBeats_ - d * 0.5 * (getWidth() - kStripW) / ppb_);
    } else if (mode_ == Mode::Track) {
        if (wheel.deltaX != 0.0f)
            scrollBeats_ = std::max(0.0, scrollBeats_
                                             - wheel.deltaX * 0.5 * (getWidth() - kStripW) / ppb_);
        if (wheel.deltaY != 0.0f) {
            if (e.mods.isAltDown())
                rollRowH_ = juce::jlimit(3.0f, 24.0f,
                                         rollRowH_ * (wheel.deltaY > 0 ? 1.15f : 1.0f / 1.15f));
            else {
                rollScrollAcc_ += wheel.deltaY * 8.0f;
                const float whole = std::floor(rollScrollAcc_);
                if (whole != 0.0f) {
                    rollScrollAcc_ -= whole;
                    const int lo = -rollTopPitch_, hi = 127 - rollTopPitch_;
                    const int want = rollScrollSemis_ + (int) whole;
                    rollScrollSemis_ = juce::jlimit(juce::jmin(lo, 0), juce::jmax(hi, 0), want);
                    if (rollScrollSemis_ != want) rollScrollAcc_ = 0.0f;
                }
            }
        }
    } else {
        if (wheel.deltaX != 0.0f)
            scrollBeats_ = std::max(0.0, scrollBeats_
                                             - wheel.deltaX * 0.5 * (getWidth() - kStripW) / ppb_);
        if (wheel.deltaY != 0.0f)
            applyVScroll(vScroll_ - (int) std::round(wheel.deltaY * 240.0));
    }
    repaint();
}

}
