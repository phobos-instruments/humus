#pragma once
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/Pattern.h"
#include "gui/OrganismEditor.h"
#include "gui/EngineHost.h"
#include "gui/ParamSlider.h"
#include "gui/SoundFileSlot.h"

namespace hum {

struct PatternEditorSpec {
    bool valid = false;
    int laneCount = 8;
    std::string masterVolumeParam = "Volume";
    std::string muteParam = "Mute";
    std::string gateParam;
    std::string volumeParamPrefix = "Volume_";
    std::string enableParamPrefix = "Enable_";
    std::string fileParamPrefix = "File_";
};

class PatternEditor : public OrganismEditor, private juce::Timer {
public:
    PatternEditor(EngineHost& host, std::string organism, PatternEditorSpec spec);

    void reloadValues() override;
    void refreshAutomatedValues() override {}
    void timerCallback() override;
    int preferredContentWidth() const override { return 600; }
    int preferredContentHeight(int width) const override;

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;

private:
    static constexpr int kKnobRowH = 62, kRulerH = 22, kLaneH = 24, kToolbarH = 28;
    static constexpr int kLeftW = 290, kNumW = 16, kEnW = 20, kChipW = 38, kNudgeW = 15;
    static constexpr int kGridPad = 10;

    int laneTop(int i) const { return kKnobRowH + kRulerH + i * kLaneH; }
    int lanesBottom() const { return kKnobRowH + kRulerH + spec_.laneCount * kLaneH; }
    int gridX() const { return kLeftW + kGridPad; }
    int laneAt(int y) const;
    double ppt() const { return ppb_ / Pattern::kTicksPerBeat; }
    float tickToX(int tick) const { return (float) (gridX() + (tick - scrollTicks_) * ppt()); }
    int xToTick(float x) const { return (int) std::lround(scrollTicks_ + (x - gridX()) / ppt()); }
    int snapTicks() const;
    int laneSnapTicks(int lane) const;
    juce::String laneSnapText(int lane) const;
    int snapTickForLane(int lane, int tick) const;

    const Pattern* pattern() const;
    int nearestTrigger(int lane, int tick, int tolTicks) const;
    void fitZoom();
    void build();
    void bindKnob(ParamSlider& k, const std::string& param);
    void paintRuler(juce::Graphics&);
    void paintLane(juce::Graphics&, int i);

    EngineHost& host_;
    std::string name_;
    PatternEditorSpec spec_;

    std::unique_ptr<ParamSlider> masterKnob_;
    std::unique_ptr<juce::ToggleButton> muteToggle_, gateToggle_;
    std::vector<std::unique_ptr<ParamSlider>> volKnobs_;
    std::vector<std::unique_ptr<juce::ToggleButton>> enableToggles_;
    std::vector<std::unique_ptr<SoundFileSlot>> fileSlots_;
    std::unique_ptr<juce::ComboBox> snapBox_;
    std::unique_ptr<juce::TextButton> coarseL_, coarseR_, fineL_, fineR_;
    std::vector<std::unique_ptr<juce::TextButton>> snapChips_;
    std::vector<std::unique_ptr<juce::TextButton>> laneNudgeL_, laneNudgeR_;

    double ppb_ = 96.0;
    double scrollTicks_ = 0.0;

    int dragLane_ = -1, dragTick_ = -1;
    bool dragAdded_ = false, dragMoved_ = false, deleteOnUp_ = false;

    void repaintTimeline();
    double playTick_ = 0.0;
    bool showPlayhead_ = false;
};

}
