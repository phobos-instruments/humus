// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/properties/MappingInspector.h"

#include <cmath>

#include "gui/style/ControlGlyph.h"

namespace hum {

namespace {
constexpr int kPad = 16, kRowH = 26, kCaptionH = 16;
}

MappingInspector::MappingInspector() {
    label(title_, {}, 14.0f, false);
    title_.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    label(live_, {}, 11.5f, true);
    live_.setJustificationType(juce::Justification::centredRight);
    label(origin_, {}, 12.0f, false);
    label(kindLabel_, tr("mapping-shape.kind", "KIND"), 10.5f, true);
    label(modeLabel_, tr("mapping-shape.behaviour", "BEHAVIOUR"), 10.5f, true);
    label(hint_, {}, 11.5f, true);
    hint_.setJustificationType(juce::Justification::topLeft);
    label(empty_, {}, 12.5f, true);
    empty_.setJustificationType(juce::Justification::centred);

    for (auto* b : {&message, &channel, &device}) addChildComponent(*b);
    message.setTooltip(tr("parameter-control.which-message", "Which kind of MIDI message this answers to"));
    channel.setTooltip(tr("parameter-control.which-channel", "Which MIDI channel this answers to"));
    device.setTooltip(tr("parameter-control.which-controller", "Which controller this mapping answers to"));
    number.setJustification(juce::Justification::centred);
    addChildComponent(number);
    for (auto* ed : {&min, &max}) {
        ed->setJustification(juce::Justification::centred);
        addChildComponent(*ed);
    }
    addChildComponent(held_);

    types_.onPick = [this](ControlType t) { shape_.type = t; emit(); refresh(); };
    buttons_.onPick = [this](ButtonMode m) { shape_.button = m; emit(); refresh(); };
    faders_.onPick = [this](FaderMode m) { shape_.fader = m; emit(); refresh(); };
    encoders_.onPick = [this](EncoderFormat f) { shape_.encoder = f; emit(); refresh(); };
    encoders_.setOptions({EncoderFormat::TwosComplement, EncoderFormat::Offset64, EncoderFormat::SignBit},
                         [](EncoderFormat f) { return modeText(f); });
    for (juce::Component* c : {(juce::Component*) &types_, (juce::Component*) &buttons_,
                               (juce::Component*) &faders_, (juce::Component*) &encoders_})
        addChildComponent(*c);

    label(smoothLabel_, tr("mapping-shape.smoothing-s", "Smoothing (s)"), 12.0f, true);
    smooth_.setInputRestrictions(6, "0123456789.");
    smooth_.setRange(0.0, kMaxSmoothingSeconds, false, kSmoothingPerPixel);
    smooth_.format = [](double v) { return juce::String(v, 2); };
    commitOn(smooth_, [this] {
        shape_.smoothing = juce::jlimit(0.0, kMaxSmoothingSeconds, smooth_.getText().getDoubleValue());
    });
    label(thresholdLabel_, tr("mapping-shape.threshold-0-127", "Threshold (0-127)"), 12.0f, true);
    threshold_.setInputRestrictions(3, "0123456789");
    threshold_.setRange(0.0, kMidiMaxD, true);
    threshold_.parse = [](const juce::String& t) { return (double) t.getIntValue(); };
    threshold_.format = [](double v) { return juce::String((int) std::lround(v)); };
    commitOn(threshold_, [this] {
        shape_.threshold = juce::jlimit(0, kMidiMax, threshold_.getText().getIntValue()) / kMidiMaxD;
    });
    label(stepLabel_, tr("mapping-shape.step-percent", "Step (%)"), 12.0f, true);
    step_.setInputRestrictions(6, "0123456789.");
    step_.setRange(kMinStepPercent, 100.0, false, 0.1);
    step_.format = [](double v) { return juce::String(v, 1); };
    commitOn(step_, [this] {
        shape_.step = juce::jlimit(kMinStepPercent, 100.0, step_.getText().getDoubleValue()) / 100.0;
    });
    inverted_.setButtonText(tr("mapping-shape.inverted", "Inverted"));
    inverted_.onClick = [this] { shape_.inverted = inverted_.getToggleState(); emit(); refresh(); };
    addChildComponent(inverted_);

    label(fromWord_, tr("parameter-control.from-word", "from"), 11.5f, true);
    fromWord_.setJustificationType(juce::Justification::centredLeft);
    label(toWord_, tr("parameter-control.to-word", "to"), 11.5f, true);
    toWord_.setJustificationType(juce::Justification::centred);
    label(unitWord_, {}, 11.5f, true);

    curve_.onChanged = [this] { shape_.curve = curve_.points(); emit(); };
    addChildComponent(curve_);
    addChildComponent(timeline_);
    addChildComponent(stairs_);
    clear({});
}

void MappingInspector::label(juce::Label& l, const juce::String& text, float size, bool dim) {
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::FontOptions(size));
    l.setColour(juce::Label::textColourId, dim ? Palette::textDim : Palette::text);
    l.setInterceptsMouseClicks(false, false);
    addChildComponent(l);
}

void MappingInspector::commitOn(DragNumberEditor& ed, std::function<void()> write) {
    ed.setJustification(juce::Justification::centred);
    auto commit = [this, write] { write(); emit(); refresh(); };
    ed.onReturnKey = commit;
    ed.onFocusLost = commit;
    addChildComponent(ed);
}

void MappingInspector::show(const InspectedSource& s) {
    has_ = true;
    shape_ = s.shape;
    midi_ = s.family == ControlFamily::Midi;
    numbered_ = s.numbered;
    bounded_ = s.bounded;
    title_.setText(s.title, juce::dontSendNotification);
    origin_.setText(s.origin, juce::dontSendNotification);
    message.setButtonText(s.message);
    number.setText(s.number, juce::dontSendNotification);
    channel.setButtonText(s.channel);
    device.setButtonText(s.device);
    held_.setHeld(s.held);
    combo_ = !s.held.empty();
    min.setText(s.minText, juce::dontSendNotification);
    max.setText(s.maxText, juce::dontSendNotification);
    unitWord_.setText(s.unit, juce::dontSendNotification);
    curve_.setAxes(s.axes);
    if (s.family != family_ || types_.count() == 0) {
        family_ = s.family;
        types_.setOptions(controlTypesFor(family_), [](ControlType t) { return modeText(t); },
                          [](juce::Graphics& g, ControlType t, juce::Rectangle<float> r, juce::Colour c) {
                              paintControlGlyph(g, t, r, c);
                          });
        buttons_.setOptions(buttonModesFor(family_), [](ButtonMode m) { return modeText(m); });
        faders_.setOptions(faderModesFor(family_), [](FaderMode m) { return modeText(m); });
    }
    refresh();
}

void MappingInspector::clear(const juce::String& why) {
    has_ = false;
    empty_.setText(why, juce::dontSendNotification);
    live_.setText({}, juce::dontSendNotification);
    refresh();
}

void MappingInspector::setLive(const juce::String& readout, double input01, double waitingAt01) {
    if (live_.getText() != readout) live_.setText(readout, juce::dontSendNotification);
    curve_.setLive(input01, waitingAt01);
}

void MappingInspector::refresh() {
    const bool fader = has_ && shape_.isFader();
    const bool button = has_ && shape_.isButton();
    const bool encoder = has_ && shape_.isEncoder();
    const bool stepped = encoder || (button && buttonModeUsesStep(shape_.button));
    for (juce::Component* c : {(juce::Component*) &title_, (juce::Component*) &live_,
                               (juce::Component*) &kindLabel_, (juce::Component*) &modeLabel_,
                               (juce::Component*) &hint_, (juce::Component*) &types_})
        c->setVisible(has_);
    empty_.setVisible(!has_);
    for (juce::Component* c : {(juce::Component*) &message, (juce::Component*) &channel,
                               (juce::Component*) &device})
        c->setVisible(has_ && midi_);
    number.setVisible(has_ && midi_ && numbered_);
    held_.setVisible(has_ && midi_ && combo_);
    origin_.setVisible(has_ && !midi_);
    for (juce::Component* c : {(juce::Component*) &min, (juce::Component*) &max,
                               (juce::Component*) &fromWord_, (juce::Component*) &toWord_,
                               (juce::Component*) &unitWord_})
        c->setVisible(has_ && bounded_);
    types_.setSelected(shape_.type);
    buttons_.setVisible(button);
    buttons_.setSelected(shape_.button);
    faders_.setVisible(fader && faders_.count() > 1);
    faders_.setSelected(shape_.fader);
    encoders_.setVisible(encoder);
    encoders_.setSelected(shape_.encoder);
    curve_.setVisible(fader);
    timeline_.setVisible(button);
    stairs_.setVisible(encoder);
    smoothLabel_.setVisible(fader);
    smooth_.setVisible(fader);
    thresholdLabel_.setVisible(button);
    threshold_.setVisible(button);
    stepLabel_.setVisible(stepped);
    step_.setVisible(stepped);
    inverted_.setVisible(button || encoder);
    curve_.setPoints(shape_.curve);
    timeline_.setShape(shape_);
    stairs_.setShape(shape_);
    smooth_.setText(juce::String(shape_.smoothing, 2), juce::dontSendNotification);
    threshold_.setText(juce::String((int) std::lround(shape_.threshold * kMidiMaxD)), juce::dontSendNotification);
    step_.setText(juce::String(shape_.step * 100.0, 1), juce::dontSendNotification);
    inverted_.setToggleState(shape_.inverted, juce::dontSendNotification);
    hint_.setText(has_ ? behaviourHint(shape_) : juce::String(), juce::dontSendNotification);
    resized();
    repaint();
}

void MappingInspector::paint(juce::Graphics& g) {
    g.setColour(Palette::border.withAlpha(alpha::strong));
    g.drawVerticalLine(0, 0.0f, (float) getHeight());
}

void MappingInspector::layoutSettings(juce::Rectangle<int> row) {
    auto place = [&row](juce::Component& l, juce::Component& ed, int lw) {
        if (!l.isVisible()) return;
        l.setBounds(row.removeFromLeft(lw));
        ed.setBounds(row.removeFromLeft(58).reduced(0, 2));
        row.removeFromLeft(18);
    };
    place(smoothLabel_, smooth_, 92);
    place(thresholdLabel_, threshold_, 116);
    place(stepLabel_, step_, 62);
    if (inverted_.isVisible()) inverted_.setBounds(row.removeFromLeft(100));
}

void MappingInspector::resized() {
    auto area = getLocalBounds().reduced(kPad);
    empty_.setBounds(area);
    if (!has_) return;
    auto head = area.removeFromTop(22);
    live_.setBounds(head.removeFromRight(190));
    title_.setBounds(head);
    area.removeFromTop(8);
    auto line = area.removeFromTop(kRowH).reduced(0, 2);
    if (midi_) {
        auto pill = line.removeFromLeft(held_.isVisible() ? 110 : 0);
        held_.setBounds(pill);
        if (held_.isVisible()) line.removeFromLeft(6);
        message.setBounds(line.removeFromLeft(78));
        line.removeFromLeft(4);
        if (number.isVisible()) {
            number.setBounds(line.removeFromLeft(54).reduced(0, 1));
            line.removeFromLeft(10);
        }
        channel.setBounds(line.removeFromLeft(66));
        line.removeFromLeft(4);
        device.setBounds(line.removeFromLeft(juce::jmin(130, line.getWidth())));
    } else {
        origin_.setBounds(line);
    }
    area.removeFromTop(16);
    kindLabel_.setBounds(area.removeFromTop(kCaptionH));
    area.removeFromTop(2);
    types_.setBounds(area.removeFromTop(30).withWidth(juce::jmin(area.getWidth(), 138 * types_.count())));
    area.removeFromTop(12);
    modeLabel_.setBounds(area.removeFromTop(kCaptionH));
    area.removeFromTop(2);
    auto modes = area.removeFromTop(kRowH);
    buttons_.setBounds(modes.withWidth(juce::jmin(modes.getWidth(), 82 * buttons_.count())));
    faders_.setBounds(modes.withWidth(juce::jmin(modes.getWidth(), 100 * faders_.count())));
    encoders_.setBounds(modes.withWidth(juce::jmin(modes.getWidth(), 88 * 3)));
    area.removeFromTop(6);
    hint_.setBounds(area.removeFromTop(32));
    if (bounded_) {
        auto range = area.removeFromBottom(kRowH);
        fromWord_.setBounds(range.removeFromLeft(40));
        min.setBounds(range.removeFromLeft(96).reduced(0, 1));
        toWord_.setBounds(range.removeFromLeft(32));
        max.setBounds(range.removeFromLeft(96).reduced(0, 1));
        range.removeFromLeft(6);
        unitWord_.setBounds(range.removeFromLeft(60));
        area.removeFromBottom(10);
    }
    layoutSettings(area.removeFromBottom(kRowH));
    area.removeFromBottom(10);
    for (juce::Component* c : {(juce::Component*) &curve_, (juce::Component*) &timeline_,
                               (juce::Component*) &stairs_})
        c->setBounds(area);
}

}
