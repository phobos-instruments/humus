// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/settings/ColourSwatch.h"
#include "gui/settings/MenuStylePreview.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class AppearanceSettingsView : public juce::Component {
public:
    static constexpr int kUserThemeBase = 200;

    explicit AppearanceSettingsView(std::function<void()> onAppearanceChanged);

    void resized() override;

private:
    static constexpr int kNumRoles = 9;

    void rebuildFromState();
    void rebuildThemeCombo();
    void promptSaveTheme();
    void updateThemeButtons();
    void exportTheme();
    void importTheme();
    void pushAppearance();
    void applyMenuStyle(bool modern);
    ThemeColours coloursFromSwatches() const;
    void setSwatches(const ThemeColours&);

    std::function<void()> onAppearanceChanged_;

    juce::Label title_;
    juce::Label themeLabel_;
    juce::ComboBox themeCombo_;
    juce::Label languageLabel_;
    juce::ComboBox languageCombo_;

    std::array<std::unique_ptr<ColourSwatch>, kNumRoles> swatches_;
    std::array<std::unique_ptr<juce::Label>, kNumRoles> swatchLabels_;
    juce::TextButton saveCustomBtn_{tr("settings.save-theme", "Save Theme...")};
    juce::TextButton deleteThemeBtn_{tr("settings.delete-theme", "Delete Theme")};
    juce::TextButton exportThemeBtn_{"Export..."};
    juce::TextButton importThemeBtn_{"Import..."};
    std::unique_ptr<juce::FileChooser> chooser_;

    juce::Label brightnessLabel_, contrastLabel_;
    juce::Slider brightness_, contrast_;
    juce::ToggleButton flowLights_;

    juce::Label menuStyleLabel_;
    MenuStylePreviewCard classicCard_, modernCard_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppearanceSettingsView)
};

}
