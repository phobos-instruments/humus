// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/OrganismEditor.h"
#include "gui/editor/juce/JuceView.h"
#include "gui/editor/views/RichView.h"
#include "gui/host/EditorHost.h"
#include "hum/LayoutSpec.h"

namespace hum {

class JuceRichView : public RichView, public JucePartsOwner {
public:
    explicit JuceRichView(std::unique_ptr<juce::Component> component) : component_(std::move(component)) {
        parts_.push_back(component_.get());
    }

    void setViewBounds(Rect bounds) override { component_->setBounds(toJuce(bounds)); }
    void setViewVisible(bool visible) override { component_->setVisible(visible); }
    bool viewVisible() const override { return component_->isVisible(); }
    void setViewFade(float alpha, bool enabled) override {
        component_->setAlpha(alpha);
        component_->setEnabled(enabled);
    }
    float viewAlpha() const override { return component_->getAlpha(); }
    bool viewEnabled() const override { return component_->isEnabled(); }
    void setViewTooltip(const std::string& tip) override {
        if (auto* client = dynamic_cast<juce::SettableTooltipClient*>(component_.get()))
            client->setTooltip(viewText(tip));
    }

    void reloadValues() override { if (onReload) onReload(); }
    void reloadText() override { if (onReloadText) onReloadText(); }
    void refreshLive() override { if (onLive) onLive(); }
    void showMarks(bool controlled, bool locked) override { if (onMarks) onMarks(controlled, locked); }
    void openClip(int clip) override { if (onClip) onClip(clip); }
    std::string shownText() const override { return onText ? onText() : std::string(); }

    std::function<void()> onReload, onReloadText, onLive;
    std::function<void(bool, bool)> onMarks;
    std::function<void(int)> onClip;
    std::function<std::string()> onText;

private:
    std::unique_ptr<juce::Component> component_;
};

inline std::unique_ptr<JuceRichView> brickView(juce::Component& parent, std::unique_ptr<juce::Component> widget) {
    parent.addAndMakeVisible(*widget);
    return std::make_unique<JuceRichView>(std::move(widget));
}

template <class Widget, class Reload>
std::unique_ptr<JuceRichView> reloadingView(juce::Component& parent, std::unique_ptr<Widget> widget,
                                            Reload reload) {
    auto& w = *widget;
    auto view = brickView(parent, std::move(widget));
    view->onReload = [&w, reload] { reload(w); };
    view->onReloadText = view->onReload;
    return view;
}

inline std::unique_ptr<JuceRichView> editorView(juce::Component& parent, std::unique_ptr<OrganismEditor> editor) {
    auto& e = *editor;
    auto view = brickView(parent, std::move(editor));
    view->onReload = [&e] { e.reloadValues(); };
    view->onLive = [&e] { e.refreshAutomatedValues(); };
    view->onClip = [&e](int clip) { e.openClip(clip); };
    return view;
}

inline std::function<void()> changedHook(RichOwner& owner) {
    return [&owner] { owner.automationChanged(); };
}

std::unique_ptr<RichView> makeJucePlayerBrick(juce::Component& parent, EditorHost& host, const LayoutSpec& spec,
                                              const LayoutSpec::Control& s, RichOwner& owner);
std::unique_ptr<RichView> makeJuceReadoutBrick(juce::Component& parent, EditorHost& host, const LayoutSpec& spec,
                                               const LayoutSpec::Control& s, RichOwner& owner);
std::unique_ptr<RichView> makeJucePickerBrick(juce::Component& parent, EditorHost& host, const LayoutSpec& spec,
                                              const LayoutSpec::Control& s, RichOwner& owner);

}
