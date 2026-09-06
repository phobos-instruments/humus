#include "gui/SettingsComponent.h"

#include "gui/Localisation.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include "gui/AppSettings.h"
#include "gui/AudioSettingsPanel.h"
#include "gui/LicensePanel.h"
#include "gui/OrganismManagerView.h"
#include "gui/PickerLauncher.h"
#include "gui/PluginManagerView.h"
#include "gui/ThemeStore.h"
#include "gui/VideoCacheSettingsView.h"

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
constexpr int userThemeBase() { return 200; }
}

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
    juce::Desktop::getInstance().setGlobalScaleFactor(
        (float) juce::jlimit(0.8, 1.6, s.getDouble("uiScale", 1.0)));
}

void SettingsComponent::Swatch::paint(juce::Graphics& g) {
    g.fillAll(colour);
    g.setColour(Palette::border);
    g.drawRect(getLocalBounds(), 1);
}

void SettingsComponent::Swatch::mouseDown(const juce::MouseEvent&) {
    struct BatchedSelector : juce::ColourSelector {
        using juce::ColourSelector::ColourSelector;
        ~BatchedSelector() override { AppSettings::instance().endBatch(); }
    };
    AppSettings::instance().beginBatch();
    auto sel = std::make_unique<BatchedSelector>(
        juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders
        | juce::ColourSelector::showColourspace);
    sel->setCurrentColour(colour);
    sel->setSize(240, 280);
    sel->addChangeListener(this);
    juce::CallOutBox::launchAsynchronously(std::move(sel), getScreenBounds(), nullptr);
}

void SettingsComponent::Swatch::changeListenerCallback(juce::ChangeBroadcaster* src) {
    if (auto* s = dynamic_cast<juce::ColourSelector*>(src)) {
        colour = s->getCurrentColour();
        repaint();
        if (onChange) onChange(colour);
    }
}

SettingsComponent::SettingsComponent(std::function<void()> onAppearanceChanged,
                                     EngineHost* host)
    : onAppearanceChanged_(std::move(onAppearanceChanged)) {
    addAndMakeVisible(categories_);
    categories_.setRowHeight(24);
    midiView_ = std::make_unique<MidiSettingsView>(host);
    videoView_ = std::make_unique<VideoCacheSettingsView>();
    addChildComponent(*videoView_);
    addChildComponent(*midiView_);
    audioView_ = std::make_unique<AudioSettingsPanel>(host);
    addChildComponent(*audioView_);
    {
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
    }

    appearanceTitle_.setText(tr("settings.graphics-and-ui", "Graphics & UI"), juce::dontSendNotification);
    appearanceTitle_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    addAndMakeVisible(appearanceTitle_);

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
        } else if (id >= userThemeBase()) {
            const auto themes = themestore::list();
            const size_t idx = (size_t) (id - userThemeBase());
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

        swatches_[(size_t) r] = std::make_unique<Swatch>();
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
        if (id < userThemeBase()) return;
        const auto themes = themestore::list();
        const size_t idx = (size_t) (id - userThemeBase());
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
    languageCombo_.addItem(tr("settings.english-as-written", "English (as written)"), 1);
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
    languageHint_.setText(tr("settings.menus-and-windows-follow-the",
       "Menus and windows follow the language they are built in, so a "
       "change reads fully on the next launch. Drop a catalogue in "
       "Documents/Humus/assets/Translations to add one."),
                          juce::dontSendNotification);
    languageHint_.setFont(juce::FontOptions(11.5f));
    languageHint_.setColour(juce::Label::textColourId, Palette::textDim);
    languageHint_.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(languageHint_);

    uiScaleLabel_.setText(tr("settings.ui-scale", "UI scale"), juce::dontSendNotification);
    addAndMakeVisible(uiScaleLabel_);
    uiScale_.setRange(0.8, 1.6, 0.05);
    uiScale_.setSliderStyle(juce::Slider::LinearHorizontal);
    uiScale_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    uiScale_.setDoubleClickReturnValue(true, 1.0);
    uiScale_.setValue(AppSettings::instance().getDouble("uiScale", 1.0),
                      juce::dontSendNotification);
    addAndMakeVisible(uiScale_);
    uiScale_.onValueChange = [this] {
        juce::Desktop::getInstance().setGlobalScaleFactor((float) uiScale_.getValue());
        AppSettings::instance().set("uiScale", uiScale_.getValue());
    };

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
    uiScale_.onDragStart = [] { AppSettings::instance().beginBatch(); };
    uiScale_.onDragEnd   = [] { AppSettings::instance().endBatch(); };

    buildAiPanel();
    rebuildFromState();
    categories_.selectRow(0);
    showCategory(0);
    setSize(700, 560);
}

void SettingsComponent::applyMenuStyle(bool modern) {
    setModernMenus(modern);
    classicCard_.selected = !modern;
    modernCard_.selected = modern;
    classicCard_.repaint();
    modernCard_.repaint();
}

void SettingsComponent::selectedRowsChanged(int lastRow) {
    if (lastRow >= 0) showCategory(lastRow);
}

void SettingsComponent::selectCategory(int index) {
    if (index >= 0 && index < kNumCategories) categories_.selectRow(index);
}

void SettingsComponent::showCategory(int index) {
    category_ = index;
    const bool appearance = index == kAppearance;
    appearanceTitle_.setVisible(appearance);
    themeLabel_.setVisible(appearance);
    themeCombo_.setVisible(appearance);
    for (auto& l : swatchLabels_) l->setVisible(appearance);
    for (auto& sw : swatches_) sw->setVisible(appearance);
    saveCustomBtn_.setVisible(appearance);
    deleteThemeBtn_.setVisible(appearance);
    exportThemeBtn_.setVisible(appearance);
    importThemeBtn_.setVisible(appearance);
    brightnessLabel_.setVisible(appearance);
    brightness_.setVisible(appearance);
    contrastLabel_.setVisible(appearance);
    contrast_.setVisible(appearance);
    languageLabel_.setVisible(appearance);
    languageCombo_.setVisible(appearance);
    languageHint_.setVisible(appearance);
    uiScaleLabel_.setVisible(appearance);
    uiScale_.setVisible(appearance);
    flowLights_.setVisible(appearance);
    menuStyleLabel_.setVisible(appearance);
    classicCard_.setVisible(appearance);
    modernCard_.setVisible(appearance);

    const bool ai = index == kAi;
    aiTitle_.setVisible(ai);
    providerLabel_.setVisible(ai);
    providerCombo_.setVisible(ai);
    endpointLabel_.setVisible(ai);
    endpointEdit_.setVisible(ai);
    modelLabel_.setVisible(ai);
    modelEdit_.setVisible(ai);
    keyLabel_.setVisible(ai);
    keyEdit_.setVisible(ai);
    testBtn_.setVisible(ai);
    aiHint_.setVisible(ai);

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

void SettingsComponent::rebuildThemeCombo() {
    themeCombo_.clear(juce::dontSendNotification);
    for (int i = 0; i < numThemes(); ++i) themeCombo_.addItem(themeName(i), i + 1);
    const auto user = themestore::list();
    if (!user.empty()) {
        themeCombo_.addSeparator();
        for (size_t i = 0; i < user.size(); ++i)
            themeCombo_.addItem(user[i].name, userThemeBase() + (int) i);
    }
    themeCombo_.addSeparator();
    themeCombo_.addItem(tr("settings.custom", "Custom"), customComboId());
}

void SettingsComponent::updateThemeButtons() {
    deleteThemeBtn_.setEnabled(themeCombo_.getSelectedId() >= userThemeBase());
}

void SettingsComponent::promptSaveTheme() {
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
        [safe = juce::Component::SafePointer<SettingsComponent>(this), aw](int r) {
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
                    safe->themeCombo_.setSelectedId(userThemeBase() + (int) i,
                                                    juce::dontSendNotification);
            safe->updateThemeButtons();
        }), true);
}

void SettingsComponent::exportTheme() {
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
        [safe = juce::Component::SafePointer<SettingsComponent>(this)](
            const juce::FileChooser& fc) {
            auto f = fc.getResult();
            if (f == juce::File() || safe == nullptr) return;
            if (!f.hasFileExtension("humtheme")) f = f.withFileExtension("humtheme");
            f.replaceWithText(juce::JSON::toString(themestore::themeFileVar(
                {f.getFileNameWithoutExtension(), safe->coloursFromSwatches()})));
        });
}

void SettingsComponent::importTheme() {
    chooser_ = std::make_unique<juce::FileChooser>(
        "Import theme",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.humtheme");
    chooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<SettingsComponent>(this)](
            const juce::FileChooser& fc) {
            const auto f = fc.getResult();
            if (f == juce::File() || safe == nullptr) return;
            themestore::UserTheme t;
            if (!themestore::parseThemeFile(juce::JSON::parse(f.loadFileAsString()), t)) {
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
                    safe->themeCombo_.setSelectedId(userThemeBase() + (int) i,
                                                    juce::dontSendNotification);
            safe->updateThemeButtons();
            safe->pushAppearance();
        });
}

void SettingsComponent::rebuildFromState() {
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
                themeCombo_.setSelectedId(userThemeBase() + (int) i, juce::dontSendNotification);
    } else {
        themeCombo_.setSelectedId(s.getInt("themeIndex", 0) + 1, juce::dontSendNotification);
    }
    setSwatches(baseColours());
    brightness_.setValue(brightnessAdjustment(), juce::dontSendNotification);
    contrast_.setValue(contrastAdjustment(), juce::dontSendNotification);
    updateThemeButtons();
}

void SettingsComponent::pushAppearance() {
    recomputePalette();
    if (onAppearanceChanged_) onAppearanceChanged_();
}

ThemeColours SettingsComponent::coloursFromSwatches() const {
    ThemeColours c;
    for (int r = 0; r < kNumRoles; ++r) c.*kThemeRolePtrs[r] = swatches_[(size_t) r]->colour;
    return c;
}

void SettingsComponent::setSwatches(const ThemeColours& c) {
    for (int r = 0; r < kNumRoles; ++r) {
        swatches_[(size_t) r]->colour = c.*kThemeRolePtrs[r];
        swatches_[(size_t) r]->repaint();
    }
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

    if (category_ == kAi) {
        auto panel = area.reduced(16, 12);
        aiTitle_.setBounds(panel.removeFromTop(26));
        panel.removeFromTop(10);
        auto row = panel.removeFromTop(26);
        providerLabel_.setBounds(row.removeFromLeft(130));
        providerCombo_.setBounds(row.removeFromLeft(220));
        panel.removeFromTop(10);
        row = panel.removeFromTop(26);
        endpointLabel_.setBounds(row.removeFromLeft(130));
        endpointEdit_.setBounds(row.removeFromLeft(260));
        row.removeFromLeft(8);
        testBtn_.setBounds(row.removeFromLeft(60));
        panel.removeFromTop(10);
        row = panel.removeFromTop(26);
        modelLabel_.setBounds(row.removeFromLeft(130));
        modelEdit_.setBounds(row.removeFromLeft(260));
        panel.removeFromTop(10);
        const bool claude = providerCombo_.getSelectedId() == 2;
        keyLabel_.setVisible(claude);
        keyEdit_.setVisible(claude);
        endpointLabel_.setEnabled(!claude);
        endpointEdit_.setEnabled(!claude);
        if (claude) {
            row = panel.removeFromTop(26);
            keyLabel_.setBounds(row.removeFromLeft(130));
            keyEdit_.setBounds(row.removeFromLeft(260));
            panel.removeFromTop(10);
        }
        panel.removeFromTop(6);
        aiHint_.setBounds(panel.removeFromTop(64));
        return;
    }

    auto panel = area.reduced(16, 12);
    appearanceTitle_.setBounds(panel.removeFromTop(26));
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
    languageHint_.setBounds(panel.removeFromTop(34));
    panel.removeFromTop(6);
    auto scaleRow = panel.removeFromTop(26);
    uiScaleLabel_.setBounds(scaleRow.removeFromLeft(80));
    uiScale_.setBounds(scaleRow.removeFromLeft(360));
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
