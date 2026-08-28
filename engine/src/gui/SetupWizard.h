#pragma once
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/WizardFlow.h"
#include "gui/MenuStylePreview.h"

namespace hum {

class EngineHost;
class AudioSettingsPanel;
class MidiSettingsView;

class SetupWizard : public juce::Component {
public:
    struct Callbacks {
        std::function<void()> onAppearanceChanged;
        std::function<void()> onFinished;
    };

    SetupWizard(EngineHost& host, Callbacks cbs, bool firstBoot);
    ~SetupWizard() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
    bool keyPressed(const juce::KeyPress& k) override;

private:
    class ThemeGallery;

    void setPage(wizard::Page p);
    void finish();
    juce::Rectangle<int> contentArea() const;
    std::vector<std::string> summary() const;

    EngineHost& host_;
    Callbacks cbs_;
    const bool firstBoot_;
    wizard::Page page_ = wizard::kAudio;

    std::unique_ptr<AudioSettingsPanel> audio_;
    std::unique_ptr<MidiSettingsView> midi_;
    std::unique_ptr<ThemeGallery> themes_;
    juce::Slider uiScale_;
    juce::Label uiScaleLabel_, hintLabel_, menuStyleLabel_;
    MenuStylePreviewCard classicCard_, modernCard_;
    juce::ToggleButton telemetryToggle_, updatesToggle_;

    juce::TextButton backBtn_{"Back"}, nextBtn_{"Next"}, skipBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetupWizard)
};

}
