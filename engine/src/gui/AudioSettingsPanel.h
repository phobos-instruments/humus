#pragma once
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

namespace hum {

class AudioSettingsPanel : public juce::Component, private juce::Timer {
public:
    explicit AudioSettingsPanel(EngineHost* host) : host_(host) {
        title_.setText(tr("audio-settings.audio", "Audio"), juce::dontSendNotification);
        title_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        addAndMakeVisible(title_);

        hint_.setJustificationType(juce::Justification::topLeft);
        hint_.setFont(juce::FontOptions(11.5f));
        hint_.setColour(juce::Label::textColourId, Palette::textDim);
        addAndMakeVisible(hint_);

        if (host_) {
            selector_ = std::make_unique<juce::AudioDeviceSelectorComponent>(
                host_->audioDevices(),
 0, 32, 2, 32,
 false, false,
 false, false);
            selViewport_.setViewedComponent(selector_.get(), false);
            selViewport_.setScrollBarsShown(true, false);
            addAndMakeVisible(selViewport_);
            startTimer(1000);
        } else {
            hint_.setText(tr("audio-settings.no-engine-in-this-preview", "No engine in this preview."), juce::dontSendNotification);
        }
    }

    ~AudioSettingsPanel() override {
        stopTimer();
        if (host_) host_->persistAudioState();
    }

    void visibilityChanged() override {
        if (isVisible() && host_ && !ensured_) {
            ensured_ = true;
            host_->ensureAudio();
        }
    }

    void resized() override {
        auto area = getLocalBounds().reduced(16, 12);
        title_.setBounds(area.removeFromTop(26));
        area.removeFromTop(4);
        if (selector_ == nullptr) { hint_.setBounds(area); return; }
        selViewport_.setBounds(area);
        selector_->setSize(area.getWidth() - 12, juce::jmax(area.getHeight(), 420));
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

private:
    void filterBufferSizes() {
        auto* dev = host_->audioDevices().getCurrentAudioDevice();
        if (dev == nullptr || selector_ == nullptr) return;
        auto* combo = findBufferCombo(*selector_);
        if (combo == nullptr) return;

        const auto avail = dev->getAvailableBufferSizes();
        const int current = dev->getCurrentBufferSizeSamples();
        juce::Array<int> wanted;
        for (int bs : {32, 64, 128, 256, 512, 1024, 2048})
            if (avail.contains(bs)) wanted.add(bs);
        if (current > 0 && !wanted.contains(current)) wanted.add(current);
        wanted.sort();
        if (wanted.isEmpty()) return;

        bool alreadyTrimmed = combo->getNumItems() == wanted.size();
        for (int i = 0; alreadyTrimmed && i < wanted.size(); ++i)
            alreadyTrimmed = combo->getItemId(i) == wanted[i];
        if (alreadyTrimmed) return;

        const double rate = dev->getCurrentSampleRate();
        combo->clear(juce::dontSendNotification);
        for (int bs : wanted)
            combo->addItem(juce::String(bs) + tr("audio-settings.samples", " samples (")
                               + juce::String(bs * 1000.0 / rate, 1) + " ms)", bs);
        combo->setSelectedId(current, juce::dontSendNotification);
    }

    static juce::ComboBox* findBufferCombo(juce::Component& root) {
        for (auto* child : root.getChildren()) {
            if (auto* cb = dynamic_cast<juce::ComboBox*>(child))
                if (cb->getNumItems() > 0 && cb->getItemText(0).contains("samples"))
                    return cb;
            if (auto* found = findBufferCombo(*child)) return found;
        }
        return nullptr;
    }

    void timerCallback() override {
        if (!host_ || !isShowing()) return;
        filterBufferSizes();
    }

    EngineHost* host_;
    bool ensured_ = false;
    juce::Label title_, hint_;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> selector_;
    juce::Viewport selViewport_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsPanel)
};

}
