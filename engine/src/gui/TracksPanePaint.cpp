#include "gui/TracksPane.h"

#include "core/ClipOps.h"
#include "hum/ClipStack.h"

#include <climits>
#include <cmath>
#include <cstdint>

#include "core/Categories.h"
#include "gui/ClipColors.h"
#include "gui/EnvelopePainter.h"
#include "gui/LookAndFeel.h"
#include "gui/TimelineChrome.h"
#include "gui/WaveformCache.h"

namespace hum {

void TracksPane::paintToolbar(juce::Graphics& g) {
    int i = 0;
    for (const auto t : kToolbar) {
        const auto b = toolBox(i++);
        const bool on = effectiveTool() == t;
        g.setColour(on ? Palette::accent.withAlpha(0.25f) : Palette::panel);
        g.fillRoundedRectangle(b.toFloat(), 3.0f);
        g.setColour(on ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(b.toFloat().reduced(0.5f), 3.0f, 1.0f);
        const auto r = b.toFloat().reduced(6.0f, 5.0f);
        g.setColour(on ? Palette::accent : Palette::textDim);
        timelinechrome::paintToolIcon(g, r, t);
    }
    g.setColour(Palette::panel);
    g.fillRect(0, 0, getWidth(), kTopH);
    const auto fb = followBox();
    g.setColour(follow_ ? Palette::accent : Palette::panelLight);
    g.fillRoundedRectangle(fb.toFloat(), 3.0f);
    g.setColour(follow_ ? Palette::background : Palette::textDim);
    g.setFont(juce::FontOptions(9.5f));
    g.drawText("Follow Play", fb, juce::Justification::centred);

    if (hasSel_ && selTo_ > selFrom_) {
        const float x0 = juce::jmax((float) kStripW, beatToX(selFrom_));
        const float x1 = beatToX(selTo_);
        if (x1 > x0) {
            g.setColour(Palette::accent.withAlpha(0.12f));
            g.fillRect(x0, (float) rulerTop(), x1 - x0, (float) getHeight());
        }
    }

    const auto ab = addTrackBox();
    g.setColour(Palette::panel);
    g.fillRoundedRectangle(ab.toFloat(), 3.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(ab.toFloat().reduced(0.5f), 3.0f, 1.0f);
    g.setColour(Palette::accent);
    const auto ac = ab.toFloat().reduced(7.0f, 6.0f);
    g.fillRect(ac.getX(), ac.getCentreY() - 0.75f, ac.getWidth(), 1.5f);
    g.fillRect(ac.getCentreX() - 0.75f, ac.getY(), 1.5f, ac.getHeight());

    timelinechrome::paintSnapChip(g, snapBox(), gridBeats(), snapChoice_ != 0.0,
                                  snapChoice_ < 0.0 ? "Free" : nullptr);
}

void TracksPane::paintRuler(juce::Graphics& g) {
    g.setColour(Palette::panel);
    g.fillRect(kStripW, 0, getWidth() - kStripW, headerH());

    if (host_.automation().loopEnabled()) {
        g.saveState();
        g.reduceClipRegion(kStripW, 0, getWidth() - kStripW, headerH());
        const float a = beatToX(host_.automation().loopStartBeat()), b = beatToX(host_.automation().loopEndBeat());
        const float top = (float) loopTop() + 1.0f, h = (float) kLoopH - 2.0f;
        g.setColour(Palette::accent.withAlpha(0.35f));
        g.fillRect(a, top, std::max(2.0f, b - a), h);
        g.setColour(Palette::accent);
        for (const float x : { a, b })
            g.fillRoundedRectangle(x - (float) kLoopGrip * 0.5f, top, (float) kLoopGrip, h, 1.5f);
        g.setColour(Palette::background.withAlpha(0.55f));
        for (const float x : { a, b })
            g.drawVerticalLine((int) x, top + 1.5f, top + h - 1.5f);
        g.restoreState();
    }
    g.setColour(Palette::border);
    g.drawHorizontalLine(rulerTop(), (float) kStripW, (float) getWidth());

    const int bpb = host_.automation().timeSigNumerator();
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 9.5f,
                                juce::Font::plain));
    const auto cb = g.getClipBounds();
    const float barPixels = (float) (bpb * ppb_);
    const int stride = timelinechrome::barLabelStride(barPixels);
    const int firstBar = std::max(0, (int) (scrollBeats_ / bpb));
    for (int bar = firstBar;; ++bar) {
        const float x = beatToX(bar * (double) bpb);
        if (x > (float) getWidth() || x > (float) cb.getRight()) break;
        if (x < (float) kStripW || x < (float) cb.getX() - 37.0f) continue;
        g.setColour(Palette::border);
        g.drawVerticalLine((int) x, (float) rulerTop(), (float) headerH());
        if (bar % stride != 0) continue;
        g.setColour(Palette::textDim);
        g.drawText(timelinechrome::barLabel(bar + 1, bar * (double) bpb,
                                            host_.model().clock.tempo,
                                            barPixels * (float) stride),
                   (int) x + 3, rulerTop(), 80, kRulerH - 2,
                   juce::Justification::centredLeft, false);
    }
}

void TracksPane::paintRow(juce::Graphics& g, int row) {
    const auto& node = rows_[(size_t) row];
    const int y = rowTop(row);
    const auto cb = g.getClipBounds();

    if (cb.getX() < kStripW) paintRowHeader(g, row, y);

    g.setColour(Palette::background.brighter(row % 2 ? 0.02f : 0.05f));
    g.fillRect(kStripW, y, getWidth() - kStripW, kRowH - 1);
    const int bpb = host_.automation().timeSigNumerator();
    for (int bar = std::max(0, (int) (scrollBeats_ / bpb));; ++bar) {
        const float x = beatToX(bar * (double) bpb);
        if (x > (float) getWidth() || x > (float) cb.getRight()) break;
        if (x < (float) kStripW || x < (float) cb.getX()) continue;
        g.setColour(Palette::border.withAlpha(0.3f));
        g.drawVerticalLine((int) x, (float) y, (float) (y + kRowH - 1));
    }
    if (const double grid = gridBeats(); grid < (double) bpb) {
        g.setColour(Palette::border.withAlpha(0.12f));
        for (int i = std::max(0, (int) (scrollBeats_ / grid));; ++i) {
            const double b = i * grid;
            const float x = beatToX(b);
            if (x > (float) getWidth() || x > (float) cb.getRight()) break;
            if (x < (float) kStripW || x < (float) cb.getX()) continue;
            if (std::abs(b / bpb - std::round(b / bpb)) < 1e-9) continue;
            g.drawVerticalLine((int) x, (float) y, (float) (y + kRowH - 1));
        }
    }

    const juce::Colour laneCol = timelinechrome::laneAccent(host_.model(), node);
    bool quiet = host_.bypassed(node);
    if (nodeMuted(node)) quiet = true;
    auto dim = [quiet](juce::Colour c) {
        return quiet ? c.withSaturation(c.getSaturation() * 0.25f).withAlpha(0.75f) : c;
    };
    auto cCol = [&](int idx) { return dim(idx > 0 ? clipColour(idx) : laneCol); };
    auto cFill = [&](int idx) { return dim(idx > 0 ? clipFill(idx) : laneCol.darker(0.72f)); };

    for (const auto& ci : host_.clips().list(node)) {
        auto b = clipBounds(row, ci);
        if (b.getRight() < kStripW || b.getX() > getWidth()) {
            if (!ci.looped) continue;
        }
        if (!ci.looped && (b.getRight() < cb.getX() || b.getX() > cb.getRight()))
            continue;
        const int unclampedLeft = b.getX();
        b.setLeft(std::max(b.getX(), kStripW));
        if (b.getWidth() > 0 && b.getRight() >= cb.getX() && b.getX() <= cb.getRight()) {
            const bool sel = (row == selClipRow_ && ci.index == selClip_)
                             || clipSelected(row, ci.id);
            g.setColour(sel ? cFill(ci.color).brighter(0.4f) : cFill(ci.color));
            g.fillRoundedRectangle(b.toFloat(), 3.0f);
            if (ci.isAudio) paintWaveform(g, b, unclampedLeft, ci, cCol(ci.color));
            else            paintNotes(g, b, unclampedLeft, node, ci, cCol(ci.color));
            if (sel) {
                g.setColour(Palette::text.withAlpha(0.16f));
                g.fillRoundedRectangle(b.toFloat(), 3.0f);
                g.setColour(Palette::text.withAlpha(0.9f));
                g.drawRoundedRectangle(b.toFloat().reduced(1.0f), 3.0f, 2.0f);
            } else {
                g.setColour(cCol(ci.color));
                g.drawRoundedRectangle(b.toFloat(), 3.0f, 1.2f);
            }
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(10.0f));
            const auto label = ci.name.empty()
                ? juce::String("clip ") + juce::String(ci.index + 1) : juce::String(ci.name);
            const bool marks = unclampedLeft == b.getX() && b.getWidth() > 3 * kFadeGrip;
            if (b.getWidth() > 30)
                g.drawText(label, b.reduced(4, 2).withTrimmedLeft(marks ? kFadeGrip : 0),
                           juce::Justification::topLeft, true);
            timelinechrome::paintFades(g, b, ci.fadeInTicks, ci.fadeOutTicks,
                                       ci.lengthTicks, cCol(ci.color),
                                       ci.fadeInCurve, ci.fadeOutCurve);
            if (marks)
                timelinechrome::paintFadeGrips(g, b, kFadeGrip, ci.fadeInTicks,
                                               ci.fadeOutTicks, cCol(ci.color));
            timelinechrome::paintFadeCurveGrips(g, b, ci.fadeInTicks, ci.fadeOutTicks,
                                                ci.lengthTicks, ci.fadeInCurve,
                                                ci.fadeOutCurve, cCol(ci.color));
            if (!ci.looped)
                timelinechrome::paintRepeatGrip(g, b, kFadeGrip, cCol(ci.color), sel);
            if (ci.isAudio)
                timelinechrome::paintWarpBadge(g, b, ci.sourceBpm, host_.tempo(),
                                               ci.warpMode, cCol(ci.color));
        }

        if (ci.looped && ci.lengthTicks > 0) {
            int ghostEnd = ci.startTick;
            if (const auto* cm = host_.model().byName(node)) {
                const auto chans = clipops::clipChannels(cm->pattern);
                if (ci.index < (int) chans.size()) {
                    const auto sp = clipstack::soundingSpans(cm->pattern,
                                                             chans[(size_t) ci.index]);
                    if (!sp.empty())
                        ghostEnd = sp.back().startTick + sp.back().lengthTicks;
                }
            }
            g.setColour(cFill(ci.color).withAlpha(0.35f));
            for (int start = ci.startTick + ci.lengthTicks; start < ghostEnd;
                 start += ci.lengthTicks) {
                const float gx0 = tickToX(start);
                const float gx1 = tickToX(std::min(start + ci.lengthTicks, ghostEnd));
                if (gx0 > (float) getWidth() || gx0 > (float) cb.getRight()) break;
                if (gx1 < (float) kStripW || gx1 < (float) cb.getX()) continue;
                g.fillRoundedRectangle(std::max(gx0, (float) kStripW), (float) b.getY(),
                                       std::max(4.0f, gx1 - std::max(gx0, (float) kStripW)),
                                       (float) b.getHeight(), 3.0f);
            }
        }
    }

    if (!wantsBoxRow(row)) paintBoxes(g, row);

    if (auto* cr = dynamic_cast<ClipRecorder*>(host_.liveOrganism(node));
        cr != nullptr && cr->takeActive() && cr->takeLengthSamples() > 0) {
        const float x0 = std::max(beatToX(cr->takeStartBeat()), (float) kStripW);
        const float x1 = std::max(beatToX(playBeat_), x0 + 4.0f);
        const juce::Rectangle<float> r(x0, (float) y + 2.0f, x1 - x0, (float) kRowH - 5.0f);
        g.setColour(juce::Colour(0xffb04040).withAlpha(0.35f));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(juce::Colour(0xffb04040));
        g.drawRoundedRectangle(r, 3.0f, 1.0f);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("REC", r.reduced(4.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft);
    }
}

void TracksPane::paintBoxes(juce::Graphics& g, int row) {
    const auto& node = rows_[(size_t) row];
    const auto cb = g.getClipBounds();
    const auto& boxes = host_.automation().boxes();
    const auto* cm = host_.model().byName(node);
    for (int i = 0; i < (int) boxes.size(); ++i) {
        if (boxes[(size_t) i].organism != node) continue;
        auto b = boxBounds(row, boxes[(size_t) i]);
        if (b.getRight() < kStripW || b.getX() > getWidth()) continue;
        b.setLeft(std::max(b.getX(), kStripW));
        if (b.getRight() < cb.getX() || b.getX() > cb.getRight()) continue;
        const bool sel = i == selBox_ || boxSelected(i);
        g.setColour((sel ? Palette::accent : Palette::accentDim).withAlpha(0.28f));
        g.fillRoundedRectangle(b.toFloat(), 3.0f);
        g.setColour(sel ? Palette::accent : Palette::accentDim);
        g.drawRoundedRectangle(b.toFloat(), 3.0f, sel ? 1.6f : 1.1f);
        if (b.getWidth() > 24) {
            const int gy = b.getCentreY() - 4;
            g.fillRoundedRectangle((float) b.getX() + 2.0f, (float) gy, 2.0f, 8.0f, 1.0f);
            g.fillRoundedRectangle((float) b.getRight() - 4.0f, (float) gy, 2.0f, 8.0f, 1.0f);
        }
        if (cm != nullptr && b.getWidth() > 14) {
            const auto& box = boxes[(size_t) i];
            const double delta = dragBox_ == i ? boxDragDelta_ : 0.0;
            g.saveState();
            g.reduceClipRegion(b);
            int drawn = 0;
            for (const auto& l : cm->automation) {
                if (drawn >= 3 || l.points.empty()) continue;
                double lo = 1e18, hi = -1e18;
                for (const auto& pt : l.points) {
                    lo = std::min({lo, pt.value, pt.valueMax});
                    hi = std::max({hi, pt.value, pt.valueMax});
                }
                if (hi <= lo) { lo -= 0.5; hi += 0.5; }
                juce::Path path;
                bool first = true;
                for (const auto& pt : l.points) {
                    if (pt.beat < box.startBeat || pt.beat > box.endBeat) continue;
                    const float px = beatToX(pt.beat + delta);
                    const float py = (float) (b.getBottom() - 3
                        - (pt.value - lo) / (hi - lo) * (b.getHeight() - 8));
                    if (first) { path.startNewSubPath(px, py); first = false; }
                    else path.lineTo(px, py);
                }
                g.setColour(Palette::text.withAlpha(0.45f - 0.1f * (float) drawn));
                g.strokePath(path, juce::PathStrokeType(1.0f));
                ++drawn;
            }
            g.restoreState();
            g.setColour(Palette::text.withAlpha(0.8f));
            g.setFont(juce::FontOptions(9.0f));
            int nLanes = 0;
            for (const auto& l : cm->automation) if (!l.points.empty()) ++nLanes;
            if (b.getWidth() > 40)
                g.drawText(juce::String(nLanes) + (nLanes == 1 ? " param" : " params"),
                           b.reduced(4, 1), juce::Justification::topLeft, false);
        }
    }
}

void TracksPane::paintBoxRow(juce::Graphics& g, const trackslayout::Slot& s) {
    const auto& node = rows_[(size_t) s.track];
    g.setColour(Palette::panel);
    g.fillRect(0, s.y, kStripW, s.h - 1);
    const auto fb = boxFoldBox(s).toFloat();
    const bool open = expanded_.count(node) != 0;
    juce::Path tri;
    if (open) tri.addTriangle(fb.getX() + 2, fb.getY() + 4, fb.getRight() - 2, fb.getY() + 4,
                              fb.getCentreX(), fb.getBottom() - 3);
    else      tri.addTriangle(fb.getX() + 4, fb.getY() + 2, fb.getX() + 4, fb.getBottom() - 2,
                              fb.getRight() - 3, fb.getCentreY());
    g.setColour(Palette::textDim);
    g.fillPath(tri);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("Automation", 20, s.y, kStripW - 24, s.h - 1, juce::Justification::centredLeft, false);
    g.setColour(Palette::background.brighter(0.035f));
    g.fillRect(kStripW, s.y, getWidth() - kStripW, s.h - 1);
    timelinechrome::paintTimeGrid(g, {kStripW, s.y, getWidth() - kStripW, s.h - 1}, kStripW,
                                  scrollBeats_, ppb_, host_.automation().timeSigNumerator(),
                                  gridBeats());
    paintBoxes(g, s.track);
}

void TracksPane::paintBackCrumb(juce::Graphics& g) {
    const auto back = crumbBackBox();
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(back.toFloat(), 3.0f);
    g.setColour(Palette::textDim);
    g.drawText(juce::String::fromUTF8("\xe2\x86\x90 Back to Timeline"), back,
               juce::Justification::centred, false);
}

void TracksPane::repaintCutGuide(int fromX, int toX) {
    const auto guideX = [this](int x) {
        return (int) beatToX(snapBeats(std::max(0.0, xToBeat((float) x)),
                                       juce::ModifierKeys::getCurrentModifiersRealtime()
                                           .isAltDown()));
    };
    const int a = guideX(fromX), b = guideX(toX);
    repaint(std::min(a, b) - 3, 0, std::abs(b - a) + 6, getHeight());
}

void TracksPane::paintCutGuide(juce::Graphics& g) {
    if (effectiveTool() != Tool::Scissors || hover_.x < kStripW || hover_.y < headerH()) return;
    const double beat = snapBeats(std::max(0.0, xToBeat((float) hover_.x)),
                                  juce::ModifierKeys::getCurrentModifiersRealtime().isAltDown());
    const float x = beatToX(beat);
    g.setColour(Palette::text.withAlpha(0.7f));
    const float dash[] = {3.0f, 3.0f};
    g.drawDashedLine(juce::Line<float>(x, (float) headerH(), x, (float) getHeight()), dash, 2, 1.0f);
}

void TracksPane::paintBoxCrumb(juce::Graphics& g) {
    paintBackCrumb(g);
    const auto back = crumbBackBox();
    g.setFont(juce::FontOptions(10.0f));
    const juce::Rectangle<int> name(back.getRight() + 10, 1, 300, kTopH - 2);
    g.setColour(timelinechrome::laneAccent(host_.model(), boxNode_));
    g.fillEllipse((float) name.getX(), (float) name.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour(Palette::text);
    g.drawText(juce::String(boxNode_) + juce::String::fromUTF8(" \xe2\x80\xba Automation"),
               name.withTrimmedLeft(10), juce::Justification::centredLeft, true);
}

void TracksPane::paintLinePreview(juce::Graphics& g) {
    if (drag_ != Drag::Line || lineSlot_ < 0 || lineSlot_ >= (int) slots_.size()) return;
    const auto& sl = slots_[(size_t) lineSlot_];
    const auto [lo, hi] = laneRange(dragAutoNode_, dragAutoParam_);
    g.setColour(Palette::accent.withAlpha(0.9f));
    g.drawLine(beatToX(lineBeat0_), laneYAtValue(sl, lineVal0_, lo, hi),
               beatToX(lineBeat1_), laneYAtValue(sl, lineVal1_, lo, hi), 1.5f);
}

void TracksPane::paintRowHeader(juce::Graphics& g, int row, int y) {
    const auto& node = rows_[(size_t) row];
    g.setColour(Palette::panel);
    g.fillRect(0, y, kStripW, kRowH - 1);
    if (row == selClipRow_) {
        const auto ac = timelinechrome::laneAccent(host_.model(), node);
        g.setColour(ac.withAlpha(0.16f));
        g.fillRect(0, y, kStripW - 1, kRowH - 1);
        g.setColour(ac);
        g.fillRect(0, y, 3, kRowH - 1);
    }
    int nameX = 6;
    if (hasLanes(row) && !wantsBoxRow(row)) {
        const auto fb = foldBox(row).toFloat();
        const bool open = expanded_.count(node) != 0;
        juce::Path tri;
        if (open) tri.addTriangle(fb.getX() + 2, fb.getY() + 4, fb.getRight() - 2, fb.getY() + 4,
                                  fb.getCentreX(), fb.getBottom() - 3);
        else      tri.addTriangle(fb.getX() + 4, fb.getY() + 2, fb.getX() + 4, fb.getBottom() - 2,
                                  fb.getRight() - 3, fb.getCentreY());
        g.setColour(Palette::textDim);
        g.fillPath(tri);
        nameX = 20;
    }
    g.setColour(timelinechrome::laneAccent(host_.model(), node));
    g.fillRoundedRectangle((float) nameX, (float) y + 7.0f, 8.0f, 8.0f, 2.0f);
    nameX += 12;

    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(12.0f));
    const auto* ncm = host_.model().byName(node);
    const auto pseudo = ncm != nullptr ? pseudoOwnerLabel(ncm->displayClass) : std::string();
    juce::String shown = pseudo.empty() ? juce::String(node) : juce::String(pseudo);
    if (pseudo.empty())
        if (const auto pod = podOfRow(row); !pod.empty())
            shown = juce::String(node.substr(pod.size() + 1));
    g.drawText(shown, nameX, y + 2, kStripW - nameX - 82, 18,
               juce::Justification::centredLeft, true);
    const bool rowHeld = host_.automation().anyHeld(node);
    timelinechrome::paintDestChip(g, {nameX, y + 21, kStripW - nameX - (rowHeld ? 30 : 8), 13},
                                  host_.model(), node);
    if (rowHeld) timelinechrome::paintHeldBadge(g, heldBox(row));

    const bool muted = nodeMuted(node);
    bool recParam = false;
    if (const auto* cm = host_.model().byName(node))
        for (const auto& p : cm->properties)
            if (p.name == "Record") { recParam = p.value >= 0.5; break; }
    const bool armed = host_.nodeRecordsAudio(node) ? recParam
                                                    : host_.midi().isRecordTarget(node);
    auto box = [&](juce::Rectangle<int> r, const char* t, bool on, juce::Colour onCol) {
        g.setColour(on ? onCol : Palette::panelLight);
        g.fillRect(r);
        g.setColour(Palette::border);
        g.drawRect(r, 1);
        g.setColour(on ? Palette::background : Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(t, r, juce::Justification::centred, false);
    };
    box(muteBox(row), "M", muted, Palette::accent);
    if (arrangeable_.count(node) != 0)
        box(soloBox(row), "S", host_.soloed(node), Palette::warnAmber());
    box(recBox(row), "R", armed, juce::Colour(0xffb04040));
}

void TracksPane::paintAutoLane(juce::Graphics& g, const trackslayout::Slot& slot) {
    const auto& node = rows_[(size_t) slot.track];
    const auto* cm = host_.model().byName(node);
    if (!cm) return;
    const hum::AutomationLane* lane = nullptr;
    for (const auto& l : cm->automation)
        if (l.propertyName == slot.param) { lane = &l; break; }
    const int y = slot.y, h = slot.h;

    g.setColour(Palette::panel.darker(0.15f));
    g.fillRect(0, y, kStripW, h - 1);
    g.setColour(Palette::border.withAlpha(0.4f));
    g.drawHorizontalLine(y, 0.0f, (float) getWidth());
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(juce::String::fromUTF8("\xe2\x86\xb3 ") + slot.param, 20, y, kStripW - 76, h,
               juce::Justification::centredLeft, true);
    const bool muted = lane && lane->mute;
    auto box = [&](juce::Rectangle<int> r, const char* t, bool on, juce::Colour onCol) {
        g.setColour(on ? onCol : Palette::panelLight);
        g.fillRect(r);
        g.setColour(on ? Palette::background : Palette::textDim);
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(t, r, juce::Justification::centred);
    };
    box({kStripW - 54, y + (h - 12) / 2, 22, 12}, "M", muted, Palette::accent);
    const bool held = host_.automation().isHeld(node, slot.param);
    if (held) timelinechrome::paintHeldBadge(g, heldLaneBox(slot));

    if (!lane || lane->points.empty()) return;
    const auto range = envpaint::laneRange(cm, slot.param,
                                           (int) host_.model().metapad.snapshots.size());
    const double lo = range.first, hi = range.second;
    const double span = hi > lo ? hi - lo : 1.0;
    g.saveState();
    g.reduceClipRegion(kStripW, y, getWidth() - kStripW, h);
    envpaint::draw(g, lane->points, lane->kind, (float) kStripW, (float) y, (float) h,
                   [this](double b) { return beatToX(b); },
                   [=](double v) { return (float) (y + h - 3 - (v - lo) / span * (h - 6)); },
                   muted ? Palette::textDim : (held ? Palette::warnAmber() : Palette::accent),
                   2.5f);
    const int me = (int) (&slot - slots_.data());
    if (hoverPt_ >= 0 && hoverPtSlot_ == me && hoverPt_ < (int) lane->points.size()) {
        const auto& pt = lane->points[(size_t) hoverPt_];
        const float px = beatToX(pt.beat);
        const float py = (float) (y + h - 3 - (pt.value - lo) / span * (h - 6));
        g.setColour(Palette::text);
        g.drawEllipse(px - 5.0f, py - 5.0f, 10.0f, 10.0f, 1.5f);
    }
    g.restoreState();
}

void TracksPane::paintPodHeader(juce::Graphics& g, const trackslayout::Slot& slot) {
    const int y = slot.y, h = slot.h;
    const bool folded = collapsedPods_.count(slot.param) != 0;
    g.setColour(Palette::panelLight);
    g.fillRect(0, y, getWidth(), h - 1);
    g.setColour(Palette::border);
    g.drawHorizontalLine(y, 0.0f, (float) getWidth());

    const auto fb = podFoldBox(slot).toFloat();
    juce::Path tri;
    if (folded) tri.addTriangle(fb.getX() + 3, fb.getY() + 1, fb.getX() + 3, fb.getBottom() - 1,
                                fb.getRight() - 2, fb.getCentreY());
    else        tri.addTriangle(fb.getX() + 1, fb.getY() + 3, fb.getRight() - 1, fb.getY() + 3,
                                fb.getCentreX(), fb.getBottom() - 2);
    g.setColour(Palette::textDim);
    g.fillPath(tri);

    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(11.5f));
    g.drawText(juce::String(slot.param), 18, y, kStripW - 24, h,
               juce::Justification::centredLeft, true);
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(9.5f));
    const int n = trackslayout::podRowCount((int) rows_.size(),
                                            [this](int t) { return podOfRow(t); }, slot.param);
    g.drawText(juce::String(n) + (n == 1 ? " lane" : " lanes"),
               kStripW - 60, y, 54, h, juce::Justification::centredRight);

    if (folded) {
        int lo = INT_MAX, hi = 0;
        for (int t = 0; t < (int) rows_.size(); ++t) {
            if (podOfRow(t) != slot.param) continue;
            for (const auto& ci : host_.clips().list(rows_[(size_t) t])) {
                lo = std::min(lo, ci.startTick);
                hi = std::max(hi, ci.startTick + ci.lengthTicks);
            }
        }
        if (hi > lo && lo != INT_MAX) {
            const float x0 = std::max((float) kStripW, tickToX(lo));
            const float x1 = tickToX(hi);
            if (x1 > x0) {
                g.setColour(Palette::accent.withAlpha(0.22f));
                g.fillRoundedRectangle(x0, (float) y + 4.0f, x1 - x0, (float) h - 9.0f, 2.0f);
            }
        }
    }
}

void TracksPane::paintDropHint(juce::Graphics& g) {
    g.setColour(Palette::accent.withAlpha(0.10f));
    g.fillRect(getLocalBounds().withTrimmedTop(headerH()));
    g.setColour(Palette::accent);
    g.drawRect(getLocalBounds().withTrimmedTop(headerH()).reduced(2), 2);
}

void TracksPane::paintWaveform(juce::Graphics& g, juce::Rectangle<int> b, int clipLeft,
                               const ClipEditor::ClipInfo& ci, juce::Colour accent) {
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (!peaks || !peaks->ready || peaks->binSamples <= 0 || peaks->sourceSamples <= 0) return;

    const double spb = (host_.tempo() > 0.0 ? 60.0 / host_.tempo() : 0.5) * host_.sampleRate();
    const double clipSrcLen = (double) ci.lengthTicks / Pattern::kTicksPerBeat * spb;
    const int fullW = b.getRight() - clipLeft;
    if (fullW <= 0 || clipSrcLen <= 0.0) return;

    const double sessionToFileRate = peaks->fileSampleRate > 0.0
                                         ? peaks->fileSampleRate / host_.sampleRate() : 1.0;
    const float midY = b.getCentreY();
    const float halfH = b.getHeight() * 0.5f - 2.0f;
    const auto peakCol = accent.withAlpha(0.45f);
    const auto bodyCol = accent.brighter(0.35f).withAlpha(0.75f);
    const bool haveRms = peaks->rms.size() == peaks->hi.size();
    for (int x = b.getX(); x < b.getRight(); ++x) {
        auto binAt = [&](int px) {
            const double frac = (double) (px - clipLeft) / fullW;
            const double rel = ci.audioReverse ? (1.0 - frac) * clipSrcLen : frac * clipSrcLen;
            const std::int64_t sm = ci.audioOffset + (std::int64_t) rel;
            return (int) ((double) sm * sessionToFileRate / peaks->binSamples);
        };
        const int ba = binAt(x), bb = binAt(x + 1);
        const int b0 = std::min(ba, bb), b1 = std::max(b0 + 1, std::max(ba, bb));
        float hi = 0.0f, lo = 0.0f, rms = 0.0f;
        int n = 0;
        for (int k = std::max(0, b0); k < std::min(b1, (int) peaks->hi.size()); ++k) {
            hi = std::max(hi, peaks->hi[(size_t) k]);
            lo = std::min(lo, peaks->lo[(size_t) k]);
            if (haveRms) rms = std::max(rms, peaks->rms[(size_t) k]);
            ++n;
        }
        if (n == 0) continue;
        g.setColour(peakCol);
        g.drawVerticalLine(x, midY - hi * halfH, midY - lo * halfH + 1.0f);
        if (rms > 0.002f) {
            g.setColour(bodyCol);
            g.drawVerticalLine(x, midY - rms * halfH, midY + rms * halfH);
        }
    }
}

void TracksPane::paintNotes(juce::Graphics& g, juce::Rectangle<int> b, int clipLeft,
                            const std::string& node, const ClipEditor::ClipInfo& ci,
                            juce::Colour accent) {
    const auto notes = host_.clips().notes(node, ci.index);
    if (notes.empty()) return;
    int lo = 127, hi = 0;
    for (const auto& n : notes) { lo = std::min(lo, n.pitch); hi = std::max(hi, n.pitch); }
    const auto np = timelinechrome::notePlot(b, b.getRight() - clipLeft, ci.lengthTicks,
                                             lo, hi, Pattern::kTicksPerBeat);
    if (!np.usable) return;

    g.setColour(accent.brighter(0.5f).withAlpha(np.dense ? 0.4f : 0.9f));
    for (const auto& n : notes) {
        const float x = (float) clipLeft + (float) n.tick * np.perTick;
        const float w = juce::jmax(1.0f, (float) std::max(1, n.lengthTicks) * np.perTick);
        if (x + w < (float) b.getX() || x > (float) b.getRight()) continue;
        const float x0 = juce::jmax(x, (float) b.getX());
        const float w0 = juce::jmin(x + w, (float) b.getRight()) - x0;
        if (w0 <= 0.0f) continue;
        if (np.dense) g.fillRect(x0, np.top, juce::jmin(w0, 1.5f), np.h);
        else          g.fillRect(x0, np.yFor(n.pitch), w0, np.rowH);
    }
}

void TracksPane::paint(juce::Graphics& g) {
    paintField(g);
    paintZoom(g);
}

void TracksPane::paintZoom(juce::Graphics& g) {
    g.setColour(Palette::panel);
    g.fillRect(0, fieldBottom(), getWidth(), kZoomGut);
    g.fillRect(getWidth() - kZoomGut, headerH(), kZoomGut, fieldBottom() - headerH());
    g.setColour(Palette::border);
    g.drawHorizontalLine(fieldBottom(), 0.0f, (float) getWidth());
    g.drawVerticalLine(getWidth() - kZoomGut, (float) headerH(), (float) fieldBottom());

    const int axes = mode_ == Mode::Box ? 1 : 2;
    for (int axis = 0; axis < axes; ++axis) {
        const auto groove = zoomGroove(axis).toFloat();
        g.setColour(Palette::background);
        g.fillRoundedRectangle(groove.reduced(axis == 0 ? 0.0f : 4.0f,
                                              axis == 0 ? 4.0f : 0.0f), 2.0f);
        const float span = (axis == 0 ? groove.getWidth() : groove.getHeight()) - 16.0f;
        const float t = (float) zoomNorm(axis);
        const auto thumb = axis == 0
            ? juce::Rectangle<float>(groove.getX() + t * span, groove.getY() + 2.0f,
                                     16.0f, groove.getHeight() - 4.0f)
            : juce::Rectangle<float>(groove.getX() + 2.0f, groove.getBottom() - 16.0f - t * span,
                                     groove.getWidth() - 4.0f, 16.0f);
        const bool hot = zoomDrag_ == axis || thumb.contains(hover_.toFloat());
        g.setColour(hot ? Palette::accent : Palette::textDim);
        g.fillRoundedRectangle(thumb, 2.0f);
        for (int i = 0; i < 2; ++i) {
            const auto cap = zoomBox(axis * 2 + i);
            g.setColour(Palette::panelLight.withAlpha(cap.contains(hover_) ? 0.95f : 0.55f));
            g.fillRoundedRectangle(cap.toFloat().reduced(2.0f), 2.0f);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(i ? "+" : "-", cap, juce::Justification::centred, false);
        }
    }
}

void TracksPane::paintField(juce::Graphics& g) {
    g.fillAll(Palette::background);
    paintRuler(g);
    paintToolbar(g);
    if (mode_ == Mode::Clip) { paintClip(g); return; }
    if (mode_ == Mode::Box) paintBoxCrumb(g);
    if (mode_ == Mode::Track) {
        paintRoll(g);
        paintCutGuide(g);
        paintRollSelection(g);
        paintRollCrumb(g);
        const auto pb = beatToX(playBeat_);
        timelinechrome::paintPlayhead(g, pb, (float) rulerTop(), (float) getHeight(),
                                      (float) kStripW, (float) getWidth(), true);
        return;
    }

    if (rows_.empty()) {
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f));
        auto r = getLocalBounds().withTrimmedTop(headerH());
        g.drawText("Drop a sound file here", r.removeFromTop(r.getHeight() / 2 + 10),
                   juce::Justification::centredBottom);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("... or press + to add an audio or MIDI track",
                   r, juce::Justification::centredTop);
        if (dropHot_) paintDropHint(g);
        return;
    }
    if (dropHot_) paintDropHint(g);
    const auto cb = g.getClipBounds();
    g.saveState();
    g.reduceClipRegion(0, headerH(), getWidth(), getHeight() - headerH());
    for (const auto& s : slots_) {
        if (s.y + s.h < cb.getY() || s.y > cb.getBottom()) continue;
        if (s.kind == trackslayout::Kind::Track)          paintRow(g, s.track);
        else if (s.kind == trackslayout::Kind::PodHeader) paintPodHeader(g, s);
        else if (s.kind == trackslayout::Kind::BoxRow)    paintBoxRow(g, s);
        else                                              paintAutoLane(g, s);
    }
    g.restoreState();

    if (drag_ == Drag::ClipMarquee && !clipMarquee_.isEmpty()) {
        g.setColour(Palette::accent.withAlpha(0.14f));
        g.fillRect(clipMarquee_);
        g.setColour(Palette::accent);
        g.drawRect(clipMarquee_, 1);
    }
    paintLinePreview(g);
    paintCutGuide(g);
    paintPointSelection(g);

    timelinechrome::paintSongEnd(g, beatToX(host_.songEndBeat()), (float) rulerTop(),
                                 (float) getHeight(), (float) kStripW, (float) getWidth());

    timelinechrome::paintPlayhead(g, beatToX(playBeat_), (float) rulerTop(),
                                  (float) getHeight(), (float) kStripW,
                                  (float) getWidth(), true);

    g.setColour(Palette::border);
    g.drawVerticalLine(kStripW - 1, 0.0f, (float) getHeight());
}

}
