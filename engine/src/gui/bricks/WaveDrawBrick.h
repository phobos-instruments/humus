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
#include "hum/dsp/WaveTable.h"
#include "hum/dsp/WaveWarp.h"
#include "gui/common/Localisation.h"

namespace hum {

class WaveDrawBrick : public PolledBrick, public juce::FileDragAndDropTarget {
public:
    WaveDrawBrick(BrickHost& host, std::string organism, std::string param, const Bindings& bound);

    void reloadValues() override { pull(host_.liveParamText(name_, pn_)); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 584; }
    int preferredContentHeight(int) const override { return 150; }

    void paint(juce::Graphics& g) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;

    void fileDragEnter(const juce::StringArray&, int, int) override;

    void fileDragExit(const juce::StringArray&) override;

    void filesDropped(const juce::StringArray& files, int, int) override;

    void seedFileForTest(const juce::File& f) { seedFromFile(f); }
    int frameCountForTest() const { return frameCount_; }
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

    int curFrame() const { return juce::jlimit(0, frameCount_ - 1,
                                                (int) std::lround(pos_ * (frameCount_ - 1))); }
    void blend(int& f0, int& f1, float& fr) const;
    float valueAt(double phase) const;

    void paintSample(const juce::MouseEvent& e);

    void addFrame();
    void removeFrame();

    void paintStrip(juce::Graphics& g);

    void chooseSeed();

    void seedFromFile(const juce::File& f);

    bool seedFromSignal(const signalfile::Signal& sig);

    bool loadAudioSeed(const juce::File& f, juce::AudioBuffer<float>& buf);

    void pull(const std::string& text);

    void push();

    void poll() override;

    static constexpr int kMaxFrames = 16;
    std::string seededKind_, seededLabel_;
    bool dropping_ = false;
    std::string pn_;
    std::string positionParam_, warpParam_, warpModeParam_;
    float frames_[kMaxFrames][kWaveTableLen] = {};
    int frameCount_ = 1;
    double pos_ = 0.0;
    std::string cachedText_;
    int activePreset_ = -1;
    int lastWm_ = -1;
    float lastWa_ = -1.0f;
    bool drawing_ = false;
    int lastIdx_ = -1;
    float lastVal_ = 0.0f;
    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveDrawBrick)
};

}
