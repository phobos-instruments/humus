// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <cmath>

#include "core/library/UserLibrary.h"
#include "core/midi/MidiSource.h"
#include "io/ControlShapeXml.h"
#include "io/PatchWriteInternal.h"
#include "io/XmlText.h"

namespace hum {

void addValue(xml::Element& prop, const Parameter& p) {
    if (p.isRange) {
        auto* r = prop.addChild("range");
        r->addChild("min")->addText(xmltext::plainNumber(p.rangeMin));
        r->addChild("max")->addText(xmltext::plainNumber(p.rangeMax));
    } else if (p.type == "bool") {
        prop.addChild("bool")->addText(p.value >= 0.5 ? "1" : "0");
    } else if (p.type == "enum") {
        prop.addChild("enum")->addText(std::to_string((int) p.value));
    } else if (p.type == "int") {
        prop.addChild("int")->addText(std::to_string((int) p.value));
    } else if (p.type == "soundfile") {
        prop.addChild("soundfile")
            ->addText(library::referenceFor(p.text));
    } else if (p.type == "rhythmic-unit") {
        prop.addChild("rhythmic-unit")->addText(p.text);
    } else if (p.type == "text") {
        prop.addChild("text")->addText(p.text);
    } else {
        prop.addChild("double")->addText(xmltext::plainNumber(p.value));
    }
}

void writeProperty(xml::Element& props, const Parameter& p) {
    auto* pe = props.addChild("property");
    pe->setAttribute("index", p.index);
    pe->setAttribute("name", p.name);
    addValue(*pe, p);
}

void writePattern(xml::Element& propEl, const Pattern& pat) {
    auto* pe = propEl.addChild("pattern");
    pe->setAttribute("duration", pat.duration);
    pe->setAttribute("matrix-resolution", pat.matrixResolution);
    for (const auto& ch : pat.channels) {
        auto* ce = pe->addChild("pattern-channel");
        ce->setAttribute("channel-type", ch.type);
        if (!ch.snap.empty()) ce->setAttribute("snap", ch.snap);
        if (ch.swing >= 0.0) ce->setAttribute("swing", ch.swing);
        if (ch.startTick >= 0) {
            ce->setAttribute("start", ch.startTick);
            ce->setAttribute("length", ch.lengthTicks);
            if (ch.loopClip) ce->setAttribute("loop", 1);
            if (!ch.name.empty()) ce->setAttribute("clip-name", ch.name);
            if (ch.color > 0) ce->setAttribute("color", ch.color);
            if (ch.id > 0) ce->setAttribute("clip-id", ch.id);
            if (ch.fadeInTicks > 0) ce->setAttribute("fade-in", ch.fadeInTicks);
            if (ch.fadeOutTicks > 0) ce->setAttribute("fade-out", ch.fadeOutTicks);
            if (ch.fadeInCurve != 0.0) ce->setAttribute("fade-in-curve", ch.fadeInCurve);
            if (ch.fadeOutCurve != 0.0) ce->setAttribute("fade-out-curve", ch.fadeOutCurve);
        }
        if (ch.type == "audio-clip" || ch.type == "video-clip") {
            ce->setAttribute("file", ch.audioFile);
            if (ch.audioOffset != 0)
                ce->setAttribute("offset", std::to_string((long long) ch.audioOffset));
            if (ch.audioGain != 1.0) ce->setAttribute("clip-gain", ch.audioGain);
            if (ch.sourceBpm > 0.0) ce->setAttribute("source-bpm", ch.sourceBpm);
            if (ch.warpMode != 0) ce->setAttribute("warp", ch.warpMode);
            if (ch.audioReverse) ce->setAttribute("reverse", 1);
            if (ch.audioPitch != 0.0) ce->setAttribute("pitch", ch.audioPitch);
            continue;
        }
        if (ch.type == "time-signatures") {
            std::string t;
            for (const auto& ts : ch.timeSignatures)
                t += std::to_string(ts.tick) + "\t" + std::to_string(ts.numerator) + "\t|\t" + std::to_string(ts.denominator)
                     + "\n";
            ce->addChild("time-signature-timepoints")->addText(t);
        } else if (!ch.matrix.empty()) {
            ce->addText(ch.matrix);
        } else {
            std::string t;
            for (int tk : ch.triggers) t += std::to_string(tk) + "\n";
            ce->addChild("trigger-timepoints")->addText(t);
        }
    }
}

void writePresets(xml::Element& presets, const OrganismModel& c) {
    if (c.currentPreset > 0) presets.setAttribute("current-preset", c.currentPreset);
    presets.setAttribute("current-preset-dirty", c.presetDirty ? 1 : 0);
    if (!c.currentPresetName.empty()) {
        presets.setAttribute("current-preset-name", c.currentPresetName);
        presets.setAttribute("current-preset-source", c.currentPresetSource);
    }
    for (const auto& pr : c.presets) {
        auto* pe = presets.addChild("preset");
        pe->setAttribute("number", pr.number);
        pe->setAttribute("name", pr.name);
        for (const auto& p : pr.properties) writeProperty(*pe, p);
    }
}

namespace {
void writeAutomationLane(xml::Element& mod, const AutomationLane& lane) {
    auto* ps = mod.addChild("property-sources");
    ps->setAttribute("property-name", lane.propertyName);
    ps->setAttribute("property-index", lane.propertyIndex);
    auto* ac = ps->addChild("automation-controller");
    ac->setAttribute("mute", lane.mute ? 1 : 0);
    ac->setAttribute("record", lane.record ? 1 : 0);
    if (lane.kind == "step") ac->setAttribute("hold", 1);
    const char* tag = lane.kind == "range" ? "range-timepoints"
                    : lane.kind == "trigger" ? "trigger-timepoints" : "double-timepoints";
    std::string text;
    for (const auto& p : lane.points) {
        if (lane.kind == "range")
            text += xmltext::plainNumber(p.beat) + "\t" + xmltext::plainNumber(p.value) + "\t"
                    + xmltext::plainNumber(p.valueMax) + "\n";
        else if (lane.kind == "trigger")
            text += xmltext::plainNumber(p.beat) + "\n";
        else
            text += xmltext::plainNumber(p.beat) + "\t" + xmltext::plainNumber(p.value) + "\n";
    }
    ac->addChild(tag)->addText(text);
    bool anyCurve = false;
    for (const auto& p : lane.points) anyCurve = anyCurve || p.curve != 0.0;
    if (anyCurve) {
        std::string curves;
        for (const auto& p : lane.points) curves += xmltext::plainNumber(p.curve) + "\n";
        ac->addChild("curve-timepoints")->addText(curves);
    }
}
}

void writeAutomationLanes(xml::Element& mod, const OrganismModel& c) {
    for (const auto& lane : c.automation) writeAutomationLane(mod, lane);
}

void writeMidiSources(xml::Element& mod, const OrganismModel& c) {
    for (const auto& s : c.midiSources) {
        auto* ps = mod.addChild("property-sources");
        ps->setAttribute("property-name", s.propertyName);
        ps->setAttribute("property-index", s.propertyIndex);
        auto* mc = ps->addChild("midi-controller");
        controlxml::writeBehaviour(*mc, s.shape);
        if (s.shape.isFader()) {
            if (!s.shape.curve.empty()) mc->setAttribute("use-non-linear-scale", 1);
            mc->setAttribute("smoothing", s.shape.smoothing);
        }
        mc->setAttribute("map-minimum", s.mapMin);
        mc->setAttribute("map-maximum", s.mapMax);
        auto* spec = mc->addChild("midi-message-spec");
        spec->setAttribute("type", modeWord(messageTypeOf(s.cc)));
        spec->setAttribute("port", s.port);
        if (!s.device.empty()) spec->setAttribute("device", s.device);
        spec->setAttribute("channel", s.legacyChannel);
        if (midiChannelSlot(s.channel) != kAnyMidiChannel) spec->setAttribute("midi-channel", s.channel);
        spec->setAttribute("number", messageTypeOf(s.cc) == MidiMessageType::ControlChange ? s.cc
                                                                                          : sourceNumber(s.cc));
        for (int h : s.held) spec->addChild("held")->setAttribute("number", h);
        if (s.shape.isFader())
            controlxml::writeCurve(*mc, "midi-mapping-curve",
                                   s.shape.curve.empty()
                                       ? std::vector<std::pair<double, double>>{{0.0, 0.0}, {1.0, 1.0}}
                                       : s.shape.curve);
    }
}

void writeOscSources(xml::Element& mod, const OrganismModel& c) {
    for (const auto& s : c.oscSources) {
        auto* ps = mod.addChild("property-sources");
        ps->setAttribute("property-name", s.propertyName);
        ps->setAttribute("property-index", s.propertyIndex);
        auto* oc = ps->addChild("osc-controller");
        oc->setAttribute("address", s.address);
        oc->setAttribute("map-minimum", s.mapMin);
        oc->setAttribute("map-maximum", s.mapMax);
        if (s.shape.smoothing > 0.0) oc->setAttribute("smoothing", s.shape.smoothing);
        controlxml::writeBehaviour(*oc, s.shape);
        if (!s.shape.curve.empty()) controlxml::writeCurve(*oc, "osc-mapping-curve", s.shape.curve);
    }
}

void writeModSources(xml::Element& mod, const OrganismModel& c) {
    for (const auto& s : c.modSources) {
        auto* ps = mod.addChild("property-sources");
        ps->setAttribute("property-name", s.propertyName);
        ps->setAttribute("property-index", s.propertyIndex);
        auto* mo = ps->addChild("mod-controller");
        mo->setAttribute("source-organism", s.sourceOrganism);
        mo->setAttribute("source-value", s.sourceValue);
        mo->setAttribute("map-minimum", s.mapMin);
        mo->setAttribute("map-maximum", s.mapMax);
        if (s.shape.smoothing > 0.0) mo->setAttribute("smoothing", s.shape.smoothing);
        controlxml::writeBehaviour(*mo, s.shape);
        if (!s.shape.curve.empty()) controlxml::writeCurve(*mo, "mod-mapping-curve", s.shape.curve);
    }
}

void writeAutomationView(xml::Element& e, const AutomationView& v) {
    e.setAttribute("contraption-name", v.organismName);
    e.setAttribute("target-type", "property");
    e.setAttribute("property-index", v.propertyIndex);
    e.setAttribute("property-name", v.propertyName);
    e.setAttribute("index", v.index);
    e.setAttribute("height", v.height);
    e.setAttribute("snap-to", v.snapTo ? 1 : 0);
}

std::string pluginStateTag(const std::string& kind) {
    if (kind == "vst3") return "vst3-class-info";
    if (kind == "lv2") return "lv2-class-info";
    if (kind == "au") return "au-state";
    return {};
}

void writeMidiSettings(xml::Element& ce, const OrganismModel& c) {
    auto* vms = ce.addChild("vst-midi-settings");
    const char* mode = c.midiReceiveMode == OrganismModel::kMidiCordsOnly ? "off"
                     : c.midiReceiveMode == OrganismModel::kMidiChannel   ? "channel" : "omni";
    vms->setAttribute("receive-mode", mode);
    if (c.midiReceiveMode == OrganismModel::kMidiChannel)
        vms->setAttribute("receive-channel", c.midiReceiveChannel);
    vms->setAttribute("receive-port", c.midiReceivePort);
}

void writeOrganism(xml::Element& ce, const OrganismModel& c) {
    ce.setAttribute("class", c.classRaw.empty() ? c.displayClass : c.classRaw);
    ce.setAttribute("name", c.name);
    if (c.internal) ce.setAttribute("internal", 1);
    auto* props = ce.addChild("properties");
    for (const auto& p : c.properties) {
        if (p.type == "pattern" && c.pattern.present) {
            auto* pe = props->addChild("property");
            pe->setAttribute("index", p.index);
            pe->setAttribute("name", p.name);
            writePattern(*pe, c.pattern);
        } else {
            writeProperty(*props, p);
        }
    }
    if (const auto tag = pluginStateTag(c.kind); !tag.empty() && !c.pluginState.empty())
        props->addChild(tag)->addText(c.pluginState);
    writePresets(*ce.addChild("presets"), c);
    writeRollLocks(ce, c);
    writeRangeModes(ce, c);
    writeTrackInput(ce, c);
    writeTimelineRow(ce, c);
    auto* mod = ce.addChild("modulation-sources");
    writeAutomationLanes(*mod, c);
    writeMidiSources(*mod, c);
    writeOscSources(*mod, c);
    writeModSources(*mod, c);
    if (!pluginStateTag(c.kind).empty() || c.midiReceiveMode != OrganismModel::kMidiCordsOnly)
        writeMidiSettings(ce, c);
}

void writeTrackInput(xml::Element& ce, const OrganismModel& c) {
    if (c.trackInput == OrganismModel::kTrackInputAuto) return;
    auto* ti = ce.addChild("track-input");
    if (c.trackInput == OrganismModel::kTrackInputAll) ti->setAttribute("port", "all");
    else if (c.trackInput == OrganismModel::kTrackInputNone) ti->setAttribute("port", "none");
    else ti->setAttribute("port", c.trackInput + 1);
}

void writeTimelineRow(xml::Element& ce, const OrganismModel& c) {
    if (c.timelineRow < 0) return;
    ce.addChild("timeline-row")->setAttribute("index", c.timelineRow);
}

void writeRollLocks(xml::Element& ce, const OrganismModel& c) {
    if (c.rollLocked.empty()) return;
    auto* nr = ce.addChild("no-random");
    for (const auto& name : c.rollLocked)
        nr->addChild("property")->setAttribute("name", name);
}

void writeRangeModes(xml::Element& ce, const OrganismModel& c) {
    if (c.rangeModes.empty()) return;
    auto* rm = ce.addChild("range-mode");
    for (const auto& [name, mode] : c.rangeModes) {
        auto* pe = rm->addChild("property");
        pe->setAttribute("name", name);
        pe->setAttribute("mode", mode);
    }
}

void writeConnection(xml::Element& e, const ConnectionModel& conn) {
    e.setAttribute("from", conn.src);
    e.setAttribute("from-outlet", conn.srcOutlet);
    e.setAttribute("to", conn.dst);
    e.setAttribute("to-inlet", conn.dstInlet);
    if (conn.midiChannel > 0) e.setAttribute("channel", conn.midiChannel);
}

void writeView(xml::Element& ve, const OrganismView& v) {
    ve.setAttribute("contraption-name", v.organismName);
    ve.setAttribute("patcher-x", v.patcherX);
    ve.setAttribute("patcher-y", v.patcherY);
    if (v.hasEditor) {
        ve.setAttribute("editor-visible", v.editorVisible ? 1 : 0);
        ve.setAttribute("editor-x", v.editorX);
        ve.setAttribute("editor-y", v.editorY);
        if (v.editorMode >= 0) ve.setAttribute("editor-mode", v.editorMode);
        if (v.editorW > 0) ve.setAttribute("editor-w", v.editorW);
        if (v.editorH > 0) ve.setAttribute("editor-h", v.editorH);
        if (v.editorHalf >= 0) ve.setAttribute("editor-half", v.editorHalf);
    }
    if (v.editorCollapsed) ve.setAttribute("editor-collapsed", 1);
    if (v.editorFloating) {
        ve.setAttribute("editor-float", 1);
        ve.setAttribute("float-x", v.floatX);
        ve.setAttribute("float-y", v.floatY);
        ve.setAttribute("float-w", v.floatW);
        ve.setAttribute("float-h", v.floatH);
    }
}

void writeMetapad(xml::Element& root, const MetapadModel& ms) {
    if (auto* o = root.child("document-snapshots")) root.removeChild(o);
    if (auto* o = root.child("metasurface")) root.removeChild(o);
    if (!ms.present) return;

    auto* me = root.addChild("metasurface");
    if (ms.temperature != 1.0) me->setAttribute("temperature", ms.temperature);
    auto* mask = me->addChild("document-snapshot-restore-mask");
    xml::Element* cur = nullptr; std::string curName;
    for (const auto& e : ms.mask) {
        if (!cur || curName != e.organismName) {
            cur = mask->addChild("contraption-snapshot-restore-mask");
            cur->setAttribute("contraption-name", e.organismName);
            curName = e.organismName;
        }
        auto* pe = cur->addChild("property-snapshot-restore-mask");
        pe->setAttribute("property-index", e.propertyIndex);
        pe->setAttribute("restore", e.restore ? 1 : 0);
    }
    auto* snaps = me->addChild("document-snapshots");
    for (const auto& s : ms.snapshots) {
        auto* se = snaps->addChild("document-snapshot");
        se->setAttribute("index", s.index);
        se->setAttribute("name", s.name);
        se->setAttribute("colour", s.colour);
        for (const auto& c : s.organisms) {
            auto* ce = se->addChild("contraption-snapshot");
            ce->setAttribute("contraption-name", c.organismName);
            for (const auto& v : c.values) {
                auto* pe = ce->addChild("property-snapshot");
                pe->setAttribute("property-index", v.propertyIndex);
                if (v.type == "range") {
                    auto* re = pe->addChild("range");
                    re->addChild("min")->addText(xmltext::plainNumber(v.value));
                    re->addChild("max")->addText(xmltext::plainNumber(v.value2));
                } else {
                    pe->addChild(v.type.empty() ? std::string("double") : v.type)
                        ->addText(xmltext::plainNumber(v.value));
                }
            }
        }
        for (const auto& sp : s.patterns) {
            auto* pe = se->addChild("pattern-snapshot");
            pe->setAttribute("contraption-name", sp.organismName);
            writePattern(*pe, sp.pattern);
        }
    }
    auto* pts = me->addChild("metasurface-points");
    for (const auto& p : ms.points) {
        auto* pe = pts->addChild("metasurface-point");
        pe->setAttribute("snapshot-index", p.snapshotIndex);
        pe->setAttribute("x", p.x);
        pe->setAttribute("y", p.y);
    }
}

}
