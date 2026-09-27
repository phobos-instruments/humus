// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/bricks/StrandColours.h"
#include "gui/editor/inputs/StrandsModel.h"

namespace hum {

class StrandsView : public PolledBrick, public juce::FileDragAndDropTarget {
public:
    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        if (draggingOut_) return false;
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        return files.size() == 1 && fm.findFormatForFileExtension(juce::File(files[0]).getFileExtension()) != nullptr;
    }
    void filesDropped(const juce::StringArray& files, int x, int) override {
        strands_.dropFile(strandAtX(x), juce::File(files[0]).getFullPathName().toStdString());
        dropStrand_ = -1;
        repaint();
    }
    void fileDragEnter(const juce::StringArray&, int x, int) override { dropStrand_ = strandAtX(x); repaint(); }
    void fileDragMove(const juce::StringArray&, int x, int) override {
        if (const int s = strandAtX(x); s != dropStrand_) { dropStrand_ = s; repaint(); }
    }
    void fileDragExit(const juce::StringArray&) override { dropStrand_ = -1; repaint(); }
    StrandsView(BrickHost& host, std::string organism, const Bindings& bound)
        : PolledBrick(host, organism, 2),
          strands_(host, organism, bound(bind::kFilePrefix), bound(bind::kMutePrefix),
                   bound(bind::kSoloPrefix), bound(bind::kSlicePrefix), bound(bind::kNudgePrefix)) {}

    void mouseDown(const juce::MouseEvent& e) override {
        if (const auto [s, direction] = nudgeAt(e.position); s >= 0) {
            nudged_ = s;
            strands_.setNudge(s, direction);
            repaint();
            return;
        }
        if (const int s = centreAt(e.position); s >= 0) {
            dragOut_ = s;
            return;
        }
        const int q = quarterAt(e.position);
        if (q <= 0) return;
        held_ = strandAtX((int) e.position.x);
        strands_.setSlice(held_, q);
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragOut_ >= 0 && e.getDistanceFromDragStart() > kDragStartPx) {
            const int strand = std::exchange(dragOut_, -1);
            const auto path = host_.exportAudioTake(name_, strand);
            if (!path.empty()) {
                draggingOut_ = true;
                juce::DragAndDropContainer::performExternalDragDropOfFiles(
                    {juce::String(path)}, false, this, [this] { draggingOut_ = false; });
            }
            return;
        }
        if (held_ < 0) return;
        if (const int q = quarterAt(e.position); q > 0 && strandAtX((int) e.position.x) == held_) {
            strands_.setSlice(held_, q);
            repaint();
        }
    }
    void mouseMove(const juce::MouseEvent& e) override {
        setMouseCursor(centreAt(e.position) >= 0 ? juce::MouseCursor::DraggingHandCursor
                                                 : juce::MouseCursor::NormalCursor);
        const int over = quarterAt(e.position) > 0 ? strandAtX((int) e.position.x) : -1;
        if (over == hover_) return;
        hover_ = over;
        repaint();
    }
    void mouseExit(const juce::MouseEvent&) override {
        if (hover_ < 0) return;
        hover_ = -1;
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override {
        dragOut_ = -1;
        if (nudged_ >= 0) {
            strands_.setNudge(nudged_, 0);
            nudged_ = -1;
            repaint();
        }
        if (held_ < 0) return;
        strands_.setSlice(held_, 0);
        held_ = -1;
        repaint();
    }

    int preferredContentWidth() const override { return 584; }
    int preferredContentHeight(int) const override { return 100; }

    void paint(juce::Graphics& g) override {
        const int n = strands_.count();
        if (n <= 0) return;
        const float colW = (float) getWidth() / (float) n;
        const float r = std::min(colW * 0.5f, (float) getHeight() * 0.5f) - 8.0f;

        for (int i = 0; i < n; ++i) {
            const auto strand = strands_.strand(i);
            const int state = strand.state;
            const float phase = strand.phase;
            const int layers = strand.layers;
            const bool pending = strand.pending;
            const bool audible = strand.audible;
            const float cx = colW * ((float) i + 0.5f);
            const float cy = (float) getHeight() * 0.5f;
            const juce::Rectangle<float> ring(cx - r, cy - r, r * 2.0f, r * 2.0f);

            juce::Colour c = strandStateColour(state);
            if (!audible) c = c.withAlpha(alpha::scrim);
            if (pending && strands_.blinkLit()) c = c.withAlpha(alpha::muted);

            if (state == StrandStatus::kRecord || state == StrandStatus::kDub) {
                g.setColour(c.withAlpha(alpha::scrim));
                g.fillEllipse(ring);
            }
            g.setColour(c);
            g.drawEllipse(ring, state == StrandStatus::kEmpty ? 1.0f : 2.0f);

            if (i == dropStrand_) {
                g.setColour(Palette::accent);
                g.drawEllipse(ring.expanded(3.0f), 2.0f);
            }

            paintNudges(g, i, c);
            paintSlices(g, i, ring, c);
            paintWave(g, i, ring, c, phase);
            paintInput(g, i, ring);

            if (phase >= 0.0f) {
                juce::Path arc;
                arc.addArc(ring.getX() + 3.0f, ring.getY() + 3.0f,
                           ring.getWidth() - 6.0f, ring.getHeight() - 6.0f,
                           0.0f, phase * juce::MathConstants<float>::twoPi, true);
                g.setColour(c);
                g.strokePath(arc, juce::PathStrokeType(3.0f));
            }
            if (layers > 0) {
                g.setColour(state == StrandStatus::kStopped ? Palette::textDim : Palette::text);
                g.setFont(juce::FontOptions(std::max(11.0f, r * 0.4f)));
                g.drawText(juce::String(layers), ring.toNearestInt(),
                           juce::Justification::centred, false);
            }
        }
    }

private:
    void paintWave(juce::Graphics& g, int strand, juce::Rectangle<float> ring, juce::Colour c,
                   float phase) {
        std::array<float, StrandWave::kWaveBins> wave{};
        const int bins = strands_.waveOf(strand, wave.data(), (int) wave.size());
        if (bins <= 0) return;
        const float cx = ring.getCentreX(), cy = ring.getCentreY();
        const float outer = ring.getWidth() * 0.5f - 3.0f;
        const float inner = outer * 0.42f;
        const float step = juce::MathConstants<float>::twoPi / (float) bins;
        juce::Path ahead, behind;
        for (int b = 0; b < bins; ++b) {
            const float turn = (float) b / (float) bins;
            const float a = turn * juce::MathConstants<float>::twoPi
                            - juce::MathConstants<float>::halfPi;
            const float mid = (inner + outer) * 0.5f;
            const float reach =
                (outer - inner) * 0.5f * std::sqrt(juce::jlimit(0.0f, 1.0f, wave[(size_t) b]));
            const float lo = mid - std::max(reach, 0.5f), hi = mid + std::max(reach, 0.5f);
            auto& into = (phase >= 0.0f && turn <= phase) ? behind : ahead;
            into.startNewSubPath(cx + lo * std::cos(a), cy + lo * std::sin(a));
            into.lineTo(cx + hi * std::cos(a), cy + hi * std::sin(a));
        }
        const float thick = std::max(1.0f, ring.getWidth() * step * 0.32f);
        g.setColour(c.withAlpha(alpha::muted));
        g.strokePath(ahead, juce::PathStrokeType(thick));
        g.setColour(c);
        g.strokePath(behind, juce::PathStrokeType(thick));
    }

    juce::Rectangle<float> nudgeBounds(int strand, int direction) const {
        const int n = strands_.count();
        const float colW = (float) getWidth() / (float) n;
        const float right = colW * (float) strand + colW - 4.0f;
        const float x = direction < 0 ? right - 2.0f * kNudgeW - 2.0f : right - kNudgeW;
        return {x, (float) getHeight() - 17.0f, kNudgeW, 15.0f};
    }

    bool nudgeable(int strand) const {
        return strands_.nudges() && strands_.strand(strand).state == StrandStatus::kPlay;
    }

    std::pair<int, int> nudgeAt(juce::Point<float> at) const {
        for (int i = 0; i < strands_.count(); ++i)
            for (const int direction : {-1, 1})
                if (nudgeable(i) && nudgeBounds(i, direction).contains(at)) return {i, direction};
        return {-1, 0};
    }

    void paintNudges(juce::Graphics& g, int strand, juce::Colour c) {
        if (!nudgeable(strand)) return;
        const int held = strands_.nudgeOf(strand);
        for (const int direction : {-1, 1}) {
            const auto box = nudgeBounds(strand, direction);
            g.setColour(c.withAlpha(held == direction ? alpha::mid : alpha::scrim));
            g.fillRoundedRectangle(box, 3.0f);
            const float cx = box.getCentreX(), cy = box.getCentreY(), w = 3.0f * (float) direction;
            juce::Path arrow;
            arrow.addTriangle(cx + w, cy, cx - w, cy - 3.5f, cx - w, cy + 3.5f);
            g.setColour(c);
            g.fillPath(arrow);
        }
    }

    void paintSlices(juce::Graphics& g, int strand, juce::Rectangle<float> ring, juce::Colour c) {
        if (!strands_.slices() || strands_.strand(strand).state == StrandStatus::kEmpty) return;
        const int held = strands_.sliceOf(strand);
        if (held <= 0 && strand != hover_) return;
        const float cx = ring.getCentreX(), cy = ring.getCentreY();
        const float outer = ring.getWidth() * 0.5f;
        const float inner = outer * kCentreShare;
        for (int q = 0; q < input::StrandsModel::kSlices; ++q) {
            const float a = (float) q / (float) input::StrandsModel::kSlices
                                * juce::MathConstants<float>::twoPi
                            - juce::MathConstants<float>::halfPi;
            g.setColour(c.withAlpha(alpha::scrim));
            g.drawLine(cx + inner * std::cos(a), cy + inner * std::sin(a),
                       cx + outer * std::cos(a), cy + outer * std::sin(a), 1.0f);
        }
        if (held <= 0) return;
        juce::Path wedge;
        const float step = juce::MathConstants<float>::twoPi / (float) input::StrandsModel::kSlices;
        const float from = (float) (held - 1) * step;
        wedge.addPieSegment(ring.reduced(1.0f), from, from + step, inner / outer);
        g.setColour(Palette::accent.withAlpha(alpha::muted));
        g.fillPath(wedge);
    }

    void paintInput(juce::Graphics& g, int strand, juce::Rectangle<float> ring) {
        const float level = juce::jlimit(0.0f, 1.0f, strands_.inputOf(strand));
        if (level <= 0.01f) return;
        const float grow = 3.0f + 5.0f * level;
        g.setColour(Palette::text.withAlpha(alpha::scrim + (alpha::strong - alpha::scrim) * level));
        g.drawEllipse(ring.expanded(grow), 1.0f + 2.0f * level);
    }

    void poll() override {
        if (strands_.poll()) repaint();
    }

    int strandAtX(int x) const { return strands_.strandAt(x, getWidth()); }

    int centreAt(juce::Point<float> at) const {
        const int n = strands_.count();
        if (n <= 0) return -1;
        const int strand = strandAtX((int) at.x);
        const int state = strands_.strand(strand).state;
        if (state == StrandStatus::kEmpty || state == StrandStatus::kRecord) return -1;
        const float colW = (float) getWidth() / (float) n;
        const float r = std::min(colW * 0.5f, (float) getHeight() * 0.5f) - 8.0f;
        const juce::Point<float> centre(colW * ((float) strand + 0.5f), (float) getHeight() * 0.5f);
        return at.getDistanceFrom(centre) < r * kCentreShare ? strand : -1;
    }

    int quarterAt(juce::Point<float> at) const {
        const int n = strands_.count();
        if (n <= 0 || !strands_.slices()) return 0;
        const int strand = strandAtX((int) at.x);
        if (strands_.strand(strand).state == StrandStatus::kEmpty) return 0;
        const float colW = (float) getWidth() / (float) n;
        const float r = std::min(colW * 0.5f, (float) getHeight() * 0.5f) - 8.0f;
        const juce::Point<float> centre(colW * ((float) strand + 0.5f), (float) getHeight() * 0.5f);
        const float reach = at.getDistanceFrom(centre);
        if (reach > r || reach < r * kCentreShare) return 0;
        float turn = std::atan2(at.y - centre.y, at.x - centre.x) / juce::MathConstants<float>::twoPi;
        turn += 0.25f;
        if (turn < 0.0f) turn += 1.0f;
        return 1 + juce::jlimit(0, input::StrandsModel::kSlices - 1,
                                (int) (turn * (float) input::StrandsModel::kSlices));
    }

    input::StrandsModel strands_;
    static constexpr float kNudgeW = 17.0f, kCentreShare = 0.38f;
    static constexpr int kDragStartPx = 6;
    int dropStrand_ = -1, held_ = -1, hover_ = -1, nudged_ = -1, dragOut_ = -1;
    bool draggingOut_ = false;
};

}
