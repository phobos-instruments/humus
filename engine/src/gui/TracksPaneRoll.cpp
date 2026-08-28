#include <set>

#include "gui/TracksPane.h"

#include <algorithm>
#include <cmath>

#include "core/ClipOps.h"
#include "gui/ClipColors.h"
#include "gui/LookAndFeel.h"
#include "hum/ClipStack.h"

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
    lo = 127; hi = 0;
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
    const int bottom = fieldBottom() - kRibbonH - (rollShowsVelocity() ? kVelH : 0);
    return {kStripW, top, std::max(0, getWidth() - kStripW), std::max(0, bottom - top)};
}

timelinechrome::RollPlot TracksPane::rollPlot() const {
    const auto f = rollField();
    timelinechrome::RollPlot rp;
    rp.rowH = rollRowH_;
    rp.top = (float) f.getY() + rollScrollAcc_ * rp.rowH;
    rp.h = (float) f.getHeight();
    rp.topPitch = juce::jlimit(0, 127, rollTopPitch_ + rollScrollSemis_);
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
    g.setFont(juce::FontOptions(10.0f));
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
            if (pitch < 0 || pitch > 127) continue;
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
            g.drawText(ci.name.empty() ? "clip " + juce::String(ci.index + 1)
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
                                      0.25f, 1.0f, 0.45f + 0.55f * n.velocity / 127.0f))
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
            if (pitch < 0 || pitch > 127) continue;
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
        const int top = fieldBottom() - kVelH;
        g.setColour(Palette::background.darker(0.15f));
        g.fillRect(0, top, getWidth(), kVelH);
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
                const float h = (kVelH - 6.0f) * n.velocity / 127.0f;
                g.fillRect(x, (float) (top + kVelH - 3) - h, 3.0f, h);
            }
        }
    }

    if (clips.empty()) {
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f));
        auto r = f.reduced(0, f.getHeight() / 3);
        g.drawText("No clips on this track yet", r.removeFromTop(r.getHeight() / 2),
                   juce::Justification::centredBottom);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Drag one in, or arm R and play", r, juce::Justification::centredTop);
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
    pitch = juce::jlimit(0, 127, pitch);
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
        if (addTrackBox().contains(p)) return "Add a track, wired";
        if (snapBox().contains(p))
            return snapChoice_ < 0.0 ? "Snap: free - click to choose a grid"
                 : snapChoice_ > 0.0 ? "Snap: chosen - click to change or follow the zoom"
                                     : "Snap follows the zoom - click to choose one";
        if (followBox().contains(p)) return "Follow the playhead while it plays";
    }
    if (mode_ == Mode::Track && p.y < kTopH) {
        if (crumbBackBox().contains(p)) return "Back to the timeline (Esc)";
        if (crumbNameBox().contains(p)) return "Choose a track (Alt+Up / Alt+Down)";
    }
    if (mode_ == Mode::Track && rollPlot().usable && rollField().contains(p)) {
        if (effectiveTool() == Tool::Draw) return "Draw a note - drag to set its length";
        if (effectiveTool() == Tool::Scissors) return "Split the note at the click";
        if (effectiveTool() == Tool::Eraser) return "Sweep to delete notes";
        int clip = -1, index = -1;
        bool nl = false, nr = false;
        noteAt(p, clip, index, nl, nr);
        if (index < 0) return "Drag to marquee - right-click for the note menu";
        if (nl || nr) return "Drag the edge to resize - Alt-drag the body for velocity";
        return "Drag to move - Alt-drag for velocity, right-click for the menu";
    }
    if (mode_ == Mode::Song && p.x < kStripW && p.y >= headerH()) {
        if (const int row = rowAt(p.y); row >= 0) {
            if (muteBox(row).contains(p)) return "Mute";
            if (soloBox(row).contains(p)) return "Solo";
            if (recBox(row).contains(p)) return "Arm";
            if (heldBox(row).contains(p)) return "A hand is holding this lane - click to let go";
            if (foldBox(row).contains(p)) return "Show what is folded under this track";
        }
    }
    return {};
}

}
