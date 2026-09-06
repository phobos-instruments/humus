#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "hum/Capabilities.h"
#include "hum/SliceEdits.h"
#include "gui/Localisation.h"

namespace hum {

class SliceMapBrick : public PolledBrick {
public:
    SliceMapBrick(EngineHost& host, std::string organism)
        : PolledBrick(host, std::move(organism), 2) {}

    int preferredContentWidth() const override { return 576; }
    int preferredContentHeight(int) const override { return 108; }

    void reloadValues() override { pullEdits(); repaint(); }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(Palette::background.darker(0.15f));
        g.fillRoundedRectangle(r, 5.0f);

        auto* src = source();
        const auto area = getLocalBounds().reduced(3).toFloat();
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
                g.drawText(juce::String(tr("slice-map.load-a-loop-it-will", "Load a loop - it will be cut at its transients")),
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
        if (playing >= 0 && playing < (int) starts_.size()) {
            g.setColour(Palette::accent.withAlpha(0.18f));
            g.fillRect(sliceX(playing), area.getY(), sliceX(playing + 1) - sliceX(playing),
                       area.getHeight());
        }
        if (sel_ >= 0 && sel_ < (int) starts_.size()) {
            g.setColour(Palette::accent.withAlpha(0.14f));
            g.fillRect(sliceX(sel_), area.getY(), sliceX(sel_ + 1) - sliceX(sel_), area.getHeight());
        }

        const auto& peaks = src->slicePeaks();
        g.setColour(Palette::text.withAlpha(0.55f));
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

        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        const int s = sliceAt(e.x);
        if (s < 0) return;
        sel_ = s;
        if (e.mods.isPopupMenu()) { showMenu(s); repaint(); return; }
        dragPitch0_ = edits_[s].pitch;
        dragY0_ = e.y;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (sel_ < 0 || e.mods.isPopupMenu()) return;
        const int p = juce::jlimit(-24, 24, dragPitch0_ + (dragY0_ - e.y) / 6);
        auto& ed = edits_[sel_];
        if (ed.pitch == p) return;
        ed.pitch = p;
        pushEdits();
        repaint();
    }

private:
    SliceSource* source() const { return dynamic_cast<SliceSource*>(host_.liveOrganism(name_)); }

    int sliceAt(int px) const {
        const auto area = getLocalBounds().reduced(3);
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
        m.showMenuAsync(juce::PopupMenu::Options(), [this, s](int r) {
            if (r == 1) edits_[s].reverse = !edits_[s].reverse;
            else if (r == 2) edits_[s].gainPct = edits_[s].gainPct == 0 ? 100 : 0;
            else if (r == 3) edits_.erase(s);
            else if (r == 4) edits_.clear();
            else return;
            pushEdits();
            repaint();
        });
    }

    void pullEdits() {
        edits_ = parseSliceEdits(host_.liveParamText(name_, "SliceEdits"));
    }
    void pushEdits() {
        for (auto it = edits_.begin(); it != edits_.end();)
            it = it->second.isDefault() ? edits_.erase(it) : std::next(it);
        host_.setParamText(name_, "SliceEdits", encodeSliceEdits(edits_));
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
    int sel_ = -1, dragPitch0_ = 0, dragY0_ = 0;
    unsigned lastGen_ = ~0u;
    int lastPlaying_ = -2;
};

}
