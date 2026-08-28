#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Categories.h"
#include "core/Randomize.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/StepPlayhead.h"
#include "gui/UiTicker.h"

namespace hum {

class PatternStepGrid : public juce::Component {
public:
    enum class Mode { Bassline, Arp };

    PatternStepGrid(EngineHost& host, std::string name, Mode mode)
        : host_(host), name_(std::move(name)), mode_(mode) {
        tickerId_ = UiTicker::instance().add([this] { pollPlayhead(); });
    }
    ~PatternStepGrid() override { UiTicker::instance().remove(tickerId_); }

    void reload() { repaint(); }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel.darker(0.06f));
        if (mode_ == Mode::Bassline) paintBassline(g);
        else                         paintArp(g);
        paintPlayhead(g);
    }

    int playheadStep() const {
        const auto* cm = host_.model().byName(name_);
        return cm == nullptr ? -1
                             : stepPlayhead(host_.isPlaying(), host_.positionBeats(),
                                            cm->pattern.matrixResolution, stepCount());
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (mode_ == Mode::Arp && e.mods.isRightButtonDown()) { stampMenu(); return; }
        host_.pushUndo();
        apply(e, true);
    }
    void mouseDrag(const juce::MouseEvent& e) override { apply(e, false); }

private:
    static constexpr int kMinNote = 36, kMaxNote = 84;
    static constexpr int kRowH = 16;

    int stepCount() const {
        return mode_ == Mode::Bassline ? (int) host_.patterns().basslineSteps(name_).size()
                                       : (int) host_.patterns().arpSteps(name_).size();
    }
    int colAt(int x) const {
        const int n = std::max(1, stepCount());
        return std::clamp(x * n / std::max(1, getWidth()), 0, n - 1);
    }

    void paintBassline(juce::Graphics& g) {
        auto steps = host_.patterns().basslineSteps(name_);
        const int n = std::max(1, (int) steps.size());
        const float cw = getWidth() / (float) n;
        const int laneBottom = getHeight() - 2 * kRowH;
        const float range = (float) (kMaxNote - kMinNote);
        const auto fam = familyColour();
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            if (i % 4 == 0) { g.setColour(Palette::panelLight.withAlpha(0.5f)); g.fillRect(x, 0.0f, cw, (float) getHeight()); }
            g.setColour(Palette::background.withAlpha(0.6f));
            g.drawVerticalLine((int) x, 0.0f, (float) getHeight());
            const auto& s = steps[(size_t) i];
            if (s.gate) {
                const float ny = laneBottom * (1.0f - (std::clamp(s.note, kMinNote, kMaxNote) - kMinNote) / range);
                const juce::Rectangle<float> bar(x + 1, ny - 3, cw - 2, 6.0f);
                if (s.accent) {
                    g.setColour(fam.withAlpha(0.25f));
                    g.fillRoundedRectangle(bar.expanded(2.0f, 2.5f), 4.0f);
                    g.setColour(fam.brighter(0.3f));
                } else {
                    g.setColour(Palette::text);
                }
                g.fillRoundedRectangle(bar, 2.5f);
            }
            auto cell = [&](int row, bool on, const char* lbl) {
                juce::Rectangle<float> r(x + 1, (float) (laneBottom + row * kRowH) + 1, cw - 2, (float) kRowH - 2);
                if (on) sporeCap(g, r, fam);
                else    soilCell(g, r);
                g.setColour(on ? Palette::background : Palette::textDim);
                g.setFont(10.0f); g.drawText(lbl, r, juce::Justification::centred);
            };
            cell(0, s.accent, "A");
            cell(1, s.slide, "S");
        }
        g.setColour(Palette::background.withAlpha(0.6f));
        g.drawHorizontalLine(laneBottom, 0.0f, (float) getWidth());
    }

    void applyBassline(const juce::MouseEvent& e, bool down) {
        auto steps = host_.patterns().basslineSteps(name_);
        if (steps.empty()) return;
        const int i = colAt(e.x);
        auto s = steps[(size_t) i];
        const int laneBottom = getHeight() - 2 * kRowH;
        if (e.y < laneBottom) {
            if (down && e.mods.isRightButtonDown()) { s.gate = !s.gate; }
            else {
                const float t = std::clamp(1.0f - e.y / (float) laneBottom, 0.0f, 1.0f);
                s.note = kMinNote + (int) std::lround(t * (kMaxNote - kMinNote));
                s.gate = true;
            }
        } else if (down) {
            const int row = (e.y - laneBottom) / kRowH;
            if (row == 0) s.accent = !s.accent; else s.slide = !s.slide;
        } else return;
        host_.patterns().setBasslineStep(name_, i, s);
        repaint();
    }

    void sporeCap(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fam) const {
        g.setColour(fam.withAlpha(0.16f));
        g.fillRoundedRectangle(r.expanded(1.6f), 3.5f);
        juce::ColourGradient glow(fam.brighter(0.35f), r.getCentreX(), r.getY() + r.getHeight() * 0.22f,
                                  fam.darker(0.28f), r.getCentreX(), r.getBottom(), false);
        g.setGradientFill(glow);
        g.fillRoundedRectangle(r, 2.5f);
        g.setColour(fam.brighter(0.6f).withAlpha(0.75f));
        g.drawRoundedRectangle(r.reduced(0.4f), 2.5f, 0.9f);
    }

    void soilCell(juce::Graphics& g, juce::Rectangle<float> r) const {
        g.setColour(Palette::background.darker(0.12f));
        g.fillRoundedRectangle(r, 2.5f);
        juce::ColourGradient lip(juce::Colours::black.withAlpha(0.35f), 0.0f, r.getY(),
                                 juce::Colours::transparentBlack, 0.0f, r.getY() + 3.5f, false);
        g.setGradientFill(lip);
        g.fillRoundedRectangle(r, 2.5f);
    }

    juce::Colour familyColour() const {
        const auto* cm = host_.model().byName(name_);
        return Palette::familyAccent(cm ? familyOf(cm->displayClass) : Family::Voice);
    }

    void paintArp(juce::Graphics& g) {
        auto steps = host_.patterns().arpSteps(name_);
        auto ups = host_.patterns().arpUps(name_);
        const int n = std::max(1, (int) steps.size());
        const float cw = getWidth() / (float) n;
        const int upTop = getHeight() - 2 * kRowH;
        const int tieTop = getHeight() - kRowH;
        const auto fam = familyColour();
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            if (i % 4 == 0) { g.setColour(Palette::panelLight.withAlpha(0.5f)); g.fillRect(x, 0.0f, cw, (float) getHeight()); }
            const auto& s = steps[(size_t) i];
            juce::Rectangle<float> cell(x + 2, 4.0f, cw - 4, (float) (upTop - 8));
            if (s.trigger) sporeCap(g, cell, fam);
            else           soilCell(g, cell);
        }
        g.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            const bool up = (size_t) i < ups.size() && ups[(size_t) i];
            const auto socket = juce::Rectangle<float>(x + 1, (float) upTop + 1,
                                                       cw - 2, (float) kRowH - 2)
                                    .reduced(cw * 0.28f, 4.0f);
            if (up) sporeCap(g, socket, fam);
            else    soilCell(g, socket);
            g.setColour(up ? Palette::background : Palette::textDim.withAlpha(0.55f));
            g.drawText("^", socket.expanded(2.0f, 3.0f), juce::Justification::centred);
        }
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            const auto& s = steps[(size_t) i];
            const auto socket = juce::Rectangle<float>(x + 1, (float) tieTop + 1,
                                                       cw - 2, (float) kRowH - 2)
                                    .reduced(cw * 0.28f, 4.0f);
            if (s.tie) sporeCap(g, socket, fam);
            else       soilCell(g, socket);
        }
        g.setColour(Palette::background.withAlpha(0.6f));
        g.drawHorizontalLine(tieTop, 0.0f, (float) getWidth());
    }

    void applyArp(const juce::MouseEvent& e, bool down) {
        if (!down) return;
        auto steps = host_.patterns().arpSteps(name_);
        if (steps.empty()) return;
        const int i = colAt(e.x);
        if (e.y >= getHeight() - 2 * kRowH && e.y < getHeight() - kRowH) {
            const auto ups = host_.patterns().arpUps(name_);
            host_.patterns().setArpUp(name_, i,
                                      !((size_t) i < ups.size() && ups[(size_t) i]));
            repaint();
            return;
        }
        auto s = steps[(size_t) i];
        if (e.y >= getHeight() - kRowH) s.tie = !s.tie;
        else                            s.trigger = !s.trigger;
        host_.patterns().setArpStep(name_, i, s);
        repaint();
    }

    void stampMenu() {
        juce::PopupMenu m;
        m.addItem(1, "Roll -111 (offbeat 16ths)");
        m.addItem(2, "Offbeat --1- (8ths)");
        m.addItem(3, "Full 1111");
        m.addItem(5, "Random");
        m.addItem(4, "Clear");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                        [this](int r) {
            if (r == 0) return;
            auto steps = host_.patterns().arpSteps(name_);
            const auto ups = host_.patterns().arpUps(name_);
            const int n = (int) steps.size();
            auto& rng = juce::Random::getSystemRandom();
            const auto rolled = randomTriggerRow(n, 0.35 + rng.nextDouble() * 0.35, rng);
            host_.pushUndo();
            for (int i = 0; i < n; ++i) {
                auto s = steps[(size_t) i];
                s.trigger = r == 1 ? (i % 4) != 0
                          : r == 2 ? (i % 4) == 2
                          : r == 3 ? true
                          : r == 5 && rolled[(size_t) i];
                s.tie = r == 5 && rng.nextDouble() < 0.12;
                host_.patterns().setArpStep(name_, i, s);
                const bool haveUp = (size_t) i < ups.size() && ups[(size_t) i];
                const bool wantUp = r == 5 && s.trigger && rng.nextDouble() < 0.18;
                if (wantUp != haveUp) host_.patterns().setArpUp(name_, i, wantUp);
            }
            repaint();
        });
    }

    void apply(const juce::MouseEvent& e, bool down) {
        if (mode_ == Mode::Bassline) applyBassline(e, down);
        else                         applyArp(e, down);
    }

    juce::Rectangle<int> columnRect(int col) const {
        const int n = std::max(1, stepCount());
        const float cw = getWidth() / (float) n;
        return juce::Rectangle<float>(col * cw, 0.0f, cw, (float) getHeight()).getSmallestIntegerContainer();
    }
    void paintPlayhead(juce::Graphics& g) {
        if (playhead_ < 0 || playhead_ >= stepCount()) return;
        const auto col = columnRect(playhead_).toFloat();
        const auto fam = familyColour();
        g.setColour(fam.withAlpha(0.14f));
        g.fillRect(col);
        g.setColour(fam.brighter(0.5f).withAlpha(0.9f));
        g.fillRect(col.withHeight(2.0f));
    }
    void pollPlayhead() {
        const int cur = playheadStep();
        if (cur == playhead_) return;
        if (playhead_ >= 0) repaint(columnRect(playhead_));
        playhead_ = cur;
        if (playhead_ >= 0) repaint(columnRect(playhead_));
    }

    EngineHost& host_;
    std::string name_;
    Mode mode_;
    int tickerId_ = 0;
    int playhead_ = -1;
};

}
