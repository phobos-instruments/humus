// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/AppearanceSettingsView.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include "gui/app/AppSettings.h"
#include "gui/common/Localisation.h"
#include "gui/patcher/PickerLauncher.h"
#include "gui/settings/ThemeStore.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
struct RoleName { const char* key; const char* name; };
const RoleName kRoleNames[9] = {{"settings.role-background", "Background"},
                                {"settings.role-panel", "Panel"},
                                {"settings.role-panel-light", "Panel Light"},
                                {"settings.role-border", "Border"},
                                {"settings.role-accent", "Accent"},
                                {"settings.role-accent-dim", "Accent Dim"},
                                {"settings.role-text", "Text"},
                                {"settings.role-text-dim", "Text Dim"},
                                {"settings.role-cord", "Cord"}};
constexpr int customComboId() { return 100; }
}

AppearanceSettingsView::AppearanceSettingsView(std::function<void()> onAppearanceChanged)
    : onAppearanceChanged_(std::move(onAppearanceChanged)) {
    title_.setText(tr("settings.graphics-and-ui", "Graphics & UI"), juce::dontSendNotification);
    title_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    addAndMakeVisible(title_);

    themeLabel_.setText(tr("settings.theme", "Theme"), juce::dontSendNotification);
    addAndMakeVisible(themeLabel_);
    rebuildThemeCombo();
    addAndMakeVisible(themeCombo_);
    themeCombo_.onChange = [this] {
        int id = themeCombo_.getSelectedId();
        auto& s = AppSettings::instance();
        if (id >= 1 && id <= numThemes()) {
            applyTheme(id - 1);
            s.set("themeMode", juce::String("preset"));
            s.set("themeIndex", id - 1);
            setSwatches(baseColours());
        } else if (id >= kUserThemeBase) {
            const auto themes = themestore::list();
            const size_t idx = (size_t) (id - kUserThemeBase);
            if (idx < themes.size()) {
                setBaseColours(themes[idx].colours);
                setSwatches(themes[idx].colours);
                s.set("themeMode", juce::String("user"));
                s.set("themeName", themes[idx].name);
            }
        } else {
            auto cols = themeColoursFromString(s.getString("customColours"), baseColours());
            setBaseColours(cols);
            setSwatches(cols);
            s.set("themeMode", juce::String("custom"));
        }
        updateThemeButtons();
        pushAppearance();
    };

    for (int r = 0; r < kNumRoles; ++r) {
        swatchLabels_[(size_t) r] = std::make_unique<juce::Label>();
        swatchLabels_[(size_t) r]->setText(tr(kRoleNames[r].key, kRoleNames[r].name),
                                          juce::dontSendNotification);
        swatchLabels_[(size_t) r]->setFont(juce::FontOptions(12.0f));
        addAndMakeVisible(*swatchLabels_[(size_t) r]);

        swatches_[(size_t) r] = std::make_unique<ColourSwatch>();
        swatches_[(size_t) r]->onChange = [this](juce::Colour) {
            auto cols = coloursFromSwatches();
            setBaseColours(cols);
            themeCombo_.setSelectedId(customComboId(), juce::dontSendNotification);
            auto& s = AppSettings::instance();
            s.set("themeMode", juce::String("custom"));
            s.set("customColours", themeColoursToString(cols));
            updateThemeButtons();
            pushAppearance();
        };
        addAndMakeVisible(*swatches_[(size_t) r]);
    }

    addAndMakeVisible(saveCustomBtn_);
    saveCustomBtn_.onClick = [this] { promptSaveTheme(); };
    addAndMakeVisible(deleteThemeBtn_);
    deleteThemeBtn_.onClick = [this] {
        const int id = themeCombo_.getSelectedId();
        if (id < kUserThemeBase) return;
        const auto themes = themestore::list();
        const size_t idx = (size_t) (id - kUserThemeBase);
        if (idx >= themes.size()) return;
        themestore::remove(themes[idx].name);
        auto& s = AppSettings::instance();
        s.set("themeMode", juce::String("custom"));
        s.set("customColours", themeColoursToString(coloursFromSwatches()));
        rebuildThemeCombo();
        themeCombo_.setSelectedId(customComboId(), juce::dontSendNotification);
        updateThemeButtons();
    };
    addAndMakeVisible(exportThemeBtn_);
    exportThemeBtn_.onClick = [this] { exportTheme(); };
    addAndMakeVisible(importThemeBtn_);
    importThemeBtn_.onClick = [this] { importTheme(); };

    auto setupAdj = [this](juce::Slider& sl, juce::Label& lab, const juce::String& name) {
        lab.setText(name, juce::dontSendNotification);
        addAndMakeVisible(lab);
        sl.setRange(-1.0, 1.0, 0.01);
        sl.setSliderStyle(juce::Slider::LinearHorizontal);
        sl.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        addAndMakeVisible(sl);
    };
    setupAdj(brightness_, brightnessLabel_, "Brightness");
    setupAdj(contrast_, contrastLabel_, "Contrast");
    brightness_.onValueChange = [this] {
        setAppearanceAdjustments(brightness_.getValue(), contrast_.getValue());
        AppSettings::instance().set("brightness", brightness_.getValue());
        pushAppearance();
    };
    contrast_.onValueChange = [this] {
        setAppearanceAdjustments(brightness_.getValue(), contrast_.getValue());
        AppSettings::instance().set("contrast", contrast_.getValue());
        pushAppearance();
    };
    languageLabel_.setText(tr("settings.language", "Language"), juce::dontSendNotification);
    addAndMakeVisible(languageLabel_);
    languageCombo_.addItem(tr("settings.english", "English"), 1);
    {
        const auto langs = i18n::available();
        for (int i = 0; i < langs.size(); ++i)
            languageCombo_.addItem(langs[i].name, i + 2);
        const auto saved = i18n::saved();
        int id = 1;
        for (int i = 0; i < langs.size(); ++i)
            if (langs[i].code == saved) id = i + 2;
        languageCombo_.setSelectedId(id, juce::dontSendNotification);
        languageCombo_.onChange = [this] {
            const auto found = i18n::available();
            const int i = languageCombo_.getSelectedId() - 2;
            i18n::choose(i >= 0 && i < found.size() ? found[i].code
                                                   : juce::String(i18n::kSystem));
        };
    }
    addAndMakeVisible(languageCombo_);

    flowLights_.setButtonText(tr("settings.flow-lights-nodes-glow-with", "Flow lights (nodes glow with audio, MIDI ports blink)"));
    flowLights_.setToggleState(AppSettings::instance().getInt("ui.flowLights", 1) != 0,
                               juce::dontSendNotification);
    addAndMakeVisible(flowLights_);
    flowLights_.onClick = [this] {
        AppSettings::instance().set("ui.flowLights", flowLights_.getToggleState() ? 1 : 0);
    };

    menuStyleLabel_.setText(tr("settings.creation-menus", "Creation menus"), juce::dontSendNotification);
    addAndMakeVisible(menuStyleLabel_);
    modernCard_.modern = true;
    classicCard_.selected = !modernMenusEnabled();
    modernCard_.selected = modernMenusEnabled();
    classicCard_.onSelect = [this] { applyMenuStyle(false); };
    modernCard_.onSelect = [this] { applyMenuStyle(true); };
    addAndMakeVisible(classicCard_);
    addAndMakeVisible(modernCard_);

    brightness_.onDragStart = contrast_.onDragStart = [] { AppSettings::instance().beginBatch(); };
    brightness_.onDragEnd   = contrast_.onDragEnd   = [] { AppSettings::instance().endBatch(); };
    rebuildFromState();
}

void AppearanceSettingsView::applyMenuStyle(bool modern) {
    setModernMenus(modern);
    classicCard_.selected = !modern;
    modernCard_.selected = modern;
    classicCard_.repaint();
    modernCard_.repaint();
}

void AppearanceSettingsView::rebuildThemeCombo() {
    themeCombo_.clear(juce::dontSendNotification);
    for (int i = 0; i < numThemes(); ++i) themeCombo_.addItem(themeName(i), i + 1);
    const auto user = themestore::list();
    if (!user.empty()) {
        themeCombo_.addSeparator();
        for (size_t i = 0; i < user.size(); ++i)
            themeCombo_.addItem(user[i].name, kUserThemeBase + (int) i);
    }
    themeCombo_.addSeparator();
    themeCombo_.addItem(tr("settings.custom", "Custom"), customComboId());
}

void AppearanceSettingsView::updateThemeButtons() {
    deleteThemeBtn_.setEnabled(themeCombo_.getSelectedId() >= kUserThemeBase);
}

void AppearanceSettingsView::rebuildFromState() {
    auto& s = AppSettings::instance();
    const auto mode = s.getString("themeMode", "preset");
    if (mode == "custom") {
        themeCombo_.setSelectedId(customComboId(), juce::dontSendNotification);
    } else if (mode == "user") {
        const auto themes = themestore::list();
        const auto name = s.getString("themeName");
        themeCombo_.setSelectedId(customComboId(), juce::dontSendNotification);
        for (size_t i = 0; i < themes.size(); ++i)
            if (themes[i].name == name)
                themeCombo_.setSelectedId(kUserThemeBase + (int) i, juce::dontSendNotification);
    } else {
        themeCombo_.setSelectedId(s.getInt("themeIndex", 0) + 1, juce::dontSendNotification);
    }
    setSwatches(baseColours());
    brightness_.setValue(brightnessAdjustment(), juce::dontSendNotification);
    contrast_.setValue(contrastAdjustment(), juce::dontSendNotification);
    updateThemeButtons();
}

void AppearanceSettingsView::pushAppearance() {
    recomputePalette();
    if (onAppearanceChanged_) onAppearanceChanged_();
}

ThemeColours AppearanceSettingsView::coloursFromSwatches() const {
    ThemeColours c;
    for (int r = 0; r < kNumRoles; ++r) c.*kThemeRolePtrs[r] = swatches_[(size_t) r]->colour();
    return c;
}

void AppearanceSettingsView::setSwatches(const ThemeColours& c) {
    for (int r = 0; r < kNumRoles; ++r) {
        swatches_[(size_t) r]->show(c.*kThemeRolePtrs[r]);
    }
}

void AppearanceSettingsView::resized() {
    auto panel = getLocalBounds().reduced(16, 12);
    title_.setBounds(panel.removeFromTop(26));
    panel.removeFromTop(8);

    auto themeRow = panel.removeFromTop(26);
    themeLabel_.setBounds(themeRow.removeFromLeft(70));
    themeCombo_.setBounds(themeRow.removeFromLeft(220));
    panel.removeFromTop(12);

    constexpr int cols = 3, cellW = 124, labelH = 16, swatchH = 22, cellH = labelH + swatchH + 8;
    auto gridTop = panel.removeFromTop((kNumRoles + cols - 1) / cols * cellH);
    for (int r = 0; r < kNumRoles; ++r) {
        int col = r % cols, rowIdx = r / cols;
        juce::Rectangle<int> cell(gridTop.getX() + col * cellW, gridTop.getY() + rowIdx * cellH,
                                  cellW - 10, cellH - 6);
        swatchLabels_[(size_t) r]->setBounds(cell.removeFromTop(labelH));
        swatches_[(size_t) r]->setBounds(cell.removeFromTop(swatchH));
    }
    panel.removeFromTop(6);
    auto btnRow = panel.removeFromTop(26);
    saveCustomBtn_.setBounds(btnRow.removeFromLeft(140));
    btnRow.removeFromLeft(8);
    deleteThemeBtn_.setBounds(btnRow.removeFromLeft(120));
    btnRow.removeFromLeft(8);
    exportThemeBtn_.setBounds(btnRow.removeFromLeft(90));
    btnRow.removeFromLeft(8);
    importThemeBtn_.setBounds(btnRow.removeFromLeft(90));
    panel.removeFromTop(14);

    auto brightRow = panel.removeFromTop(26);
    brightnessLabel_.setBounds(brightRow.removeFromLeft(80));
    brightness_.setBounds(brightRow.removeFromLeft(360));
    panel.removeFromTop(8);
    auto contrastRow = panel.removeFromTop(26);
    contrastLabel_.setBounds(contrastRow.removeFromLeft(80));
    contrast_.setBounds(contrastRow.removeFromLeft(360));
    panel.removeFromTop(8);
    auto langRow = panel.removeFromTop(26);
    languageLabel_.setBounds(langRow.removeFromLeft(80));
    languageCombo_.setBounds(langRow.removeFromLeft(220));
    panel.removeFromTop(10);
    flowLights_.setBounds(panel.removeFromTop(24).removeFromLeft(kA4Hz));
    panel.removeFromTop(10);
    menuStyleLabel_.setBounds(panel.removeFromTop(18));
    panel.removeFromTop(4);
    auto cardRow = panel.removeFromTop(juce::jmax(84, panel.getHeight()));
    classicCard_.setBounds(cardRow.removeFromLeft(240));
    cardRow.removeFromLeft(10);
    modernCard_.setBounds(cardRow.removeFromLeft(240));
}

}
