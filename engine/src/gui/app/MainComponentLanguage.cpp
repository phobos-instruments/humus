// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"

#include "gui/app/FreeWindow.h"
#include "gui/properties/MetapadView.h"
#include "gui/settings/SettingsComponent.h"
#include "gui/common/Localisation.h"

namespace hum {

namespace {

void name(juce::Button& b, const juce::String& text) {
    b.setTooltip(text);
    b.setButtonText(text);
}

}

void MainComponent::applyTexts() {
    name(undoBtn_, tr("main.undo", "Undo (Ctrl+Z)"));
    name(redoBtn_, tr("main.redo", "Redo (Ctrl+Shift+Z)"));
    name(playFromStartBtn_, tr("main.play-from-start", "Play From Start"));
    name(playBtn_, tr("main.play-space", "Play (Space)"));
    name(stopBtn_, tr("main.stop", "Stop"));
    name(recordBtn_, tr("main.record-the-performance", "Record the performance"));
    name(panicBtn_, tr("main.panic", "Panic - stop every hanging note"));
    name(keepBtn_, tr("main.keep-the-last-8-bars", "Keep the last 8 bars"));
    name(goStartBtn_, tr("main.go-to-start", "Go to start"));
    name(goEndBtn_, tr("main.go-to-end", "Go to end"));
    name(loopBtn_, tr("main.enable-automation-loop", "Enable Automation Loop"));
    name(enableAudioBtn_, tr("main.audio-on-off", "Audio on or off"));
    name(enableMidiBtn_, tr("main.midi-on-off", "MIDI on or off"));
    name(qwertyBtn_, tr("main.play-notes-from-the-computer-keyboard", "Play notes from the computer keyboard"));
    name(globalDiceBtn_, tr("main.randomise-everything", "Randomise everything"));
    name(overflowBtn_, tr("main.toolbar-controls-that-do-not-fit", "Toolbar controls that do not fit at this window width"));
    name(viewPatcher_, tr("main-dock.patcher", "Patcher"));
    name(viewProperties_, tr("main-dock.properties", "Properties"));
    name(viewAutomation_, tr("main-dock.timeline", "Timeline"));
    name(viewMetapad_, tr("main.metapad-rail", "Metapad (morph the whole patch between snapshots)"));
    name(viewParamControl_, tr("main.parameter-control-rail", "Parameter Control (MIDI / OSC / modulation and follow routes, F3)"));
    name(viewNotes_, tr("main.notes-free-text-saved-with", "Notes (free text, saved with the patch)"));
    name(viewDocSwitcher_, tr("main.document-switcher-rail", "Document Switcher (a set of patches for live switching, F9)"));
    name(viewHelp_, tr("main.help-browse-every-organism-s", "Help (browse every organism's reference)"));
    name(viewLibrary_, tr("main.library-rail", "Library (your samples and impulses - drag onto file slots)"));
    name(settingsBtn_, tr("main.settings", "Settings"));

    tempo_.setTooltip(tr("main-transport.tempo-drag-to-change", "Tempo - drag to change"));
    tapBtn_.setButtonText(tr("main-transport.tap", "Tap"));
    tapBtn_.setTooltip(tr("main-transport.tap-tempo-tip", "Tap tempo"));
    beat1Btn_.setTooltip(tr("main-transport.mark-now-as-beat-1", "Mark now as beat 1"));
    metroBtn_.setButtonText(tr("main-transport.click", "Click"));
    metroBtn_.setTooltip(tr("main-transport.metronome", "Metronome"));
    linkBtn_.setTooltip(tr("main-transport.link-tip", "Ableton Link - the number is connected peers"));
    masterCaption_.setText(tr("main-transport.out", "Out"), juce::dontSendNotification);
    masterLevel_.setTooltip(tr("main-transport.master-output-level", "Master output level"));
    masterLevel_.paramLabel = tr("main-transport.master-out", "Master out");
    limiterBtn_.setTooltip(tr("main-transport.master-limiter-tip", "Master limiter - amber while it is working"));
    dspLabel_.setTooltip(tr("main-transport.audio-load", "Audio load, and audio glitches since the start after the ! (humus.log says which kind)"));
    groove_.applyTexts();
    tsig_.applyTexts();

    paneCenter_.setTitle(tr("main-dock.patcher", "Patcher"));
    paneRight_.setTitle(tr("main-dock.properties", "Properties"));
    paneBottom_.setTitle(tr("main-dock.timeline", "Timeline"));
    for (auto* pane : {&paneCenter_, &paneRight_, &paneBottom_}) pane->applyTexts();
}

void MainComponent::installMacMenu() {
#if JUCE_MAC
    juce::PopupMenu appleExtras;
    appleExtras.addItem(50, tr("main.about-humus", "About Humus..."));
    appleExtras.addItem(53, tr("main.check-for-updates", "Check for Updates..."));
    appleExtras.addSeparator();
    appleExtras.addItem(16, tr("main.settings-menu", "Settings..."));
    juce::MenuBarModel::setMacMainMenu(this, &appleExtras);
#endif
}

void MainComponent::reopenRailWindows() {
    auto reopen = [](IconButton& rail, auto& window) {
        if (!rail.getToggleState() || window == nullptr) return;
        const auto at = window->getBounds();
        rail.setToggleState(false, juce::sendNotification);
        window.reset();
        rail.setToggleState(true, juce::sendNotification);
        if (window != nullptr) window->setBounds(at);
    };
    reopen(viewMetapad_, metaWindow_);
    reopen(viewParamControl_, paramControlWindow_);
    reopen(viewNotes_, notesWindow_);
    reopen(viewDocSwitcher_, docSwitcherWindow_);
    reopen(viewHelp_, helpWindow_);
    reopen(viewLibrary_, libraryWindow_);
}

void MainComponent::changeLanguage() {
    juce::PopupMenu::dismissAllActiveMenus();
    applyTexts();
#if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(nullptr);
#endif
    installMacMenu();
    rebuildPanes();
    reopenRailWindows();
    menuItemsChanged();
    if (settingsWindow_ != nullptr)
        if (auto* current = dynamic_cast<SettingsComponent*>(settingsWindow_->getContentComponent()))
            settingsWindow_->setContentOwned(makeSettingsContent(current->category()).release(), false);
    refreshAppearance();
    resized();
}

}
