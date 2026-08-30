#include "gui/TracksPane.h"

#include "core/Automation.h"
#include "core/PodModel.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

#include "gui/EnvelopePainter.h"
#include "gui/LookAndFeel.h"

namespace hum {

void TracksPane::traceView(const char* what) const {
    static const bool on = std::getenv("HUMUS_VIEW_DEBUG") != nullptr;
    if (!on) return;
    std::fprintf(stderr, "[view] %-12s rows=%d scroll=%.3f v=%d size=%dx%d ppb=%.2f mode=%d "
                         "follow=%d live=%d\n",
                 what, (int) rows_.size(), scrollBeats_, vScroll_, getWidth(), getHeight(),
                 ppb_, (int) mode_, (int) follow_, (int) liveRec_);
}

void TracksPane::rebuild() {
    traceView("rebuild");
    rows_.clear();
    arrangeable_.clear();
    for (const auto& n : host_.arrangeableNodes()) {
        rows_.push_back(n);
        arrangeable_.insert(n);
    }
    for (const auto& cm : host_.model().organisms) {
        if (arrangeable_.count(cm.name)) continue;
        bool hasBox = !cm.automation.empty();
        for (const auto& b : host_.automation().boxes())
            if (b.organism == cm.name) { hasBox = true; break; }
        if (!hasBox) continue;
        rows_.push_back(cm.name);
        if (autoOnlyRows_.insert(cm.name).second) expanded_.insert(cm.name);
    }
    if (mode_ == Mode::Box) {
        const bool alive = std::find(rows_.begin(), rows_.end(), boxNode_) != rows_.end();
        if (!alive) leaveBoxMode();
        else {
            rows_ = {boxNode_};
            expanded_.insert(boxNode_);
        }
    }
    {
        std::vector<std::string> soloable;
        for (const auto& n : rows_) if (arrangeable_.count(n) != 0) soloable.push_back(n);
        host_.setSoloable(std::move(soloable));
    }
    if (mode_ == Mode::Clip && clipOrdinal() < 0) leaveClipMode();
    rebuildSlots();
    const int contentH = trackslayout::totalHeight(slots_, headerH() - vScroll_) + vScroll_;
    if (const int maxV = std::max(0, contentH - getHeight() + kZoomGut); vScroll_ > maxV) {
        vScroll_ = maxV;
        rebuildSlots();
    }
    repaint();
}

static double zoomLo(int axis, bool roll) { return axis == 0 ? 2.0 : roll ? 4.0 : 22.0; }

double TracksPane::zoomNorm(int axis) const {
    const bool roll = mode_ == Mode::Track;
    const double lo = zoomLo(axis, roll);
    const double hi = axis == 0 ? (mode_ == Mode::Clip ? kClipMaxPpb : 120.0)
                                : roll ? 44.0 : (double) kRowHMax;
    const double v = axis == 0 ? ppb_ : roll ? (double) rollRowH_ : (double) kRowH;
    return juce::jlimit(0.0, 1.0, std::log(v / lo) / std::log(hi / lo));
}

void TracksPane::setZoomNorm(int axis, double t) {
    const bool roll = mode_ == Mode::Track;
    const double lo = zoomLo(axis, roll);
    const double hi = axis == 0 ? (mode_ == Mode::Clip ? kClipMaxPpb : 120.0)
                                : roll ? 44.0 : (double) kRowHMax;
    const double v = lo * std::pow(hi / lo, juce::jlimit(0.0, 1.0, t));
    if (axis == 0) setPixelsPerBeat(v);
    else if (roll) { rollRowH_ = (float) v; repaint(); }
    else { kRowH = (int) std::lround(v); rebuildSlots(); repaint(); }
}

void TracksPane::zoomBy(int axis, double factor) {
    if (axis == 0) {
        setPixelsPerBeat(ppb_ * factor);
    } else if (mode_ == Mode::Track) {
        rollRowH_ = (float) juce::jlimit(4.0, 44.0, rollRowH_ * factor);
    } else {
        kRowH = juce::jlimit(kRowHMin, kRowHMax,
                             (int) std::lround(kRowH * factor));
        rebuildSlots();
    }
    repaint();
}

void TracksPane::rebuildSlots() {
    if (mode_ == Mode::Box && !rows_.empty()) {
        const int n = std::max(1, (int) lanesOf(0).size());
        const int avail = getHeight() - headerH() - kBoxRowH;
        boxLaneH_ = juce::jlimit(kLaneH * 2, 160, avail / n);
    }
    slots_ = trackslayout::build(
        (int) rows_.size(),
        [this](int t) { return expanded_.count(rows_[(size_t) t]) != 0; },
        [this](int t) { return lanesOf(t); },
        headerH() - vScroll_, mode_ == Mode::Box ? 0 : kRowH,
        mode_ == Mode::Box ? boxLaneH_ : kLaneH,
        [this](int t) { return podOfRow(t); },
        [this](const std::string& pod) { return collapsedPods_.count(pod) != 0; },
        kPodH,
        [this](int t) { return wantsBoxRow(t); },
        kBoxRowH);
}

bool TracksPane::wantsBoxRow(int row) const {
    if (row < 0 || row >= (int) rows_.size()) return false;
    const auto& node = rows_[(size_t) row];
    if (mode_ == Mode::Box) return true;
    if (arrangeable_.count(node) == 0) return false;
    if (const auto* cm = host_.model().byName(node); cm != nullptr && !cm->automation.empty())
        return true;
    for (const auto& b : host_.automation().boxes())
        if (b.organism == node) return true;
    return false;
}

int TracksPane::boxRowSlot(int row) const {
    for (int i = 0; i < (int) slots_.size(); ++i)
        if (slots_[(size_t) i].kind == trackslayout::Kind::BoxRow && slots_[(size_t) i].track == row)
            return i;
    return -1;
}

std::string TracksPane::podOfRow(int t) const {
    if (t < 0 || t >= (int) rows_.size()) return {};
    return pods::childPodOf(rows_[(size_t) t], "");
}

void TracksPane::applyVScroll(int v) {
    traceView("vscroll");
    const int contentH = trackslayout::totalHeight(slots_, headerH() - vScroll_) + vScroll_;
    const int maxV = std::max(0, contentH - getHeight() + kZoomGut);
    v = juce::jlimit(0, maxV, v);
    if (v == vScroll_) return;
    vScroll_ = v;
    rebuildSlots();
    repaint();
}

void TracksPane::zoomAbout(float x, double factor) {
    const double beatAt = xToBeat(x);
    ppb_ = juce::jlimit(2.0, mode_ == Mode::Clip ? kClipMaxPpb : 120.0, ppb_ * factor);
    scrollBeats_ = juce::jlimit(0.0, std::max(0.0, contentEndBeat() - 1.0),
                                beatAt - (x - kStripW) / ppb_);
    repaint();
}

double TracksPane::contentEndBeat() const {
    if (mode_ == Mode::Clip) {
        ClipEditor::ClipInfo ci;
        if (clipInfo(ci))
            return (ci.startTick + ci.lengthTicks) / (double) Pattern::kTicksPerBeat;
    }
    return host_.songEndBeat();
}

void TracksPane::mouseMagnify(const juce::MouseEvent& e, float scaleFactor) {
    zoomAbout((float) e.getPosition().x, scaleFactor);
}

std::vector<trackslayout::AutoLaneInfo> TracksPane::lanesOf(int row) const {
    std::vector<trackslayout::AutoLaneInfo> out;
    if (row < 0 || row >= (int) rows_.size()) return out;
    if (const auto* cm = host_.model().byName(rows_[(size_t) row]))
        for (const auto& l : cm->automation)
            out.push_back({l.propertyName, l.kind});
    return out;
}

int TracksPane::rowTop(int row) const {
    const int y = trackslayout::trackY(slots_, row);
    return y >= 0 ? y : headerH() + row * kRowH;
}

std::pair<double, double> TracksPane::laneRange(const std::string& node,
                                                const std::string& param) const {
    return envpaint::laneRange(host_.model().byName(node), param,
                               (int) host_.model().metapad.snapshots.size());
}

float TracksPane::laneYAtValue(const trackslayout::Slot& s, double v, double lo, double hi) const {
    const double span = hi > lo ? hi - lo : 1.0;
    return (float) (s.y + s.h - 3 - (v - lo) / span * (s.h - 6));
}
double TracksPane::laneValueAtY(const trackslayout::Slot& s, int py, double lo, double hi) const {
    const double span = hi > lo ? hi - lo : 1.0;
    const double v = lo + ((double) (s.y + s.h - 3) - py) / (s.h - 6) * span;
    return juce::jlimit(std::min(lo, hi), std::max(lo, hi), v);
}

int TracksPane::autoPointAt(const trackslayout::Slot& s, const std::string& node,
                            const std::string& param, juce::Point<int> p) const {
    const auto* cm = host_.model().byName(node);
    if (!cm) return -1;
    const hum::AutomationLane* lane = nullptr;
    for (const auto& l : cm->automation) if (l.propertyName == param) { lane = &l; break; }
    if (!lane) return -1;
    const auto [lo, hi] = laneRange(node, param);
    int best = -1; float bestD = 7.0f;
    for (int i = 0; i < (int) lane->points.size(); ++i) {
        const auto& pt = lane->points[(size_t) i];
        const float dx = beatToX(pt.beat) - p.x;
        if (lane->kind == "trigger") {
            const float d = std::abs(dx);
            if (d < bestD) { bestD = d; best = i; }
            continue;
        }
        float dy = laneYAtValue(s, pt.value, lo, hi) - p.y;
        if (lane->kind == "range") {
            const float dyHi = laneYAtValue(s, pt.valueMax, lo, hi) - p.y;
            if (std::abs(dyHi) < std::abs(dy)) dy = dyHi;
        }
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

TracksPane::AutoGrip TracksPane::nearerRangeEdge(const trackslayout::Slot& s,
                                                 const std::string& node,
                                                 const std::string& param, int index,
                                                 juce::Point<int> p) const {
    const auto* cm = host_.model().byName(node);
    if (cm == nullptr || index < 0) return AutoGrip::RangeLo;
    for (const auto& l : cm->automation) {
        if (l.propertyName != param) continue;
        if (index >= (int) l.points.size()) break;
        const auto& pt = l.points[(size_t) index];
        const auto [lo, hi] = laneRange(node, param);
        const float dLo = std::abs(laneYAtValue(s, pt.value, lo, hi) - p.y);
        const float dHi = std::abs(laneYAtValue(s, pt.valueMax, lo, hi) - p.y);
        return dHi < dLo ? AutoGrip::RangeHi : AutoGrip::RangeLo;
    }
    return AutoGrip::RangeLo;
}

void TracksPane::setPlaybackBeat(double beat) {
    if (std::abs(beat - playBeat_) < 1e-4) return;
    const float oldX = beatToX(playBeat_);
    playBeat_ = beat;
    if (!isVisible()) return;
    if (follow_ && drag_ == Drag::None && beatToX(beat) > (float) getWidth()) {
        traceView("follow-jump");
        scrollBeats_ = std::max(0.0, beat - 4.0);
        repaint();
        return;
    }
    if (liveRec_) { repaint(); return; }
    const float newX = beatToX(beat);
    repaint((int) std::floor(juce::jmin(oldX, newX)) - 6, 0,
            (int) std::ceil(std::abs(newX - oldX)) + 12, getHeight());
}

int TracksPane::rowAt(int y) const {
    const int s = trackslayout::slotAt(slots_, y);
    if (s < 0 || slots_[(size_t) s].kind != trackslayout::Kind::Track) return -1;
    return slots_[(size_t) s].track;
}

void TracksPane::setScrollBeats(double b) {
    traceView("scroll");
    scrollBeats_ = std::max(0.0, b);
    repaint();
}

void TracksPane::setPixelsPerBeat(double ppb) {
    ppb_ = juce::jlimit(2.0, mode_ == Mode::Clip ? kClipMaxPpb : 120.0, ppb);
    rebuildSlots();
    repaint();
}

int TracksPane::segmentAt(const trackslayout::Slot& s, const std::string& node,
                          const std::string& param, juce::Point<int> p) const {
    const auto* cm = host_.model().byName(node);
    if (cm == nullptr) return -1;
    const AutomationLane* lane = nullptr;
    for (const auto& l : cm->automation) if (l.propertyName == param) { lane = &l; break; }
    if (lane == nullptr || lane->kind != "double" || lane->points.size() < 2) return -1;
    const auto [lo, hi] = laneRange(node, param);
    for (int i = 1; i < (int) lane->points.size(); ++i) {
        const auto& a = lane->points[(size_t) (i - 1)];
        const auto& b = lane->points[(size_t) i];
        const float ax = beatToX(a.beat), bx = beatToX(b.beat);
        if ((float) p.x < ax || (float) p.x > bx || bx <= ax) continue;
        const double t = (p.x - ax) / (double) (bx - ax);
        const double v = a.value + (b.value - a.value) * shapeT(t, a.curve);
        if (std::abs(laneYAtValue(s, v, lo, hi) - p.y) <= 5.0f) return i - 1;
    }
    return -1;
}

std::string TracksPane::selectedNoteNode() const {
    if (selClipRow_ < 0 || selClipRow_ >= (int) rows_.size()) return {};
    const auto& node = rows_[(size_t) selClipRow_];
    return host_.nodeRecordsAudio(node) ? std::string() : node;
}

double TracksPane::gridBeats() const {
    if (snapChoice_ > 0.0) return snapChoice_;
    return trackslayout::gridBeats(ppb_, host_.automation().timeSigNumerator());
}

bool TracksPane::nodeHasMuteParam(const std::string& node) const {
    auto& h = const_cast<EngineHost&>(host_);
    if (auto* org = h.liveOrganism(node)) return org->params.byName("Mute") != nullptr;
    return false;
}

bool TracksPane::nodeMuted(const std::string& node) const {
    if (nodeHasMuteParam(node)) {
        if (const auto* cm = host_.model().byName(node))
            for (const auto& pr : cm->properties)
                if (pr.name == "Mute") return pr.value >= 0.5;
        return false;
    }
    return host_.trackMuted(node);
}

void TracksPane::setNodeMuted(const std::string& node, bool muted) {
    if (nodeHasMuteParam(node)) host_.setParam(node, "Mute", muted ? 1.0 : 0.0);
    else host_.setTrackMuted(node, muted);
}

void TracksPane::showSnapMenu(juce::Point<int> sp) {
    const double bar = juce::jmax(1, host_.automation().timeSigNumerator());
    struct Item { const char* name; double beats; };
    const Item items[] = {{"Auto (follows the zoom)", 0.0}, {"Free", -1.0}, {"Bar", bar},
                          {"1/2", 2.0}, {"1/4", 1.0}, {"1/8", 0.5}, {"1/16", 0.25},
                          {"1/32", 0.125}, {"1/64", 0.0625}};
    juce::PopupMenu m;
    for (int i = 0; i < (int) std::size(items); ++i)
        m.addItem(i + 1, items[i].name, true, std::abs(snapChoice_ - items[i].beats) < 1e-9);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                    [this, bar](int r) {
        const double beats[] = {0.0, -1.0, bar, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625};
        if (r >= 1 && r <= 9) { snapChoice_ = beats[r - 1]; repaint(); }
    });
}

void TracksPane::cycleSnap() {
    const double bar = juce::jmax(1, host_.automation().timeSigNumerator());
    const double ladder[] = {0.0, 0.25, 0.5, 1.0, bar};
    int at = 0;
    for (int i = 0; i < 5; ++i)
        if (std::abs(snapChoice_ - ladder[i]) < 1e-9) at = i;
    snapChoice_ = ladder[(at + 1) % 5];
    repaint();
}

double TracksPane::snapBeats(double beat, bool bypass) const {
    if (bypass || snapChoice_ < 0.0) return std::max(0.0, beat);
    const double res = gridBeats();
    return std::max(0.0, std::round(beat / res) * res);
}

juce::Rectangle<int> TracksPane::clipBounds(int row, const ClipEditor::ClipInfo& ci) const {
    const int x0 = (int) tickToX(ci.startTick);
    const int x1 = (int) tickToX(ci.startTick + ci.lengthTicks);
    return {x0, rowTop(row) + 2, std::max(6, x1 - x0), kRowH - 5};
}

int TracksPane::clipAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const {
    leftEdge = rightEdge = false;
    if (row < 0) return -1;
    const auto clips = host_.clips().list(rows_[(size_t) row]);
    for (int i = (int) clips.size() - 1; i >= 0; --i) {
        const auto b = clipBounds(row, clips[(size_t) i]);
        if (!b.contains(p)) continue;
        leftEdge = p.x <= b.getX() + 5;
        rightEdge = p.x >= b.getRight() - 5;
        return i;
    }
    return -1;
}

juce::Rectangle<int> TracksPane::boxBounds(int row, const PerformanceBox& b) const {
    const int y = rowTop(row);
    double s = b.startBeat, e = b.endBeat;
    if (dragBox_ >= 0 && dragBox_ < (int) host_.automation().boxes().size()
        && &host_.automation().boxes()[(size_t) dragBox_] == &b) {
        s += boxDragDelta_ + boxTrimL_;
        e += boxDragDelta_ + boxTrimR_;
    }
    const int x0 = (int) beatToX(s), x1 = (int) beatToX(e);
    if (const int bs = boxRowSlot(row); bs >= 0) {
        const auto& sl = slots_[(size_t) bs];
        return {x0, sl.y + 2, std::max(10, x1 - x0), sl.h - 4};
    }
    return {x0, y + 2, std::max(10, x1 - x0), kRowH - 4};
}

int TracksPane::boxAt(int row, juce::Point<int> p) const {
    bool l = false, r = false;
    return boxAt(row, p, l, r);
}

int TracksPane::boxAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const {
    leftEdge = rightEdge = false;
    const auto& boxes = host_.automation().boxes();
    for (int i = (int) boxes.size() - 1; i >= 0; --i) {
        if (boxes[(size_t) i].organism != rows_[(size_t) row]) continue;
        const auto b = boxBounds(row, boxes[(size_t) i]);
        if (!b.contains(p)) continue;
        leftEdge = p.x <= b.getX() + 5;
        rightEdge = p.x >= b.getRight() - 5;
        return i;
    }
    return -1;
}

void TracksPane::enterBoxMode(const std::string& node) {
    if (node.empty() || mode_ == Mode::Track || mode_ == Mode::Clip) return;
    if (mode_ == Mode::Song) { songPpb_ = ppb_; songScroll_ = scrollBeats_; }
    mode_ = Mode::Box;
    boxNode_ = node;
    boxWasExpanded_ = expanded_.count(node) != 0;
    hasSel_ = false;
    clearClipSel();
    clearPointSelection();
    selBox_ = -1;
    double s = 1e18, e = -1e18;
    for (const auto& b : host_.automation().boxes())
        if (b.organism == node) { s = std::min(s, b.startBeat); e = std::max(e, b.endBeat); }
    if (const auto* cm = host_.model().byName(node))
        for (const auto& l : cm->automation)
            for (const auto& pt : l.points) { s = std::min(s, pt.beat); e = std::max(e, pt.beat); }
    if (e > s) {
        const double w = std::max(60, getWidth() - kStripW) * 0.9;
        ppb_ = juce::jlimit(2.0, 600.0, w / std::max(1.0, e - s));
        scrollBeats_ = std::max(0.0, s - (w / 0.9 - w) * 0.5 / ppb_);
    }
    rebuild();
}

void TracksPane::leaveBoxMode() {
    if (mode_ != Mode::Box) return;
    mode_ = Mode::Song;
    if (!boxWasExpanded_) expanded_.erase(boxNode_);
    boxNode_.clear();
    ppb_ = songPpb_;
    scrollBeats_ = songScroll_;
    lineSlot_ = -1;
    rebuild();
}

void TracksPane::replaceSpan(const std::string& node, const std::string& param, double from,
                             double to, std::vector<AutomationBreakpoint> add, bool openFrom) {
    const auto* cm = host_.model().byName(node);
    if (cm == nullptr) return;
    std::vector<AutomationBreakpoint> pts;
    for (const auto& l : cm->automation)
        if (l.propertyName == param) { pts = l.points; break; }
    pts.erase(std::remove_if(pts.begin(), pts.end(), [&](const AutomationBreakpoint& b) {
                  return (openFrom ? b.beat > from : b.beat >= from) && b.beat <= to;
              }), pts.end());
    pts.insert(pts.end(), add.begin(), add.end());
    std::stable_sort(pts.begin(), pts.end(),
                     [](const AutomationBreakpoint& a, const AutomationBreakpoint& b) {
                         return a.beat < b.beat;
                     });
    host_.automation().setPoints(node, param, pts);
}

void TracksPane::commitLine(const std::string& node, const std::string& param,
                            double b0, double v0, double b1, double v1) {
    if (b1 < b0) { std::swap(b0, b1); std::swap(v0, v1); }
    AutomationBreakpoint a, b;
    a.beat = b0; a.value = a.valueMax = v0;
    b.beat = b1; b.value = b.valueMax = v1;
    if (b1 - b0 < 1e-6) replaceSpan(node, param, b0, b0, {a});
    else replaceSpan(node, param, b0, b1, {a, b});
}

void TracksPane::pencilStep(const std::string& node, const std::string& param, double beat,
                            double v) {
    AutomationBreakpoint p;
    p.beat = beat; p.value = p.valueMax = v;
    if (pencilLast_ < 0.0 || pencilLast_ == beat) replaceSpan(node, param, beat, beat, {p});
    else if (beat > pencilLast_) replaceSpan(node, param, pencilLast_, beat, {p}, true);
    else {
        replaceSpan(node, param, beat, pencilLast_, {p});
        AutomationBreakpoint keep;
        keep.beat = pencilLast_;
        keep.value = keep.valueMax = lastPencilVal_;
        replaceSpan(node, param, pencilLast_, pencilLast_, {keep});
    }
    lastPencilVal_ = v;
    pencilLast_ = beat;
}

}
