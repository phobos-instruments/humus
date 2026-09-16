// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/style/OriginColours.h"
#include "gui/bricks/PolledBrick.h"
#include "hum/caps/Samples.h"
#include "hum/SliceEdits.h"
#include "gui/common/Localisation.h"

namespace hum {

class SliceMapBrick : public PolledBrick {
public:
    static constexpr int kRulerH = 10;
    static constexpr int kNudgePx = 14;
    static constexpr int kAxisPx = 4;
    static constexpr int kFootH = 3;
    static constexpr float kLabelMinW = 44.0f;

    SliceMapBrick(BrickHost& host, std::string organism, const Bindings& bound)
        : PolledBrick(host, std::move(organism), 2), editsParam_(bound(bind::kEdits)) {}

    int preferredContentWidth() const override { return 576; }
    int preferredContentHeight(int) const override { return 108; }

    void reloadValues() override { pullEdits(); repaint(); }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(Palette::background.darker(0.15f));
        g.fillRoundedRectangle(r, 5.0f);

        auto* src = source();
        const auto area = waveArea().toFloat();
        if (src == nullptr || src->slicePeaks().empty()
            || *std::max_element(src->slicePeaks().begin(), src->slicePeaks().end()) <= 0.0f) {
            if (!starts_.empty()) {
                g.setColour(Palette::accentDim);
                for (float s : starts_)
                    g.drawVerticalLine((int) (area.getX() + s * area.getWidth()),
                                       area.getY(), area.getBottom());
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(12.0f));
                g.drawText(tr("slice-map.no-audio-in-this-file",
                              "Slice map loaded - no audio in this file. "
                              "Put a sound-file bounce with the same name beside it."),
                           getLocalBounds(), juce::Justification::centred);
            } else {
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(12.0f));
                g.drawText(bench() != nullptr
                               ? tr("slice-map.drop-sources",
                                    "Drop sound files in the slots above - they get stitched into one")
                               : juce::String(tr("slice-map.load-a-loop-it-will",
                                                 "Load a loop - it will be cut at its transients")),
                           getLocalBounds(), juce::Justification::centred);
            }
            g.setColour(Palette::border);
            g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
            return;
        }

        const int playing = src->playingSlice();
        auto sliceX = [&](int i) {
            return i < (int) starts_.size() ? area.getX() + starts_[(size_t) i] * area.getWidth()
                                            : area.getRight();
        };
        for (int i = 0; i < (int) starts_.size(); ++i) {
            const juce::Colour tint = originTint(i);
            if (tint.isTransparent()) continue;
            g.setColour(i == sel_ ? tint.brighter(0.5f) : tint);
            g.fillRect(sliceX(i), area.getY(), sliceX(i + 1) - sliceX(i), area.getHeight());
        }
        if (playing >= 0 && playing < (int) starts_.size()) {
            g.setColour(Palette::accent.withAlpha(alpha::veil));
            g.fillRect(sliceX(playing), area.getY(), sliceX(playing + 1) - sliceX(playing),
                       area.getHeight());
        }
        if (sel_ >= 0 && sel_ < (int) starts_.size()) {
            g.setColour(Palette::accent.withAlpha(alpha::mist));
            g.fillRect(sliceX(sel_), area.getY(), sliceX(sel_ + 1) - sliceX(sel_), area.getHeight());
        }

        for (const auto& [index, held] : pins_) {
            if (index < 0 || index >= (int) starts_.size()) continue;
            g.setColour(Palette::accent);
            g.fillRect(sliceX(index), area.getY(), sliceX(index + 1) - sliceX(index), 2.0f);
        }

        const auto& peaks = src->slicePeaks();
        g.setColour(Palette::text.withAlpha(alpha::mid));
        const float mid = area.getCentreY();
        const int n = (int) peaks.size();
        for (int i = 0; i < n; ++i) {
            const float h = peaks[(size_t) i] * area.getHeight() * 0.48f;
            const float x = area.getX() + area.getWidth() * (float) i / (float) n;
            g.drawVerticalLine((int) x, mid - h, mid + h + 1.0f);
        }

        for (int i = 0; i < (int) starts_.size(); ++i) {
            g.setColour(i == sel_ ? Palette::accent : Palette::accentDim);
            g.drawVerticalLine((int) sliceX(i), area.getY(), area.getBottom());
        }

        g.setFont(juce::FontOptions(10.0f));
        for (const auto& [idx, e] : edits_) {
            if (idx >= (int) starts_.size()) continue;
            juce::String tag;
            if (e.pitch != 0) tag << (e.pitch > 0 ? "+" : "") << e.pitch;
            if (e.reverse) tag << (tag.isEmpty() ? "" : " ") << juce::String::fromUTF8("\xe2\x86\xba");
            if (e.gainPct == 0) tag = "mute";
            if (tag.isEmpty()) continue;
            g.setColour(Palette::accent);
            g.drawText(tag, juce::Rectangle<float>(sliceX(idx) + 2.0f, area.getY() + 1.0f,
                                                   sliceX(idx + 1) - sliceX(idx) - 3.0f, 12.0f),
                       juce::Justification::topLeft, false);
        }

        paintOrigins(g, area, sliceX);
        paintRuler(g, area);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        onRuler_ = showsRuler() && rulerArea().contains(e.getPosition());
        if (onRuler_) { setStartFrom(e.x); return; }
        const int s = sliceAt(e.x);
        if (s < 0) return;
        sel_ = s;
        if (e.mods.isPopupMenu()) { showMenu(s); repaint(); return; }
        dragPitch0_ = edits_[s].pitch;
        dragY0_ = e.y;
        dragX0_ = e.x;
        axis_ = Axis::Undecided;
        nudged_ = 0;
        auto* w = bench();
        nudgeFrom_ = w != nullptr ? w->pinValue(s) : std::string();
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (onRuler_ || e.mods.isPopupMenu() || e.mouseWasDraggedSinceMouseDown()) return;
        auto* w = bench();
        const int s = sliceAt(e.x);
        if (w != nullptr && s >= 0) w->auditionSlice(s);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (onRuler_) { setStartFrom(e.x); return; }
        if (sel_ < 0 || e.mods.isPopupMenu()) return;
        if (axis_ == Axis::Undecided) {
            const int dx = std::abs(e.x - dragX0_), dy = std::abs(e.y - dragY0_);
            if (std::max(dx, dy) < kAxisPx) return;
            axis_ = dx > dy && !nudgeFrom_.empty() ? Axis::Across : Axis::Up;
        }
        if (axis_ == Axis::Across) { nudgeTo((e.x - dragX0_) / kNudgePx, e.mods.isShiftDown()); return; }
        const int p = juce::jlimit(-24, 24, dragPitch0_ + (dragY0_ - e.y) / 6);
        auto& ed = edits_[sel_];
        if (ed.pitch == p) return;
        ed.pitch = p;
        pushEdits();
        repaint();
    }

private:
    std::string editsParam_;
    SliceSource* source() const { return dynamic_cast<SliceSource*>(host_.liveOrganism(name_)); }

    int sliceAt(int px) const {
        const auto area = waveArea();
        if (starts_.empty() || area.getWidth() <= 0) return -1;
        const float f = (float) (px - area.getX()) / (float) area.getWidth();
        int s = -1;
        for (int i = 0; i < (int) starts_.size(); ++i) if (starts_[(size_t) i] <= f) s = i;
        return s;
    }

    void showMenu(int s) {
        juce::PopupMenu m;
        m.addItem(1, tr("slice-map.reverse-this-slice", "Reverse this slice"), true, edits_[s].reverse);
        m.addItem(2, tr("slice-map.mute-this-slice", "Mute this slice"), true, edits_[s].gainPct == 0);
        m.addItem(3, tr("slice-map.reset-this-slice", "Reset this slice"));
        m.addSeparator();
        m.addItem(4, tr("slice-map.clear-all-slice-edits", "Clear all slice edits"));
        if (bench() != nullptr) {
            m.addSeparator();
            m.addItem(5, tr("slice-map.keep-this-slice", "Keep this slice through a re-roll"),
                      true, pins_.count(s) != 0);
            m.addItem(7, tr("slice-map.roll-this-slice", "Roll this slice again"));
            m.addItem(6, tr("slice-map.keep-none", "Let every slice re-roll"), !pins_.empty());
        }
        m.showMenuAsync(juce::PopupMenu::Options(), [this, s](int r) {
            if (r == 5 || r == 6) { togglePin(s, r == 5); return; }
            if (r == 7) { rollOne(s); return; }
            if (r == 1) edits_[s].reverse = !edits_[s].reverse;
            else if (r == 2) edits_[s].gainPct = edits_[s].gainPct == 0 ? 100 : 0;
            else if (r == 3) edits_.erase(s);
            else if (r == 4) edits_.clear();
            else return;
            pushEdits();
            repaint();
        });
    }

    SliceWorkbench* bench() const {
        return dynamic_cast<SliceWorkbench*>(host_.liveOrganism(name_));
    }

    juce::Colour originTint(int slice) const {
        auto* w = bench();
        if (w == nullptr) return {};
        const int origin = w->sliceOrigin(slice);
        if (origin < 0 || origin >= w->originCount()) return {};
        return originBand(origin);
    }

    template <class SliceX>
    void paintOrigins(juce::Graphics& g, juce::Rectangle<float> wave, SliceX sliceX) {
        auto* w = bench();
        if (w == nullptr) return;
        g.setFont(juce::FontOptions(10.0f));
        for (int i = 0; i < (int) starts_.size(); ++i) {
            const int origin = w->sliceOrigin(i);
            if (origin < 0) continue;
            const float x0 = sliceX(i), x1 = sliceX(i + 1);
            g.setColour(originColour(origin));
            g.fillRect(x0 + 1.0f, wave.getBottom() - (float) kFootH, x1 - x0 - 1.0f, (float) kFootH);
            if (x1 - x0 < kLabelMinW) continue;
            g.setColour(Palette::text.withAlpha(alpha::strong));
            g.drawText(juce::String(juce::CharPointer_UTF8(w->originName(origin).c_str())),
                       juce::Rectangle<float>(x0 + 3.0f, wave.getBottom() - kFootH - 13.0f,
                                              x1 - x0 - 5.0f, 12.0f),
                       juce::Justification::bottomLeft, true);
        }
    }

    bool showsRuler() const {
        auto* w = bench();
        return w != nullptr && !w->startParam().empty();
    }
    juce::Rectangle<int> waveArea() const {
        auto area = getLocalBounds().reduced(3);
        if (showsRuler()) area.removeFromBottom(kRulerH);
        return area;
    }
    juce::Rectangle<int> rulerArea() const {
        return getLocalBounds().reduced(3).removeFromBottom(kRulerH);
    }

    void paintRuler(juce::Graphics& g, juce::Rectangle<float> wave) {
        auto* w = bench();
        if (w == nullptr || w->startParam().empty()) return;
        const auto ruler = rulerArea().toFloat();
        g.setColour(Palette::background.darker(0.35f));
        g.fillRect(ruler);
        const float start = (float) host_.liveParamValue(name_, w->startParam());
        const float x = ruler.getX() + juce::jlimit(0.0f, 1.0f, start) * ruler.getWidth();
        g.setColour(ink::slice::start);
        g.drawVerticalLine((int) x, wave.getY(), ruler.getBottom());
        juce::Path flag;
        flag.addTriangle(x - 4.0f, ruler.getBottom(), x + 4.0f, ruler.getBottom(), x, ruler.getY() + 1.0f);
        g.fillPath(flag);
    }

    void setStartFrom(int px) {
        auto* w = bench();
        const auto ruler = rulerArea();
        if (w == nullptr || ruler.getWidth() <= 0) return;
        const double at = juce::jlimit(0.0, 0.999, (double) (px - ruler.getX()) / ruler.getWidth());
        host_.setParam(name_, w->startParam(), at);
        repaint();
    }

    void nudgeTo(int steps, bool bySource) {
        auto* w = bench();
        if (w == nullptr || steps == nudged_) return;
        nudged_ = steps;
        const std::string value = w->pinNudged(nudgeFrom_, bySource ? steps : 0, bySource ? 0 : steps);
        if (value.empty()) return;
        pins_[sel_] = value;
        host_.setParamText(name_, w->pinParam(), encodeSlicePins(pins_));
        w->auditionSlice(sel_);
        repaint();
    }

    void togglePin(int s, bool one) {
        auto* w = bench();
        if (w == nullptr) return;
        if (!one) pins_.clear();
        else if (pins_.erase(s) == 0) {
            const std::string value = w->pinValue(s);
            if (value.empty()) return;
            pins_[s] = value;
        }
        host_.setParamText(name_, w->pinParam(), encodeSlicePins(pins_));
        repaint();
    }

    void rollOne(int s) {
        auto* w = bench();
        if (w == nullptr) return;
        const std::string value = w->pinAlternative(s);
        if (value.empty()) return;
        pins_[s] = value;
        host_.setParamText(name_, w->pinParam(), encodeSlicePins(pins_));
        repaint();
    }

    void pullEdits() {
        edits_ = parseSliceEdits(host_.liveParamText(name_, editsParam_));
        if (auto* w = bench()) pins_ = parseSlicePins(host_.liveParamText(name_, w->pinParam()));
    }
    void pushEdits() {
        for (auto it = edits_.begin(); it != edits_.end();)
            it = it->second.isDefault() ? edits_.erase(it) : std::next(it);
        host_.setParamText(name_, editsParam_, encodeSliceEdits(edits_));
    }

    void poll() override {
        auto* src = source();
        if (src == nullptr) return;
        const unsigned gen = src->sliceGeneration();
        const int playing = src->playingSlice();
        auto starts = src->sliceStarts();
        if (gen == lastGen_ && playing == lastPlaying_ && starts == starts_) return;
        lastGen_ = gen;
        lastPlaying_ = playing;
        starts_ = std::move(starts);
        repaint();
    }

    std::vector<float> starts_;
    std::map<int, SliceEdit> edits_;
    std::map<int, std::string> pins_;
    enum class Axis { Undecided, Up, Across };
    Axis axis_ = Axis::Undecided;
    bool onRuler_ = false;
    int dragX0_ = 0;
    int nudged_ = 0;
    std::string nudgeFrom_;
    int sel_ = -1, dragPitch0_ = 0, dragY0_ = 0;
    unsigned lastGen_ = ~0u;
    int lastPlaying_ = -2;
};

}
