#include "gui/MainComponent.h"

#include "gui/SampleLab.h"
#include "gui/SettingsComponent.h"

#include "hum/Capabilities.h"

namespace hum {

static std::vector<std::string> videoOutNames(EngineHost& host) {
    std::vector<std::string> out;
    for (const auto& c : host.model().organisms) {
        auto* v = dynamic_cast<VideoNode*>(host.liveOrganism(c.name));
        if (v != nullptr && v->numVideoOutputs() == 0 && v->numVideoInputs() > 0
            && dynamic_cast<VisualSource*>(host.liveOrganism(c.name)) == nullptr)
            out.push_back(c.name);
    }
    return out;
}

juce::StringArray MainComponent::getMenuBarNames() {
    return {"File", "Edit", "Control", "Help"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int index, const juce::String&) {
    juce::PopupMenu m;
    if (index == 0) {
#if JUCE_MAC
        m.addItem(1, "New (Cmd+N)");
        m.addItem(2, "Open... (Cmd+O)");
#else
        m.addItem(1, "New (Ctrl+N)");
        m.addItem(2, "Open... (Ctrl+O)");
#endif
        {
            juce::PopupMenu recent;
            recentMenu_ = recents::get();
            while (recentMenu_.size() > 10) recentMenu_.remove(recentMenu_.size() - 1);
            const auto names = recents::labels(recentMenu_);
            for (int i = 0; i < recentMenu_.size(); ++i)
                recent.addItem(recents::kMenuIdBase + i, names[i]);
            recent.addSeparator();
            recent.addItem(59, "Clear Menu", !recentMenu_.isEmpty());
            m.addSubMenu("Open Recent", recent, !recentMenu_.isEmpty());
        }
        m.addSeparator();
#if JUCE_MAC
        m.addItem(3, "Save (Cmd+S)", !currentFile_.isEmpty());
        m.addItem(4, "Save As... (Cmd+Shift+S)");
#else
        m.addItem(3, "Save (Ctrl+S)", !currentFile_.isEmpty());
        m.addItem(4, "Save As... (Ctrl+Shift+S)");
#endif
        m.addItem(5, "Export to Sound File...");
        m.addSeparator();
        m.addItem(6, "Revert", !currentFile_.isEmpty());
#if JUCE_MAC
        m.addItem(8, "Close Project (Cmd+W)");
#else
        m.addItem(8, "Close Project (Ctrl+W)");
#endif
#if !JUCE_MAC
        m.addSeparator();
        m.addItem(7, "Quit");
#endif
    } else if (index == 1) {
        m.addItem(15, "Undo", host_.canUndo());
        m.addItem(17, "Redo", host_.canRedo());
        m.addSeparator();
        m.addItem(14, "Cut");
        m.addItem(10, "Copy");
        m.addItem(11, "Paste");
        m.addItem(12, "Duplicate");
        m.addItem(13, "Clear");
        m.addItem(18, "Select All");
        m.addItem(34, "Rename Organism... (Cmd+R)", !canvas_->selected().empty());
        m.addSeparator();
        m.addItem(33, "Auto-arrange");
#if !JUCE_MAC
        m.addSeparator();
        m.addItem(16, "Settings...");
#endif
    } else if (index == 2) {
        m.addItem(20, "Enable Audio", true, host_.audioRunning());
        m.addItem(27, "Audio Settings...");
        m.addItem(60, "Enable MIDI", true, host_.midi().enabled());
        m.addItem(38, "Parameter Control... (F3)");
        m.addItem(31, "Generate MIDI Clock", true,
                  host_.midiSyncMode() == EngineHost::kSyncGenerate);
        m.addItem(32, "Chase MIDI Clock", true,
                  host_.midiSyncMode() == EngineHost::kSyncChase);
        m.addItem(25, "Capture Performance", true, host_.record().armed());
        m.addItem(37, "Keep Last 8 Bars (retroactive)", host_.isPlaying());
        m.addItem(28, host_.isMixRecording() ? "Stop Recording Master Mix" : "Record Master Mix...",
                  true, host_.isMixRecording());
        m.addSeparator();
        m.addItem(21, "Play From Start");
        m.addItem(22, host_.isPlaying() ? "Stop" : "Play");
        m.addItem(24, "Go to Start");
        m.addSeparator();
        double f, t;
        const bool sel = tracksPane_ && tracksPane_->timeSelection(f, t);
        m.addItem(40, "Loop Selection", sel);
        m.addItem(26, host_.automation().loopEnabled() ? "Disable Loop" : "Enable Loop");
        m.addSeparator();
        juce::PopupMenu autoEdit;
        autoEdit.addItem(41, "Cut Time", sel);
        autoEdit.addItem(42, "Copy Time", sel);
        autoEdit.addItem(43, "Paste Time", host_.automation().hasClip());
        autoEdit.addItem(44, "Insert Time", sel);
        autoEdit.addItem(45, "Delete Time", sel);
        autoEdit.addItem(46, "Clear", sel);
        m.addSubMenu("Automation", autoEdit);
        m.addSeparator();
        m.addSeparator();
        {
            juce::PopupMenu vids;
            const auto outs = videoOutNames(host_);
            for (int i = 0; i < (int) outs.size() && i < 10; ++i)
                vids.addItem(70 + i, juce::String(outs[(size_t) i]));
            m.addSubMenu("Video Outputs", vids, !outs.empty());
        }
        m.addSeparator();
        m.addItem(29, "AI Assistant...");
        m.addItem(30, "AI Sample Lab...");
    } else if (index == 3) {
        m.addItem(55, "Humus Help");
        m.addItem(52, "Meet Humus...");
        m.addItem(51, "Setup Wizard...");
        m.addSeparator();
        m.addItem(54, "Enter License...");
#if !JUCE_MAC
        m.addItem(53, "Check for Updates...");
        m.addSeparator();
        m.addItem(50, "About Humus...");
#endif
    }
    return m;
}

void MainComponent::menuItemSelected(int id, int) {
    if (id >= 70 && id <= 79) {
        const auto outs = videoOutNames(host_);
        if (id - 70 < (int) outs.size()) openVisualUI(outs[(size_t) (id - 70)]);
        return;
    }
    switch (id) {
        case 1: newPatch(); break;
        case 2: openPatch(); break;
        case 3: savePatch(); break;
        case 4: savePatchAs(); break;
        case 5: exportSound(); break;
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
        case 10: canvas_->copySelection(); break;
        case 14: canvas_->cutSelection(); break;
        case 11: canvas_->pasteClipboard(); break;
        case 12: canvas_->duplicateSelection(); break;
        case 13: canvas_->deleteSelection(); break;
        case 18: canvas_->selectAll(); break;
        case 34: canvas_->renameSelection(); break;
        case 33:
            canvas_->autoArrange();
            setStatus("arranged as a top-down flow (Undo restores the old layout)");
            break;
        case 20: toggleAudio(); break;
        case 27: openAudioSettings(); break;
        case 60: toggleMidi(); break;
        case 38: openParameterControl(); break;
        case 31:
            host_.setMidiSyncMode(host_.midiSyncMode() == EngineHost::kSyncGenerate
                                      ? EngineHost::kSyncOff : EngineHost::kSyncGenerate);
            setStatus(host_.midiSyncMode() == EngineHost::kSyncGenerate
                          ? "sending MIDI clock" : "MIDI clock off");
            break;
        case 32:
            host_.setMidiSyncMode(host_.midiSyncMode() == EngineHost::kSyncChase
                                      ? EngineHost::kSyncOff : EngineHost::kSyncChase);
            setStatus(host_.midiSyncMode() == EngineHost::kSyncChase
                          ? "chasing incoming MIDI clock" : "MIDI clock off");
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
    aboutWin_ = std::make_unique<StartWindow>(std::move(a));
    aboutWin_->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
    aboutWin_->setCentrePosition(getScreenBounds().getCentre());
    aboutWin_->setVisible(true);
    aboutWin_->toFront(true);
}

}
