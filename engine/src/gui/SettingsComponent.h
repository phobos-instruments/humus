#pragma once
#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"
#include "gui/MenuStylePreview.h"
#include "gui/MidiSettingsView.h"

namespace hum {

class EngineHost;

void applySavedAppearance();

class SettingsComponent : public juce::Component,
                          private juce::ListBoxModel {
public:
    enum Category { kAi = 0, kAudio, kAppearance, kLicense, kMidi, kPacks, kPlugins,
                    kNumCategories };

    explicit SettingsComponent(std::function<void()> onAppearanceChanged,
                               EngineHost* host = nullptr);

    void selectCategory(int index);

    std::function<void()> onPacksChanged;
    std::function<void()> onPluginsChanged;

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    class Swatch : public juce::Component, private juce::ChangeListener {
    public:
        juce::Colour colour;
        std::function<void(juce::Colour)> onChange;
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
    private:
        void changeListenerCallback(juce::ChangeBroadcaster*) override;
    };

    int getNumRows() override { return kNumCategories; }
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged(int lastRow) override;

    void showCategory(int index);
    void buildAiPanel();
    void rebuildFromState();
    void rebuildThemeCombo();
    void promptSaveTheme();
    void updateThemeButtons();
    void exportTheme();
    void importTheme();
    void pushAppearance();
    ThemeColours coloursFromSwatches() const;
    void setSwatches(const ThemeColours&);

    std::function<void()> onAppearanceChanged_;

    juce::ListBox categories_{"categories", this};
    juce::Label appearanceTitle_;

    juce::Label themeLabel_;
    juce::ComboBox themeCombo_;

    static constexpr int kNumRoles = 9;
    std::array<std::unique_ptr<Swatch>, kNumRoles> swatches_;
    std::array<std::unique_ptr<juce::Label>, kNumRoles> swatchLabels_;
    juce::TextButton saveCustomBtn_{"Save Theme..."};
    juce::TextButton deleteThemeBtn_{"Delete Theme"};
    juce::TextButton exportThemeBtn_{"Export..."};
    juce::TextButton importThemeBtn_{"Import..."};
    std::unique_ptr<juce::FileChooser> chooser_;

    juce::Label brightnessLabel_, contrastLabel_, uiScaleLabel_;
    juce::Slider brightness_, contrast_, uiScale_;
    juce::ToggleButton flowLights_;

    juce::Label menuStyleLabel_;
    MenuStylePreviewCard classicCard_, modernCard_;
    void applyMenuStyle(bool modern);

    juce::Label aiTitle_, providerLabel_, endpointLabel_, modelLabel_, keyLabel_, aiHint_;
    juce::ComboBox providerCombo_;
    juce::TextEditor endpointEdit_, modelEdit_, keyEdit_;
    juce::TextButton testBtn_{"Test"};

    std::unique_ptr<MidiSettingsView> midiView_;
    std::unique_ptr<juce::Component> audioView_;
    std::unique_ptr<juce::Component> packsView_;
    std::unique_ptr<juce::Component> pluginsView_;
    std::unique_ptr<juce::Component> licenseView_;
    int category_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

}
