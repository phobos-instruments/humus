// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/AppearanceSettingsView.h"
#include "gui/common/Localisation.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include "gui/app/AppSettings.h"
#include "gui/settings/ThemeStore.h"

namespace hum {

void AppearanceSettingsView::promptSaveTheme() {
    auto& s = AppSettings::instance();
    const juce::String initial = s.getString("themeMode") == "user"
                                     ? s.getString("themeName") : juce::String("My Theme");
    auto* aw = new juce::AlertWindow("Save Theme",
                                     "Name this theme. Saving to an existing name replaces it.",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", initial);
    aw->addButton(tr("settings.save", "Save"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton(tr("settings.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe = juce::Component::SafePointer<AppearanceSettingsView>(this), aw](int r) {
            const juce::String name = aw->getTextEditorContents("name").trim();
            if (r != 1 || name.isEmpty() || safe == nullptr) return;
            themestore::save(name, safe->coloursFromSwatches());
            auto& st = AppSettings::instance();
            st.set("themeMode", juce::String("user"));
            st.set("themeName", name);
            safe->rebuildThemeCombo();
            const auto themes = themestore::list();
            for (size_t i = 0; i < themes.size(); ++i)
                if (themes[i].name == name)
                    safe->themeCombo_.setSelectedId(kUserThemeBase + (int) i,
                                                    juce::dontSendNotification);
            safe->updateThemeButtons();
        }), true);
}

void AppearanceSettingsView::exportTheme() {
    auto& s = AppSettings::instance();
    const auto mode = s.getString("themeMode", "preset");
    const juce::String nm = mode == "user"
                                ? s.getString("themeName")
                            : mode == "preset"
                                ? juce::String(themeName(s.getInt("themeIndex", 0)))
                                : juce::String(tr("settings.my-theme", "My Theme"));
    chooser_ = std::make_unique<juce::FileChooser>(
        "Export theme",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile(juce::File::createLegalFileName(nm) + ".humtheme"),
        "*.humtheme");
    chooser_->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe = juce::Component::SafePointer<AppearanceSettingsView>(this)](
            const juce::FileChooser& fc) {
            auto f = fc.getResult();
            if (f == juce::File() || safe == nullptr) return;
            if (!f.hasFileExtension("humtheme")) f = f.withFileExtension("humtheme");
            f.replaceWithText(juce::JSON::toString(themestore::themeFileVar(
                {f.getFileNameWithoutExtension(), safe->coloursFromSwatches()})));
        });
}

void AppearanceSettingsView::importTheme() {
    chooser_ = std::make_unique<juce::FileChooser>(
        "Import theme",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.humtheme");
    chooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<AppearanceSettingsView>(this)](
            const juce::FileChooser& fc) {
            const auto f = fc.getResult();
            if (f == juce::File() || safe == nullptr) return;
            themestore::UserTheme t;
            if (!themestore::parseThemeFile(juce::JSON::parse(f.loadFileAsString()), t,
                                            presetColours(0), f.getFileNameWithoutExtension())) {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon, "Import theme",
                    "This is not a Humus theme file.");
                return;
            }
            themestore::save(t.name, t.colours);
            auto& s = AppSettings::instance();
            s.set("themeMode", juce::String("user"));
            s.set("themeName", t.name);
            setBaseColours(t.colours);
            safe->setSwatches(t.colours);
            safe->rebuildThemeCombo();
            const auto themes = themestore::list();
            for (size_t i = 0; i < themes.size(); ++i)
                if (themes[i].name == t.name)
                    safe->themeCombo_.setSelectedId(kUserThemeBase + (int) i,
                                                    juce::dontSendNotification);
            safe->updateThemeButtons();
            safe->pushAppearance();
        });
}

}
