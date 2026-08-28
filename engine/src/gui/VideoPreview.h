#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "gui/VideoPreviewStore.h"

namespace hum {

class VideoPreview : public PolledBrick {
public:
    VideoPreview(EngineHost& host, std::string organism)
        : PolledBrick(host, std::move(organism), 2) {
        VideoPreviewStore::instance().want(name_, true);
    }
    ~VideoPreview() override { VideoPreviewStore::instance().want(name_, false); }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 269; }
    int preferredContentHeight(int w) const override {
        return juce::roundToInt((float) w * 9.0f / 16.0f);
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.fillAll(juce::Colours::black);
        if (frame_.isValid()) {
            g.drawImage(frame_, r.toFloat(), juce::RectanglePlacement::centred);
        } else {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            auto* c = host_.liveOrganism(name_);
            const bool on = c != nullptr && c->params.get("Preview", 1.0) >= 0.5;
            g.drawFittedText(on ? juce::String::fromUTF8("no picture yet\xe2\x80\xa6")
                                : juce::String::fromUTF8(
                                      "preview is off - tick Preview below"),
                             r.reduced(14), juce::Justification::centred, 3);
        }
        g.setColour(Palette::border);
        g.drawRect(r);
    }

private:
    void poll() override {
        if (VideoPreviewStore::instance().take(name_, gen_, frame_)) repaint();
    }

    juce::Image frame_;
    unsigned gen_ = 0;
};

}
