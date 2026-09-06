#include "gui/AboutCredits.h"
#include "gui/MainComponent.h"

#include "gui/VisualPlanBuilder.h"

#include "gui/SampleLab.h"
#include "gui/SettingsComponent.h"

#include "hum/Capabilities.h"
#include "gui/Localisation.h"

namespace hum {

static std::vector<std::string> videoOutNames(EngineHost& host) {
    std::vector<std::string> out;
    for (const auto& c : host.model().organisms)
        if (isVideoOutputNode(host, c.name)) out.push_back(c.name);
    return out;
}

juce::StringArray MainComponent::getMenuBarNames() {
    return {tr("menu.file", "File"), tr("menu.edit", "Edit"), tr("menu.control", "Control"), tr("menu.help", "Help")};
}

juce::PopupMenu MainComponent::getMenuForIndex(int index, const juce::String&) {
    juce::PopupMenu m;
    if (index == 0) {
#if JUCE_MAC
        m.addItem(1, tr("menu.new-mac", "New (Cmd+N)"));
        m.addItem(2, tr("menu.open-mac", "Open... (Cmd+O)"));
#else
        m.addItem(1, tr("menu.new-win", "New (Ctrl+N)"));
        m.addItem(2, tr("menu.open-win", "Open... (Ctrl+O)"));
#endif
        {
            juce::PopupMenu recent;
            recentMenu_ = recents::get();
            while (recentMenu_.size() > 10) recentMenu_.remove(recentMenu_.size() - 1);
            const auto names = recents::labels(recentMenu_);
            for (int i = 0; i < recentMenu_.size(); ++i)
                recent.addItem(recents::kMenuIdBase + i, names[i]);
            recent.addSeparator();
            recent.addItem(59, tr("menu.clear-menu", "Clear Menu"), !recentMenu_.isEmpty());
            m.addSubMenu(tr("menu.open-recent", "Open Recent"), recent, !recentMenu_.isEmpty());
        }
        m.addSeparator();
#if JUCE_MAC
        m.addItem(3, tr("menu.save-mac", "Save (Cmd+S)"), !currentFile_.isEmpty());
        m.addItem(4, tr("menu.save-as-mac", "Save As... (Cmd+Shift+S)"));
#else
        m.addItem(3, tr("menu.save-win", "Save (Ctrl+S)"), !currentFile_.isEmpty());
        m.addItem(4, tr("menu.save-as-win", "Save As... (Ctrl+Shift+S)"));
#endif
        m.addItem(5, tr("menu.bounce", "Bounce..."));
        m.addSeparator();
        m.addItem(6, tr("menu.revert", "Revert"), !currentFile_.isEmpty());
#if JUCE_MAC
        m.addItem(8, tr("menu.close-project-mac", "Close Project (Cmd+W)"));
#else
        m.addItem(8, tr("menu.close-project-win", "Close Project (Ctrl+W)"));
#endif
#if !JUCE_MAC
        m.addSeparator();
        m.addItem(7, tr("menu.quit", "Quit"));
#endif
    } else if (index == 1) {
        m.addItem(15, tr("menu.undo", "Undo"), host_.canUndo());
        m.addItem(17, tr("menu.redo", "Redo"), host_.canRedo());
        m.addSeparator();
        m.addItem(14, tr("menu.cut", "Cut"));
        m.addItem(10, tr("menu.copy", "Copy"));
        m.addItem(11, tr("menu.paste", "Paste"));
        m.addItem(12, tr("menu.duplicate", "Duplicate"));
        m.addItem(13, tr("menu.clear", "Clear"));
        m.addItem(18, tr("menu.select-all", "Select All"));
        m.addItem(34, tr("menu.rename-organism-mac", "Rename Organism... (Cmd+R)"), !canvas_->selected().empty());
        m.addSeparator();
        m.addItem(33, tr("menu.auto-arrange", "Auto-arrange"));
#if !JUCE_MAC
        m.addSeparator();
        m.addItem(16, tr("menu.settings", "Settings..."));
#endif
    } else if (index == 2) {
        m.addItem(20, tr("menu.enable-audio", "Enable Audio"), true, host_.audioRunning());
        m.addItem(27, tr("menu.audio-settings", "Audio Settings..."));
        {
            juce::PopupMenu vids;
            const auto outs = videoOutNames(host_);
            for (int i = 0; i < (int) outs.size() && i < 10; ++i)
                vids.addItem(70 + i, juce::String(outs[(size_t) i]), true,
                             visualWindows_.count(outs[(size_t) i]) != 0);
            m.addSubMenu(tr("menu.video-outputs", "Video Outputs"), vids, !outs.empty());
        }
        m.addItem(60, tr("menu.enable-midi", "Enable MIDI"), true, host_.midi().enabled());
        m.addItem(38, tr("menu.parameter-control", "Parameter Control... (F3)"));
        m.addItem(31, tr("menu.generate-midi-clock", "Generate MIDI Clock"), true,
                  host_.midiSyncMode() == EngineHost::kSyncGenerate);
        m.addItem(32, tr("menu.chase-midi-clock", "Chase MIDI Clock"), true,
                  host_.midiSyncMode() == EngineHost::kSyncChase);
        m.addItem(25, tr("menu.capture-performance", "Capture Performance"), true, host_.record().armed());
        m.addItem(37, tr("menu.keep-last-8-bars-retroactive", "Keep Last 8 Bars (retroactive)"), host_.isPlaying());
        m.addItem(28, host_.isMixRecording() ? tr("menu.stop-recording-live-performance", "Stop Recording Live Performance") : tr("menu.record-live-performance", "Record Live Performance..."),
                  true, host_.isMixRecording());
        m.addSeparator();
        m.addItem(21, tr("menu.play-from-start", "Play From Start"));
        m.addItem(22, host_.isPlaying() ? tr("menu.stop", "Stop") : tr("menu.play", "Play"));
        m.addItem(24, tr("menu.go-to-start", "Go to Start"));
        m.addSeparator();
        double f, t;
        const bool sel = tracksPane_ && tracksPane_->timeSelection(f, t);
        m.addItem(40, tr("menu.loop-selection", "Loop Selection"), sel);
        m.addItem(26, host_.automation().loopEnabled() ? tr("menu.disable-loop", "Disable Loop") : tr("menu.enable-loop", "Enable Loop"));
        m.addSeparator();
        juce::PopupMenu autoEdit;
        autoEdit.addItem(41, tr("menu.cut-time", "Cut Time"), sel);
        autoEdit.addItem(42, tr("menu.copy-time", "Copy Time"), sel);
        autoEdit.addItem(43, tr("menu.paste-time", "Paste Time"), host_.automation().hasClip());
        autoEdit.addItem(44, tr("menu.insert-time", "Insert Time"), sel);
        autoEdit.addItem(45, tr("menu.delete-time", "Delete Time"), sel);
        autoEdit.addItem(46, tr("menu.clear", "Clear"), sel);
        m.addSubMenu(tr("menu.automation", "Automation"), autoEdit);
        m.addSeparator();
        m.addSeparator();
        m.addItem(29, tr("menu.ai-assistant", "AI Assistant..."));
        m.addItem(30, tr("menu.ai-sample-lab", "AI Sample Lab..."));
    } else if (index == 3) {
        m.addItem(55, tr("menu.humus-help", "Humus Help"));
        m.addItem(52, tr("menu.meet-humus", "Meet Humus..."));
        m.addItem(51, tr("menu.setup-wizard", "Setup Wizard..."));
        m.addSeparator();
        m.addItem(56, tr("menu.start-window", "Start Window..."));
        m.addItem(57, tr("menu.report-a-bug", "Report a Bug..."));
        m.addItem(54, tr("menu.enter-license", "Enter License..."));
#if !JUCE_MAC
        m.addItem(53, tr("menu.check-for-updates", "Check for Updates..."));
        m.addSeparator();
        m.addItem(50, tr("menu.about-humus", "About Humus..."));
#endif
    }
    return m;
}

void MainComponent::menuItemSelected(int id, int) {
    if (id >= 70 && id <= 79) {
        const auto outs = videoOutNames(host_);
        if (id - 70 < (int) outs.size()) {
            const auto& n = outs[(size_t) (id - 70)];
            if (visualWindows_.count(n) != 0) closeVisualUI(n);
            else openVisualUI(n);
        }
        return;
    }
    switch (id) {
        case 1: newPatch(); break;
        case 2: openPatch(); break;
        case 3: savePatch(); break;
        case 4: savePatchAs(); break;
        case 5: bounce(); break;
        case 6: revertPatch(); break;
        case 7: juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;
        case 8: closeProject(); break;
        case 15: canvas_->undo(); break;
        case 17: canvas_->redo(); break;
        case 16: openSettings(); break;
        case 51: openSetupWizard(false); break;
        case 52: openGuide(false); break;
        case 53: checkForUpdatesManually(); break;
        case 54: openSettings(SettingsComponent::kLicense); break;
        case 55: openHelpBrowser(); break;
        case 56: showStartWindow(); break;
        case 57: openBugReport(); break;
        case 10: canvas_->copySelection(); break;
        case 14: canvas_->cutSelection(); break;
        case 11: canvas_->pasteClipboard(); break;
        case 12: canvas_->duplicateSelection(); break;
        case 13: canvas_->deleteSelection(); break;
        case 18: canvas_->selectAll(); break;
        case 34: canvas_->renameSelection(); break;
        case 33:
            canvas_->autoArrange();
            setStatus(tr("menu.arranged-as-a-top-down", "arranged as a top-down flow (Undo restores the old layout)"));
            break;
        case 20: toggleAudio(); break;
        case 27: openAudioSettings(); break;
        case 60: toggleMidi(); break;
        case 38: openParameterControl(); break;
        case 31:
            host_.setMidiSyncMode(host_.midiSyncMode() == EngineHost::kSyncGenerate
                                      ? EngineHost::kSyncOff : EngineHost::kSyncGenerate);
            setStatus(host_.midiSyncMode() == EngineHost::kSyncGenerate
                          ? tr("menu.sending-midi-clock", "sending MIDI clock") : tr("menu.midi-clock-off", "MIDI clock off"));
            break;
        case 32:
            host_.setMidiSyncMode(host_.midiSyncMode() == EngineHost::kSyncChase
                                      ? EngineHost::kSyncOff : EngineHost::kSyncChase);
            setStatus(host_.midiSyncMode() == EngineHost::kSyncChase
                          ? tr("menu.chasing-incoming-midi-clock", "chasing incoming MIDI clock") : tr("menu.midi-clock-off", "MIDI clock off"));
            break;
        case 21: ensureAudio(); host_.playFromStart(); break;
        case 22: togglePlay(); break;
        case 24: host_.goToStart(); break;
        case 25: host_.record().captureToggle(); break;
        case 37: host_.keepLast(8); refreshTimelinePanes(); break;
        case 28: toggleMixRecording(); break;
        case 26: host_.automation().setLoop(host_.automation().loopStartBeat(), host_.automation().loopEndBeat(), !host_.automation().loopEnabled());
                 if (tracksPane_) tracksPane_->repaint(); break;
        case 40: { double f, t; if (tracksPane_ && tracksPane_->timeSelection(f, t)) {
                     host_.automation().setLoop(f, t, true); tracksPane_->repaint(); } } break;
        case 41: { double f, t; if (tracksPane_ && tracksPane_->timeSelection(f, t)) {
                     host_.automation().cutTimeRange(f, t); tracksPane_->rebuild(); } } break;
        case 42: { double f, t; if (tracksPane_ && tracksPane_->timeSelection(f, t)) host_.automation().copyTimeRange(f, t); } break;
        case 43: { double f, t; const double at = (tracksPane_ && tracksPane_->timeSelection(f, t)) ? f : 0.0;
                     if (host_.automation().pasteTimeRange(at) && tracksPane_) tracksPane_->rebuild(); } break;
        case 44: { double f, t; if (tracksPane_ && tracksPane_->timeSelection(f, t)) {
                     host_.automation().insertTime(f, t - f); tracksPane_->rebuild(); } } break;
        case 45: { double f, t; if (tracksPane_ && tracksPane_->timeSelection(f, t)) {
                     host_.automation().deleteTimeRange(f, t); tracksPane_->rebuild(); } } break;
        case 46: { double f, t; if (tracksPane_ && tracksPane_->timeSelection(f, t)) {
                     host_.automation().clearTimeRange(f, t); tracksPane_->rebuild(); } } break;
        case 29: openAssistant(); break;
        case 30:
            samplelab::open(host_, [this](juce::String s) {
                setStatus(s);
                canvas_->refresh();
                propsPane_->reload();
            });
            break;
        case 50: showAbout(); break;
        case 59: recents::clear(); break;
        default:
            if (id >= recents::kMenuIdBase && id < recents::kMenuIdBase + 10) {
                if (const auto f = recents::resolve(recentMenu_, id); f != juce::File())
                    confirmDiscardThenRun([this, f] { openFileAt(f); });
            }
            break;
    }
}

void MainComponent::showAbout() {
    if (aboutWin_) { aboutWin_->toFront(true); return; }
    auto dismiss = [this] {
        if (auto* w = aboutWin_.release())
            juce::MessageManager::callAsync([w] { delete w; });
    };
    AboutWindow::Actions a;
    a.onDismiss = dismiss;
    a.onEnterLicense = [this, dismiss] {
        dismiss();
        openSettings(SettingsComponent::kLicense);
    };
    aboutWin_ = std::make_unique<AboutWindow>(std::move(a));
    aboutWin_->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
    aboutWin_->setCentrePosition(getScreenBounds().getCentre());
    aboutWin_->setVisible(true);
    aboutWin_->toFront(true);
}

void MainComponent::showStartWindow() {
    if (startWin_) { startWin_->toFront(true); return; }
    auto dismiss = [this] {
        if (auto* w = startWin_.release())
            juce::MessageManager::callAsync([w] { delete w; });
    };
    StartWindow::Actions a;
    a.onDismiss = dismiss;
    a.onEnterLicense = [this, dismiss] {
        dismiss();
        openSettings(SettingsComponent::kLicense);
    };
    a.onNewSession = [this, dismiss] { dismiss(); newPatch(); };
    a.onOpenFile = [this, dismiss](juce::File f) {
        dismiss();
        confirmDiscardThenRun([this, f] { openFileAt(f); });
    };
    a.onOpenOther = [this, dismiss] { dismiss(); openPatch(); };
    a.onTour = [this, dismiss] { dismiss(); openGuide(false); };
    a.onWizard = [this, dismiss] { dismiss(); openSetupWizard(false); };
    a.onHelp = [this, dismiss] { dismiss(); openHelpBrowser(); };
    startWin_ = std::make_unique<StartWindow>(std::move(a));
    startWin_->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
    startWin_->setCentrePosition(getScreenBounds().getCentre());
    startWin_->setVisible(true);
    startWin_->toFront(true);
}

}

namespace hum {
namespace {
juce::URL bugReportUrl() {
#if JUCE_MAC && JUCE_ARM
    const char* platform = "macos-arm";
#elif JUCE_MAC
    const char* platform = "macos-intel";
#elif JUCE_WINDOWS
    const char* platform = "win10+";
#elif JUCE_LINUX
    const char* platform = "linux";
#else
    const char* platform = "other";
#endif
    auto* app = juce::JUCEApplication::getInstance();
    return juce::URL(juce::String(about::kSite) + "/report")
        .withParameter("version", app != nullptr ? app->getApplicationVersion() : juce::String())
        .withParameter("platform", platform)
        .withParameter("os", juce::SystemStats::getOperatingSystemName());
}
}

void MainComponent::openBugReport() { bugReportUrl().launchInDefaultBrowser(); }

}
