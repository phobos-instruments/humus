#include "gui/MainComponent.h"

#include "core/PackRegistry.h"
#include "gui/AppSettings.h"
#include "gui/AssistantPane.h"
#include "gui/DocSwitcherView.h"
#include "gui/HelpBrowser.h"
#include "gui/LibraryPane.h"
#include "gui/NotesView.h"
#include "gui/ParameterControlView.h"
#include "gui/SettingsComponent.h"
#include "gui/Localisation.h"

namespace hum {

void MainComponent::buildWorkspaceRail() {
    IconButton* paneButtons[] = {&viewPatcher_, &viewProperties_, &viewAutomation_};
    IconButton* windowButtons[] = {&viewMetapad_, &viewParamControl_, &viewNotes_,
                                   &viewDocSwitcher_, &viewHelp_, &viewLibrary_};
    for (auto* b : paneButtons) b->setRail(IconButton::Rail::Pane);
    for (auto* b : windowButtons) b->setRail(IconButton::Rail::Window);
    viewPatcher_.setGlyph(IconGlyph::DockCentre);
    viewProperties_.setGlyph(IconGlyph::DockRight);
    viewAutomation_.setGlyph(IconGlyph::DockBottom);
    for (auto* b : paneButtons) {
        addAndMakeVisible(*b);
        b->setClickingTogglesState(true);
    }
    for (auto* b : windowButtons) {
        addAndMakeVisible(*b);
        b->setClickingTogglesState(true);
    }
    settingsBtn_.setRail(IconButton::Rail::Window);
    addAndMakeVisible(settingsBtn_);
    settingsBtn_.onClick = [this] { openSettings(); };
    viewPatcher_.setToggleState(true, juce::dontSendNotification);
    viewProperties_.setToggleState(true, juce::dontSendNotification);
    viewAutomation_.setToggleState(true, juce::dontSendNotification);
    viewPatcher_.onClick = [this] { updateDock(); persistDock(); };
    viewProperties_.onClick = [this] { updateDock(); persistDock(); };
    viewAutomation_.onClick = [this] { updateDock(); persistDock(); };
    viewMetapad_.onClick = [this] { toggleMetapad(); };
    viewParamControl_.onClick = [this] {
        if (viewParamControl_.getToggleState()) openParameterControl();
        else paramControlWindow_.reset();
    };
    viewNotes_.onClick = [this] {
        if (viewNotes_.getToggleState()) openNotes();
        else notesWindow_.reset();
    };
    viewHelp_.onClick = [this] {
        if (viewHelp_.getToggleState()) openHelpBrowser();
        else helpWindow_.reset();
    };
    viewLibrary_.onClick = [this] {
        if (viewLibrary_.getToggleState()) openLibrary();
        else libraryWindow_.reset();
    };
    viewDocSwitcher_.onClick = [this] {
        if (viewDocSwitcher_.getToggleState()) openDocSwitcher();
        else docSwitcherWindow_.reset();
    };
}

void MainComponent::openSettings(int category) {
    if (wizard_) {
        wizard_->toFront(true);
        return;
    }
    if (settingsWindow_ != nullptr) {
        if (category >= 0)
            if (auto* sc = dynamic_cast<SettingsComponent*>(settingsWindow_->getContentComponent()))
                sc->selectCategory(category);
        settingsWindow_->toFront(true);
        return;
    }
    auto content = std::make_unique<SettingsComponent>([this] { refreshAppearance(); }, &host_);
    content->onPacksChanged = [this] {
        juce::StringArray disabled;
        for (const auto& pack : PackRegistry::instance().packs())
            if (!pack.enabled) disabled.add(pack.manifest.id);
        AppSettings::instance().set("packs.disabled", disabled.joinIntoString(","));
        host_.applyPackChanges();
    };
    content->onPluginsChanged = [] {};
    if (category >= 0) content->selectCategory(category);
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned(content.release());
    o.dialogTitle = "Settings";
    o.dialogBackgroundColour = Palette::background;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    o.resizable = false;
    settingsWindow_ = o.launchAsync();
}

void MainComponent::wireGlobalKeys(FreeWindow& w) {
    w.onUnhandledKey = [this](const juce::KeyPress& k) { return keyPressed(k); };
    w.onUnhandledKeyState = [this](bool down) { return keyStateChanged(down); };
}

void MainComponent::openParameterControl(const std::string& organism,
                                         const std::string& param) {
    if (!paramControlWindow_) {
        paramControlWindow_ = std::make_unique<FreeWindow>(tr("main-windows.parameter-control", "Parameter Control"),
                                                           new ParameterControlView(host_));
        paramControlWindow_->onClose = [this] { paramControlWindow_.reset(); };
        wireGlobalKeys(*paramControlWindow_);
    }
    paramControlWindow_->setVisible(true);
    paramControlWindow_->toFront(true);
    if (!organism.empty())
        if (auto* v = dynamic_cast<ParameterControlView*>(paramControlWindow_->getContentComponent()))
            v->selectParam(organism, param);
}

void MainComponent::openHelpBrowser() {
    if (!helpWindow_) {
        auto* browser = new HelpBrowser();
        if (const auto& sel = canvas_->selected(); !sel.empty())
            if (const auto* cm = host_.model().byName(sel)) browser->showClass(cm->displayClass);
        helpWindow_ = std::make_unique<FreeWindow>("Help", browser);
        helpWindow_->onClose = [this] { helpWindow_.reset(); };
        wireGlobalKeys(*helpWindow_);
    }
    helpWindow_->setVisible(true);
    helpWindow_->toFront(true);
}

void MainComponent::openNotes() {
    if (!notesWindow_) {
        notesWindow_ = std::make_unique<FreeWindow>("Notes", new NotesView(host_));
        notesWindow_->onClose = [this] { notesWindow_.reset(); };
        wireGlobalKeys(*notesWindow_);
    }
    notesWindow_->setVisible(true);
    notesWindow_->toFront(true);
}

void MainComponent::openLibrary() {
    if (!libraryWindow_) {
        libraryWindow_ = std::make_unique<FreeWindow>("Library", new LibraryPane());
        libraryWindow_->onClose = [this] { libraryWindow_.reset(); };
        wireGlobalKeys(*libraryWindow_);
    }
    libraryWindow_->setVisible(true);
    libraryWindow_->toFront(true);
}

void MainComponent::openDocSwitcher() {
    if (!docSwitcherWindow_) {
        DocSwitcherView::Actions a;
        a.openDocument = [this](const juce::File& f) {
            confirmDiscardThenRun([this, f] { openFileAt(f); });
        };
        a.currentPath = [this] { return currentFile_; };
        docSwitcherWindow_ = std::make_unique<FreeWindow>(tr("main-windows.document-switcher", "Document Switcher"),
                                                          new DocSwitcherView(std::move(a)));
        docSwitcherWindow_->onClose = [this] { docSwitcherWindow_.reset(); };
        wireGlobalKeys(*docSwitcherWindow_);
    }
    docSwitcherWindow_->setVisible(true);
    docSwitcherWindow_->toFront(true);
}

void MainComponent::openAudioSettings() { openSettings(SettingsComponent::kAudio); }

void MainComponent::openAssistant() {
    if (!assistantWin_) {
        assistantWin_ = std::make_unique<AssistantWindow>(host_, [this] {
            canvas_->refresh();
            propsPane_->reload();
            refreshTimelinePanes();
            setStatus(tr("main-windows.assistant-edited-the-patch-one", "Assistant edited the patch (one undo step)"));
        });
    }
    assistantWin_->setVisible(true);
    assistantWin_->toFront(true);
}

void MainComponent::toggleMetapad() {
    if (viewMetapad_.getToggleState()) {
        if (!metaWindow_) {
            metaWindow_ = std::make_unique<MetapadWindow>(host_);
            metaWindow_->onClose = [this] {
                viewMetapad_.setToggleState(false, juce::dontSendNotification);
                metaWindow_.reset();
            };
            wireGlobalKeys(*metaWindow_);
        }
        metaWindow_->setVisible(true);
        metaWindow_->toFront(true);
    } else {
        metaWindow_.reset();
    }
}

}
