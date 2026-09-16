// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class SettingsHost;
class AiSettingsView;
class AppearanceSettingsView;
class MidiSettingsView;

void applySavedAppearance();

class SettingsComponent : public juce::Component,
                          private juce::ListBoxModel {
public:
    enum Category { kAi = 0, kAudio, kAppearance, kLicense, kMidi, kPacks, kPlugins, kVideo,
                    kNumCategories };

    explicit SettingsComponent(std::function<void()> onAppearanceChanged,
                               SettingsHost* host = nullptr);
    ~SettingsComponent() override;

    void selectCategory(int index);

    std::function<void()> onPacksChanged;
    std::function<void()> onPluginsChanged;

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    int getNumRows() override { return kNumCategories; }
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged(int lastRow) override;
    void showCategory(int index);

    juce::ListBox categories_{"categories", this};
    std::unique_ptr<AppearanceSettingsView> appearanceView_;
    std::unique_ptr<AiSettingsView> aiView_;
    std::unique_ptr<MidiSettingsView> midiView_;
    std::unique_ptr<juce::Component> audioView_;
    std::unique_ptr<juce::Component> packsView_;
    std::unique_ptr<juce::Component> pluginsView_;
    std::unique_ptr<juce::Component> licenseView_;
    std::unique_ptr<juce::Component> videoView_;
    int category_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

}
