#pragma once
#include <deque>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"

namespace hum {

class EventLogView : public PolledBrick {
public:
    EventLogView(EngineHost& host, std::string name)
        : PolledBrick(host, std::move(name), 2) {
        pause_.setClickingTogglesState(true);
        addAndMakeVisible(pause_);
        addAndMakeVisible(clear_);
        clear_.onClick = [this] { lines_.clear(); repaint(); };
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 368; }
    int preferredContentHeight(int) const override { return 246; }

    void resized() override {
        auto r = getLocalBounds();
        auto top = r.removeFromTop(22);
        pause_.setBounds(top.removeFromRight(64).reduced(2, 0));
        clear_.setBounds(top.removeFromRight(58).reduced(2, 0));
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        auto top = r.removeFromTop(22);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(headerText(), top.reduced(4, 0), juce::Justification::centredLeft);
        g.setColour(Palette::background.darker(0.15f));
        g.fillRoundedRectangle(r.toFloat(), 5.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 5.0f, 1.0f);
        r.reduce(8, 5);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f,
                                    juce::Font::plain));
        const int rowH = 15;
        const int visible = juce::jmax(1, r.getHeight() / rowH);
        const int first = juce::jmax(0, (int) lines_.size() - visible);
        int y = r.getY();
        for (size_t i = (size_t) first; i < lines_.size(); ++i) {
            const auto& ln = lines_[i];
            g.setColour(ln.accent ? Palette::accent : Palette::text);
            g.drawText(ln.text, r.getX(), y, r.getWidth(), rowH,
                       juce::Justification::centredLeft, true);
            y += rowH;
        }
        if (lines_.empty()) {
            g.setColour(Palette::textDim);
            g.drawText(emptyText(), r, juce::Justification::centred);
        }
    }

protected:
    virtual void drain() = 0;
    virtual juce::String headerText() const = 0;
    virtual juce::String emptyText() const = 0;

    void push(juce::String text, bool accent) {
        lines_.push_back({std::move(text), accent});
        pushed_ = true;
    }
    bool paused() const { return pause_.getToggleState(); }
    auto* node() const { return host_.liveOrganism(name_); }
    size_t lineCount() const { return lines_.size(); }

private:
    struct Line { juce::String text; bool accent = false; };

    void poll() override {
        pushed_ = false;
        drain();
        if (!pushed_) return;
        while (lines_.size() > kMaxLines) lines_.pop_front();
        repaint();
    }

    static constexpr size_t kMaxLines = 400;
    std::deque<Line> lines_;
    bool pushed_ = false;
    juce::TextButton pause_{"Pause"}, clear_{"Clear"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventLogView)
};

}
