#include "gui/SetupWizard.h"

#include "gui/AppSettings.h"
#include "gui/AudioSettingsPanel.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/MidiSettingsView.h"
#include "gui/PickerLauncher.h"
#include "gui/StartWindow.h"
#include "gui/Telemetry.h"
#include "gui/TelemetryEvents.h"

namespace hum {

namespace {
constexpr int kW = 760, kHeaderH = 64, kFooterH = 54;
constexpr int kContentH = 540;
const juce::Colour kBrandBg(0xfff4ecdc), kBrandInk(0xff33402a);

void styleBrandButton(juce::TextButton& b) {
    b.setColour(juce::TextButton::buttonColourId, kBrandInk);
    b.setColour(juce::TextButton::textColourOffId, kBrandBg);
}
}

class SetupWizard::ThemeGallery : public juce::Component {
public:
    std::function<void(int)> onPick;
    int selected = -1;

    ThemeGallery() {
        if (AppSettings::instance().getString("themeMode", "preset") == "preset")
            selected = AppSettings::instance().getInt("themeIndex", 0);
    }

    int idealHeight() const {
        return ((numThemes() + kCols - 1) / kCols) * (kCardH + kGap);
    }

    void paint(juce::Graphics& g) override {
        for (int i = 0; i < numThemes(); ++i) {
            const auto c = presetColours(i);
            const auto r = cardBounds(i).toFloat();
            g.setColour(c.panel);
            g.fillRoundedRectangle(r, 6.0f);
            g.setColour(i == selected ? Palette::accent
                                      : (i == hover_ ? c.text.withAlpha(0.5f) : c.border));
            g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, i == selected ? 2.0f : 1.0f);
            auto inner = cardBounds(i).reduced(10, 6);
            g.setColour(c.text);
            g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
            g.drawText(themeName(i), inner.removeFromTop(16), juce::Justification::centredLeft);
            inner.removeFromTop(4);
            auto strip = inner.removeFromTop(10);
            const juce::Colour chips[] = {c.background, c.panelLight, c.accent, c.cord, c.text};
            const int chipW = strip.getWidth() / 5;
            for (const auto& chip : chips) {
                g.setColour(chip);
                g.fillRoundedRectangle(strip.removeFromLeft(chipW).reduced(2, 0).toFloat(), 2.0f);
            }
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int h = cardAt(e.getPosition());
        if (h != hover_) { hover_ = h; repaint(); }
    }
    void mouseExit(const juce::MouseEvent&) override { hover_ = -1; repaint(); }
    void mouseDown(const juce::MouseEvent& e) override {
        const int i = cardAt(e.getPosition());
        if (i < 0) return;
        selected = i;
        repaint();
        if (onPick) onPick(i);
    }

private:
    static constexpr int kCols = 3, kCardH = 46, kGap = 8;

    juce::Rectangle<int> cardBounds(int i) const {
        const int w = (getWidth() - (kCols - 1) * kGap) / kCols;
        return {(i % kCols) * (w + kGap), (i / kCols) * (kCardH + kGap), w, kCardH};
    }
    int cardAt(juce::Point<int> p) const {
        for (int i = 0; i < numThemes(); ++i)
            if (cardBounds(i).contains(p)) return i;
        return -1;
    }
    int hover_ = -1;
};

SetupWizard::SetupWizard(EngineHost& host, Callbacks cbs, bool firstBoot)
    : host_(host), cbs_(std::move(cbs)), firstBoot_(firstBoot) {
    themes_ = std::make_unique<ThemeGallery>();
    themes_->onPick = [this](int i) {
        applyTheme(i);
        auto& s = AppSettings::instance();
        s.beginBatch();
        s.set("themeMode", juce::String("preset"));
        s.set("themeIndex", i);
        s.endBatch();
        if (cbs_.onAppearanceChanged) cbs_.onAppearanceChanged();
        repaint();
    };
    addChildComponent(*themes_);

    uiScaleLabel_.setText("UI scale", juce::dontSendNotification);
    addChildComponent(uiScaleLabel_);
    uiScale_.setRange(0.8, 1.6, 0.05);
    uiScale_.setSliderStyle(juce::Slider::LinearHorizontal);
    uiScale_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    uiScale_.setDoubleClickReturnValue(true, 1.0);
    uiScale_.setValue(AppSettings::instance().getDouble("uiScale", 1.0),
                      juce::dontSendNotification);
    uiScale_.onValueChange = [this] {
        juce::Desktop::getInstance().setGlobalScaleFactor((float) uiScale_.getValue());
        AppSettings::instance().set("uiScale", uiScale_.getValue());
    };
    uiScale_.onDragStart = [] { AppSettings::instance().beginBatch(); };
    uiScale_.onDragEnd = [] { AppSettings::instance().endBatch(); };
    addChildComponent(uiScale_);

    hintLabel_.setText(juce::String::fromUTF8(
                           "Custom colours, brightness and contrast live in "
                           "Settings \xe2\x80\xba Graphics & UI."),
                       juce::dontSendNotification);
    hintLabel_.setColour(juce::Label::textColourId, Palette::textDim);
    hintLabel_.setFont(juce::FontOptions(12.0f));
    addChildComponent(hintLabel_);

    if (firstBoot_ && AppSettings::instance().getString("menu.style", "").isEmpty())
        setModernMenus(true);
    menuStyleLabel_.setText("How would you like to pick organisms when patching?",
                            juce::dontSendNotification);
    addChildComponent(menuStyleLabel_);
    modernCard_.modern = true;
    classicCard_.selected = !modernMenusEnabled();
    modernCard_.selected = modernMenusEnabled();
    auto pickStyle = [this](bool modern) {
        setModernMenus(modern);
        classicCard_.selected = !modern;
        modernCard_.selected = modern;
        classicCard_.repaint();
        modernCard_.repaint();
    };
    classicCard_.onSelect = [pickStyle] { pickStyle(false); };
    modernCard_.onSelect = [pickStyle] { pickStyle(true); };
    addChildComponent(classicCard_);
    addChildComponent(modernCard_);

    telemetryToggle_.setButtonText("Share anonymous usage data");
    telemetryToggle_.setToggleState(
        AppSettings::instance().getInt("telemetry.enabled", 0) != 0,
        juce::dontSendNotification);
    addChildComponent(telemetryToggle_);
    updatesToggle_.setButtonText("Check for updates at startup");
    updatesToggle_.setToggleState(
        AppSettings::instance().getInt("updates.auto", 0) != 0,
        juce::dontSendNotification);
    addChildComponent(updatesToggle_);

    for (auto* b : {&backBtn_, &nextBtn_, &skipBtn_}) {
        styleBrandButton(*b);
        addAndMakeVisible(*b);
    }
    skipBtn_.setButtonText(firstBoot_ ? "Skip setup" : "Close");
    backBtn_.onClick = [this] { setPage(wizard::back(page_)); };
    nextBtn_.onClick = [this] {
        if (wizard::isLast(page_)) finish();
        else setPage(wizard::next(page_));
    };
    skipBtn_.onClick = [this] { finish(); };

    setWantsKeyboardFocus(true);
    setSize(kW, kHeaderH + kContentH + kFooterH);
    setPage(wizard::kAudio);
}

SetupWizard::~SetupWizard() = default;

juce::Rectangle<int> SetupWizard::contentArea() const {
    return {0, kHeaderH, getWidth(), kContentH};
}

void SetupWizard::setPage(wizard::Page p) {
    page_ = p;
    if (p == wizard::kAudio && !audio_) {
        audio_ = std::make_unique<AudioSettingsPanel>(&host_);
        addChildComponent(*audio_);
    }
    if (p == wizard::kMidi && !midi_) {
        midi_ = std::make_unique<MidiSettingsView>(&host_);
        addChildComponent(*midi_);
    }

    if (audio_) audio_->setVisible(p == wizard::kAudio);
    if (midi_) midi_->setVisible(p == wizard::kMidi);
    const bool appearance = p == wizard::kAppearance;
    themes_->setVisible(appearance);
    uiScaleLabel_.setVisible(appearance);
    uiScale_.setVisible(appearance);
    hintLabel_.setVisible(appearance);
    const bool menus = p == wizard::kMenuStyle;
    menuStyleLabel_.setVisible(menus);
    classicCard_.setVisible(menus);
    modernCard_.setVisible(menus);
    telemetryToggle_.setVisible(p == wizard::kTelemetry);
    updatesToggle_.setVisible(p == wizard::kTelemetry);

    backBtn_.setEnabled(!wizard::isFirst(p));
    nextBtn_.setButtonText(wizard::isLast(p) ? "Finish" : "Next");
    skipBtn_.setVisible(!wizard::isLast(p));
    resized();
    repaint();
}

void SetupWizard::finish() {
    auto& st = AppSettings::instance();
    st.set("setup.completed", 1);
    st.set("telemetry.enabled", telemetryToggle_.getToggleState() ? 1 : 0);
    st.set("updates.auto", updatesToggle_.getToggleState() ? 1 : 0);
    telemetrySyncConsent();
    telemetryCount(telemetry::kWizardCompleted);
    if (cbs_.onFinished) cbs_.onFinished();
}

void SetupWizard::resized() {
    auto content = contentArea().reduced(24, 18);
    if (audio_) audio_->setBounds(contentArea());
    if (midi_) midi_->setBounds(contentArea());

    auto ap = content;
    auto scaleRow = ap.removeFromBottom(26);
    ap.removeFromBottom(6);
    auto hintRow = ap.removeFromBottom(20);
    ap.removeFromBottom(10);
    themes_->setBounds(ap.withHeight(juce::jmin(ap.getHeight(), themes_->idealHeight())));
    uiScaleLabel_.setBounds(scaleRow.removeFromLeft(70));
    uiScale_.setBounds(scaleRow.removeFromLeft(340));
    hintLabel_.setBounds(hintRow);

    auto mp = content;
    menuStyleLabel_.setBounds(mp.removeFromTop(22));
    mp.removeFromTop(12);
    auto cards = mp.removeFromTop(juce::jmin(mp.getHeight(), 260));
    const int cw = (cards.getWidth() - 16) / 2;
    classicCard_.setBounds(cards.removeFromLeft(cw));
    cards.removeFromLeft(16);
    modernCard_.setBounds(cards.removeFromLeft(cw));

    telemetryToggle_.setBounds(content.getX(), content.getY() + 320,
                               juce::jmin(content.getWidth(), 300), 26);
    updatesToggle_.setBounds(content.getX(), content.getY() + 350,
                             juce::jmin(content.getWidth(), 300), 26);

    auto footer = getLocalBounds().removeFromBottom(kFooterH).reduced(24, 11);
    backBtn_.setBounds(footer.removeFromLeft(90));
    nextBtn_.setBounds(footer.removeFromRight(110));
    footer.removeFromRight(10);
    skipBtn_.setBounds(footer.removeFromRight(100));
}

void SetupWizard::paint(juce::Graphics& g) {
    paintBrandPanel(g, getLocalBounds(), kBrandBg, kBrandInk);
    g.setColour(Palette::background);
    g.fillRect(contentArea());

    drawHumusLogo(g, juce::Rectangle<float>(24.0f, 8.0f, 110.0f, (float) kHeaderH - 16.0f));
    g.setColour(kBrandInk);
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText("Setup", 150, 0, 200, kHeaderH, juce::Justification::centredLeft);
    g.setColour(kBrandInk.withAlpha(0.65f));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(wizard::pageTitle(page_), 0, 0, getWidth() - 24, kHeaderH,
               juce::Justification::centredRight);

    const int n = wizard::kNumPages, dot = 8, gap = 14;
    const int x0 = (getWidth() - (n * dot + (n - 1) * (gap - dot))) / 2;
    const float cy = (float) getHeight() - kFooterH * 0.5f;
    for (int i = 0; i < n; ++i) {
        g.setColour(i == (int) page_ ? Palette::accent
                                     : kBrandInk.withAlpha(i < (int) page_ ? 0.55f : 0.22f));
        g.fillEllipse((float) (x0 + i * gap), cy - dot * 0.5f, (float) dot, (float) dot);
    }

    const auto content = contentArea().reduced(24, 18);
    g.setColour(Palette::text);
    if (page_ == wizard::kTelemetry) {
        auto r = content;
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("Help improve Humus?", r.removeFromTop(30),
                   juce::Justification::centredLeft);
        r.removeFromTop(14);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(14.0f));
        for (const char* line :
             {"If you opt in, Humus sends anonymous usage counts (like which",
              "organisms you plant or how often you save) and whether the",
              "last session crashed. It is tied to a random ID, not to you -",
              "no names, no emails, no patch contents, ever.", "",
              "Checking for updates at startup asks our server once per",
              "launch; it sees the same random ID, the app version and the",
              "platform.", "",
              "Both are off until you turn them on. With them off, Humus",
              "never touches the network except when you ask it to. Change",
              "either any time in Settings > License & Privacy."})
            g.drawText(line, r.removeFromTop(22), juce::Justification::centredLeft);
    } else if (page_ == wizard::kFinish) {
        auto r = content;
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("All set", r.removeFromTop(30), juce::Justification::centredLeft);
        r.removeFromTop(14);
        g.setFont(juce::FontOptions(14.5f));
        for (const auto& line : summary()) {
            g.setColour(Palette::text);
            g.drawText(juce::String::fromUTF8(line.c_str()), r.removeFromTop(24),
                       juce::Justification::centredLeft);
            r.removeFromTop(4);
        }
        r.removeFromTop(14);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.5f));
        g.drawText(juce::String::fromUTF8("Rerun this anytime from Help \xe2\x80\xba "
                                          "Setup Wizard."),
                   r.removeFromTop(20), juce::Justification::centredLeft);
    }
}

std::vector<std::string> SetupWizard::summary() const {
    std::string device;
    if (auto* d = host_.audioDevices().getCurrentAudioDevice())
        device = d->getName().toStdString();
    int midiIns = 0;
    for (int n = 1; n <= EngineHost::kMidiPorts; ++n)
        if (AppSettings::instance().getString("midi.in." + juce::String(n)).isNotEmpty())
            ++midiIns;
    std::string theme;
    const auto mode = AppSettings::instance().getString("themeMode", "preset");
    if (mode == "preset")
        theme = themeName(juce::jlimit(0, numThemes() - 1,
                                       AppSettings::instance().getInt("themeIndex", 0)));
    else if (mode == "user")
        theme = AppSettings::instance().getString("themeName").toStdString();
    else
        theme = "Custom";
    return wizard::summaryLines(device, midiIns, theme, modernMenusEnabled(),
                                telemetryToggle_.getToggleState(),
                                updatesToggle_.getToggleState());
}

bool SetupWizard::keyPressed(const juce::KeyPress& k) {
    if (k.getKeyCode() == juce::KeyPress::escapeKey) {
        finish();
        return true;
    }
    if (k.getKeyCode() == juce::KeyPress::returnKey) {
        nextBtn_.triggerClick();
        return true;
    }
    return false;
}

}
