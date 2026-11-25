// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include <clap/ext/gui.h>

#include "gui/plugins/clap/ClapNativeParent.h"

namespace hum::claphost {

class Instance;

class Editor : public juce::AudioProcessorEditor {
public:
    explicit Editor(Instance& instance);
    ~Editor() override;

    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
    void resized() override;
    bool attached() const { return attached_; }

private:
    void attach();
    bool resizeFromPlugin(uint32_t w, uint32_t h);
    juce::Point<int> logical(uint32_t w, uint32_t h) const;
    uint32_t physical(int logicalSize) const;

    Instance& instance_;
    const clap_plugin_t* plugin_;
    const clap_plugin_gui_t* gui_;
    std::unique_ptr<NativeParent> native_;
    float scale_ = 1.0f;
    bool created_ = false, attached_ = false, applying_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

}
