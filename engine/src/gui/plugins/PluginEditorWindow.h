// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/plugins/HostedPlugin.h"
#include "gui/host/PluginsHost.h"
#include "gui/app/FreeWindow.h"
#include "gui/style/LookAndFeel.h"
#include "gui/plugins/PluginEditorTeardown.h"
#include "gui/plugins/PluginParamMirror.h"
#include "gui/app/QwertyPiano.h"

#if JUCE_MAC
namespace hum {
void pluginWindowClearTrackingAreas(juce::ComponentPeer*);
void pluginWindowAttachToMain(juce::ComponentPeer* plugin, juce::ComponentPeer* main);
void pluginWindowDetachFromMain(juce::ComponentPeer* plugin, juce::ComponentPeer* main);
}
#endif

namespace hum {

class PluginEditorWindow : public juce::DocumentWindow, private juce::Timer {
public:
    PluginEditorWindow(PluginsHost& host, std::string name, HostedPlugin& plugin,
                       juce::Component* mainComponent,
                       std::function<void(const std::string&)> onClosed)
        : juce::DocumentWindow(juce::String(name),
                               Palette::background,
                               juce::DocumentWindow::closeButton),
          host_(host), name_(std::move(name)), plugin_(plugin),
          mainPeer_(mainComponent ? mainComponent->getPeer() : nullptr),
          onClosed_(std::move(onClosed)) {
        setUsingNativeTitleBar(true);
        {
            EditorOpGuard g(plugin_.classRaw(), "plugins.uiQuarantine", "opening its editor");
            editor_.reset(plugin_.instance()->createEditorIfNeeded());
        }
        const bool pluginResizable = editor_ && editor_->getConstrainer() != nullptr;
        setResizable(pluginResizable, false);
        if (editor_) setContentNonOwned(editor_.get(), true);
        setVisible(true);
        startTimerHz(30);
#if JUCE_MAC
        pluginWindowAttachToMain(getPeer(), mainPeer_);
#endif
    }

    ~PluginEditorWindow() override {
        stopTimer();
#if JUCE_MAC
        pluginWindowDetachFromMain(getPeer(), mainPeer_);
        pluginWindowClearTrackingAreas(getPeer());
        removeFromDesktop();
        clearContentComponent();
        if (shouldLeakEditor(plugin_.classRaw())) {
            (void) editor_.release();
        } else {
            EditorOpGuard g(plugin_.classRaw(), "plugins.leakEditor", "closing its editor");
            editor_.reset();
        }
#else
        clearContentComponent();
#endif
    }

    void closeButtonPressed() override { if (onClosed_) onClosed_(name_); }

    bool keyPressed(const juce::KeyPress& k) override {
        if (isCloseWindowKey(k)) { closeButtonPressed(); return true; }
        return QwertyPiano::instance().handleKeyPress(k);
    }
    bool keyStateChanged(bool) override {
        return QwertyPiano::instance().handleKeyState();
    }

private:
    void timerCallback() override {
        if (mirrorPluginParams(host_, name_, plugin_)) host_.pokeLiveRefresh();
    }

    PluginsHost& host_;
    std::string name_;
    HostedPlugin& plugin_;
    juce::ComponentPeer* mainPeer_;
    std::function<void(const std::string&)> onClosed_;
    std::unique_ptr<juce::AudioProcessorEditor> editor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

}
