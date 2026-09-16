// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"
#include "gui/app/AboutWindow.h"
#include "gui/app/AppUpdater.h"
#include "gui/app/BounceJob.h"
#include "gui/help/GuideView.h"
#include "gui/properties/MetapadView.h"
#include "gui/app/UpdateNotice.h"
#include "gui/plugins/PluginEditorWindow.h"
#include "gui/app/SetupWizard.h"
#include "gui/app/StartWindow.h"
#include "gui/tracks/TracksPane.h"
#include "gui/app/TransportStrip.h"
#include "gui/video/VideoRenderService.h"
#include "gui/video/VideoTrackerFeed.h"
#include "gui/video/VisualWindow.h"
#include "gui/app/FloatingWindows.h"
#include "gui/app/FreeWindow.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"
#include "gui/common/UiTicker.h"
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
#include "gui/plugins/EmbeddedDebug.h"
#include "gui/app/NativeMac.h"
#include "gui/settings/SettingsComponent.h"
#include "gui/common/Localisation.h"

namespace hum {

MainComponent::MainComponent()
    : watchdog_(UiWatchdog::defaultBreadcrumb(), UiWatchdog::defaultLog(),
                AppSettings::instance().getInt("watchdog.stallSec", 10) * 1000) {
    setWantsKeyboardFocus(true);
    beginAppKeepAwake();
    registerBuiltinOrganisms();
    if (AppSettings::instance().getInt("watchdog.stallSec", 10) > 0) {
        watchdog_.makeActive();
        watchdog_.startThread();
    }
    if (const auto msg = EditorOpGuard::sweepAtStartup(); msg.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                               tr("main.plugin-safe-mode", "Plugin safe mode"), msg);
    PackLoader::instance().loadInstalledPacks();
    PluginHost::instance().restoreKnownListFromXml(pluginListStore::load());
    {
        juce::StringArray disabled;
        disabled.addTokens(AppSettings::instance().getString("packs.disabled"), ",", "");
        for (auto& id : disabled)
            if (id.isNotEmpty()) PackRegistry::instance().setPackEnabled(id.toStdString(), false);
    }
    BridgeRing::sweepStale();
    PluginHost::instance().setBridgeExe(
        juce::File::getSpecialLocation(juce::File::currentExecutableFile));
    PluginHost::instance().setBridgePolicy(
        [](const std::string& classRaw) { return bridgePolicy::shouldBridge(classRaw); });
    HostedPlugin::shouldLeakInstance = [](const std::string& classRaw) {
        return matchesPluginList(classRaw, "plugins.leakInstance", "Synplant");
    };
    HostedPlugin::disposeGuard = [](const std::string& classRaw) -> std::shared_ptr<void> {
        return std::make_shared<EditorOpGuard>(classRaw, "plugins.leakInstance",
                                               "disposing its instance");
    };

    canvas_ = std::make_unique<PatcherCanvas>(host_);
    propsPane_ = std::make_unique<PropertiesPane>(host_);
    canvas_->onActivate = [this](const std::string& n) { propsPane_->openFor(n); };

    canvas_->onSelect = [this](const std::string& n) { propsPane_->setSelected(n); };
    propsPane_->onSelect = [this](const std::string& n) {
        canvas_->select(n);
        canvas_->refresh();
    };

    canvas_->onAddRequest = [this](const std::string& cls, juce::Point<int> at) { addAt(cls, at); };
    propsPane_->onOpenPluginUI = [this](const std::string& n) { openPluginUI(n); };
    propsPane_->onFloatKey = [this](const juce::KeyPress& k) { return keyPressed(k); };
    propsPane_->onFloatKeyState = [this](bool down) { return keyStateChanged(down); };
    propsPane_->onAddOrganism = [this](const std::string& cls) {
        auto centre = patcherView_.getViewArea().getCentre();
        addAt(cls, {centre.x - 75, centre.y - 36});
    };
    canvas_->onOpenPluginUI = [this](const std::string& n) { openPluginUI(n); };
    canvas_->onOpenVisuals = [this](const std::string& n) { openVisualUI(n); };
    canvas_->onRecordNode = [this](const std::string& tk) {
        refreshTimelinePanes();
        if (!viewAutomation_.getToggleState())
            viewAutomation_.setToggleState(true, juce::sendNotification);
        setStatus(juce::String::fromUTF8("Armed \xe2\x80\x9c") + tk
                  + juce::String::fromUTF8("\xe2\x80\x9d - press \xe2\x8f\xba Record, then \xe2\x96\xb6 Play to capture the take"));
    };
    host_.onBeforeRebuild = [this](const std::function<bool(const std::string&)>& survives) {
        for (auto it = pluginWindows_.begin(); it != pluginWindows_.end();)
            it = survives(it->first) ? std::next(it) : pluginWindows_.erase(it);
        propsPane_->releaseEmbeddedEditorsWhere(
            [&](const std::string& n) { return !survives(n); });
    };
    host_.openVisuals = [this](const std::string& n) { openVisualUI(n); };
    addChildComponent(cards_);
    host_.showCard = [this](std::unique_ptr<juce::Component> card) {
        cards_.push(std::move(card));
        placeUpdateNotice();
        cards_.toFront(false);
    };
    deviceWatch_ = std::make_unique<DeviceWatch>(host_.audioDevices());
    deviceWatch_->onChange = [this](const DeviceChange& c) { showDeviceNotice(c); };
    host_.openParameterControl = [this](const std::string& c, const std::string& p) {
        openParameterControl(c, p);
    };
    host_.onBuildFailed = [this](const std::string& e) {
        setStatus(tr("main.rebuild-failed", "the patch could not be rebuilt, the last good one keeps playing: ")
                  + juce::String(e));
    };
    host_.onTopologyChanged = [this] {
        canvas_->refresh();
        propsPane_->reload();
    };
    host_.onArrangementChanged = [this] {
        refreshTimelinePanes();
        if (!viewAutomation_.getToggleState())
            viewAutomation_.setToggleState(true, juce::sendNotification);
    };
    host_.onNodeRolled = [this](const std::string& n) { propsPane_->reloadValuesFor(n); };
    host_.onPanelEdit = [this](const std::string& n) { propsPane_->reloadValuesFor(n); };
    host_.gamepads().setEnabled(AppSettings::instance().getInt("gamepad.enabled", 1) != 0);
    canvas_->onUndoRedo = [this] { propsPane_->reload(); refreshTimelinePanes(); };
    QwertyPiano::instance().emitDirect = [this](const juce::MidiMessage& m) {
        host_.injectLiveMidi(m);
    };
    host_.record().onSessionEnded = [this] { refreshTimelinePanes(); };
    host_.record().onStartRolling = [this] { ensureAudio(); host_.play(); };
    propsPane_->onAutomationChanged = [this] {
        refreshTimelinePanes();
        if (!viewAutomation_.getToggleState())
            viewAutomation_.setToggleState(true, juce::sendNotification);
    };

#if JUCE_MAC
    {
        juce::PopupMenu appleExtras;
        appleExtras.addItem(50, juce::String::fromUTF8("About Humus\xe2\x80\xa6"));
        appleExtras.addItem(53, juce::String::fromUTF8("Check for Updates\xe2\x80\xa6"));
        appleExtras.addSeparator();
        appleExtras.addItem(16, juce::String::fromUTF8("Settings\xe2\x80\xa6"));
        juce::MenuBarModel::setMacMainMenu(this, &appleExtras);
    }
#else
    addAndMakeVisible(menuBar_);
#endif

    buildTransportRow();
    buildWorkspaceRail();

    addAndMakeVisible(statusLabel_);
    statusLabel_.setColour(juce::Label::textColourId, Palette::textDim);

    applySavedAppearance();
    refreshAppearance();
    buildDock();
    restoreDock();

    if (AppSettings::instance().getInt("audio.enabled", 1) != 0)
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<MainComponent>(this)] {
            if (safe != nullptr && !safe->host_.audioRunning()) safe->ensureAudio();
        });

    if (!EngineHost::headless() && AppSettings::instance().getInt("midi.enabled", 1) != 0)
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<MainComponent>(this)] {
            if (safe != nullptr && !safe->host_.midi().enabled())
                safe->host_.midi().setEnabled(true);
        });

    MidiLearner::instance().onStatus =
        [safe = juce::Component::SafePointer<MainComponent>(this)](const juce::String& s) {
            if (safe != nullptr) safe->setStatus(s);
        };
    OscLearner::instance().onStatus = MidiLearner::instance().onStatus;
    FollowPicker::instance().onStatus = MidiLearner::instance().onStatus;

    host_.applyMidiSyncFromSettings();
    host_.applyLinkFromSettings();

    tickerId_ = UiTicker::instance().add([this] { timerCallback(); });

    if (juce::JUCEApplication::getInstance() != nullptr) {
        const auto marker = appDataDir().getChildFile("running.marker");
        crashedLastRun_ = marker.existsAsFile();
        marker.create();

        telemetrySyncConsent();
        telemetrySpool().addFromJson(telemetryFile().loadFileAsString());
        if (crashedLastRun_ && telemetrySpool().enabled())
            telemetrySpool().setCrashedLastRun(true);
        telemetryCount(telemetry::kBoot);
        sessionStartMs_ = juce::Time::currentTimeMillis();
        lastTelemetryFlushMs_ = sessionStartMs_;
        flushTelemetry();
    }

    startupCheckin();

    setSize(1180, 720);
}

void MainComponent::ensureKeyboardFocus() {
    if (!isShowing() || hasKeyboardFocus(true)) return;
    grabKeyboardFocus();
    if (hasKeyboardFocus(true)) return;
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<MainComponent>(this)] {
        if (safe != nullptr && safe->isShowing() && !safe->hasKeyboardFocus(true))
            safe->grabKeyboardFocus();
    });
}

void MainComponent::parentHierarchyChanged() { ensureKeyboardFocus(); }
void MainComponent::visibilityChanged() { ensureKeyboardFocus(); }

MainComponent::~MainComponent() {
#if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(nullptr);
#endif
    MidiLearner::instance().onStatus = {};
    OscLearner::instance().onStatus = {};
    FollowPicker::instance().cancel();
    FollowPicker::instance().onStatus = {};
    MidiLearner::instance().cancel();
    OscLearner::instance().cancel();
    UiTicker::instance().remove(tickerId_);
    stopTimer();
    persistDock();
    host_.stopAudio();
    if (juce::JUCEApplication::getInstance() != nullptr) {
        if (sessionStartMs_ > 0)
            telemetryCount(telemetry::kSessionMinutes,
                           (juce::Time::currentTimeMillis() - sessionStartMs_) / 60000);
        telemetrySave();
        appDataDir().getChildFile("running.marker").deleteFile();
    }
}

void MainComponent::addAt(const std::string& className, juce::Point<int> at) {
    auto name = host_.addOrganism(className, at, canvas_->scope());
    canvas_->select(name);
    propsPane_->openFor(name);
    canvas_->refresh();
}

void MainComponent::timerCallback() {
    playBtn_.setOn(host_.isPlaying());
    enableAudioBtn_.setOn(host_.audioRunning() || host_.audioStarting());
    enableMidiBtn_.setOn(host_.midi().enabled());
    if (viewParamControl_.getToggleState() != (paramControlWindow_ != nullptr))
        viewParamControl_.setToggleState(paramControlWindow_ != nullptr,
                                         juce::dontSendNotification);
    if (viewNotes_.getToggleState() != (notesWindow_ != nullptr))
        viewNotes_.setToggleState(notesWindow_ != nullptr, juce::dontSendNotification);
    if (viewLibrary_.getToggleState() != (libraryWindow_ != nullptr))
        viewLibrary_.setToggleState(libraryWindow_ != nullptr, juce::dontSendNotification);
    if (viewHelp_.getToggleState() != (helpWindow_ != nullptr))
        viewHelp_.setToggleState(helpWindow_ != nullptr, juce::dontSendNotification);
    if (viewDocSwitcher_.getToggleState() != (docSwitcherWindow_ != nullptr))
        viewDocSwitcher_.setToggleState(docSwitcherWindow_ != nullptr,
                                        juce::dontSendNotification);
    qwertyBtn_.setOn(QwertyPiano::instance().enabled());
    if (!masterLevel_.isMouseButtonDown()
        && std::abs(masterLevel_.getValue() - (double) host_.outputGain()) > 1e-4)
        masterLevel_.setValue(host_.outputGain(), juce::dontSendNotification);
    groove_.refresh();
    if (limiterBtn_.getToggleState() != host_.limiterEnabled())
        limiterBtn_.setToggleState(host_.limiterEnabled(), juce::dontSendNotification);
    {
        const bool glow = host_.limiterEnabled() && host_.limiterReduction() > 0.01f;
        if (glow != limGlowLit_) {
            limGlowLit_ = glow;
            limiterBtn_.setColour(juce::TextButton::textColourOnId,
                                  glow ? Palette::warnAmber() : Palette::accent);
        }
    }
    if (linkBtn_.getToggleState() != host_.linkEnabled())
        linkBtn_.setToggleState(host_.linkEnabled(), juce::dontSendNotification);
    {
        const int peers = host_.linkPeers();
        const auto label = peers > 0 ? juce::String(peers) + " Link" : juce::String("Link");
        if (linkBtn_.getButtonText() != label) linkBtn_.setButtonText(label);
    }
    UiTicker::instance().setBusy(host_.audioRunning());
    host_.pollMidiControl();
    host_.pollBridges();
    host_.pumpPluginTunings();
    host_.pollTuningProbes();
    host_.serviceCountIn();
    host_.pumpOscOut();
    host_.files().pollRecorders();
    if (sessionStartMs_ > 0 && telemetrySpool().enabled()) {
        const auto now = juce::Time::currentTimeMillis();
        if (TelemetrySpool::flushDue(now, lastTelemetryFlushMs_, true,
                                     telemetrySpool().empty()))
            flushTelemetry();
        if (now - lastTelemetrySaveMs_ >= 60'000 && !telemetrySpool().empty()) {
            lastTelemetrySaveMs_ = now;
            telemetrySave();
        }
    }
    undoBtn_.setEnabled(host_.canUndo());
    redoBtn_.setEnabled(host_.canRedo());
    const bool preRoll = host_.record().preRolling();
    const bool recArmed = host_.record().armed() || preRoll;
    const bool pending = preRoll || (recArmed && !host_.isPlaying());
    recordBtn_.setOn(recArmed && (!pending || (++recBlink_ / 15) % 2 == 0));
    keepBtn_.setEnabled(host_.isPlaying());
    loopBtn_.setOn(host_.automation().loopEnabled());
    clock_.setPosition(host_.positionBar(), host_.positionBeat(),
                       host_.positionSeconds());
    tsig_.repaint();
    if (tracksPane_) {
        tracksPane_->setLiveRecording(host_.isPlaying()
                                      && (host_.record().capturing()
                                          || host_.midi().anyRecordTarget()));
        tracksPane_->setPlaybackBeat(host_.positionBeats());
    }
    if (const unsigned ls = host_.laneStamp(); ls != lastLaneStamp_) {
        lastLaneStamp_ = ls;
        refreshTimelinePanes();
    }
    const unsigned liveGen = host_.liveControlGeneration();
    if (host_.isPlaying() || host_.routesAlive() || liveGen != lastLiveGen_) propsPane_->refreshLiveValues();
    lastLiveGen_ = liveGen;
    meter_.push(host_.outputLevel(0), host_.outputLevel(1));
    updateDspReadout();
    propsPane_->prune();
    if (const char* flip = std::getenv("HUM_DEBUG_FLIP")) {
        static int flipTicks = 0;
        ++flipTicks;
        if (flipTicks == 1) std::cerr << "[flip] armed for " << flip << std::endl;
        auto dumpBoxes = [this](const char* tag) {
            for (const auto& cm : host_.model().organisms)
                if (auto* b = propsPane_->boxFor(cm.name))
                    std::cerr << "[flip] " << tag << " " << cm.name << " "
                              << b->getBounds().toString()
                              << " h@w=" << b->heightAtWidth(b->getWidth()) << "\n";
            std::cerr << std::flush;
        };
        auto shoot = [this](const char* tag) {
#if JUCE_MAC
            const auto f = juce::File("/tmp/hum-flip-" + juce::String(tag) + ".png");
            std::cerr << "[flip] shot " << tag << " ok="
                      << (debugCaptureOwnWindow(getPeer(), f) ? 1 : 0) << std::endl;
#endif
        };
        if (flipTicks == 20) propsPane_->openFor(flip);
        if (flipTicks == 70) {
            dumpBoxes("before");
            shoot("before");
            host_.setEditorMode(flip, 0);
            if (auto* b = propsPane_->boxFor(flip)) b->reloadValues();
            dumpBoxes("after");
        }
        if (flipTicks == 100) {
            shoot("after");
#if JUCE_MAC
            std::cerr << "[flip] occlusion map:\n"
                      << debugDumpPeerSubviews(getPeer()) << std::endl;
#endif
        }
        if (flipTicks == 110) {
            dumpBoxes("late");
#if JUCE_MAC
            for (const auto& cm : host_.model().organisms)
                if (auto* hp = host_.hostedPluginFor(cm.name))
                    if (auto* act = hp->instance()->getActiveEditor())
                        std::cerr << "[flip] native " << cm.name << " painted="
                                  << embeddedEffectiveFrame(*act).toString()
                                  << std::endl;
#endif
        }
    }
    serviceTrackerFeeds();
    propsPane_->refreshPresetState();
    propsPane_->refreshTextEdits();
    if (const double bpm = host_.liveTempo();
        !tempo_.userDragging() && std::abs(tempo_.getValue() - bpm) > 0.01)
        tempo_.setValue(bpm, juce::dontSendNotification);
    if (const auto clock = host_.clockNodeNameIfAny(); !clock.empty())
        tempo_.setExternallyControlled(host_.isExternallyControlled(clock, kTempoParam));
    updateWindowTitle();
    autosaveTick();
    watchdog_.heartbeat();
}

}