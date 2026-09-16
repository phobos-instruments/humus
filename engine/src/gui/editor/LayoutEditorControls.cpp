// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/LayoutEditor.h"
#include "gui/editor/ParamReset.h"
#include "gui/bricks/FileTransport.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/common/PerfLog.h"
#include "gui/editor/BrickBindings.h"
#include "gui/tracks/StripHover.h"
#include <cmath>
#include <cstdlib>
#include "core/packs/PackRegistry.h"
#include "gui/editor/Mappable.h"
#include "gui/common/Localisation.h"
#include "gui/style/OriginColours.h"
#include "hum/ParseInt.h"

namespace hum {

namespace {
class WaveGlyphButton : public juce::Button {
public:
    WaveGlyphButton(const juce::String& name, WaveGlyph g, juce::Colour lit)
        : juce::Button(name), glyph_(g), lit_(lit) {}

    juce::String glyphName() const { return waveGlyphDescription(glyph_); }

    void paintButton(juce::Graphics& g, bool over, bool) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(getToggleState() ? lit_
                    : over            ? Palette::panelLight.brighter(0.15f)
                                      : Palette::panelLight);
        g.fillRect(r);
        g.setColour(Palette::border);
        g.drawRect(r, 1.0f);

        drawWaveGlyph(g, glyph_, r,
                      getToggleState() ? Palette::background : Palette::textDim);
    }

private:
    WaveGlyph glyph_;
    juce::Colour lit_;
};

}

void LayoutEditor::buildValueControl(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    switch (s.type) {
    case LayoutSpec::ControlType::Knob:
        case LayoutSpec::ControlType::VSlider:
        case LayoutSpec::ControlType::HSlider:
            buildSliderControl(s, c, v);
            break;
    case LayoutSpec::ControlType::RotarySwitch:
            buildRotarySwitch(s, c, v);
            break;
    case LayoutSpec::ControlType::RangeVSlider:
            buildRangeSlider(s, c, v);
            break;
    case LayoutSpec::ControlType::Toggle:
            buildToggle(s, c, v);
            break;
    case LayoutSpec::ControlType::MiniToggle:
            buildMiniToggle(s, c, v);
            break;
    case LayoutSpec::ControlType::EnumButtons:
            buildEnumButtons(s, c, v);
            break;
    case LayoutSpec::ControlType::RhythmicUnit:
            buildRhythmicUnit(s, c, v);
            break;
    case LayoutSpec::ControlType::SoundFile:
            buildSoundFile(s, c, v);
            break;
    case LayoutSpec::ControlType::ScaleFile:
            buildScaleFile(s, c, v);
            break;
    case LayoutSpec::ControlType::BankFile:
            buildBankFile(s, c, v);
            break;
    case LayoutSpec::ControlType::FileTransport:
            buildFileTransport(s, c, v);
            break;
    case LayoutSpec::ControlType::Combo:
            buildCombo(s, c, v);
            break;
    case LayoutSpec::ControlType::IntSpinner:
        case LayoutSpec::ControlType::DoubleSpinner:
            buildSpinner(s, c, v);
            break;
        default:
            break;
    }
}

void LayoutEditor::buildSliderControl(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    double lo = v.lo;
    double hi = v.hi;
    const bool paramIsInt = v.isInt;
    const Unit paramUnit = v.unit;
    const std::string pn = v.pn;
    const std::string cn = v.cn;
    const int famProp = v.family;
        const bool fader = s.type != LayoutSpec::ControlType::Knob;
        auto style = s.type == LayoutSpec::ControlType::Knob
                         ? juce::Slider::RotaryVerticalDrag
                         : (s.type == LayoutSpec::ControlType::VSlider
                                ? juce::Slider::LinearVertical
                                : juce::Slider::LinearHorizontal);
        auto textPos = fader ? juce::Slider::NoTextBox : juce::Slider::TextBoxBelow;
        auto slider = std::make_unique<ParamSlider>(style, textPos);
        slider->getProperties().set("family", famProp);
        slider->paramLabel = juce::String::fromUTF8((s.label.empty() ? s.param : s.label).c_str());
        slider->setRange(lo, hi, 0.0);
        if (s.logarithmic) slider->configureScaling();
        slider->setNumDecimalPlacesToDisplay(paramIsInt ? 0 : s.decimalPlaces);
        slider->setUnit(paramUnit);
        if (const auto m = s.extraOr("meter"); !m.empty()) {
            c.meterChannel = std::atoi(m.c_str());
            meters_ = true;
        }
        if (fader) slider->setPopupDisplayEnabled(true, true, this);
        slider->setValue(modelValue(pn), juce::dontSendNotification);
        enableDoubleClickReset(*slider, host_, cn, pn);
        auto* sp = slider.get();
        const auto sv = s.extraOr("show-value");
        c.showValue = (sv == "1" || sv == "true") && c.label != nullptr;
        auto* lb = c.showValue ? c.label.get() : nullptr;
        if (lb) lb->setText(sp->getTextFromValue(sp->getValue()),
                            juce::dontSendNotification);
        slider->onValueChange = [this, sp, cn, pn, lb] {
            host_.editParam(cn, pn, sp->getValue());
            if (lb) lb->setText(sp->getTextFromValue(sp->getValue()),
                                juce::dontSendNotification);
        };
        slider->onDragStart = [this, cn, pn] { host_.beginParamDrag(cn, pn); };
        slider->onDragEnd = [this] { host_.endParamDrag(); };
        sp->setParamId(cn, pn);
        sp->onPopup = [this, cn, pn](juce::Point<int> pos) {
            showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
        };
        if (!stripChannelInlets(pn).empty())
            sp->tooltipProvider = [this, cn, pn] {
                return stripSourceTip(pn, host_.model().connections, cn);
            };
        addAndMakeVisible(*slider);
        c.slider = std::move(slider);
}

void LayoutEditor::buildRotarySwitch(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    double lo = v.lo;
    double hi = v.hi;
    const std::string pn = v.pn;
    const std::string cn = v.cn;
    const int famProp = v.family;
        auto slider = std::make_unique<ParamSlider>(juce::Slider::RotaryVerticalDrag,
                                                    juce::Slider::TextBoxBelow);
        slider->getProperties().set("family", famProp);
        slider->paramLabel = juce::String::fromUTF8((s.label.empty() ? s.param : s.label).c_str());
        slider->setRange(lo, hi, 1.0);
        const auto opts = s.options;
        slider->textFromValueFunction = [opts](double v) {
            const int i = (int) std::lround(v);
            return i >= 0 && i < (int) opts.size()
                       ? juce::String::fromUTF8(opts[(size_t) i].c_str())
                       : juce::String(i);
        };
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 56, 16);
        slider->setColour(juce::Slider::textBoxTextColourId, Palette::text);
        slider->setValue(modelValue(pn), juce::dontSendNotification);
        enableDoubleClickReset(*slider, host_, cn, pn);
        auto* sp = slider.get();
        slider->onValueChange = [this, sp, cn, pn] { host_.editParam(cn, pn, sp->getValue()); };
        slider->onDragStart = [this, cn, pn] { host_.beginParamDrag(cn, pn); };
        slider->onDragEnd = [this] { host_.endParamDrag(); };
        sp->setParamId(cn, pn);
        sp->onPopup = [this, cn, pn](juce::Point<int> pos) {
            showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
        };
        addAndMakeVisible(*slider);
        c.slider = std::move(slider);
}

void LayoutEditor::buildRangeSlider(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    double lo = v.lo;
    double hi = v.hi;
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto range = std::make_unique<RangeSlider>(host_, cn, pn, lo, hi, s.decimalPlaces,
                                                   s.logarithmic);
        range->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
        addAndMakeVisible(*range);
        c.range = std::move(range);
}

void LayoutEditor::buildToggle(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto toggle = std::make_unique<Mappable<juce::ToggleButton>>(
            juce::String::fromUTF8(s.label.c_str()));
        toggle->setColour(juce::ToggleButton::textColourId, Palette::text);
        toggle->setToggleState(modelValue(pn) >= 0.5, juce::dontSendNotification);
        toggle->onClick = [this, toggle = toggle.get(), cn, pn] {
            host_.editParam(cn, pn, toggle->getToggleState() ? 1.0 : 0.0);
            applyDims();
        };
        toggle->onRightClick = [this, cn, pn](juce::Point<int> pos) {
            showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
        };
        addAndMakeVisible(*toggle);
        c.toggle = std::move(toggle);
}

void LayoutEditor::buildMiniToggle(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto tb = std::make_unique<Mappable<juce::TextButton>>(
            juce::String::fromUTF8(s.label.c_str()));
        tb->setClickingTogglesState(true);
        tb->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        tb->setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        tb->setColour(juce::TextButton::textColourOffId, Palette::textDim);
        tb->setColour(juce::TextButton::textColourOnId, Palette::background);
        tb->setToggleState(modelValue(pn) >= 0.5, juce::dontSendNotification);
        if (const auto onLabel = s.extraOr("on-label", ""); !onLabel.empty()) {
            auto sync = [p = tb.get(), on = juce::String::fromUTF8(onLabel.c_str()),
                         off = juce::String::fromUTF8(s.label.c_str())] {
                p->setButtonText(p->getToggleState() ? on : off);
            };
            tb->onStateChange = sync;
            sync();
        }
        tb->onClick = [this, p = tb.get(), cn, pn] {
            host_.editParam(cn, pn, p->getToggleState() ? 1.0 : 0.0);
            applyDims();
        };
        tb->onRightClick = [this, cn, pn](juce::Point<int> pos) {
            showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
        };
        addAndMakeVisible(*tb);
        c.textToggle = std::move(tb);
}

void LayoutEditor::buildEnumButtons(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string pn = v.pn;
    const std::string cn = v.cn;
    const int famProp = v.family;
        const bool waveIcons = s.extraOr("icons") == "waves";
        std::vector<juce::Button*> row;
        const int first = std::atoi(s.extraOr("first", "0").c_str());
        c.enumFirst = first;
        const int cur = (int) std::lround(modelValue(pn)) - first;
        for (size_t i = 0; i < s.options.size(); ++i) {
            const auto label = juce::String::fromUTF8(s.options[i].c_str());
            std::unique_ptr<juce::Button> btn;
            auto mapMenu = [this, cn, pn](juce::Point<int> pos) {
                showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
            };
            if (waveIcons) {
                auto wb = std::make_unique<Mappable<WaveGlyphButton>>(
                    label, waveGlyphFor(label),
                    Palette::familyAccent((Family) famProp));
                wb->setTooltip(wb->glyphName());
                wb->onRightClick = mapMenu;
                btn = std::move(wb);
            } else {
                auto tb = std::make_unique<Mappable<juce::TextButton>>(label);
                tb->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
                tb->setColour(juce::TextButton::buttonOnColourId,
                              Palette::familyAccent((Family) famProp));
                tb->setColour(juce::TextButton::textColourOffId, Palette::textDim);
                tb->setColour(juce::TextButton::textColourOnId, Palette::background);
                tb->setConnectedEdges(
                    (i > 0 ? juce::Button::ConnectedOnLeft : 0)
                    | (i + 1 < s.options.size() ? juce::Button::ConnectedOnRight : 0));
                tb->onRightClick = mapMenu;
                btn = std::move(tb);
            }
            btn->setToggleState((int) i == cur, juce::dontSendNotification);
            row.push_back(btn.get());
            addAndMakeVisible(*btn);
            c.radioRow.push_back(std::move(btn));
        }
        for (size_t i = 0; i < row.size(); ++i)
            row[i]->onClick = [this, cn, pn, i, row, first] {
                host_.editParam(cn, pn, (double) (first + (int) i));
                for (size_t j = 0; j < row.size(); ++j)
                    row[j]->setToggleState(j == i, juce::dontSendNotification);
                applyDims();
            };
}

void LayoutEditor::buildSpinner(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    double lo = v.lo;
    double hi = v.hi;
    const bool paramIsInt = v.isInt;
    const Unit paramUnit = v.unit;
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto style = juce::Slider::IncDecButtons;
        auto textPos = juce::Slider::TextBoxLeft;
        auto slider = std::make_unique<ParamSlider>(style, textPos);
        slider->paramLabel = juce::String::fromUTF8((s.label.empty() ? s.param : s.label).c_str());
        slider->setColour(juce::Slider::textBoxTextColourId, Palette::text);
        slider->setColour(juce::Slider::textBoxBackgroundColourId, Palette::background);
        slider->setColour(juce::Slider::textBoxOutlineColourId, Palette::border);
        {
            const juce::Font boxFont(juce::FontOptions(14.0f));
            for (double v : {lo, hi, (lo + hi) * 0.5})
                c.textBoxWant = juce::jmax(c.textBoxWant, 8 + (int) std::ceil(
                    juce::TextLayout::getStringWidth(
                        boxFont, unitFormat(paramUnit, v, lo, hi))));
        }
        slider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 20);
        const bool ints = s.type == LayoutSpec::ControlType::IntSpinner;
        slider->setRange(lo, hi, ints ? 1.0 : 0.0);
        if (!ints) {
            std::vector<double> ladder;
            juce::StringArray rungs;
            rungs.addTokens(juce::String(s.extraOr("steps")), " ,", "");
            for (const auto& r : rungs)
                if (r.trim().isNotEmpty()) ladder.push_back(r.getDoubleValue());
            slider->setStepping((hi - lo) / 100.0, std::move(ladder));
        }
        slider->setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_Vertical);
        slider->setNumDecimalPlacesToDisplay(
            (paramIsInt || s.type == LayoutSpec::ControlType::IntSpinner)
                ? 0 : s.decimalPlaces);
        slider->setUnit(paramUnit);
        slider->setValue(modelValue(pn), juce::dontSendNotification);
        enableDoubleClickReset(*slider, host_, cn, pn);
        auto* sp = slider.get();
        slider->onValueChange = [this, sp, cn, pn] { host_.editParam(cn, pn, sp->getValue()); };
        slider->onDragStart = [this, cn, pn] { host_.beginParamDrag(cn, pn); };
        slider->onDragEnd = [this] { host_.endParamDrag(); };
        sp->setParamId(cn, pn);
        sp->onPopup = [this, cn, pn](juce::Point<int> pos) {
            showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
        };
        addAndMakeVisible(*slider);
        c.slider = std::move(slider);
}

}
