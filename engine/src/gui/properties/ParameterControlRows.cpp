// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/properties/ParameterControlView.h"

#include <cmath>

#include "core/params/ParamUnit.h"

namespace hum {

bool ParameterControlView::sameAsMapped(DragNumberEditor* ed, double value) const {
    const auto it = mappedValues_.find(ed);
    return it != mappedValues_.end() && std::abs(it->second - value) <= 1e-9;
}

bool ParameterControlView::learnAimsAtForTest(const std::string& param) const {
    auto& learner = MidiLearner::instance();
    return learner.armed() && learner.armedOrganism() == selectedOrganism()
           && learner.armedParam() == param;
}

int ParameterControlView::groupHeadCountForTest() const {
    int shown = 0;
    for (const auto& head : groupHead_) if (head.isVisible()) ++shown;
    return shown;
}

bool ParameterControlView::removeRowForTest(int row) {
    if (row < 0 || row >= (int) rows_.size()) return false;
    removeRow(rows_[(size_t) row]);
    return true;
}

ParameterControlView::SourceRow& ParameterControlView::newRow(const juce::String& name, RangeEnd end,
                                                              const ControlShape& shape, bool learning) {
    SourceRow r;
    r.end = end;
    r.shape = shape;
    r.isWaiting = learning;
    const int idx = (int) rows_.size();
    r.tile = std::make_unique<SourceTile>();
    r.tile->onSelect = [this, idx] { selectSource(idx); };
    sourceRows_.addAndMakeVisible(*r.tile);
    r.remove = std::make_unique<IconButton>(IconGlyph::Trash,
                                           learning ? tr("parameter-control.stop-learning", "Stop learning")
                                                    : tr("parameter-control.remove", "Remove"));
    r.remove->onClick = [this, idx] { if (idx < (int) rows_.size()) removeRow(rows_[(size_t) idx]); };
    sourceRows_.addAndMakeVisible(*r.remove);
    r.name = name;
    rows_.push_back(std::move(r));
    auto& row = rows_.back();
    row.tile->setContent(name, shape, {}, learning);
    return row;
}

juce::String ParameterControlView::kindWord(const ControlShape& shape) {
    switch (shape.type) {
        case ControlType::Button:  return tr("control-mode.pad-short", "Pad");
        case ControlType::Encoder: return tr("control-mode.encoder", "Encoder");
        case ControlType::Fader:   break;
    }
    return tr("control-mode.knob-short", "Knob");
}

juce::String ParameterControlView::midiName(const MidiSource& src, const ControlShape& shape) {
    const auto type = messageTypeOf(src.cc);
    juce::String what = type == MidiMessageType::ControlChange ? "CC " + juce::String(src.cc)
                        : type == MidiMessageType::Note ? juce::String(midiNoteName(noteOfSource(src.cc)))
                                                         : modeText(type);
    return kindWord(shape) + juce::String::fromUTF8(" \xc2\xb7 ") + what;
}

void ParameterControlView::addWaitingRow(RangeEnd end) {
    newRow(tr("parameter-control.move-a-control", "move a control on your device..."), end, ControlShape{}, true);
}

void ParameterControlView::addCcRow(const MidiSource& src, double min, double max,
                                    const ControlShape& shape, RangeEnd end, bool bounded) {
    auto& r = newRow(midiName(src, shape), end, shape, false);
    r.source = src;
    r.min = min;
    r.max = max;
    r.bounded = bounded;
    r.tile->setContent(r.name, shape, src.held, false);
    r.tile->setTooltip(juce::String(midiSourceLabel(src)) + "  " + deviceLabel(src));
}

void ParameterControlView::addModRow(const std::string& source, const std::string& value, double min,
                                     double max, const ControlShape& shape, RangeEnd end, bool bounded) {
    const auto word = isParamSource(value) ? tr("parameter-control.follow-prefix", "Follow")
                                           : tr("parameter-control.mod-prefix", "Mod");
    auto& r = newRow(word + juce::String::fromUTF8(" \xc2\xb7 ") + juce::String(source) + " / "
                         + juce::String(paramSourceName(value)),
                     end, shape, false);
    r.isMod = true;
    r.modSource = source;
    r.modValue = value;
    r.min = min;
    r.max = max;
    r.bounded = bounded;
}

void ParameterControlView::addOscRow(const std::string& address, const ControlShape& shape, RangeEnd end) {
    auto& r = newRow("OSC" + juce::String::fromUTF8(" \xc2\xb7 ") + juce::String(address), end, shape, false);
    r.isOsc = true;
    r.oscAddress = address;
    for (const auto& e : host_.osc().map().entries())
        if (e.address == address && e.organism == selectedOrganism() && e.param == aimedParam(end)) {
            r.min = e.min;
            r.max = e.max;
        }
}

void ParameterControlView::removeRow(const SourceRow& r) {
    if (r.isWaiting) {
        MidiLearner::instance().cancel();
        later([this] { rebuildSources(); });
        return;
    }
    const auto p = aimedParam(r.end);
    const bool isMod = r.isMod, isOsc = r.isOsc;
    const auto src = r.source;
    const auto address = r.oscAddress, modSource = r.modSource, modValue = r.modValue;
    later([this, p, isMod, isOsc, src, address, modSource, modValue] {
        const auto c = selectedOrganism();
        if (isMod)      host_.mod().clearRoute(modSource, modValue, c, p);
        else if (isOsc) host_.osc().clearAddress(address, c, p);
        else            host_.midi().clearCC(src, c, p);
        rebuildSources();
        params_.repaint();
    });
}

void ParameterControlView::reaimRow(const SourceRow& r, RangeEnd to) {
    const auto c = selectedOrganism();
    const auto from = aimedParam(r.end), onto = aimedParam(to);
    if (c.empty() || from == onto) return;
    const bool isMod = r.isMod, isOsc = r.isOsc;
    const auto src = r.source;
    const auto address = r.oscAddress, modSource = r.modSource, modValue = r.modValue;
    const auto shape = r.shape;
    double mn = 0.0, mx = 1.0;
    bool found = false;
    if (isMod) {
        for (const auto& e : host_.mod().map().entries())
            if (e.organism == c && e.param == from && e.source == modSource && e.value == modValue) {
                mn = e.min; mx = e.max; found = true;
            }
    } else if (isOsc) {
        for (const auto& e : host_.osc().map().entries())
            if (e.organism == c && e.param == from && e.address == address) {
                mn = e.min; mx = e.max; found = true;
            }
    } else {
        for (const auto& e : host_.midi().map().entries())
            if (e.organism == c && e.param == from && e.source() == src) {
                mn = e.min; mx = e.max; found = true;
            }
    }
    if (!found) return;
    later([this, c, from, onto, isMod, isOsc, src, address, modSource, modValue, shape, mn, mx] {
        if (isMod) {
            host_.mod().clearRoute(modSource, modValue, c, from);
            host_.mod().mapRoute(modSource, modValue, c, onto, mn, mx);
            if (!shape.isDefault()) host_.mod().setShape(modSource, modValue, c, onto, shape);
        } else if (isOsc) {
            host_.osc().clearAddress(address, c, from);
            host_.osc().mapAddress(address, c, onto, mn, mx, false);
            if (!shape.isDefault()) host_.osc().setShape(address, c, onto, shape);
        } else {
            host_.midi().clearCC(src, c, from);
            host_.midi().mapCC(src, c, onto, mn, mx, false);
            if (!shape.isDefault()) host_.midi().setShape(src, c, onto, shape);
        }
        rebuildSources();
        params_.repaint();
    });
}

juce::String ParameterControlView::deviceLabel(const MidiSource& src) const {
    if (src.anyPort()) return tr("parameter-control.any-controller", "Any");
    const auto name = host_.midi().deviceForPort(src.port);
    if (name.empty()) return tr("parameter-control.controller-away", "(away)");
    return juce::String(name).substring(0, 14);
}

void ParameterControlView::chooseDevice(const MidiSource& src, double min, double max,
                                        const ControlShape& shape, RangeEnd end) {
    const auto devices = host_.midi().inputDevices();
    juce::PopupMenu menu;
    menu.addItem(1, tr("parameter-control.any-controller-long", "Any controller"), true,
                 src.anyPort());
    for (size_t i = 0; i < devices.size(); ++i) {
        const int port = host_.midi().portForDevice(devices[i]);
        menu.addItem((int) i + 2, juce::String(devices[i]), true,
                     !src.anyPort() && port == src.port);
    }
    menu.showMenuAsync(juce::PopupMenu::Options(), [this, src, min, max, shape, end, devices](int pick) {
        if (pick <= 0) return;
        const int port = pick == 1 ? kAnyMidiPort
                                   : host_.midi().portForDevice(devices[(size_t) pick - 2]);
        if (midiPortRow(port) == midiPortRow(src.port)) return;
        remapMidi(src, MidiSource(src.cc, src.held, port, src.channel), min, max, shape, end);
    });
}

void ParameterControlView::chooseMessage(const MidiSource& src, double min, double max,
                                         const ControlShape& shape, RangeEnd end) {
    juce::PopupMenu menu;
    const auto now = messageTypeOf(src.cc);
    for (const auto& row : ModeTable<MidiMessageType>::rows)
        menu.addItem((int) row.value + 1, modeText(row.value), true, row.value == now);
    menu.showMenuAsync(juce::PopupMenu::Options(), [this, src, min, max, shape, end, now](int pick) {
        if (pick <= 0) return;
        const auto type = (MidiMessageType) (pick - 1);
        if (type == now) return;
        const MidiSource next(sourceFromMessage(type, sourceNumber(src.cc)), src.held, src.port, src.channel);
        remapMidi(src, next, min, max, shape, end);
    });
}

void ParameterControlView::chooseChannel(const MidiSource& src, double min, double max,
                                         const ControlShape& shape, RangeEnd end) {
    juce::PopupMenu menu;
    menu.addItem(kAnyMidiChannel + 1, tr("parameter-control.all-channels", "All channels"), true,
                 src.anyChannel());
    for (int ch = 1; ch <= 16; ++ch)
        menu.addItem(ch + 1, channelText(ch), true, src.channel == ch);
    menu.showMenuAsync(juce::PopupMenu::Options(), [this, src, min, max, shape, end](int pick) {
        if (pick <= 0 || pick - 1 == src.channel) return;
        remapMidi(src, MidiSource(src.cc, src.held, src.port, pick - 1), min, max, shape, end);
    });
}

void ParameterControlView::remapMidi(const MidiSource& from, const MidiSource& to, double min,
                                     double max, const ControlShape& shape, RangeEnd end) {
    later([this, from, to, min, max, shape, end] {
        const auto c = selectedOrganism();
        const auto p = aimedParam(end);
        if (c.empty() || p.empty() || !isMidiSource(to.cc)) return;
        host_.midi().clearCC(from, c, p);
        host_.midi().mapCC(to, c, p, min, max, false);
        if (!shape.isDefault()) host_.midi().setShape(to, c, p, shape);
        rebuildSources();
        params_.repaint();
    });
}

ControlFamily ParameterControlView::familyOf(const SourceRow& r) {
    if (r.isMod) return ControlFamily::Mod;
    if (r.isOsc) return ControlFamily::Osc;
    return ControlFamily::Midi;
}

void ParameterControlView::addManualCc(int group, int cc) {
    const auto c = selectedOrganism();
    const auto ends = shownEnds();
    if (c.empty() || selectedParam().empty() || group < 0 || group >= (int) ends.size()) return;
    const auto range = endBounds(host_, c, selectedParam(), ends[(size_t) group]);
    host_.midi().mapCC(juce::jlimit(0, kMidiMax, cc), c, aimedParam(ends[(size_t) group]),
                       range.first, range.second, false);
    rebuildSources();
    params_.repaint();
}

void ParameterControlView::rebuildModSourceBox() {
    modChoices_ = host_.mod().availableSources();
}

void ParameterControlView::addModRoute(int group, int choice) {
    const auto c = selectedOrganism();
    const auto ends = shownEnds();
    if (c.empty() || selectedParam().empty() || group < 0 || group >= (int) ends.size()) return;
    if (choice < 0 || choice >= (int) modChoices_.size()) return;
    const auto& [source, value] = modChoices_[(size_t) choice];
    const auto range = endBounds(host_, c, selectedParam(), ends[(size_t) group]);
    host_.mod().mapRoute(source, value, c, aimedParam(ends[(size_t) group]), range.first, range.second);
    rebuildSources();
    params_.repaint();
}

}
