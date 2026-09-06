#include "gui/MainComponent.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

#include "core/Categories.h"
#include "hum/Registry.h"
#include "core/PackLoader.h"
#include "core/ClassString.h"
#include "core/BridgeRing.h"
#include "core/HostedPlugin.h"
#include "core/PluginHost.h"
#include "core/PluginListStore.h"
#include "core/PackRegistry.h"
#include "core/AppPaths.h"
#include "gui/AppSettings.h"
#include "gui/AssistantPane.h"
#include "gui/HubClient.h"
#include "gui/Telemetry.h"
#include "gui/TelemetryEvents.h"
#include "gui/DefaultPatchHandler.h"
#include "gui/LookAndFeel.h"
#include "gui/PluginBridgePolicy.h"
#include "gui/PluginEditorTeardown.h"
#include "gui/NodeRandomize.h"
#include "gui/QwertyPiano.h"
#include "gui/SettingsComponent.h"
#include "gui/Localisation.h"

namespace hum {

#if JUCE_MAC
void beginAppKeepAwake();
#endif

#if JUCE_MAC
juce::String nativeWindowTitle(juce::ComponentPeer*);
juce::Rectangle<int> embeddedEffectiveFrame(juce::Component&);
bool debugCaptureOwnWindow(juce::ComponentPeer*, const juce::File&);
juce::String debugDumpPeerSubviews(juce::ComponentPeer*);
#endif

MainComponent::MainComponent()
    : watchdog_(UiWatchdog::defaultBreadcrumb(), UiWatchdog::defaultLog(),
                AppSettings::instance().getInt("watchdog.stallSec", 10) * 1000) {
    setWantsKeyboardFocus(true);
#if JUCE_MAC
    beginAppKeepAwake();
#endif
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
    host_.openParameterControl = [this](const std::string& c, const std::string& p) {
        openParameterControl(c, p);
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
    const bool recArmed = host_.record().armed();
    const bool pending = recArmed && !host_.isPlaying();
    recordBtn_.setOn(recArmed && (!pending || (++recBlink_ / 15) % 2 == 0));
    keepBtn_.setEnabled(host_.isPlaying());
    loopBtn_.setOn(host_.automation().loopEnabled());
    clock_.setPosition(host_.positionBar(), host_.positionBeat(),
                       host_.positionSeconds());
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
    if (host_.isPlaying() || liveGen != lastLiveGen_) propsPane_->refreshLiveValues();
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
#if JUCE_MAC
    if (auto* peer = w->getPeer(); peer != nullptr && nativeWindowTitle(peer) != t)
        peer->setTitle(t);
#endif
}

}