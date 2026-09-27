// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "io/SignalFile.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "gui/editor/grids/WaveDrawModel.h"
#include "gui/editor/juce/JuceFilePicker.h"
#include "gui/common/Localisation.h"

namespace hum {

class WaveDrawBrick : public PolledBrick, public juce::FileDragAndDropTarget {
public:
    WaveDrawBrick(BrickHost& host, std::string organism, std::string param, const Bindings& bound);

    void reloadValues() override {
        wave_.pull(wave_.liveText());
        repaint();
    }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 584; }
    int preferredContentHeight(int) const override { return 150; }

    void paint(juce::Graphics& g) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;

    void fileDragEnter(const juce::StringArray&, int, int) override;

    void fileDragExit(const juce::StringArray&) override;

    void filesDropped(const juce::StringArray& files, int, int) override;

    void seedFileForTest(const juce::File& f) { seedFromFile(f); }
    int frameCountForTest() const { return wave_.frameCount(); }
    juce::String sourceTextForTest() const { return sourceText(); }

    juce::String sourceText() const;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent& e) override;

private:
    static constexpr int kTileH = 26, kTileGap = 4, kTiles = kWaveTablePresets + 4;

    static constexpr int kStripH = 20, kStripGap = 4;
    juce::Rectangle<int> stripRect() const;
    juce::Rectangle<int> waveArea() const;
    juce::Rectangle<int> frameSlot(int f) const;
    juce::Rectangle<int> tileRect(int i) const;

    void paintSample(const juce::MouseEvent& e);

    void paintStrip(juce::Graphics& g);

    void chooseSeed();

    void seedFromFile(const juce::File& f);

    bool loadAudioSeed(const juce::File& f);

    void poll() override;

    static constexpr int kMaxFrames = grids::WaveDrawModel::kMaxFrames;
    grids::WaveDrawModel wave_;
    bool dropping_ = false;
    JuceFilePicker picker_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveDrawBrick)
};

}
