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
#include "gui/editor/files/SliceMapModel.h"
#include "gui/common/Localisation.h"

namespace hum {

class SliceMapBrick : public PolledBrick {
public:
    static constexpr int kRulerH = 10;
    static constexpr int kFootH = 3;
    static constexpr float kLabelMinW = 44.0f;

    SliceMapBrick(BrickHost& host, std::string organism, const Bindings& bound)
        : PolledBrick(host, organism, 2), map_(host, organism, bound(bind::kEdits)) {}

    int preferredContentWidth() const override { return 576; }
    int preferredContentHeight(int) const override { return 108; }

    void reloadValues() override { map_.pull(); repaint(); }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(Palette::background.darker(0.15f));
        g.fillRoundedRectangle(r, 5.0f);

        auto* src = map_.source();
        const auto area = waveArea().toFloat();
        const auto& starts_ = map_.starts();
        const int sel_ = map_.selected();
        if (!map_.hasAudio()) {
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
                g.drawText(map_.bench() != nullptr
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

        for (const auto& [index, held] : map_.pins()) {
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
        for (const auto& [idx, e] : map_.edits()) {
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
        if (e.mods.isPopupMenu()) { map_.select(s); showMenu(s); repaint(); return; }
        map_.press(s, e.x, e.y);
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (onRuler_ || e.mods.isPopupMenu() || e.mouseWasDraggedSinceMouseDown()) return;
        map_.audition(sliceAt(e.x));
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (onRuler_) { setStartFrom(e.x); return; }
        if (e.mods.isPopupMenu()) return;
        if (map_.drag(e.x, e.y, e.mods.isShiftDown())) repaint();
    }

private:

    int sliceAt(int px) const {
        const auto area = waveArea();
        if (map_.starts().empty() || area.getWidth() <= 0) return -1;
        return map_.sliceAt((float) (px - area.getX()) / (float) area.getWidth());
    }

    void showMenu(int s) {
        juce::PopupMenu m;
        m.addItem(1, tr("slice-map.reverse-this-slice", "Reverse this slice"), true, map_.reversed(s));
        m.addItem(2, tr("slice-map.mute-this-slice", "Mute this slice"), true, map_.muted(s));
        m.addItem(3, tr("slice-map.reset-this-slice", "Reset this slice"));
        m.addSeparator();
        m.addItem(4, tr("slice-map.clear-all-slice-edits", "Clear all slice edits"));
        if (map_.bench() != nullptr) {
            m.addSeparator();
            m.addItem(5, tr("slice-map.keep-this-slice", "Keep this slice through a re-roll"),
                      true, map_.pins().count(s) != 0);
            m.addItem(7, tr("slice-map.roll-this-slice", "Roll this slice again"));
            m.addItem(6, tr("slice-map.keep-none", "Let every slice re-roll"), !map_.pins().empty());
        }
        m.showMenuAsync(juce::PopupMenu::Options(), [this, s](int r) {
            if (r == 5 || r == 6) { if (map_.togglePin(s, r == 5)) repaint(); return; }
            if (r == 7) { if (map_.rollOne(s)) repaint(); return; }
            if (r == 1) map_.toggleReverse(s);
            else if (r == 2) map_.toggleMute(s);
            else if (r == 3) map_.reset(s);
            else if (r == 4) map_.clearAll();
            else return;
            repaint();
        });
    }


    juce::Colour originTint(int slice) const {
        if (!map_.originShown(slice)) return {};
        return originBand(map_.origin(slice));
    }

    template <class SliceX>
    void paintOrigins(juce::Graphics& g, juce::Rectangle<float> wave, SliceX sliceX) {
        if (map_.bench() == nullptr) return;
        g.setFont(juce::FontOptions(10.0f));
        for (int i = 0; i < (int) map_.starts().size(); ++i) {
            const int origin = map_.origin(i);
            if (origin < 0) continue;
            const float x0 = sliceX(i), x1 = sliceX(i + 1);
            g.setColour(originColour(origin));
            g.fillRect(x0 + 1.0f, wave.getBottom() - (float) kFootH, x1 - x0 - 1.0f, (float) kFootH);
            if (x1 - x0 < kLabelMinW) continue;
            g.setColour(Palette::text.withAlpha(alpha::strong));
            g.drawText(juce::String(map_.originName(origin)),
                       juce::Rectangle<float>(x0 + 3.0f, wave.getBottom() - kFootH - 13.0f,
                                              x1 - x0 - 5.0f, 12.0f),
                       juce::Justification::bottomLeft, true);
        }
    }

    bool showsRuler() const { return map_.showsRuler(); }
    juce::Rectangle<int> waveArea() const {
        auto area = getLocalBounds().reduced(3);
        if (showsRuler()) area.removeFromBottom(kRulerH);
        return area;
    }
    juce::Rectangle<int> rulerArea() const {
        return getLocalBounds().reduced(3).removeFromBottom(kRulerH);
    }

    void paintRuler(juce::Graphics& g, juce::Rectangle<float> wave) {
        if (!map_.showsRuler()) return;
        const auto ruler = rulerArea().toFloat();
        g.setColour(Palette::background.darker(0.35f));
        g.fillRect(ruler);
        const float start = map_.start();
        const float x = ruler.getX() + juce::jlimit(0.0f, 1.0f, start) * ruler.getWidth();
        g.setColour(ink::slice::start);
        g.drawVerticalLine((int) x, wave.getY(), ruler.getBottom());
        juce::Path flag;
        flag.addTriangle(x - 4.0f, ruler.getBottom(), x + 4.0f, ruler.getBottom(), x, ruler.getY() + 1.0f);
        g.fillPath(flag);
    }

    void setStartFrom(int px) {
        const auto ruler = rulerArea();
        if (ruler.getWidth() <= 0) return;
        if (map_.setStart((double) (px - ruler.getX()) / ruler.getWidth())) repaint();
    }

    void poll() override {
        if (map_.poll()) repaint();
    }

    files::SliceMapModel map_;
    bool onRuler_ = false;
};

}
