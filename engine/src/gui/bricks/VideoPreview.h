// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/video/FpsMeter.h"
#include "gui/style/StatusDot.h"
#include "gui/video/VideoTakeSink.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/video/VideoPreviewStore.h"
#include "gui/editor/video/VideoModels.h"

namespace hum {

class VideoPreview : public PolledBrick {
public:
    VideoPreview(BrickHost& host, std::string organism, const Bindings& bound)
        : PolledBrick(host, organism, 2), preview_(host, organism, bound(bind::kPreview)) {
        VideoPreviewStore::instance().want(name_, true);
        meter_.watches(name_);
    }
    ~VideoPreview() override { VideoPreviewStore::instance().want(name_, false); }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 269; }
    int preferredContentHeight(int w) const override {
        if (frame_.isValid() && frame_.getWidth() > 0)
            return juce::roundToInt((float) w * (float) frame_.getHeight()
                                    / (float) frame_.getWidth());
        return juce::roundToInt((float) w * 9.0f / 16.0f);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (meter_.clickToggles(e.getPosition(), getLocalBounds())) repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.fillAll(juce::Colours::black);
        if (frame_.isValid()) {
            g.drawImage(frame_, r.toFloat(),
                        juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
            meter_.paint(g, r);
        } else {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            const bool on = preview_.waitingForPicture();
            g.drawFittedText(on ? juce::String::fromUTF8("no picture yet\xe2\x80\xa6")
                                : juce::String::fromUTF8(
                                      "preview is off - tick Preview below"),
                             r.reduced(14), juce::Justification::centred, 3);
        }
        meter_.paintButton(g, r);
        if (showsARecording()) {
            const auto dot = StatusDot::rec();
            dot.paintIn(g, r);
        }
        g.setColour(Palette::border);
        g.drawRect(r);
    }

private:
    static constexpr int kUpstreamHops = 8;

    bool showsARecording() const {
        auto& takes = VideoTakeStore::instance();
        if (!takes.anyOpen()) return false;
        std::string at = name_;
        for (int hop = 0; hop < kUpstreamHops; ++hop) {
            if (takes.isOpen(at)) return true;
            std::string next;
            for (const auto& c : host_.model().videoConnections)
                if (c.dst == at && c.dstInlet == 0) { next = c.src; break; }
            if (next.empty() || next == at) break;
            at = next;
        }
        return false;
    }

    void poll() override {
        if (preview_.shouldDropFrame()) {
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

    video::VideoPreviewModel preview_;
    juce::Image frame_;
    unsigned gen_ = 0;
    FpsMeter meter_;
};

}
