#pragma once
#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/ParamSchema.h"
#include "io/PatchFormat.h"
#include "plugin/FxLook.h"
#include "plugin/HumusProcessor.h"
#include "plugin/MacroPanel.h"
#include "plugin/OrganismParamPanel.h"
#include "plugin/PatchMapView.h"

namespace hum {

class HumusEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit HumusEditor(HumusProcessor& p)
        : juce::AudioProcessorEditor(p), proc_(p), params_(p), macros_(p) {
        load_.setButtonText("Load patch...");
        load_.onClick = [this] { choose(); };
        addAndMakeVisible(load_);

        name_.setJustificationType(juce::Justification::centredLeft);
        name_.setFont(juce::FontOptions(13.0f));
        name_.setColour(juce::Label::textColourId, fxlook::text());
        addAndMakeVisible(name_);

        status_.setJustificationType(juce::Justification::centredLeft);
        status_.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(status_);

        playBtn_.setClickingTogglesState(true);
        playBtn_.setToggleState(proc_.freeRun(), juce::dontSendNotification);
        playBtn_.setColour(juce::TextButton::buttonOnColourId,
                           fxlook::accent().withAlpha(0.35f));
        playBtn_.setTooltip("Free-run the patch while the DAW is stopped");
        playBtn_.onClick = [this] { proc_.setFreeRun(playBtn_.getToggleState()); };
        addAndMakeVisible(playBtn_);
        rewindBtn_.setTooltip("Back to bar 1");
        rewindBtn_.onClick = [this] { proc_.rewindTransport(); };
        addAndMakeVisible(rewindBtn_);
        clock_.setJustificationType(juce::Justification::centredLeft);
        clock_.setFont(juce::FontOptions(12.0f));
        clock_.setColour(juce::Label::textColourId, fxlook::text());
        addAndMakeVisible(clock_);

        map_.onSelect = [this](const std::string& organism) {
            map_.setSelected(organism);
            params_.setTarget(organism);
        };
        addAndMakeVisible(map_);
        addAndMakeVisible(params_);
        addAndMakeVisible(macros_);

        refreshAll();
        setSize(780, 584);
        startTimerHz(20);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(fxlook::bg());
        g.setColour(fxlook::accent());
        g.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        g.drawText("Humus", 12, 8, 100, 22, juce::Justification::centredLeft, false);
        auto meter = getLocalBounds().removeFromBottom(16).reduced(12, 4);
        for (int c = 0; c < 2; ++c) {
            auto lane = meter.removeFromTop(3);
            meter.removeFromTop(2);
            g.setColour(fxlook::box());
            g.fillRect(lane);
            g.setColour(fxlook::accent());
            g.fillRect(lane.removeFromLeft((int) ((float) lane.getWidth()
                                                  * juce::jlimit(0.0f, 1.0f, proc_.lastPeak(c)))));
        }
    }

    void resized() override {
        auto r = getLocalBounds().reduced(12);
        auto top = r.removeFromTop(24);
        top.removeFromLeft(96);
        load_.setBounds(top.removeFromRight(110));
        top.removeFromRight(8);
        name_.setBounds(top);
        r.removeFromTop(4);
        auto transport = r.removeFromTop(22);
        playBtn_.setBounds(transport.removeFromLeft(56));
        transport.removeFromLeft(4);
        rewindBtn_.setBounds(transport.removeFromLeft(30));
        transport.removeFromLeft(10);
        clock_.setBounds(transport.removeFromLeft(180));
        r.removeFromTop(2);
        status_.setBounds(r.removeFromTop(16));
        r.removeFromTop(4);
        r.removeFromBottom(14);
        auto macroArea = r.removeFromBottom(192);
        r.removeFromBottom(6);
        macros_.setBounds(macroArea);
        params_.setBounds(r.removeFromRight(juce::jmax(220, r.getWidth() * 2 / 5)));
        r.removeFromRight(6);
        map_.setBounds(r);
    }

private:
    void choose() {
        chooser_ = std::make_unique<juce::FileChooser>("Load patch",
                                                       juce::File(), kPatchOpenFilter);
        chooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this](const juce::FileChooser& fc) {
                                  auto f = fc.getResult();
                                  if (f == juce::File{}) return;
                                  std::string err;
                                  if (!proc_.loadPatchFile(f, err))
                                      loadError_ = "Load failed: " + juce::String(err);
                                  else
                                      loadError_.clear();
                                  refreshAll();
                              });
    }

    void refreshStatus() {
        if (loadError_.isNotEmpty()) {
            status_.setColour(juce::Label::textColourId, fxlook::warn());
            status_.setText(loadError_, juce::dontSendNotification);
            return;
        }
        const auto& model = proc_.model();
        bool hasOut = model.organisms.empty();
        for (const auto& cm : model.organisms)
            if (cm.displayClass == "SoundOut" || cm.displayClass == "AuxOut") hasOut = true;
        if (!hasOut) {
            status_.setColour(juce::Label::textColourId, fxlook::warn());
            status_.setText("This patch has no SoundOut, so the plugin is silent - "
                            "add one in Humus and reload.",
                            juce::dontSendNotification);
            return;
        }
        status_.setText("", juce::dontSendNotification);
    }

    void refreshAll() {
        lastPatch_ = proc_.patchName();
        name_.setText(lastPatch_.isEmpty() ? "(no patch loaded)" : lastPatch_,
                      juce::dontSendNotification);
        map_.setModel(&proc_.model());
        if (proc_.model().byName(params_.target()) == nullptr) {
            std::string first;
            for (const auto& cm : proc_.model().organisms) {
                if (first.empty()) first = cm.name;
                if (cm.displayClass != "SoundIn" && cm.displayClass != "SoundOut"
                    && cm.displayClass != "AuxIn" && cm.displayClass != "AuxOut"
                    && !schemaFor(cm.displayClass).empty()) {
                    first = cm.name;
                    break;
                }
            }
            map_.setSelected(first);
            params_.setTarget(first);
        } else {
            params_.setTarget(params_.target());
        }
        macros_.refresh();
        refreshStatus();
        refreshTransport();
    }

    void timerCallback() override {
        repaint(getLocalBounds().removeFromBottom(16));
        if (proc_.patchName() != lastPatch_) refreshAll();
        macros_.pullValues();
        params_.refreshValues();
        refreshTransport();
    }

    void refreshTransport() {
        if (playBtn_.getToggleState() != proc_.freeRun())
            playBtn_.setToggleState(proc_.freeRun(), juce::dontSendNotification);
        const double beats = proc_.transportBeats();
        const int bar = (int) (beats / 4.0) + 1;
        const int beat = (int) std::fmod(beats, 4.0) + 1;
        clock_.setText(juce::String(bar) + "." + juce::String(beat) + "   "
                           + juce::String(proc_.transportTempo(), 1) + " bpm"
                           + (proc_.transportPlaying() ? "" : "   (stopped)"),
                       juce::dontSendNotification);
    }

    HumusProcessor& proc_;
    juce::TextButton load_;
    juce::TextButton playBtn_{"Play"}, rewindBtn_{"|<"};
    juce::Label clock_;
    juce::Label name_, status_;
    PatchMapView map_;
    OrganismParamPanel params_;
    MacroPanel macros_;
    juce::String lastPatch_, loadError_;
    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HumusEditor)
};

}
