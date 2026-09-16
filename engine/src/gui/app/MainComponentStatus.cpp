// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"
#include "gui/app/NativeMac.h"
#include "gui/video/VideoRenderService.h"
#include "gui/video/VideoTrackerFeed.h"
#include "gui/editor/AutomateMenu.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "core/packs/Categories.h"
#include "hum/Registry.h"
#include "core/packs/PackLoader.h"
#include "core/packs/ClassString.h"
#include "core/plugins/BridgeRing.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginHost.h"
#include "core/plugins/PluginListStore.h"
#include "core/packs/PackRegistry.h"
#include "core/app/AppPaths.h"
#include "gui/app/AppSettings.h"
#include "gui/assistant/AssistantPane.h"
#include "gui/app/HubClient.h"
#include "gui/app/Telemetry.h"
#include "gui/app/TelemetryEvents.h"
#include "gui/app/DefaultPatchHandler.h"
#include "gui/style/LookAndFeel.h"
#include "gui/plugins/PluginBridgePolicy.h"
#include "gui/plugins/PluginEditorTeardown.h"
#include "gui/host/NodeRandomize.h"
#include "gui/app/QwertyPiano.h"
#include "gui/settings/SettingsComponent.h"
#include "gui/common/Localisation.h"

namespace hum {

void MainComponent::refreshAppearance() {
    hypha_[0] = hypha_[1] = juce::Image();
    rebuildChromeTextures();
    if (auto* laf = dynamic_cast<HumLookAndFeel*>(&getLookAndFeel())) laf->refreshColours();
    sendLookAndFeelChange();
    repaint();
    if (settingsWindow_ != nullptr) {
        settingsWindow_->sendLookAndFeelChange();
        settingsWindow_->repaint();
    }
}

void MainComponent::serviceTrackerFeeds() {
    if (renderService_ == nullptr && juce::JUCEApplication::getInstance() != nullptr) {
        renderService_ = std::make_unique<VideoRenderService>(host_);
        renderService_->setSuppressed([this](const std::string& n) {
            return bounce_ != nullptr || visualWindows_.count(n) != 0;
        });
    }
    std::set<std::string> keep;
    for (const auto& c : host_.model().videoConnections) {
        if (c.dstInlet != 0) continue;
        if (dynamic_cast<hum::VideoFrameSink*>(host_.liveOrganism(c.dst)) == nullptr)
            continue;
        keep.insert(c.dst);
        if (trackerFeeds_.count(c.dst) == 0)
            trackerFeeds_[c.dst] = std::make_unique<VideoTrackerFeed>(
                host_, c.dst, renderService_.get());
    }
    for (auto it = trackerFeeds_.begin(); it != trackerFeeds_.end();)
        it = keep.count(it->first) ? std::next(it) : trackerFeeds_.erase(it);
}

void MainComponent::updateDspReadout() {
    if (!host_.audioRunning()) {
        dspLabel_.setText(host_.audioStarting() ? juce::String::fromUTF8("\xe2\x80\xa6") : juce::String(), juce::dontSendNotification);
        return;
    }
    const unsigned restarts = host_.deviceRestartCount();
    if (restarts != lastDeviceRestarts_) {
        lastDeviceRestarts_ = restarts;
        setStatus(tr("main.audio-device-restarted-opening-aux", "audio device restarted (opening aux hardware channels)"));
    }
    dspLoadHold_ = juce::jmax(host_.audioLoad(), dspLoadHold_ * 0.94f);
    if (dspLoadHold_ < 0.005f) dspLoadHold_ = 0.0f;
    const unsigned drops = host_.dropoutCount();
    if (drops != lastDropouts_) { lastDropouts_ = drops; dropFlashTicks_ = 60; }
    juce::String s(juce::String((int) std::lround(dspLoadHold_ * 100.0f)) + "%");
    if (drops > 0) s << " !" << juce::String(drops);
    dspLabel_.setText(s, juce::dontSendNotification);
    dspLabel_.setColour(juce::Label::textColourId,
                        dropFlashTicks_ > 0 ? juce::Colours::orangered : Palette::textDim);
    if (dropFlashTicks_ > 0) --dropFlashTicks_;
}

void MainComponent::updateWindowTitle() {
    const juce::String project = currentFile_.isEmpty()
        ? "Untitled"
        : juce::File(currentFile_).getFileNameWithoutExtension();
    juce::String t = (host_.isDirty() ? "*" : "") + project
                   + juce::String("  -  Humus");
    auto* w = getTopLevelComponent();
    if (w == nullptr) return;
    if (t != lastTitle_) {
        lastTitle_ = t;
        w->setName(t);
    }
    if (auto* peer = w->getPeer()) syncNativeWindowTitle(*peer, t);
}

}
