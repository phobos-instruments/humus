#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/Mappable.h"
#include "gui/OrganismEditor.h"

namespace hum {

class MomentaryButton : public juce::TextButton, private juce::Timer {
public:
    MomentaryButton(EngineHost& host, std::string organism, std::string param,
                    const juce::String& caption, bool holdable = false)
        : juce::TextButton(caption), host_(host),
          name_(std::move(organism)), param_(std::move(param)), holdable_(holdable) {
        setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        setColour(juce::TextButton::textColourOffId, Palette::text);
        setColour(juce::TextButton::textColourOnId, Palette::background);
        setTriggeredOnMouseDown(true);
        if (!holdable_) onClick = [this] { hit(); };
    }
    ~MomentaryButton() override { stopTimer(); }

    void mouseDown(const juce::MouseEvent& e) override {
        juce::TextButton::mouseDown(e);
        if (!holdable_ || e.mods.isPopupMenu()) return;
        host_.setParam(name_, param_, 1.0);
        up_ = true;
        frames_ = coverTicks();
        release_ = false;
        startTimerHz(60);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        juce::TextButton::mouseUp(e);
        if (!holdable_ || e.mods.isPopupMenu() || !up_) return;
        release_ = true;
    }

private:
    int coverTicks() const {
        double ms = 25.0;
        if (auto* d = host_.audioDevices().getCurrentAudioDevice()) {
            const double sr = d->getCurrentSampleRate();
            if (sr > 0)
                ms = juce::jmax(25.0, 2000.0 * d->getCurrentBufferSizeSamples() / sr);
        }
        return juce::jmax(2, (int) std::ceil(ms / 16.7));
    }

    void hit() {
        if (up_) {
            host_.setParam(name_, param_, 0.0);
            up_ = false;
            rearm_ = coverTicks();
        } else {
            host_.setParam(name_, param_, 1.0);
            up_ = true;
            frames_ = coverTicks();
            rearm_ = 0;
        }
        startTimerHz(60);
    }

    void timerCallback() override {
        if (holdable_) {
            if (frames_ > 0) --frames_;
            if (release_ && frames_ == 0) {
                host_.setParam(name_, param_, 0.0);
                up_ = false;
                release_ = false;
                stopTimer();
            }
            return;
        }
        if (rearm_ > 0) {
            if (--rearm_ == 0) {
                host_.setParam(name_, param_, 1.0);
                up_ = true;
                frames_ = coverTicks();
            }
            return;
        }
        if (frames_ > 0 && --frames_ == 0) {
            host_.setParam(name_, param_, 0.0);
            up_ = false;
            stopTimer();
        }
    }

    EngineHost& host_;
    std::string name_, param_;
    bool holdable_ = false;
    bool up_ = false, release_ = false;
    int frames_ = 0, rearm_ = 0;
};

class LooperTrackStrip : public juce::Component, private juce::Timer {
public:
    LooperTrackStrip(EngineHost& host, std::string organism, std::string prefix, int count)
        : host_(host), name_(std::move(organism)), prefix_(std::move(prefix)) {
        for (int i = 0; i < count; ++i) {
            auto t = std::make_unique<Mappable<juce::ToggleButton>>(juce::String(i + 1));
            t->setColour(juce::ToggleButton::textColourId, Palette::text);
            t->setColour(juce::ToggleButton::tickColourId, Palette::accent);
            const int idx = i;
            t->onClick = [this, idx] {
                host_.setParam(name_, prefix_ + std::to_string(idx + 1),
                               toggles_[(size_t) idx]->getToggleState() ? 1.0 : 0.0);
            };
            t->onRightClick = [this, idx](juce::Point<int> pos) {
                showAutomateMenu(host_, name_, prefix_ + std::to_string(idx + 1), pos, nullptr);
            };
            addAndMakeVisible(*t);
            toggles_.push_back(std::move(t));
        }
        reload();
        startTimerHz(15);
    }
    ~LooperTrackStrip() override { stopTimer(); }

    void reload() {
        for (size_t i = 0; i < toggles_.size(); ++i)
            toggles_[i]->setToggleState(
                host_.liveParamValue(name_, prefix_ + std::to_string(i + 1)) >= 0.5,
                juce::dontSendNotification);
    }

    void resized() override {
        auto r = getLocalBounds();
        const int rows = std::max(1, ((int) toggles_.size() + 7) / 8);
        const int rowH = r.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            auto rr = r.removeFromTop(rowH);
            const int tw = rr.getWidth() / 8;
            for (int col = 0; col < 8; ++col) {
                const size_t idx = (size_t) (row * 8 + col);
                if (idx < toggles_.size()) toggles_[idx]->setBounds(rr.removeFromLeft(tw));
            }
        }
    }

private:
    void timerCallback() override {
        int rec = -1, armed = -1;
        host_.files().liveLooperTracks(name_, rec, armed);
        if (rec == lastRec_ && armed == lastArmed_) return;
        lastRec_ = rec; lastArmed_ = armed;
        for (int i = 0; i < (int) toggles_.size(); ++i) {
            const juce::Colour c = (i == rec)   ? juce::Colour(0xffe23b3b)
                                 : (i == armed) ? Palette::accent
                                                : Palette::text;
            toggles_[(size_t) i]->setColour(juce::ToggleButton::textColourId, c);
            toggles_[(size_t) i]->repaint();
        }
    }

    EngineHost& host_;
    std::string name_, prefix_;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles_;
    int lastRec_ = -2, lastArmed_ = -2;
};

}
