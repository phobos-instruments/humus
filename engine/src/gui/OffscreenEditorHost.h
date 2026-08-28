#pragma once
#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "gui/LookAndFeel.h"

namespace hum {

class OffscreenEditorHost : public juce::Component {
public:
    explicit OffscreenEditorHost(std::unique_ptr<juce::AudioProcessorEditor> editor)
        : editor_(std::move(editor)) {
        setOpaque(true);
        setSize(juce::jmax(40, editor_->getWidth()), juce::jmax(40, editor_->getHeight()));
        addAndMakeVisible(editor_.get());
        editor_->setTopLeftPosition(0, 0);
        addToDesktop(juce::ComponentPeer::windowIsTemporary);
        setTopLeftPosition(-16000, -16000);
        setVisible(true);
    }

    ~OffscreenEditorHost() override {
        if (editor_ != nullptr) removeChildComponent(editor_.get());
        removeFromDesktop();
        editor_.reset();
    }

    juce::AudioProcessorEditor* editor() const { return editor_.get(); }
    int naturalWidth() const { return editor_ ? editor_->getWidth() : 0; }
    int naturalHeight() const { return editor_ ? editor_->getHeight() : 0; }

    std::unique_ptr<juce::AudioProcessorEditor> releaseEditor() {
        if (editor_ != nullptr) removeChildComponent(editor_.get());
        return std::move(editor_);
    }

    std::function<void()> onNaturalSizeChanged;

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

    void childBoundsChanged(juce::Component* c) override {
        if (c != editor_.get()) return;
        setSize(juce::jmax(40, editor_->getWidth()), juce::jmax(40, editor_->getHeight()));
        editor_->setTopLeftPosition(0, 0);
        if (onNaturalSizeChanged) onNaturalSizeChanged();
    }

private:
    std::unique_ptr<juce::AudioProcessorEditor> editor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OffscreenEditorHost)
};

}
