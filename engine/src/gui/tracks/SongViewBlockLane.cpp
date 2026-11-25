// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <algorithm>
#include <cmath>

#include "core/params/LaneKind.h"
#include "core/timeline/BlockLane.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/tracks/SongView.h"
#include "gui/tracks/TimelineTools.h"

namespace hum {

namespace {

constexpr double kMinBlockBeats = 1.0 / 16.0;
constexpr int kEdgeGrabPx = 5;
constexpr int kDragStartPx = 4;
constexpr int kFirstValueId = 100;
constexpr int kDeleteBlockId = 1;
constexpr int kClearLaneId = 2;
constexpr int kDeleteLaneId = 3;
constexpr int kDuplicateId = 4;

juce::Colour blockColour(int value, int count) {
    return Palette::accent.withRotatedHue((float) value / (float) std::max(1, count));
}

}

bool SongView::isBlockLane(const std::string& node, const std::string& param) const {
    const auto* cm = host().model().byName(node);
    if (cm == nullptr || isClockPseudo(cm->displayClass)) return false;
    for (const auto& l : cm->automation)
        if (l.propertyName == param) return l.kind == "step";
    return false;
}

const std::vector<std::string>& SongView::blockLabels(const std::string& node, const std::string& param) const {
    static const std::vector<std::string> none;
    const auto* cm = host().model().byName(node);
    return cm == nullptr ? none : blockLabelsFor(cm->classRaw, param);
}

std::vector<std::string> SongView::blockLaneCandidates(const std::string& node) const {
    std::vector<std::string> out;
    const auto* cm = host().model().byName(node);
    if (cm == nullptr || isClockPseudo(cm->displayClass)) return out;
    for (const auto& d : schemaFor(cm->classRaw)) {
        if (!d.isEnum || !d.offered() || blockLabelsFor(cm->classRaw, d.name).empty()) continue;
        const bool automated = std::any_of(cm->automation.begin(), cm->automation.end(),
                                           [&](const AutomationLane& l) { return l.propertyName == d.name; });
        if (!automated) out.push_back(d.name);
    }
    return out;
}

void SongView::paintBlockLane(juce::Graphics& g, const trackslayout::Slot& slot, const AutomationLane& lane) {
    const auto& node = rows_[(size_t) slot.track];
    const auto& labels = blockLabels(node, slot.param);
    const auto [lo, hi] = laneRange(node, slot.param);
    const int count = blockChoiceCount(labels, lo, hi);
    const bool muted = lane.mute;
    const float top = (float) slot.y + 3.0f, height = (float) slot.h - 6.0f;
    const float left = (float) kStripW, right = (float) getWidth();
    g.saveState();
    g.reduceClipRegion(kStripW, slot.y, getWidth() - kStripW, slot.h);
    for (const auto& b : blocklane::blocksOf(lane.points)) {
        const float x0 = std::max(left, beatToX(b.start));
        const float x1 = b.open() ? right : beatToX(b.end);
        if (x1 <= left || x0 >= right || x1 - x0 < 1.0f) continue;
        const bool chosen = selected(blockRef(slot.track, slot.param, b.start));
        const auto colour = muted ? Palette::textDim : blockColour(b.value, count);
        const juce::Rectangle<float> r(x0 + 1.0f, top, x1 - x0 - 2.0f, height);
        g.setColour(colour.withAlpha(chosen ? 0.6f : 0.32f));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(chosen ? Palette::text : colour);
        g.drawRoundedRectangle(r, 3.0f, chosen ? 1.6f : 1.0f);
        g.setColour(colour);
        g.fillRect(r.getX(), r.getY() + 2.0f, 2.0f, r.getHeight() - 4.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8(blocklane::labelFor(labels, b.value).c_str()),
                   r.withTrimmedLeft(6.0f).withTrimmedRight(2.0f), juce::Justification::centredLeft, true);
    }
    g.restoreState();
}

juce::MouseCursor SongView::blockLaneCursor(const trackslayout::Slot& slot, juce::Point<int> p) const {
    const auto* lane = laneOfSlot(slot);
    if (lane == nullptr) return juce::MouseCursor::NormalCursor;
    const double tolerance = xToBeat((float) (p.x + kEdgeGrabPx)) - xToBeat((float) p.x);
    const double beat = xToBeat((float) p.x);
    if (blocklane::edgeNear(lane->points, beat, tolerance) >= 0) return juce::MouseCursor::LeftRightResizeCursor;
    if (blocklane::blockAt(blocklane::blocksOf(lane->points), beat) < 0) return timelinechrome::toolCursor(Tool::Draw);
    return juce::MouseCursor::DraggingHandCursor;
}

bool SongView::mouseDownBlockLane(const juce::MouseEvent& e, int s, juce::Point<int> p) {
    const auto& sl = slots_[(size_t) s];
    const auto* lane = laneOfSlot(sl);
    if (lane == nullptr) return false;
    const auto node = rows_[(size_t) sl.track];
    const double beat = std::max(0.0, xToBeat((float) p.x));
    const double tolerance = xToBeat((float) (p.x + kEdgeGrabPx)) - xToBeat((float) p.x);
    const auto tool = effectiveTool();
    const auto blocks = blocklane::blocksOf(lane->points);
    const int body = blocklane::blockAt(blocks, beat);
    const int edge = tool == Tool::Pointer ? blocklane::edgeNear(lane->points, beat, tolerance) : -1;
    const int block = edge >= 0 ? blocklane::blockOfPoint(lane->points, edge) : body;
    const int faceplate = faceplateOf(node, sl.param);
    if (e.mods.isPopupMenu()) {
        showBlockMenu(e.getScreenPosition(), s, block);
        return true;
    }
    if (tool == Tool::Eraser || tool == Tool::Scissors) {
        const auto next = tool == Tool::Eraser
                              ? blocklane::withoutBlock(lane->points, body, faceplate)
                              : blocklane::withSplit(lane->points, snapBeats(beat, e.mods.isAltDown()));
        if (!samePoints(next, lane->points)) {
            host().pushUndo();
            host().automation().setPoints(node, sl.param, next);
            pruneBlockSelection();
            repaintAll();
        }
        return true;
    }
    const bool onBody = tool == Tool::Pointer && edge < 0 && body >= 0;
    if (onBody && (e.mods.isAltDown() || e.mods.isShiftDown())) {
        pickBlock(s, blocks[(size_t) body].start, e.mods);
        repaintAll();
        return true;
    }
    const auto& labels = blockLabels(node, sl.param);
    const auto [lo, hi] = laneRange(node, sl.param);
    const int count = blockChoiceCount(labels, lo, hi);
    const int first = (int) std::lround(lo);
    const int current = block >= 0 ? blocks[(size_t) block].value : faceplate;
    block_ = {};
    block_.slot = s;
    block_.node = node;
    block_.param = sl.param;
    block_.before = lane->points;
    block_.edge = edge;
    block_.block = block;
    block_.value = first + (current - first + 1) % count;
    block_.fallback = faceplate;
    block_.anchor = snapBeats(beat, false);
    block_.downX = p.x;
    const bool copy = e.mods.isCommandDown() || e.mods.isCtrlDown();
    block_.inGap = tool == Tool::Pointer && edge < 0 && body < 0;
    block_.mode = tool != Tool::Pointer || block_.inGap ? BlockGesture::Mode::Paint
                : edge >= 0             ? BlockGesture::Mode::Edge
                : copy                  ? BlockGesture::Mode::Copy
                                        : BlockGesture::Mode::Move;
    if (onBody) {
        block_.start = blocks[(size_t) body].start;
        block_.groupWasSelected = selected(blockRef(sl.track, sl.param, block_.start)) && sel_.size() > 1;
        pickBlock(s, block_.start, {});
        repaintAll();
    }
    return true;
}

void SongView::startBlockDrag() {
    using Mode = BlockGesture::Mode;
    const bool mixed = block_.mode == Mode::Move && selectedClipsN() + selectedBoxesN() > 0;
    if (mixed) {
        groupMove_ = true;
        host().beginUndoGroup();
    }
    host().pushUndo();
    if (!mixed) return;
    moveBase_.clear();
    moveGrabId_ = -1;
    for (const auto& [r, c] : selectedClipList()) {
        const auto clips = host().clips().list(rows_[(size_t) r]);
        if (c >= 0 && c < (int) clips.size())
            moveBase_.push_back({r, r, clips[(size_t) c].id, clips[(size_t) c].startTick});
    }
}

void SongView::dragBlockLane(const juce::MouseEvent& e) {
    if (!block_.active()) return;
    if (!block_.changed) {
        if (std::abs(e.x - block_.downX) < kDragStartPx) return;
        startBlockDrag();
        block_.changed = true;
    }
    const double beat = snapBeats(std::max(0.0, xToBeat((float) e.x)), e.mods.isAltDown());
    using Mode = BlockGesture::Mode;
    if (block_.mode == Mode::Move || block_.mode == Mode::Copy) {
        double delta = beat - block_.anchor;
        if (groupMove_) {
            double floor = groupMoveFloorBeats();
            for (const auto& r : sel_)
                if (r.kind == timeline::ItemRef::Kind::Block) floor = std::min(floor, blockStartOf(r));
            delta = std::max(delta, -floor);
            if (!moveBase_.empty()) moveSelection((int) std::llround(delta * Pattern::kTicksPerBeat), 0);
            boxDragDelta_ = delta;
        }
        rideSelectedBlocks(delta, block_.mode == Mode::Copy);
        repaintAll();
        return;
    }
    std::vector<AutomationBreakpoint> next;
    if (block_.mode == Mode::Edge) {
        next = blocklane::withEdgeAt(block_.before, block_.edge, beat, kMinBlockBeats);
    } else {
        const double from = std::min(block_.anchor, beat), to = std::max(block_.anchor, beat);
        const bool tiny = to - from < kMinBlockBeats;
        next = tiny ? block_.before : blocklane::painted(block_.before, from, to, block_.value, block_.fallback);
        block_.start = tiny ? -1.0 : from;
    }
    host().automation().setPoints(block_.node, block_.param, next);
    repaintAll();
}

void SongView::endBlockLane(const juce::MouseEvent&) {
    if (!block_.active()) return;
    const auto done = block_;
    block_ = {};
    using Mode = BlockGesture::Mode;
    const int row = slots_[(size_t) done.slot].track;
    double start = done.start;
    if (done.changed && done.mode == Mode::Edge)
        if (const auto* lane = laneOfSlot(slots_[(size_t) done.slot]))
            if (const int owner = blocklane::blockOfPoint(lane->points, done.edge); owner >= 0)
                start = blocklane::blocksOf(lane->points)[(size_t) owner].start;
    const bool reselect = done.changed ? done.mode == Mode::Edge || (done.mode == Mode::Paint && start >= 0.0)
                                       : done.groupWasSelected;
    if (done.changed) endBlockRide();
    if (!done.changed && done.inGap) {
        clearClipSel();
        sel_.clear();
        repaintAll();
        return;
    }
    if (!reselect) return;
    clearClipSel();
    sel_.clear();
    sel_.insert(blockRef(row, done.param, start));
    repaintAll();
}

void SongView::blockMenuAt(int slot, juce::Point<int> p, juce::Point<int> screen) {
    if (const auto* lane = laneOfSlot(slots_[(size_t) slot]))
        showBlockMenu(screen, slot, blocklane::blockAt(blocklane::blocksOf(lane->points), std::max(0.0, xToBeat((float) p.x))));
}

void SongView::showBlockMenu(juce::Point<int> screen, int slot, int block) {
    if (slot < 0 || slot >= (int) slots_.size()) return;
    const auto& sl = slots_[(size_t) slot];
    const auto* lane = laneOfSlot(sl);
    if (lane == nullptr || block < 0) return;
    const auto node = rows_[(size_t) sl.track];
    const auto param = sl.param;
    const auto& labels = blockLabels(node, param);
    const auto [lo, hi] = laneRange(node, param);
    const int count = blockChoiceCount(labels, lo, hi);
    const int first = (int) std::lround(lo);
    const auto blocks = blocklane::blocksOf(lane->points);
    if (block >= (int) blocks.size()) return;
    const int current = blocks[(size_t) block].value;
    if (const auto ref = blockRef(sl.track, param, blocks[(size_t) block].start); !selected(ref)) {
        clearClipSel();
        sel_.clear();
        sel_.insert(ref);
        repaintAll();
    }
    const int chosen = (int) selectedOf(timeline::ItemRef::Kind::Block).size();

    juce::PopupMenu m;
    m.addSectionHeader(tr("tracks-blocks.header", "{param} block", {{"param", juce::String(param)}}));
    for (int v = first; v < first + count; ++v)
        m.addItem(kFirstValueId + v - first, juce::String::fromUTF8(blocklane::labelFor(labels, v).c_str()), true,
                  v == current);
    m.addSeparator();
    m.addItem(kDuplicateId, tr("tracks-pane-menu.duplicate-after", "Duplicate After"), !blocks[(size_t) block].open() || chosen > 1);
    m.addItem(kDeleteBlockId, chosen > 1 ? trn("tracks-blocks.delete-blocks", "Delete {n} Block", "Delete {n} Blocks", (long long) chosen)
                                         : tr("tracks-blocks.delete-block", "Delete Block"),
              true);
    m.addItem(kClearLaneId, tr("tracks-pane-input.clear-lane-points", "Clear Lane Points"));
    m.addItem(kDeleteLaneId, tr("tracks-pane-input.delete-lane", "Delete Lane"));
    juce::Component::SafePointer<SongView> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screen.x, screen.y, 1, 1}),
                    [safe, node, param, first](int r) {
        if (safe == nullptr || r <= 0) return;
        auto& v = *safe;
        if (r == kClearLaneId) { v.host().automation().clearLane(node, param); v.rebuild(); return; }
        if (r == kDeleteLaneId) { v.host().automation().remove(node, param); v.rebuild(); return; }
        if (r == kDuplicateId) v.blockItems_.duplicateAfter({});
        else if (r == kDeleteBlockId) {
            v.host().pushUndo();
            v.blockItems_.remove(v.selectedOf(timeline::ItemRef::Kind::Block));
        } else
            v.setSelectedBlocksBank({}, first + r - kFirstValueId);
        v.repaintAll();
    });
}

}
