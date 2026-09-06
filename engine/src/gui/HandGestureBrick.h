#pragma once
#include <array>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "common/GestureVec.h"
#include "core/ParamSchema.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "hum/Capabilities.h"
#include "gui/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class HandGestureBrick : public juce::Component, private juce::Timer {
public:
    HandGestureBrick(EngineHost& host, std::string organism)
        : host_(host), cn_(std::move(organism)) {
        for (int k = 0; k < gvec::kSlots; ++k) {
            auto& learn = learn_[(size_t) k];
            learn.setButtonText(tr("hand-gesture.learn", "Learn"));
            learn.onClick = [this, k] { startLearn(k, false); };
            addAndMakeVisible(learn);
            auto& reinf = reinforce_[(size_t) k];
            reinf.setButtonText(tr("hand-gesture.reinf", "Reinf"));
            reinf.setTooltip(tr("hand-gesture.reinforce",
                                "Capture a variation of the same pose and fold it "
                                "into the template (running average)"));
            reinf.onClick = [this, k] { startLearn(k, true); };
            addAndMakeVisible(reinf);
            auto& clear = clear_[(size_t) k];
            clear.setButtonText(juce::String::fromUTF8("\xc3\x97"));
            clear.onClick = [this, k] { clearSlot(k); };
            addAndMakeVisible(clear);
        }
        startTimerHz(20);
    }

    void paint(juce::Graphics& g) override {
        const auto set = currentSet();
        auto* src = source();
        std::array<float, GestureFeatureSource::kMaxFeatures> cur{};
        const bool present = src != nullptr && src->gestureFeaturesLive(cur.data());
        const float tol =
            (float) host_.liveParamValue(cn_, "Tolerance");
        float matches[gvec::kSlots] = {};
        if (present)
            gvec::matchAll(set, cur.data(), std::max(0.05f, tol), matches);
        for (int k = 0; k < gvec::kSlots; ++k) {
            const auto row = rowBounds(k);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(11.5f));
            g.drawText(juce::String(k + 1), row.withWidth(14), juce::Justification::centredLeft);
            const auto bar = barBounds(k).toFloat();
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(bar, 3.0f);
            if (learning_ == k) {
                g.setColour(Palette::accent.withAlpha(0.5f));
                g.fillRoundedRectangle(bar.withWidth(bar.getWidth()
                                                     * (float) samples_ / (float) kNeed), 3.0f);
                g.setColour(Palette::text);
                g.setFont(juce::FontOptions(10.0f));
                g.drawText(src == nullptr ? "..."
                           : present ? src->gestureHoldPrompt() : src->gestureAbsentPrompt(),
                           bar.toNearestInt(), juce::Justification::centred);
            } else if (set.learned[(size_t) k]) {
                const float thr = slotThresh(k);
                const float m = matches[k];
                g.setColour(m > thr ? Palette::accent : Palette::accentDim);
                g.fillRoundedRectangle(bar.withWidth(std::max(2.0f, bar.getWidth() * m)), 3.0f);
                const float tx = bar.getX() + bar.getWidth() * thr;
                g.setColour(Palette::text);
                g.fillRect(tx - 1.0f, bar.getY() - 2.0f, 2.0f, bar.getHeight() + 4.0f);
                if (set.count[(size_t) k] > 1) {
                    g.setColour(Palette::textDim);
                    g.setFont(juce::FontOptions(9.0f));
                    g.drawText("x" + juce::String(set.count[(size_t) k]),
                               bar.toNearestInt().reduced(4, 0),
                               juce::Justification::centredRight);
                }
            } else {
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(10.0f));
                g.drawText("empty", bar.toNearestInt(), juce::Justification::centred);
            }
            g.setColour(Palette::border);
            g.drawRoundedRectangle(bar, 3.0f, 1.0f);
            const auto nr = noteRect(k);
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(nr.toFloat(), 3.0f);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(nr.toFloat(), 3.0f, 1.0f);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(10.5f));
            g.drawText(noteName(slotNote(k)), nr, juce::Justification::centred);
        }
    }

    void resized() override {
        for (int k = 0; k < gvec::kSlots; ++k) {
            auto row = rowBounds(k);
            row.removeFromLeft(16);
            learn_[(size_t) k].setBounds(row.removeFromLeft(46).reduced(0, 1));
            row.removeFromLeft(2);
            reinforce_[(size_t) k].setBounds(row.removeFromLeft(46).reduced(0, 1));
            clear_[(size_t) k].setBounds(row.removeFromRight(22).reduced(0, 1));
        }
    }

private:
    static constexpr int kRowH = 24, kGap = 4;
    static constexpr int kNeed = 30;

    juce::Rectangle<int> rowBounds(int k) const {
        return {0, k * (kRowH + kGap), getWidth(), kRowH};
    }
    juce::Rectangle<int> barBounds(int k) const {
        auto r = rowBounds(k);
        r.removeFromLeft(16 + 46 + 2 + 46 + 6);
        r.removeFromRight(22 + 6 + 38 + 4);
        return r.reduced(0, 4);
    }
    juce::Rectangle<int> noteRect(int k) const {
        auto r = rowBounds(k);
        r.removeFromRight(22 + 6);
        return r.removeFromRight(38).reduced(0, 3);
    }

    double paramOrDefault(const std::string& name, double fallback) const {
        if (const auto* cm = host_.model().byName(cn_)) {
            for (const auto& pr : cm->properties)
                if (pr.name == name) return pr.value;
            for (const auto& d : schemaFor(cm->classRaw))
                if (d.name == name) return d.def;
        }
        return fallback;
    }
    int slotNote(int k) const {
        return juce::jlimit(
            0, kMidiMax,
            (int) paramOrDefault("GNote_" + std::to_string(k + 1), 60.0 + k));
    }
    float slotThresh(int k) const {
        return (float) juce::jlimit(
            0.05, 1.0, paramOrDefault("GThresh_" + std::to_string(k + 1), 0.6));
    }
    static juce::String noteName(int note) {
        static const char* kN[] = {"C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B"};
        return juce::String(kN[note % 12]) + juce::String(note / 12 - 1);
    }

    GestureFeatureSource* source() const {
        auto* src = dynamic_cast<GestureFeatureSource*>(host_.liveOrganism(cn_));
        if (src != nullptr)
            dims_ = juce::jlimit(1, gvec::kMaxDims, src->gestureFeatureCount());
        return src;
    }

    gvec::Set currentSet() const {
        return gvec::decode(host_.liveParamText(cn_, "Gestures").c_str(), dims_);
    }

    void writeSet(const gvec::Set& g) {
        host_.setParamText(cn_, "Gestures", gvec::encode(g));
    }

    void startLearn(int slot, bool asReinforce) {
        learning_ = slot;
        reinforcing_ = asReinforce;
        samples_ = 0;
        acc_ = {};
        repaint();
    }

    void clearSlot(int slot) {
        auto g = currentSet();
        g.learned[(size_t) slot] = false;
        writeSet(g);
        repaint();
    }

    void timerCallback() override {
        if (learning_ >= 0) {
            auto* src = source();
            std::array<float, GestureFeatureSource::kMaxFeatures> cur{};
            if (src != nullptr) {
                if (src->gestureFeaturesLive(cur.data())) {
                    for (int i = 0; i < dims_; ++i) acc_[(size_t) i] += cur[(size_t) i];
                    if (++samples_ >= kNeed) {
                        std::array<float, GestureFeatureSource::kMaxFeatures> cap{};
                        for (int i = 0; i < dims_; ++i)
                            cap[(size_t) i] = acc_[(size_t) i] / (float) kNeed;
                        auto g = currentSet();
                        if (reinforcing_) {
                            gvec::reinforce(g, learning_, cap.data());
                        } else {
                            g.tpl[(size_t) learning_] = cap;
                            g.learned[(size_t) learning_] = true;
                            g.count[(size_t) learning_] = 1;
                            gvec::finalizeWeights(g);
                        }
                        writeSet(g);
                        learning_ = -1;
                    }
                }
            } else {
                learning_ = -1;
            }
        }
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        dragThr_ = dragNote_ = -1;
        for (int k = 0; k < gvec::kSlots; ++k) {
            if (barBounds(k).expanded(0, 3).contains(e.getPosition())) {
                dragThr_ = k;
                applyThr(k, e.x);
                return;
            }
            if (noteRect(k).contains(e.getPosition())) {
                dragNote_ = k;
                dragStartY_ = e.y;
                dragStartNote_ = slotNote(k);
                return;
            }
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragThr_ >= 0) applyThr(dragThr_, e.x);
        else if (dragNote_ >= 0) {
            const int next = juce::jlimit(0, kMidiMax,
                                          dragStartNote_ + (dragStartY_ - e.y) / 4);
            host_.editParam(cn_, "GNote_" + std::to_string(dragNote_ + 1), (double) next);
            repaint();
        }
    }
    void mouseUp(const juce::MouseEvent&) override { dragThr_ = dragNote_ = -1; }
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override {
        for (int k = 0; k < gvec::kSlots; ++k)
            if (noteRect(k).contains(e.getPosition())) {
                const int next = juce::jlimit(0, kMidiMax,
                                              slotNote(k) + (wheel.deltaY > 0 ? 1 : -1));
                host_.editParam(cn_, "GNote_" + std::to_string(k + 1), (double) next);
                repaint();
                return;
            }
    }

    void applyThr(int k, int px) {
        const auto bar = barBounds(k);
        const double t = juce::jlimit(
            0.05, 1.0, (double) (px - bar.getX()) / (double) juce::jmax(1, bar.getWidth()));
        host_.editParam(cn_, "GThresh_" + std::to_string(k + 1), t);
        repaint();
    }

    EngineHost& host_;
    std::string cn_;
    mutable int dims_ = 5;
    int dragThr_ = -1, dragNote_ = -1;
    int dragStartY_ = 0, dragStartNote_ = 60;
    std::array<juce::TextButton, gvec::kSlots> learn_;
    std::array<juce::TextButton, gvec::kSlots> reinforce_;
    std::array<juce::TextButton, gvec::kSlots> clear_;
    int learning_ = -1;
    bool reinforcing_ = false;
    int samples_ = 0;
    std::array<float, GestureFeatureSource::kMaxFeatures> acc_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HandGestureBrick)
};

}
