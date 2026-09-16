// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/video/FpsMeter.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/video/VideoPreviewStore.h"

namespace hum {

class VideoPreview : public PolledBrick {
public:
    VideoPreview(BrickHost& host, std::string organism, const Bindings& bound)
        : PolledBrick(host, std::move(organism), 2), previewParam_(bound(bind::kPreview)) {
        VideoPreviewStore::instance().want(name_, true);
    }
    ~VideoPreview() override { VideoPreviewStore::instance().want(name_, false); }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 269; }
    int preferredContentHeight(int w) const override {
        return juce::roundToInt((float) w * 9.0f / 16.0f);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (FpsMeter::clickToggles(e.getPosition(), getLocalBounds())) repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.fillAll(juce::Colours::black);
        if (frame_.isValid()) {
            g.drawImage(frame_, r.toFloat(), juce::RectanglePlacement::centred);
            meter_.paint(g, r);
        } else {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            const bool on = host_.liveOrganism(name_) != nullptr && previewOn();
            g.drawFittedText(on ? juce::String::fromUTF8("no picture yet\xe2\x80\xa6")
                                : juce::String::fromUTF8(
                                      "preview is off - tick Preview below"),
                             r.reduced(14), juce::Justification::centred, 3);
        }
        FpsMeter::paintButton(g, r);
        g.setColour(Palette::border);
        g.drawRect(r);
    }

private:
    bool previewOn() const {
        return previewParam_.empty() || host_.liveParamValue(name_, previewParam_) >= 0.5;
    }

    std::string previewParam_;
    void poll() override {
        if (host_.liveOrganism(name_) != nullptr && !previewOn()) {
            if (frame_.isValid()) {
                frame_ = juce::Image();
                meter_.reset();
                repaint();
            }
            return;
        }
        if (VideoPreviewStore::instance().take(name_, gen_, frame_)) repaint();
        if (frame_.isValid()) meter_.note(gen_);
        else meter_.reset();
    }

    juce::Image frame_;
    unsigned gen_ = 0;
    FpsMeter meter_;
};

}
