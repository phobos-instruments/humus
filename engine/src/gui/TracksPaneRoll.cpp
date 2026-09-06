#include <set>

#include "gui/TracksPane.h"

#include <algorithm>
#include <cmath>

#include "core/ClipOps.h"
#include "gui/ClipColors.h"
#include "gui/LookAndFeel.h"
#include "hum/ClipStack.h"
#include "gui/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

std::vector<std::string> TracksPane::noteRows() const {
    std::vector<std::string> out;
    for (const auto& n : rows_)
        if (arrangeable_.count(n) != 0 && !host_.nodeRecordsAudio(n)) out.push_back(n);
    return out;
}

void TracksPane::enterTrackMode(const std::string& node) {
    if (node.empty()) return;
    mode_ = Mode::Track;
    trackNode_ = node;
    fitRoll();
    clearClipSel();
    clearSelection();

    syncLiveTarget();
    clearPointSelection();
    repaint();
}

void TracksPane::leaveTrackMode() {
    if (mode_ != Mode::Track) return;
    mode_ = Mode::Song;
    trackNode_.clear();
    selNotes_.clear();
    syncLiveTarget();
    rebuildSlots();
    repaint();
}

void TracksPane::stepTrack(int dir) {
    const auto rows = noteRows();
    if (rows.empty()) return;
    int at = 0;
    for (int i = 0; i < (int) rows.size(); ++i)
        if (rows[(size_t) i] == trackNode_) at = i;
    enterTrackMode(rows[(size_t) juce::jlimit(0, (int) rows.size() - 1, at + dir)]);
}

void TracksPane::rollPitchRange(int& lo, int& hi) const {
    lo = kMidiMax; hi = 0;
    bool any = false;
    for (int c = 0; c < (int) host_.clips().list(trackNode_).size(); ++c)
        for (const auto& n : host_.clips().notes(trackNode_, c)) {
            lo = std::min(lo, n.pitch);
            hi = std::max(hi, n.pitch);
            any = true;
        }
    if (!any) { lo = 36; hi = 72; }
}

juce::Rectangle<int> TracksPane::rollField() const {
    const int top = headerH() + kChipH;
    const int bottom = fieldBottom() - kRibbonH - (rollShowsVelocity() ? velH_ : 0);
    return {kStripW, top, std::max(0, getWidth() - kStripW), std::max(0, bottom - top)};
}

timelinechrome::RollPlot TracksPane::rollPlot() const {
    const auto f = rollField();
    timelinechrome::RollPlot rp;
    rp.rowH = rollRowH_;
    rp.top = (float) f.getY() + rollScrollAcc_ * rp.rowH;
    rp.h = (float) f.getHeight();
    rp.topPitch = juce::jlimit(0, kMidiMax, rollTopPitch_ + rollScrollSemis_);
    rp.usable = f.getHeight() >= 12;
    return rp;
}

void TracksPane::resized() {
    traceView("resized");
    if (mode_ == Mode::Track && rollScrollSemis_ == 0 && rollScrollAcc_ == 0.0f) fitRoll();
    if (mode_ == Mode::Box) rebuildSlots();
}

void TracksPane::fitRoll() {
    const auto f = rollField();
    int lo = 0, hi = 0;
    rollPitchRange(lo, hi);
    const auto fit = timelinechrome::rollPlot((float) f.getY(), (float) f.getHeight(),
                                              lo, hi, 0, 10.0f);
    rollTopPitch_ = fit.topPitch;
    rollRowH_ = fit.rowH;
    rollScrollSemis_ = 0;
    rollScrollAcc_ = 0.0f;
}

void TracksPane::paintRollCrumb(juce::Graphics& g) {
    paintBackCrumb(g);
    const juce::Rectangle<int> name(crumbBackBox().getRight() + 10, 1, 300, kTopH - 2);
    g.setFont(juce::FontOptions(timelinechrome::kCrumbFont));
    g.setColour(timelinechrome::laneAccent(host_.model(), trackNode_));
    g.fillEllipse((float) name.getX(), (float) name.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour(Palette::text);
    g.drawText(juce::String(trackNode_) + juce::String::fromUTF8("  \xe2\x96\xbe"),
               name.withTrimmedLeft(10), juce::Justification::centredLeft, true);
}

void TracksPane::paintRoll(juce::Graphics& g) {
    const auto f = rollField();
    const auto rp = rollPlot();
    const auto accent = timelinechrome::laneAccent(host_.model(), trackNode_);

    g.setColour(Palette::background);
    g.fillRect(0, headerH(), getWidth(), getHeight() - headerH());
    g.setColour(accent.withAlpha(0.06f));
    g.fillRect(f);
    g.setColour(accent.withAlpha(0.55f));
    g.fillRect(kStripW, headerH() - 1, getWidth() - kStripW, 1);

    g.setColour(Palette::panel);
    g.fillRect(0, headerH(), getWidth(), kChipH);
    g.setColour(Palette::border.withAlpha(0.5f));
    g.drawHorizontalLine(headerH() + kChipH - 1, 0.0f, (float) getWidth());
    g.setColour(accent);
    g.fillRect(6, headerH() + 7, 8, 8);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(juce::String(trackNode_), 20, headerH(), kStripW - 80, kChipH,
               juce::Justification::centredLeft, true);
    timelinechrome::paintDestChip(g, {kStripW + 8, headerH() + 4, 140, kChipH - 8},
                                  host_.model(), trackNode_);

    if (!rp.usable) return;

    g.saveState();
    g.reduceClipRegion(f);
    if (rp.rowH >= 4.0f) {
        for (int i = 0; i <= rp.rows(); ++i) {
            const int pitch = rp.topPitch - i;
            if (pitch < 0 || pitch > kMidiMax) continue;
            const float y = rp.yFor(pitch);
            g.setColour(timelinechrome::isBlackKey(pitch) ? Palette::background.brighter(0.02f)
                                                          : Palette::background.brighter(0.055f));
            g.fillRect((float) f.getX(), y, (float) f.getWidth(), rp.hFor(pitch));
            g.setColour(Palette::border.withAlpha(noteedit::laneRuleAlpha(pitch)));
            g.drawHorizontalLine((int) (y + rp.hFor(pitch) - 1.0f), (float) f.getX(),
                                 (float) f.getRight());
        }
    }

    timelinechrome::paintTimeGrid(g, f, kStripW, scrollBeats_, ppb_,
                                  juce::jmax(1, host_.automation().timeSigNumerator()),
                                  gridBeats());

    const auto* cm = host_.model().byName(trackNode_);
    const auto clips = host_.clips().list(trackNode_);
    const int ribbonY = f.getBottom();
    for (const auto& ci : clips) {
        const auto col = ci.color > 0 ? clipColour(ci.color) : accent;
        std::vector<clipstack::Span> spans;
        if (cm != nullptr) {
            const auto chans = clipops::clipChannels(cm->pattern);
            if (ci.index < (int) chans.size())
                spans = clipstack::soundingSpans(cm->pattern, chans[(size_t) ci.index]);
        }
        if (spans.empty()) spans.push_back({ci.startTick, ci.lengthTicks});
        for (const auto& sp : spans) {
            const float x0 = std::max(tickToX(sp.startTick), (float) kStripW);
            const float x1 = std::min(tickToX(sp.startTick + sp.lengthTicks),
                                      (float) getWidth());
            if (x1 <= x0) continue;
            g.setColour(col.withAlpha(0.035f));
            g.fillRect(x0, (float) f.getY(), x1 - x0, (float) f.getHeight());
            g.setColour(col.withAlpha(sp.startTick == ci.startTick ? 0.85f : 0.35f));
            g.fillRect(x0, (float) ribbonY, x1 - x0, (float) kRibbonH);
        }
        const float sx = tickToX(ci.startTick);
        if (sx >= (float) kStripW && sx <= (float) getWidth()) {
            g.setColour(col.withAlpha(0.30f));
            g.drawVerticalLine((int) sx, (float) f.getY(), (float) f.getBottom());
        }
        if (const float w = tickToX(ci.startTick + ci.lengthTicks) - sx;
            w > 40.0f && sx < (float) getWidth()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.5f));
            g.drawText(ci.name.empty() ? tr("tracks-pane-roll.clip", "clip ") + juce::String(ci.index + 1)
                                       : juce::String(ci.name),
                       (int) std::max(sx + 3.0f, (float) kStripW + 3.0f),
                       ribbonY - 12, 120, 11, juce::Justification::centredLeft, false);
        }
    }

    const bool showVel = rp.rowH >= 3.0f;
    for (const auto& ci : clips) {
        const auto col = ci.color > 0 ? clipColour(ci.color) : accent;
        for (const auto& n : host_.clips().notes(trackNode_, ci.index)) {
            const int at = ci.startTick + n.tick;
            const float x = tickToX(at);
            const float w = std::max(2.0f, tickToX(at + std::max(1, n.lengthTicks)) - x);
            if (x + w < (float) kStripW || x > (float) getWidth()) continue;
            const float y = rp.yFor(n.pitch);
            const float nh = rp.hFor(n.pitch);
            if (y + nh < (float) f.getY() || y > (float) f.getBottom()) continue;
            const float x0 = std::max(x, (float) kStripW);
            const float w0 = std::min(x + w, (float) getWidth()) - x0;
            g.setColour(showVel ? col.withBrightness(juce::jlimit(
                                      0.25f, 1.0f, 0.45f + 0.55f * n.velocity / kMidiMaxF))
                                : col.withAlpha(0.8f));
            if (rp.rowH >= 5.0f)
                g.fillRoundedRectangle(x0, y, w0, std::max(2.0f, nh - 1.0f), 2.0f);
            else
                g.fillRect(x0, y, w0, std::max(2.0f, nh - 1.0f));
            noteedit::paintNoteName(g, {x0, y, w0, nh - 1.0f}, n.pitch,
                                    Palette::background.withAlpha(0.75f));
            if (rp.rowH >= 7.0f && w0 >= 6.0f) {
                g.setColour(col.darker(0.5f));
                g.drawRoundedRectangle(x0, y, w0, nh - 1.0f, 2.0f, 1.0f);
            }
        }
    }
    g.restoreState();

    g.setColour(Palette::panel);
    g.fillRect(kStripW - kKeyW, f.getY(), kKeyW, f.getHeight());
    std::set<int> lit;
    if (rollKeyNote_ >= 0) lit.insert(rollKeyNote_);
    {
        const int at = (int) std::llround(playBeat_ * Pattern::kTicksPerBeat);
        for (const auto& ci : host_.clips().list(trackNode_))
            for (const auto& n : host_.clips().notes(trackNode_, ci.index)) {
                const int t0 = ci.startTick + n.tick;
                if (at >= t0 && at < t0 + std::max(1, n.lengthTicks)) lit.insert(n.pitch);
            }
    }
    g.saveState();
    g.reduceClipRegion(kStripW - kKeyW, f.getY(), kKeyW, f.getHeight());
    if (rp.rowH >= 2.5f)
        for (int i = 0; i <= rp.rows(); ++i) {
            const int pitch = rp.topPitch - i;
            if (pitch < 0 || pitch > kMidiMax) continue;
            const float y = rp.yFor(pitch);
            const float kh = rp.hFor(pitch);
            if (y + kh < (float) f.getY() || y > (float) f.getBottom()) continue;
            noteedit::paintPianoKey(g, {(float) (kStripW - kKeyW), y, (float) kKeyW - 1.0f,
                                        std::max(1.0f, kh)},
                                    pitch, lit.count(pitch) != 0, Palette::accent);
            if (((pitch % 12) + 12) % 12 == 0 && rp.rowH >= 8.0f) {
                g.setColour(juce::Colour(noteedit::kEbony));
                g.setFont(juce::FontOptions(9.0f));
                g.drawText("C" + juce::String(pitch / 12 - 1), kStripW - kKeyW + 2,
                           (int) y - 1, kKeyW - 5, (int) kh + 2,
                           juce::Justification::centredRight, false);
            }
        }
    g.restoreState();
    g.setColour(Palette::border);
    g.drawVerticalLine(kStripW - 1, (float) f.getY(), (float) f.getBottom());

    if (rollShowsVelocity()) {
        const int top = fieldBottom() - velH_;
        g.setColour(Palette::background.darker(0.15f));
        g.fillRect(0, top, getWidth(), velH_);
        g.setColour(Palette::border);
        g.drawHorizontalLine(top, 0.0f, (float) getWidth());
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(9.0f));
        g.drawText("vel", 4, top + 3, kKeyW - 8, 10, juce::Justification::centredLeft, false);
        for (const auto& ci : clips) {
            const auto col = ci.color > 0 ? clipColour(ci.color) : accent;
            g.setColour(col.withAlpha(0.45f));
            for (const auto& n : host_.clips().notes(trackNode_, ci.index)) {
                const float x = tickToX(ci.startTick + n.tick);
                if (x < (float) kStripW || x > (float) getWidth()) continue;
                const float h = ((float) velH_ - 6.0f) * n.velocity / kMidiMaxF;
                const float barTop = (float) (top + velH_ - 3) - h;
                g.setColour(col.withAlpha(0.45f));
                g.fillRect(x, barTop, 3.0f, h);
                g.setColour(col);
                g.fillEllipse(x - 1.5f, barTop - 3.0f, 6.0f, 6.0f);
            }
        }
        if (velShowX_ >= 0 && velShowVal_ >= 0) {
            const auto bubble = juce::Rectangle<int>(
                juce::jlimit(kStripW, getWidth() - 40, velShowX_ - 17), top - 18, 34, 16);
            g.setColour(Palette::accent);
            g.fillRoundedRectangle(bubble.toFloat(), 3.0f);
            g.setColour(Palette::background);
            g.setFont(juce::FontOptions(10.5f));
            g.drawText(juce::String(velShowVal_), bubble, juce::Justification::centred);
        }
    }

    if (clips.empty()) {
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f));
        auto r = f.reduced(0, f.getHeight() / 3);
        g.drawText(tr("tracks-pane-roll.no-clips-on-this-track", "No clips on this track yet"), r.removeFromTop(r.getHeight() / 2),
                   juce::Justification::centredBottom);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(tr("tracks-pane-roll.drag-one-in-or-arm", "Drag one in, or arm R and play"), r, juce::Justification::centredTop);
    }
}

bool TracksPane::mouseDownRoll(const juce::MouseEvent& e, juce::Point<int> p) {
    if (mode_ != Mode::Track) return false;
    if (p.y < kTopH) {
        if (crumbBackBox().contains(p)) { leaveTrackMode(); return true; }
        if (crumbNameBox().contains(p)) {
            juce::PopupMenu m;
            const auto rows = noteRows();
            for (int i = 0; i < (int) rows.size(); ++i)
                m.addItem(i + 1, juce::String(rows[(size_t) i]), true,
                          rows[(size_t) i] == trackNode_);
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options()
                                .withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, rows](int r) {
                if (r > 0 && r <= (int) rows.size()) enterTrackMode(rows[(size_t) (r - 1)]);
            });
            return true;
        }
        return false;
    }
    if (mouseDownRollGrid(e, p)) return true;
    const auto field = rollField();
    if (p.x >= kStripW - kKeyW && p.x < kStripW
        && p.y >= field.getY() && p.y < field.getBottom()) {
        const auto rp = rollPlot();
        if (rp.usable) { soundRollKey(rp.pitchAt((float) p.y)); return true; }
    }
    return p.y >= headerH();
}

void TracksPane::soundRollKey(int pitch) {
    pitch = juce::jlimit(0, kMidiMax, pitch);
    if (pitch == rollKeyNote_) return;
    if (rollKeyNote_ >= 0)
        host_.injectLiveMidi(juce::MidiMessage::noteOff(1, rollKeyNote_));
    rollKeyNote_ = pitch;
    host_.injectLiveMidi(juce::MidiMessage::noteOn(1, pitch, (juce::uint8) 100));
    repaintRollKeys();
}

juce::String TracksPane::getTooltip() {
    const auto p = hover_;
    if (p.x < kStripW && p.y < headerH()) {
        static const char* names[] = {
            "Pointer (1) - select, move, resize; drag a lane to select points",
            "Pencil (2) - draw notes, clips and freehand automation",
            "Line (3) - rule a straight automation segment",
            "Scissors (4) - split at the click",
            "Eraser (5) - sweep to delete",
        };
        for (int i = 0; i < kToolCount; ++i)
            if (toolBox(i).contains(p)) return names[i];
        if (addTrackBox().contains(p)) return tr("tracks-pane-roll.add-a-track-wired", "Add a track, wired");
        if (snapBox().contains(p))
            return snapChoice_ < 0.0 ? "Snap: free - click to choose a grid"
                 : snapChoice_ > 0.0 ? "Snap: chosen - click to change or follow the zoom"
                                     : "Snap follows the zoom - click to choose one";
        if (followBox().contains(p)) return tr("tracks-pane-roll.follow-the-playhead-while-it", "Follow the playhead while it plays");
    }
    if (overLoopLane(p))
        return host_.automation().loopEnabled()
            ? "Loop - drag the ends to trim, the body to move; click outside or double-click to remove"
            : "Drag to set a loop - right-click to loop the selection";
    if (mode_ == Mode::Track && p.y < kTopH) {
        if (crumbBackBox().contains(p)) return tr("tracks-pane-roll.back-to-the-timeline-esc", "Back to the timeline (Esc)");
        if (crumbNameBox().contains(p)) return "Choose a track (Alt+Up / Alt+Down)";
    }
    if (mode_ == Mode::Track && rollPlot().usable && rollField().contains(p)) {
        if (effectiveTool() == Tool::Draw) return tr("tracks-pane-roll.draw-a-note-drag-to", "Draw a note - drag to set its length");
        if (effectiveTool() == Tool::Scissors) return tr("tracks-pane-roll.split-the-note-at-the", "Split the note at the click");
        if (effectiveTool() == Tool::Eraser) return tr("tracks-pane-roll.sweep-to-delete-notes", "Sweep to delete notes");
        int clip = -1, index = -1;
        bool nl = false, nr = false;
        noteAt(p, clip, index, nl, nr);
        if (index < 0) return tr("tracks-pane-roll.drag-to-marquee-right-click", "Drag to marquee - right-click for the note menu");
        if (nl || nr) return tr("tracks-pane-roll.drag-the-edge-to-resize", "Drag the edge to resize - Alt-drag the body for velocity");
        return tr("tracks-pane-roll.drag-to-move-alt-drag", "Drag to move - Alt-drag for velocity, right-click for the menu");
    }
    const auto handleTip = [](ClipHit h) -> juce::String {
        switch (h) {
            case ClipHit::FadeL:  return tr("tracks-pane-roll.fade-in-drag-to-set", "Fade in - drag to set how long it takes");
            case ClipHit::FadeR:  return tr("tracks-pane-roll.fade-out-drag-to-set", "Fade out - drag to set how long it takes");
            case ClipHit::CurveL:
            case ClipHit::CurveR:
                return tr("tracks-pane-roll.fade-shape-drag-up-or", "Fade shape - drag up or down to bend it, double-click to straighten");
            case ClipHit::EdgeL:  return tr("tracks-pane-roll.drag-to-trim-the-start-mac", "Drag to trim the start - Cmd-drag to stretch");
            case ClipHit::EdgeR:  return tr("tracks-pane-roll.drag-to-trim-the-end-mac", "Drag to trim the end - Cmd-drag to stretch");
            default:              return {};
        }
    };
    if (mode_ == Mode::Clip && effectiveTool() == Tool::Pointer) {
        const auto h = clipEditorHit(p);
        if (const auto t = handleTip(h); t.isNotEmpty()) return t;
        if (h == ClipHit::Body) return tr("tracks-pane-roll.drag-to-select-a-range", "Drag to select a range - Alt-drag to slip the audio");
    }
    if ((mode_ == Mode::Song || mode_ == Mode::Track) && p.x >= kStripW
        && effectiveTool() == Tool::Pointer) {
        if (const int row = rowAt(p.y); row >= 0) {
            const auto h = rowClipHit(row, p);
            if (const auto t = handleTip(h); t.isNotEmpty()) return t;
            if (h == ClipHit::Body) {
                bool l = false, r = false;
                const int c = clipAt(row, p, l, r);
                const auto clips = host_.clips().list(rows_[(size_t) row]);
                if (c >= 0 && c < (int) clips.size() && !clips[(size_t) c].looped
                    && overRepeatGrip(clipBounds(row, clips[(size_t) c]), p))
                    return tr("tracks-pane-roll.drag-to-repeat-the-clip", "Drag to repeat the clip");
                return mode_ == Mode::Song
                    ? tr("tracks-pane-roll.drag-to-move-cmd-drag-mac", "Drag to move - Cmd-drag to duplicate, double-click to edit")
                    : "Drag to move - Cmd-drag to duplicate";
            }
        }
    }
    if (mode_ == Mode::Song && p.x < kStripW && p.y >= headerH()) {
        if (const int row = rowAt(p.y); row >= 0) {
            if (muteBox(row).contains(p)) return "Mute";
            if (soloBox(row).contains(p)) return "Solo";
            if (recBox(row).contains(p)) return "Arm";
            if (heldBox(row).contains(p)) return tr("tracks-pane-roll.a-hand-is-holding-this", "A hand is holding this lane - click to let go");
            if (foldBox(row).contains(p)) return tr("tracks-pane-roll.show-what-is-folded-under", "Show what is folded under this track");
        }
    }
    return {};
}

}
