// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/properties/ParameterControlView.h"

#include <cmath>

#include "core/params/ParamUnit.h"

namespace hum {

namespace {

constexpr double kActivityMs = 280.0;

bool numberedSource(int cc) {
    const auto type = messageTypeOf(cc);
    return type == MidiMessageType::ControlChange || type == MidiMessageType::Note;
}

}

InspectedSource ParameterControlView::inspected(const SourceRow& r) const {
    InspectedSource s;
    s.title = r.name;
    s.family = familyOf(r);
    s.shape = r.shape;
    s.bounded = r.bounded;
    const auto unit = juce::String(unitSuffix(rowUnit()));
    s.unit = unit;
    s.minText = numText(r.min);
    s.maxText = numText(r.max);
    const auto span = r.bounded ? std::make_pair(r.min, r.max)
                                : endBounds(host_, selectedOrganism(), selectedParam(), r.end);
    s.axes.outLow = numText(span.first) + (unit.isEmpty() ? "" : " " + unit);
    s.axes.outHigh = numText(span.second) + (unit.isEmpty() ? "" : " " + unit);
    if (r.isOsc || r.isMod) {
        s.axes.inLow = "0";
        s.axes.inHigh = "1";
        s.origin = r.isOsc ? tr("parameter-control.osc-address", "OSC address") + "   " + juce::String(r.oscAddress)
                           : tr("parameter-control.follows", "Follows") + "   " + juce::String(r.modSource)
                                 + " / " + juce::String(paramSourceName(r.modValue));
        return s;
    }
    const auto type = messageTypeOf(r.source.cc);
    s.held = r.source.held;
    s.message = modeText(type);
    s.numbered = numberedSource(r.source.cc);
    s.number = type == MidiMessageType::Note ? juce::String(midiNoteName(noteOfSource(r.source.cc)))
                                             : juce::String(sourceNumber(r.source.cc));
    s.channel = channelText(r.source.channel);
    s.device = deviceLabel(r.source);
    s.axes.inLow = type == MidiMessageType::PitchBend ? "-8192" : "0";
    s.axes.inHigh = type == MidiMessageType::PitchBend ? "8191" : "127";
    return s;
}

void ParameterControlView::selectSource(int idx) {
    selectedSource_ = idx >= 0 && idx < (int) rows_.size() ? idx : -1;
    for (int i = 0; i < (int) rows_.size(); ++i)
        rows_[(size_t) i].tile->setSelected(i == selectedSource_);
    if (selectedSource_ < 0 || rows_[(size_t) selectedSource_].isWaiting) {
        mapping_.clear(rows_.empty() ? juce::String()
                                     : tr("parameter-control.pick-a-source", "Pick a source to see how it behaves"));
        return;
    }
    const auto& r = rows_[(size_t) selectedSource_];
    mapping_.show(inspected(r));
    wireInspector(r);
}

void ParameterControlView::wireInspector(const SourceRow& r) {
    const auto src = r.source;
    const auto shape = r.shape;
    const auto end = r.end;
    const double min = r.min, max = r.max;
    const bool isMod = r.isMod, isOsc = r.isOsc, bounded = r.bounded;
    const auto address = r.oscAddress, modSource = r.modSource, modValue = r.modValue;
    const bool note = messageTypeOf(src.cc) == MidiMessageType::Note;

    mapping_.message.onClick = [this, src, min, max, shape, end] { chooseMessage(src, min, max, shape, end); };
    mapping_.channel.onClick = [this, src, min, max, shape, end] { chooseChannel(src, min, max, shape, end); };
    mapping_.device.onClick = [this, src, min, max, shape, end] { chooseDevice(src, min, max, shape, end); };

    auto& number = mapping_.number;
    number.setInputRestrictions(4, note ? "0123456789ABCDEFGabcdefg#-" : "0123456789");
    number.setRange(0.0, kMidiMaxD, true);
    number.parse = [note](const juce::String& t) {
        return note ? unitParse(Unit::MidiNote, t, 0.0, kMidiMaxD) : (double) t.getIntValue();
    };
    number.format = [note](double v) {
        return note ? juce::String(midiNoteName((int) std::lround(v))) : juce::String((int) std::lround(v));
    };
    auto applyNumber = [this, src, shape, end, min, max] {
        const int number = juce::jlimit(0, kMidiMax, (int) std::lround(mapping_.number.parse(mapping_.number.getText())));
        if (number == sourceNumber(src.cc)) return;
        remapMidi(src, MidiSource(sourceFromMessage(messageTypeOf(src.cc), number), src.held, src.port, src.channel),
                  min, max, shape, end);
    };
    number.onReturnKey = applyNumber;
    number.onFocusLost = applyNumber;

    const auto [lo, hi] = rowSpan();
    for (auto* ed : {&mapping_.min, &mapping_.max}) {
        ed->setRange(lo, hi, false);
        ed->parse = [this](const juce::String& t) { return numValue(t); };
        ed->format = [this](double v) { return numText(v); };
        ed->setInputRestrictions(12, rangeChars());
    }
    mappedValues_.clear();
    mappedValues_[&mapping_.min] = numValue(mapping_.min.getText());
    mappedValues_[&mapping_.max] = numValue(mapping_.max.getText());
    auto applyRange = [this, src, end, bounded, isMod, isOsc, address, modSource, modValue] {
        if (!bounded) return;
        const auto c = selectedOrganism();
        const auto p = aimedParam(end);
        if (c.empty() || p.empty()) return;
        const double mn = numValue(mapping_.min.getText());
        const double mx = numValue(mapping_.max.getText());
        mapping_.min.setText(numText(mn), juce::dontSendNotification);
        mapping_.max.setText(numText(mx), juce::dontSendNotification);
        if (sameAsMapped(&mapping_.min, mn) && sameAsMapped(&mapping_.max, mx)) return;
        later([this, src, c, p, mn, mx, isMod, isOsc, address, modSource, modValue] {
            if (isMod)      host_.mod().mapRoute(modSource, modValue, c, p, mn, mx);
            else if (isOsc) host_.osc().mapAddress(address, c, p, mn, mx, false);
            else            host_.midi().mapCC(src, c, p, mn, mx, false);
            rebuildSources();
            params_.repaint();
        });
    };
    for (auto* ed : {&mapping_.min, &mapping_.max}) {
        ed->onReturnKey = applyRange;
        ed->onFocusLost = applyRange;
    }
}

void ParameterControlView::applyShape(const ControlShape& sh) {
    if (selectedSource_ < 0 || selectedSource_ >= (int) rows_.size()) return;
    auto& r = rows_[(size_t) selectedSource_];
    ControlShape next = sh;
    next.logScale = next.isFader() && r.shape.logScale;
    r.shape = next;
    if (!r.isOsc && !r.isMod) r.name = midiName(r.source, next);
    r.tile->setContent(r.name, next, r.isOsc || r.isMod ? std::vector<int>{} : r.source.held, false);
    const auto c = selectedOrganism();
    const auto p = aimedParam(r.end);
    if (r.isMod) host_.mod().setShape(r.modSource, r.modValue, c, p, next);
    else if (r.isOsc) host_.osc().setShape(r.oscAddress, c, p, next);
    else host_.midi().setShape(r.source, c, p, next);
}

void ParameterControlView::updateLive() {
    const double now = juce::Time::getMillisecondCounterHiRes();
    for (auto& r : rows_)
        if (!r.isOsc && !r.isMod && !r.isWaiting) {
            const double seen = host_.midi().lastSeenMs(r.source.cc);
            r.tile->setActive(seen >= 0.0 && now - seen < kActivityMs);
        }
    if (selectedSource_ < 0 || selectedSource_ >= (int) rows_.size()) return;
    const auto& r = rows_[(size_t) selectedSource_];
    if (r.isWaiting) return;
    const auto c = selectedOrganism();
    const auto p = aimedParam(r.end);
    const double current = host_.liveParamValue(c, p);
    const auto unit = juce::String(unitSuffix(rowUnit()));
    const auto out = numText(current) + (unit.isEmpty() ? "" : " " + unit);
    double input01 = -1.0;
    juce::String readout = tr("parameter-control.now", "now") + " " + out;
    if (!r.isOsc && !r.isMod && !r.shape.isEncoder()) {
        const int raw = host_.midi().sourceValue(r.source.cc);
        if (raw >= 0) {
            input01 = raw / (double) sourceMaxValue(r.source.cc);
            readout = tr("parameter-control.in", "in") + " " + juce::String(raw)
                      + juce::String::fromUTF8("  \xe2\x86\x92  ") + out;
        }
    }
    const auto span = r.bounded ? std::make_pair(r.min, r.max) : endBounds(host_, c, selectedParam(), r.end);
    const double waiting = r.shape.picksUp() ? rangeToShaped(r.shape, span.first, span.second, current) : -1.0;
    mapping_.setLive(readout, input01, waiting);
}

DragNumberEditor* ParameterControlView::sourceFieldForTest(int row, int which) {
    if (row < 0 || row >= (int) rows_.size()) return nullptr;
    if (row != selectedSource_) selectSource(row);
    const auto& r = rows_[(size_t) row];
    if (which == 0) return !r.isOsc && !r.isMod && numberedSource(r.source.cc) ? &mapping_.number : nullptr;
    if (!r.bounded) return nullptr;
    return which == 1 ? &mapping_.min : &mapping_.max;
}

juce::Rectangle<int> ParameterControlView::rowWordForTest(int row, int which) {
    if (row < 0 || row >= (int) rows_.size() || !rows_[(size_t) row].bounded) return {};
    if (row != selectedSource_) selectSource(row);
    return which == 0 ? mapping_.fromWordBounds() : which == 1 ? mapping_.toWordBounds() : mapping_.unitWordBounds();
}

std::pair<int, int> ParameterControlView::focusedField() const {
    if (selectedSource_ < 0) return {-1, -1};
    int which = 0;
    for (const auto* ed : {&mapping_.number, &mapping_.min, &mapping_.max}) {
        if (ed->hasKeyboardFocus(true)) return {selectedSource_, which};
        ++which;
    }
    return {-1, -1};
}

}
