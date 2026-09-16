// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/SettingsComponent.h"

#include "gui/app/AppSettings.h"
#include "gui/plugins/OrganismManagerView.h"
#include "gui/plugins/PluginManagerView.h"
#include "gui/settings/AiSettingsView.h"
#include "gui/settings/AppearanceSettingsView.h"
#include "gui/settings/AudioSettingsPanel.h"
#include "gui/settings/LicensePanel.h"
#include "gui/settings/MidiSettingsView.h"
#include "gui/settings/ThemeStore.h"
#include "gui/settings/VideoCacheSettingsView.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

void applySavedAppearance() {
    auto& s = AppSettings::instance();
    if (s.getInt("themeListVersion", 1) < 2) {
        static constexpr int kRemap[] = {0, 2, 4, 0, 5};
        const int old = s.getInt("themeIndex", 0);
        if (old >= 0 && old < 5) s.set("themeIndex", kRemap[old]);
        s.set("themeListVersion", 2);
    }
    if (s.getInt("themeListVersion", 1) < 3) {
        static constexpr int kRemap[] = {0, 1, 2, 2, 0, 1, 3, 1,
                                         3, 4, 2, 4, 4, 3, 3, 4};
        const int old = s.getInt("themeIndex", 0);
        if (old >= 0 && old < (int) (sizeof(kRemap) / sizeof(kRemap[0])))
            s.set("themeIndex", kRemap[old]);
        s.set("themeListVersion", 3);
    }
    const auto mode = s.getString("themeMode", "preset");
    if (mode == "custom") {
        setBaseColours(themeColoursFromString(s.getString("customColours"), baseColours()));
    } else if (mode == "user") {
        ThemeColours c;
        if (themestore::find(s.getString("themeName"), c)) setBaseColours(c);
        else applyTheme(0);
    } else {
        applyTheme(s.getInt("themeIndex", 0));
    }
    setAppearanceAdjustments(s.getDouble("brightness", 0.0), s.getDouble("contrast", 0.0));
}

SettingsComponent::SettingsComponent(std::function<void()> onAppearanceChanged,
                                     SettingsHost* host) {
    addAndMakeVisible(categories_);
    categories_.setRowHeight(24);

    appearanceView_ = std::make_unique<AppearanceSettingsView>(std::move(onAppearanceChanged));
    addChildComponent(*appearanceView_);
    aiView_ = std::make_unique<AiSettingsView>();
    addChildComponent(*aiView_);
    midiView_ = std::make_unique<MidiSettingsView>(host);
    addChildComponent(*midiView_);
    videoView_ = std::make_unique<VideoCacheSettingsView>();
    addChildComponent(*videoView_);
    audioView_ = std::make_unique<AudioSettingsPanel>(host);
    addChildComponent(*audioView_);
    auto packs = std::make_unique<OrganismManagerView>();
    packs->onPacksChanged = [this] { if (onPacksChanged) onPacksChanged(); };
    packsView_ = std::move(packs);
    addChildComponent(*packsView_);
    auto plugins = std::make_unique<PluginManagerView>();
    plugins->onPluginsChanged = [this] { if (onPluginsChanged) onPluginsChanged(); };
    pluginsView_ = std::move(plugins);
    addChildComponent(*pluginsView_);
    licenseView_ = std::make_unique<LicensePanel>();
    addChildComponent(*licenseView_);

    categories_.selectRow(0);
    showCategory(0);
    setSize(700, 560);
}

SettingsComponent::~SettingsComponent() = default;

void SettingsComponent::selectedRowsChanged(int lastRow) {
    if (lastRow >= 0) showCategory(lastRow);
}

void SettingsComponent::selectCategory(int index) {
    if (index >= 0 && index < kNumCategories) categories_.selectRow(index);
}

void SettingsComponent::showCategory(int index) {
    category_ = index;
    appearanceView_->setVisible(index == kAppearance);
    aiView_->setVisible(index == kAi);
    audioView_->setVisible(index == kAudio);
    midiView_->setVisible(index == kMidi);
    packsView_->setVisible(index == kPacks);
    pluginsView_->setVisible(index == kPlugins);
    licenseView_->setVisible(index == kLicense);
    videoView_->setVisible(index == kVideo);
    if (index == kLicense)
        static_cast<LicensePanel*>(licenseView_.get())->refresh();
    if (index == kVideo)
        static_cast<VideoCacheSettingsView*>(videoView_.get())->refresh();
    resized();
}

void SettingsComponent::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) {
    static const char* names[] = {"AI", "Audio", "Graphics & UI", "License & Privacy",
                                  "MIDI & OSC", "Packs", "Plugins", "Video"};
    if (row < 0 || row >= kNumCategories) return;
    g.fillAll(selected ? Palette::accentDim : Palette::panel);
    g.setColour(selected ? juce::Colours::white : Palette::text);
    g.setFont(13.0f);
    g.drawText(names[row], 10, 0, w - 12, h, juce::Justification::centredLeft, true);
}

void SettingsComponent::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    g.setColour(Palette::border);
    g.fillRect(150, 0, 1, getHeight());
}

void SettingsComponent::resized() {
    auto area = getLocalBounds();
    categories_.setBounds(area.removeFromLeft(150));

    if (category_ == kAudio) { audioView_->setBounds(area); return; }
    if (category_ == kMidi)  { midiView_->setBounds(area); return; }
    if (category_ == kPacks) { packsView_->setBounds(area.reduced(8)); return; }
    if (category_ == kPlugins) { pluginsView_->setBounds(area.reduced(8)); return; }
    if (category_ == kLicense) { licenseView_->setBounds(area.reduced(8)); return; }
    if (category_ == kVideo) { videoView_->setBounds(area.reduced(8)); return; }
    if (category_ == kAi) { aiView_->setBounds(area); return; }
    appearanceView_->setBounds(area);
}

}
