// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>

#include "core/graph/DeviceChannels.h"
#include "gui/host/GuardedDeviceType.h"
#include "gui/host/SettingsHost.h"
#include "gui/host/MidiState.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class AudioSettingsPanel : public juce::Component, private juce::Timer {
public:
    explicit AudioSettingsPanel(SettingsHost* host) : host_(host) {
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
 0, kMaxDeviceChannels, 2, kMaxDeviceChannels,
 false, false,
 false, false);
            selViewport_.setViewedComponent(selector_.get(), false);
            selViewport_.setScrollBarsShown(true, false);
            addAndMakeVisible(selViewport_);
            cueLabel_.setText(tr("audio-settings.preview-output", "File previews play on"), juce::dontSendNotification);
            cueLabel_.setFont(juce::FontOptions(12.5f));
            addAndMakeVisible(cueLabel_);
            cue_.onChange = [this] {
                const int id = cue_.getSelectedId();
                host_->setPreviewOutput(id > kMainOutputsId ? id - kMainOutputsId - 1 : -1);
            };
            addAndMakeVisible(cue_);
            fillCueChoices();
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
        if (hint_.getText().isNotEmpty()) hint_.setBounds(area.removeFromBottom(kHiddenNoteH));
        auto cueRow = area.removeFromBottom(kCueRowH).withTrimmedTop(6);
        cueLabel_.setBounds(cueRow.removeFromLeft(170));
        cue_.setBounds(cueRow.withWidth(juce::jmin(cueRow.getWidth(), 280)));
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
            combo->addItem(tr("audio-settings.samples", "{n} samples ({ms} ms)",
                              {{"n", juce::String(bs)}, {"ms", juce::String(bs * 1000.0 / rate, 1)}}), bs);
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

    void fillCueChoices() {
        const auto pairs = host_->choiceItems("audio-out-pairs");
        if (cue_.getNumItems() == (int) pairs.size() + 1) return;
        cue_.clear(juce::dontSendNotification);
        cue_.addItem(tr("audio-settings.main-outputs", "Main outputs"), kMainOutputsId);
        for (const auto& [first, label] : pairs)
            if (first > 1) cue_.addItem(juce::String::fromUTF8(label.c_str()), kMainOutputsId + first);
        const int current = host_->previewOutput();
        cue_.setSelectedId(current >= 2 ? kMainOutputsId + current + 1 : kMainOutputsId, juce::dontSendNotification);
    }

    juce::String hiddenDriversNote() const {
        auto* type = dynamic_cast<GuardedDeviceType*>(host_->audioDevices().getCurrentDeviceTypeObject());
        if (type == nullptr || type->hiddenDrivers().isEmpty()) return {};
        return tr("audio-settings.hidden-drivers",
                  "Not listed because they crashed or froze when Humus tried them on their own: {drivers}",
                  {{"drivers", type->hiddenDrivers().joinIntoString(", ")}});
    }

    void timerCallback() override {
        if (!host_ || !isShowing()) return;
        filterBufferSizes();
        fillCueChoices();
        if (const auto note = hiddenDriversNote(); note != hint_.getText()) {
            hint_.setText(note, juce::dontSendNotification);
            resized();
        }
    }

    static constexpr int kHiddenNoteH = 34;
    static constexpr int kCueRowH = 34;
    static constexpr int kMainOutputsId = 1;

    SettingsHost* host_;
    bool ensured_ = false;
    juce::Label title_, hint_, cueLabel_;
    juce::ComboBox cue_;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> selector_;
    juce::Viewport selViewport_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsPanel)
};

}
