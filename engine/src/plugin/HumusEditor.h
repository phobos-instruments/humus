// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_audio_processors/juce_audio_processors.h>

#include "gui/app/AboutWindow.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "plugin/DawMacroStrip.h"
#include "plugin/HumusProcessor.h"
#include "plugin/PatchPanes.h"
#include "plugin/PluginWindows.h"

namespace hum {

class VideoRenderService;

class HumusEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit HumusEditor(HumusProcessor& p);
    ~HumusEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    PatchPanes& panesForTest() { return *panes_; }
    void rebuildPanes();
    void openSettings();
    void toggleAbout();
    AboutWindow* aboutForTest() { return aboutPanel_.get(); }

private:
    void timerCallback() override;
    void chooseFile();
    void showPaneToggles();

    HumusProcessor& proc_;
    EngineHost& host_;
    juce::SharedResourcePointer<HumLookAndFeel> laf_;
    PluginWindows windows_;
    std::unique_ptr<PatchPanes> panes_;
    std::unique_ptr<VideoRenderService> previews_;
    DawMacroStrip macros_;
    bool framePending_ = true;
    IconButton start_, play_, stop_, undo_, redo_, showPatcher_, showProperties_, about_, gear_;
    DockPane macrosFrame_;
    juce::TextButton load_;
    std::unique_ptr<juce::FileChooser> chooser_;
    std::unique_ptr<AboutWindow> aboutPanel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HumusEditor)
};

}
