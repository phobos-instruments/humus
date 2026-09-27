// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/Faceplate.h"

#include <cmath>

#include "core/packs/Categories.h"
#include "core/params/ParamSchema.h"
#include "gui/editor/ControlModel.h"
#include "gui/editor/LayoutModel.h"
#include "gui/editor/ParamRanges.h"
#include "gui/editor/ToggleModel.h"
#include "hum/dsp/LevelMeter.h"
#include "io/PatchDocument.h"

namespace hum {

namespace {

bool ownsLabel(const LayoutSpec::Control& c) {
    using CT = LayoutSpec::ControlType;
    if (c.type == CT::LitButton) return c.extraOr("icon") == "none";
    switch (c.type) {
        case CT::Toggle: case CT::MiniToggle: case CT::FaderBank: case CT::Waveform:
        case CT::MidiKeyboard: case CT::Deck: case CT::DeckPitch: case CT::DeckControls:
        case CT::Momentary: case CT::TapTempo: case CT::LooperTracks: case CT::StepGrid:
        case CT::PatternGrid: case CT::PianoRoll: case CT::SoundMap: case CT::MidiLog:
        case CT::OscLog: case CT::LevelBars: case CT::InletBars: case CT::PictureLamp: case CT::InletLamp: case CT::ThresholdMeter: case CT::PitchReadout:
        case CT::Readout: case CT::TextReadout: case CT::Camera: case CT::GainShapeCurve:
        case CT::PictureField: case CT::SequenceGrid: case CT::HandGestures: case CT::Sigil:
        case CT::IntervalRows: case CT::StepStrip: case CT::WaveDraw: case CT::VuMeter:
        case CT::FieldScope: case CT::SpectrumScope: case CT::TraceScope: case CT::RingFace: case CT::LfoScope: case CT::Formula:
        case CT::TextField: case CT::GainReduction: case CT::ClipGrid: case CT::ScreenButton: case CT::TapeLabel: case CT::KeyedTape:
            return true;
        default:
            return false;
    }
}

bool standsTall(LayoutSpec::ControlType t) {
    using CT = LayoutSpec::ControlType;
    return t == CT::Knob || t == CT::VSlider || t == CT::RangeVSlider || t == CT::RangeKnob || t == CT::RotarySwitch
        || t == CT::LitButton;
}

}

Faceplate::Faceplate(ModelHost& host, std::string organism, const LayoutSpec& spec, BrickViews& views,
                     FaceplateOwner& owner)
    : host_(host), name_(std::move(organism)), spec_(spec), views_(views), owner_(owner) {}

void Faceplate::build() {
    const int family = (int) familyOf(className());
    for (const auto& s : spec_.controls) {
        Control c;
        c.param = s.param;
        c.dimWhen = LayoutCondition::parse(s.extraOr("dim-when"), spec_);
        c.showWhen = LayoutCondition::parse(s.extraOr("show-when"), spec_);
        c.clearWhen = LayoutCondition::parse(s.extraOr("clear-when"), spec_);
        if (s.type == LayoutSpec::ControlType::Label) {
            const auto text = labelctl::setupFor(s);
            c.label = views_.label(text.pill ? LabelKind::Pill : text.section ? LabelKind::Section : LabelKind::Plain,
                                   text.text);
        } else {
            if (!s.label.empty() && !ownsLabel(s))
                c.label = views_.label(standsTall(s.type) ? LabelKind::Above : LabelKind::Beside,
                                       labelctl::setupFor(s).text);
            buildControl(s, c, family);
        }
        if (const auto tip = s.extraOr("tooltip"); !tip.empty())
            for (auto* v : viewsOf(c)) v->setViewTooltip(tip);
        controls_.push_back(std::move(c));
    }
    applyDims();
}

void Faceplate::reloadValues() {
    for (auto& c : controls_) {
        const double value = control::paramValue(host_, name_, c.param);
        if (c.value) {
            c.value->showValue(value);
            syncValueLabel(c);
        }
        if (c.combo && !c.param.empty()) {
            if (c.refillCombo) c.refillCombo();
            c.combo->showSelectedId((int) (value + (c.comboIdsAreValues ? 0 : 1)));
        }
        if (c.toggle) c.toggle->showOn(value >= 0.5);
        if (c.buttons) c.buttons->showIndex((int) std::lround(value) - c.enumFirst);
        if (c.rich) c.rich->reloadValues();
    }
    applyDims();
}

void Faceplate::reloadTextValues() {
    for (auto& c : controls_) {
        if (c.rich) c.rich->reloadText();
        if (c.combo && !c.param.empty() && c.refillCombo) c.refillCombo();
    }
    applyDims();
}

void Faceplate::refreshLive() {
    bool gateMoved = false;
    constexpr int kMaxMeters = LevelMeter::kMax;
    float meters[kMaxMeters] = {};
    const int meterCh = meters_ ? host_.nodeMeter(name_, meters, kMaxMeters) : 0;
    for (auto& c : controls_) {
        if (c.value && c.meterChannel >= 0)
            c.value->showMeter(c.meterChannel < meterCh ? meters[c.meterChannel] : 0.0f);
        if (!c.param.empty()) {
            const bool controlled = paramIsControlled(host_, name_, c.param);
            const bool locked = host_.rollLocked(name_, c.param);
            for (auto* view : viewsOf(c))
                if (view != nullptr) view->showMarks(controlled, locked);
        }
        if (c.value) {
            if (host_.isLiveTracked(name_, c.param)) {
                const double v = host_.liveParamValue(name_, c.param);
                if (std::abs(v - c.value->shownValue()) > 1e-9) {
                    c.value->showLiveValue(v);
                    syncValueLabel(c);
                }
            }
        }
        if (c.toggle && host_.isLiveTracked(name_, c.param)) {
            const bool on = host_.liveParamValue(name_, c.param) >= 0.5;
            if (c.toggle->shownOn() != on) {
                c.toggle->showOn(on);
                gateMoved = true;
            }
        }
        if (c.combo && !c.param.empty() && host_.isLiveTracked(name_, c.param)) {
            const int id = (int) std::lround(host_.liveParamValue(name_, c.param)) + (c.comboIdsAreValues ? 0 : 1);
            if (c.combo->shownSelectedId() != id) {
                c.combo->showSelectedId(id);
                gateMoved = true;
            }
        }
        if (c.refreshMomentary) c.refreshMomentary();
        if (c.buttons && host_.isLiveTracked(name_, c.param)) {
            const int cur = (int) std::lround(host_.liveParamValue(name_, c.param)) - c.enumFirst;
            if (cur >= 0 && cur < c.buttons->optionCount() && c.buttons->shownIndex() != cur) {
                c.buttons->showIndex(cur);
                gateMoved = true;
            }
        }
        if (c.rich) c.rich->refreshLive();
    }
    runClearRules();
    if (gateMoved) applyDims();
}

void Faceplate::openClip(int clip) {
    for (auto& c : controls_)
        if (c.rich) c.rich->openClip(clip);
}

void Faceplate::layout(int width, int height, int collar) {
    using CT = LayoutSpec::ControlType;
    const auto f = layout::fitFor(spec_, width);
    const int labelH = layout::labelHeightFor(f);
    const int boxH = layout::textBoxHeightFor(f);
    const int fillIndex = layout::fillControlIndex(spec_);
    const int slack = layout::slackFor(spec_, f, height, collar);
    for (size_t i = 0; i < controls_.size(); ++i) {
        const auto& s = spec_.controls[i];
        auto& c = controls_[i];
        const auto box = layout::boundsFor(spec_, (int) i, f, collar, slack, fillIndex);
        Rect bounds = box;
        if (s.type == CT::Label) {
            if (c.label) c.label->setViewBounds(bounds);
            continue;
        }
        if (c.label) {
            if (layout::labelSitsAbove(s.type)) {
                c.label->setViewBounds(takeTop(bounds, labelH));
                takeTop(bounds, layout::kLabelGap);
            } else {
                if (c.labelWant < 0) c.labelWant = 2 + c.label->textWidth();
                c.label->setViewBounds(takeLeft(bounds, layout::sideLabelWidth(f, c.labelWant, bounds.w)));
                takeLeft(bounds, layout::kSideLabelGap);
            }
        }
        if (c.value) {
            if (layout::textBoxWidened(c.textBoxWant))
                c.value->setTextBox(layout::textBoxWidth(c.textBoxWant, bounds.w), boxH);
            c.value->setViewBounds(bounds);
        }
        for (BrickView* v : {(BrickView*) c.combo.get(), (BrickView*) c.toggle.get(),
                             (BrickView*) c.buttons.get(), (BrickView*) c.momentary.get(),
                             (BrickView*) c.rich.get()})
            if (v != nullptr) v->setViewBounds(bounds);
    }
}

std::string Faceplate::className() const {
    if (auto* cm = host_.model().byName(name_)) return cm->displayClass;
    return {};
}

bool Faceplate::holds(const LayoutCondition& condition) const {
    return control::holds(host_, name_, condition);
}

std::vector<BrickView*> Faceplate::viewsOf(Control& c) {
    std::vector<BrickView*> out;
    for (BrickView* v : {(BrickView*) c.value.get(), (BrickView*) c.combo.get(), (BrickView*) c.label.get(),
                         (BrickView*) c.toggle.get(), (BrickView*) c.buttons.get(),
                         (BrickView*) c.momentary.get(), (BrickView*) c.rich.get()})
        if (v != nullptr) out.push_back(v);
    return out;
}

void Faceplate::syncValueLabel(Control& c) {
    if (c.showValue && c.label && c.value) c.label->showText(c.value->shownText());
}

void Faceplate::runClearRules() {
    for (auto& c : controls_) {
        if (c.clearWhen.empty() || c.param.empty()) continue;
        const bool now = holds(c.clearWhen);
        const int was = c.clearLatch;
        c.clearLatch = layout::nextLatch(now);
        if (layout::clearFires(was, now, !host_.liveParamText(name_, c.param).empty()))
            host_.setParamText(name_, c.param, "");
    }
}

void Faceplate::applyDims() {
    runClearRules();
    for (auto& c : controls_) {
        if (!c.dimWhen.empty()) {
            const auto state = layout::dimStateFor(holds(c.dimWhen));
            const float alpha = layout::alphaFor(state.dimmed);
            for (auto* v : viewsOf(c)) v->setViewFade(alpha, state.enabled);
        }
        if (!c.showWhen.empty()) {
            if (c.shownViews.empty())
                for (auto* v : viewsOf(c))
                    if (v->viewVisible()) c.shownViews.push_back(v);
            const bool show = layout::visibleFor(true, holds(c.showWhen));
            for (auto* v : c.shownViews) v->setViewVisible(show);
        }
    }
    owner_.gatesApplied();
}

std::optional<double> Faceplate::resetValueFor(const std::string& param) const {
    if (auto* cm = host_.model().byName(name_))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return d.def;
    return std::nullopt;
}

void Faceplate::openMenu(const std::string& param, Point at) const {
    owner_.openMenu(param, at);
}

}
