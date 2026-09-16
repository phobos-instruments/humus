// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/LayoutEditor.h"
#include "core/packs/Categories.h"
#include "gui/bricks/RootCollar.h"
#include "gui/bricks/SoundFileSlot.h"
#include "gui/app/Ember.h"
#include "hum/dsp/LevelMeter.h"
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
bool extraTrue(const std::string& v) { return v == "1" || v == "true"; }

int fillControlIndex(const LayoutSpec& spec) {
    for (size_t i = 0; i < spec.controls.size(); ++i)
        if (extraTrue(spec.controls[i].extraOr("fill-v"))) return (int) i;
    return -1;
}
}

void LayoutEditor::build() {
    const int famProp = (int) familyOf(className());
    for (const auto& s : spec_.controls) {
        Control c;
        c.param = s.param;
        c.dimWhen = LayoutCondition::parse(s.extraOr("dim-when"), spec_);
        c.showWhen = LayoutCondition::parse(s.extraOr("show-when"), spec_);
        c.clearWhen = LayoutCondition::parse(s.extraOr("clear-when"), spec_);
        const int childrenBefore = getNumChildComponents();
        const auto tip = s.extraOr("tooltip");

        if (s.type == LayoutSpec::ControlType::Label) {
            const bool pill = s.extraOr("style") == "pill";
            auto lbl = pill ? std::make_unique<PillLabel>() : std::make_unique<juce::Label>();
            lbl->setText(juce::String::fromUTF8(s.label.c_str()), juce::dontSendNotification);
            lbl->setJustificationType(pill ? juce::Justification::centred
                                           : juce::Justification::centredLeft);
            lbl->setFont(juce::FontOptions(pill ? 12.0f : 11.0f));
            lbl->setColour(juce::Label::textColourId, pill ? Palette::textDim : Palette::text);
            addAndMakeVisible(*lbl);
            c.label = std::move(lbl);
            if (!tip.empty())
            for (int k = childrenBefore; k < getNumChildComponents(); ++k)
                if (auto* t = dynamic_cast<juce::SettableTooltipClient*>(getChildComponent(k)))
                    t->setTooltip(juce::String::fromUTF8(tip.c_str()));
        controls_.push_back(std::move(c));
            continue;
        }

        using CT = LayoutSpec::ControlType;
        const bool vertical = s.type == CT::Knob || s.type == CT::VSlider
                            || s.type == CT::RangeVSlider || s.type == CT::RotarySwitch;
        const bool ownsLabel = s.type == CT::Toggle || s.type == CT::MiniToggle
                             || s.type == CT::FaderBank || s.type == CT::Waveform
                             || s.type == CT::MidiKeyboard || s.type == CT::Deck
                             || s.type == CT::DeckPitch || s.type == CT::DeckControls
                             || s.type == CT::Momentary || s.type == CT::TapTempo
                             || s.type == CT::LooperTracks
                             || s.type == CT::StepGrid || s.type == CT::PatternGrid
                             || s.type == CT::PianoRoll || s.type == CT::SoundMap
                             || s.type == CT::MidiLog || s.type == CT::OscLog || s.type == CT::LevelBars
                             || s.type == CT::ThresholdMeter
                             || s.type == CT::PitchReadout
                             || s.type == CT::Readout
                             || s.type == CT::TextReadout
                             || s.type == CT::Camera
                             || s.type == CT::GainShapeCurve
                             || s.type == CT::PictureField
                             || s.type == CT::SequenceGrid
                             || s.type == CT::HandGestures
                             || s.type == CT::Sigil
                             || s.type == CT::IntervalRows
                             || s.type == CT::StepStrip
                             || s.type == CT::WaveDraw
                             || s.type == CT::VuMeter
                             || s.type == CT::FieldScope
                             || s.type == CT::SpectrumScope
                             || s.type == CT::LfoScope
                             || s.type == CT::Formula
                             || s.type == CT::TextField
                             || s.type == CT::GainReduction
                             || s.type == CT::ClipGrid
                             || s.type == CT::ScreenButton;
        if (!s.label.empty() && !ownsLabel) {
            auto lbl = std::make_unique<juce::Label>();
            lbl->setText(juce::String::fromUTF8(s.label.c_str()), juce::dontSendNotification);
            lbl->setJustificationType(vertical ? juce::Justification::centred
                                               : juce::Justification::centredRight);
            lbl->setFont(juce::FontOptions(10.0f));
            lbl->setColour(juce::Label::textColourId, Palette::textDim);
            lbl->setMinimumHorizontalScale(0.6f);
            addAndMakeVisible(*lbl);
            c.label = std::move(lbl);
        }

        if (buildBrick(s, c)) {
            if (!tip.empty())
            for (int k = childrenBefore; k < getNumChildComponents(); ++k)
                if (auto* t = dynamic_cast<juce::SettableTooltipClient*>(getChildComponent(k)))
                    t->setTooltip(juce::String::fromUTF8(tip.c_str()));
        controls_.push_back(std::move(c));
            continue;
        }

        ValueSetup v;
        v.pn = s.param;
        v.cn = name_;
        v.family = famProp;
        for (auto& d : schemaFor(className())) {
            if (d.name == s.param) {
                v.lo = d.min; v.hi = d.max; v.isInt = d.isInt;
                v.unit = unitResolve(d.name, d.unit, d.min, d.max);
                break;
            }
        }
        buildValueControl(s, c, v);
        if (!tip.empty())
            for (int k = childrenBefore; k < getNumChildComponents(); ++k)
                if (auto* t = dynamic_cast<juce::SettableTooltipClient*>(getChildComponent(k)))
                    t->setTooltip(juce::String::fromUTF8(tip.c_str()));
        controls_.push_back(std::move(c));
    }
    applyDims();
}

void LayoutEditor::paintSkin(juce::Graphics& g) {
    skin_.paintPanel(g, getLocalBounds().toFloat());
    const auto f = fit();
    const juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale((float) f.kx, (float) f.ky)
                       .translated((float) f.xOff, (float) collarHeight()));
    skin_.paintFace(g, {0.0f, 0.0f, (float) spec_.width, (float) spec_.height});
}

void LayoutEditor::resized() {
    if (deferLayout_) {
        layoutDirty_ = true;
        return;
    }
    perf::Scope scope("editor.layout");
    using CT = LayoutSpec::ControlType;
    const auto f = fit();
    const double kx = f.kx, ky = f.ky;
    const int xOff = f.xOff;
    auto scx = [kx](int v) { return (int) std::lround(v * kx); };
    auto scy = [ky](int v) { return (int) std::lround(v * ky); };
    const int labelH = juce::jlimit(11, 18, (int) std::lround(14 * ky));
    const int boxH = ky >= 0.95 ? 20 : ky >= 0.75 ? 16 : 14;
    const int fillIndex = fillControlIndex(spec_);
    const int fillBottom = fillIndex < 0 ? 0
                                         : spec_.controls[(size_t) fillIndex].y
                                               + spec_.controls[(size_t) fillIndex].h;
    const int slack = fillIndex < 0
                          ? 0
                          : juce::jmax(0, getHeight() - collarHeight() - scy(spec_.height));
    for (size_t i = 0; i < controls_.size(); ++i) {
        const auto& s = spec_.controls[i];
        auto& c = controls_[i];
        const int dy = (fillIndex >= 0 && s.y >= fillBottom) ? slack : 0;
        const int dh = ((int) i == fillIndex) ? slack : 0;
        juce::Rectangle<int> bounds(xOff + scx(s.x), collarHeight() + scy(s.y) + dy,
                                    scx(s.w), scy(s.h) + dh);

        if (s.type == CT::Label) {
            if (c.label) c.label->setBounds(bounds);
            continue;
        }

        if (c.label) {
            const bool vertical = s.type == CT::Knob || s.type == CT::VSlider ||
                                  s.type == CT::RangeVSlider || s.type == CT::RotarySwitch;
            if (vertical) {
                c.label->setBounds(bounds.removeFromTop(labelH));
                bounds.removeFromTop(2);
            } else {
                if (c.labelWant < 0)
                    c.labelWant = 2 + (int) std::ceil(juce::TextLayout::getStringWidth(
                                          c.label->getFont(), c.label->getText()));
                const int lw = juce::jlimit(bounds.getWidth() / 5, bounds.getWidth() / 2,
                                            juce::jmin(c.labelWant, scx(72)));
                c.label->setBounds(bounds.removeFromLeft(lw));
                bounds.removeFromLeft(4);
            }
        }
        if (c.slider) {
            if (c.textBoxWant > 48)
                c.slider->setTextBoxStyle(
                    juce::Slider::TextBoxLeft, false,
                    juce::jlimit(48, juce::jmax(48, (bounds.getWidth() - 40) / 8 * 8), c.textBoxWant),
                    boxH);
            c.slider->setBounds(bounds);
        }
        if (c.stepPrev && c.stepNext) {
            const int a = std::min(18, bounds.getWidth() / 6);
            c.stepPrev->setBounds(bounds.removeFromLeft(a));
            c.stepNext->setBounds(bounds.removeFromRight(a));
        }
        if (c.combo) c.combo->setBounds(bounds);
        if (c.range) c.range->setBounds(bounds);
        if (c.toggle) c.toggle->setBounds(bounds);
        if (c.textToggle) c.textToggle->setBounds(bounds);
        if (!c.radioRow.empty()) {
            auto row = bounds;
            const int w = row.getWidth() / (int) c.radioRow.size();
            for (size_t j = 0; j < c.radioRow.size(); ++j)
                c.radioRow[j]->setBounds(j + 1 == c.radioRow.size() ? row : row.removeFromLeft(w));
        }
        if (c.brick) c.brick->setBounds(bounds);
        if (c.rich) c.rich->setBounds(bounds);
    }
}

void LayoutEditor::refreshAutomatedValues() {
    bool gateMoved = false;
    constexpr int kMaxMeters = hum::LevelMeter::kMax;
    float meters[kMaxMeters] = {};
    const int meterCh = meters_ ? host_.nodeMeter(name_, meters, kMaxMeters) : 0;
    for (auto& c : controls_) {
        if (c.slider && c.meterChannel >= 0)
            c.slider->setMeterLevel(c.meterChannel < meterCh
                                        ? meters[c.meterChannel] : 0.0f);
        if (c.slider) {
            const bool tracked = host_.isLiveTracked(name_, c.param);
            c.slider->setExternallyControlled(
                host_.isExternallyControlled(name_, c.param));
            c.slider->setRollLocked(host_.rollLocked(name_, c.param));
            if (tracked) {
                double v = host_.liveParamValue(name_, c.param);
                if (std::abs(v - c.slider->getValue()) > 1e-9) {
                    c.slider->setValue(v, juce::dontSendNotification);
                    syncValueLabel(c);
                    ember::stamp(*c.slider);
                }
            }
        }
        if (juce::Button* b = c.toggle ? (juce::Button*) c.toggle.get()
                                       : (juce::Button*) c.textToggle.get();
            b != nullptr && host_.isLiveTracked(name_, c.param)) {
            const bool on = host_.liveParamValue(name_, c.param) >= 0.5;
            if (b->getToggleState() != on) {
                b->setToggleState(on, juce::dontSendNotification);
                gateMoved = true;
            }
        }
        if (c.refreshBrickLive) c.refreshBrickLive();
        if (!c.radioRow.empty() && host_.isLiveTracked(name_, c.param)) {
            const int cur = (int) std::lround(host_.liveParamValue(name_, c.param)) - c.enumFirst;
            if (cur >= 0 && cur < (int) c.radioRow.size()
                && !c.radioRow[(size_t) cur]->getToggleState()) {
                for (size_t j = 0; j < c.radioRow.size(); ++j)
                    c.radioRow[j]->setToggleState((int) j == cur, juce::dontSendNotification);
                gateMoved = true;
            }
        }
        if (c.range) c.range->refresh();
        if (c.rich) c.rich->refreshAutomatedValues();
    }
    runClearRules();
    if (gateMoved) applyDims();
}

void LayoutEditor::paint(juce::Graphics& g) {
    perf::Scope scope("editor.paint");
    if (!skin_.empty()) paintSkin(g);
    else collar::paintSoil(g, getLocalBounds().toFloat(), familyOf(className()));
    if (wearsCollar())
        collar::paint(g, host_, name_, getLocalBounds().removeFromTop(collar::kHeight));
}

int LayoutEditor::collarHeight() const { return wearsCollar() ? collar::kHeight : 0; }

bool LayoutEditor::fileSlotShows(const std::string& param, const std::string& fileName) const {
    for (const auto& c : controls_)
        if (auto* slot = c.param == param ? dynamic_cast<SoundFileSlot*>(c.brick.get()) : nullptr)
            return slot->shownName() == juce::String(juce::CharPointer_UTF8(fileName.c_str()));
    return false;
}

}
