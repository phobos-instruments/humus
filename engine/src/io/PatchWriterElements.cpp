#include <cmath>

#include "core/UserLibrary.h"
#include "io/PatchWriteInternal.h"

namespace hum {

void addValue(juce::XmlElement& prop, const Parameter& p) {
    if (p.isRange) {
        auto* r = prop.createNewChildElement("range");
        r->createNewChildElement("min")->addTextElement(juce::String(p.rangeMin));
        r->createNewChildElement("max")->addTextElement(juce::String(p.rangeMax));
    } else if (p.type == "bool") {
        prop.createNewChildElement("bool")->addTextElement(p.value >= 0.5 ? "1" : "0");
    } else if (p.type == "enum") {
        prop.createNewChildElement("enum")->addTextElement(juce::String((int) p.value));
    } else if (p.type == "int") {
        prop.createNewChildElement("int")->addTextElement(juce::String((int) p.value));
    } else if (p.type == "soundfile") {
        prop.createNewChildElement("soundfile")
            ->addTextElement(juce::String(library::referenceFor(p.text)));
    } else if (p.type == "rhythmic-unit") {
        prop.createNewChildElement("rhythmic-unit")->addTextElement(juce::String(p.text));
    } else if (p.type == "text") {
        prop.createNewChildElement("text")->addTextElement(juce::String(p.text));
    } else {
        prop.createNewChildElement("double")->addTextElement(juce::String(p.value));
    }
}

void writeProperty(juce::XmlElement& props, const Parameter& p) {
    auto* pe = props.createNewChildElement("property");
    pe->setAttribute("index", p.index);
    pe->setAttribute("name", juce::String(p.name));
    addValue(*pe, p);
}

void writePattern(juce::XmlElement& propEl, const Pattern& pat) {
    auto* pe = propEl.createNewChildElement("pattern");
    pe->setAttribute("duration", pat.duration);
    pe->setAttribute("matrix-resolution", juce::String(pat.matrixResolution));
    for (const auto& ch : pat.channels) {
        auto* ce = pe->createNewChildElement("pattern-channel");
        ce->setAttribute("channel-type", juce::String(ch.type));
        if (!ch.snap.empty()) ce->setAttribute("snap", juce::String(ch.snap));
        if (ch.swing >= 0.0) ce->setAttribute("swing", ch.swing);
        if (ch.startTick >= 0) {
            ce->setAttribute("start", ch.startTick);
            ce->setAttribute("length", ch.lengthTicks);
            if (ch.loopClip) ce->setAttribute("loop", 1);
            if (!ch.name.empty()) ce->setAttribute("clip-name", juce::String(ch.name));
            if (ch.color > 0) ce->setAttribute("color", ch.color);
            if (ch.id > 0) ce->setAttribute("clip-id", ch.id);
            if (ch.fadeInTicks > 0) ce->setAttribute("fade-in", ch.fadeInTicks);
            if (ch.fadeOutTicks > 0) ce->setAttribute("fade-out", ch.fadeOutTicks);
            if (ch.fadeInCurve != 0.0) ce->setAttribute("fade-in-curve", ch.fadeInCurve);
            if (ch.fadeOutCurve != 0.0) ce->setAttribute("fade-out-curve", ch.fadeOutCurve);
        }
        if (ch.type == "audio-clip" || ch.type == "video-clip") {
            ce->setAttribute("file", juce::String(ch.audioFile));
            if (ch.audioOffset != 0)
                ce->setAttribute("offset", juce::String((juce::int64) ch.audioOffset));
            if (ch.audioGain != 1.0) ce->setAttribute("clip-gain", ch.audioGain);
            if (ch.sourceBpm > 0.0) ce->setAttribute("source-bpm", ch.sourceBpm);
            if (ch.warpMode != 0) ce->setAttribute("warp", ch.warpMode);
            if (ch.audioReverse) ce->setAttribute("reverse", 1);
            if (ch.audioPitch != 0.0) ce->setAttribute("pitch", ch.audioPitch);
            continue;
        }
        if (ch.type == "time-signatures") {
            juce::String t;
            for (const auto& ts : ch.timeSignatures)
                t << ts.tick << "\t" << ts.numerator << "\t|\t" << ts.denominator << "\n";
            ce->createNewChildElement("time-signature-timepoints")->addTextElement(t);
        } else if (!ch.matrix.empty()) {
            ce->addTextElement(juce::String(ch.matrix));
        } else {
            juce::String t;
            for (int tk : ch.triggers) t << tk << "\n";
            ce->createNewChildElement("trigger-timepoints")->addTextElement(t);
        }
    }
}

void writePresets(juce::XmlElement& presets, const OrganismModel& c) {
    if (c.currentPreset > 0) presets.setAttribute("current-preset", c.currentPreset);
    presets.setAttribute("current-preset-dirty", c.presetDirty ? 1 : 0);
    if (!c.currentPresetName.empty()) {
        presets.setAttribute("current-preset-name", juce::String(c.currentPresetName));
        presets.setAttribute("current-preset-source", juce::String(c.currentPresetSource));
    }
    for (const auto& pr : c.presets) {
        auto* pe = presets.createNewChildElement("preset");
        pe->setAttribute("number", pr.number);
        pe->setAttribute("name", juce::String(pr.name));
        for (const auto& p : pr.properties) writeProperty(*pe, p);
    }
}

namespace {
void writeAutomationLane(juce::XmlElement& mod, const AutomationLane& lane) {
    auto* ps = mod.createNewChildElement("property-sources");
    ps->setAttribute("property-name", juce::String(lane.propertyName));
    ps->setAttribute("property-index", lane.propertyIndex);
    auto* ac = ps->createNewChildElement("automation-controller");
    ac->setAttribute("mute", lane.mute ? 1 : 0);
    ac->setAttribute("record", lane.record ? 1 : 0);
    const char* tag = lane.kind == "range" ? "range-timepoints"
                    : lane.kind == "trigger" ? "trigger-timepoints" : "double-timepoints";
    juce::String text;
    for (const auto& p : lane.points) {
        if (lane.kind == "range")
            text << juce::String(p.beat) << "\t" << juce::String(p.value) << "\t" << juce::String(p.valueMax) << "\n";
        else if (lane.kind == "trigger")
            text << juce::String(p.beat) << "\n";
        else
            text << juce::String(p.beat) << "\t" << juce::String(p.value) << "\n";
    }
    ac->createNewChildElement(tag)->addTextElement(text);
    bool anyCurve = false;
    for (const auto& p : lane.points) anyCurve = anyCurve || p.curve != 0.0;
    if (anyCurve) {
        juce::String curves;
        for (const auto& p : lane.points) curves << juce::String(p.curve) << "\n";
        ac->createNewChildElement("curve-timepoints")->addTextElement(curves);
    }
}
}

void writeAutomationLanes(juce::XmlElement& mod, const OrganismModel& c) {
    for (const auto& lane : c.automation) writeAutomationLane(mod, lane);
}

void writeMidiSources(juce::XmlElement& mod, const OrganismModel& c) {
    for (const auto& s : c.midiSources) {
        auto* ps = mod.createNewChildElement("property-sources");
        ps->setAttribute("property-name", juce::String(s.propertyName));
        ps->setAttribute("property-index", s.propertyIndex);
        auto* mc = ps->createNewChildElement("midi-controller");
        if (s.isSwitch) {
            mc->setAttribute("invert", s.inverted ? 1 : 0);
            mc->setAttribute("toggle", s.toggle ? 1 : 0);
            mc->setAttribute("threshold", (int) std::lround(s.threshold * 16383.0));
        } else {
            if (!s.curve.empty()) mc->setAttribute("use-non-linear-scale", 1);
            mc->setAttribute("smoothing", s.smoothing);
            mc->setAttribute("map-minimum", s.mapMin);
            mc->setAttribute("map-maximum", s.mapMax);
        }
        auto* spec = mc->createNewChildElement("midi-message-spec");
        spec->setAttribute("type", juce::String(s.specType.empty() ? "7-bit-control-change" : s.specType));
        spec->setAttribute("port", s.port);
        spec->setAttribute("channel", s.channel);
        spec->setAttribute("number", s.cc);
        for (int h : s.held) spec->createNewChildElement("held")->setAttribute("number", h);
        if (!s.isSwitch) {
            auto* curve = mc->createNewChildElement("midi-mapping-curve");
            const std::vector<std::pair<double, double>> pts =
                s.curve.empty() ? std::vector<std::pair<double, double>>{{0.0, 0.0}, {1.0, 1.0}}
                                : s.curve;
            for (const auto& p : pts) {
                auto* pt = curve->createNewChildElement("mapping-point");
                pt->setAttribute("in", (int) std::lround(p.first * 16383.0));
                pt->setAttribute("out", p.second);
            }
        }
    }
}

void writeOscSources(juce::XmlElement& mod, const OrganismModel& c) {
    for (const auto& s : c.oscSources) {
        auto* ps = mod.createNewChildElement("property-sources");
        ps->setAttribute("property-name", juce::String(s.propertyName));
        ps->setAttribute("property-index", s.propertyIndex);
        auto* oc = ps->createNewChildElement("osc-controller");
        oc->setAttribute("address", juce::String(s.address));
        oc->setAttribute("map-minimum", s.mapMin);
        oc->setAttribute("map-maximum", s.mapMax);
        if (s.smoothing > 0.0) oc->setAttribute("smoothing", s.smoothing);
        if (s.isSwitch) {
            oc->setAttribute("invert", s.inverted ? 1 : 0);
            oc->setAttribute("toggle", s.toggle ? 1 : 0);
            oc->setAttribute("threshold", (int) std::lround(s.threshold * 16383.0));
        }
        if (!s.curve.empty()) {
            auto* curve = oc->createNewChildElement("osc-mapping-curve");
            for (const auto& p : s.curve) {
                auto* pt = curve->createNewChildElement("mapping-point");
                pt->setAttribute("in", (int) std::lround(p.first * 16383.0));
                pt->setAttribute("out", p.second);
            }
        }
    }
}

void writeModSources(juce::XmlElement& mod, const OrganismModel& c) {
    for (const auto& s : c.modSources) {
        auto* ps = mod.createNewChildElement("property-sources");
        ps->setAttribute("property-name", juce::String(s.propertyName));
        ps->setAttribute("property-index", s.propertyIndex);
        auto* mo = ps->createNewChildElement("mod-controller");
        mo->setAttribute("source-organism", juce::String(s.sourceOrganism));
        mo->setAttribute("source-value", juce::String(s.sourceValue));
        mo->setAttribute("map-minimum", s.mapMin);
        mo->setAttribute("map-maximum", s.mapMax);
        if (s.smoothing > 0.0) mo->setAttribute("smoothing", s.smoothing);
        if (s.isSwitch) {
            mo->setAttribute("invert", s.inverted ? 1 : 0);
            mo->setAttribute("toggle", s.toggle ? 1 : 0);
            mo->setAttribute("threshold", (int) std::lround(s.threshold * 16383.0));
        }
        if (!s.curve.empty()) {
            auto* curve = mo->createNewChildElement("mod-mapping-curve");
            for (const auto& p : s.curve) {
                auto* pt = curve->createNewChildElement("mapping-point");
                pt->setAttribute("in", (int) std::lround(p.first * 16383.0));
                pt->setAttribute("out", p.second);
            }
        }
    }
}

void writeAutomationView(juce::XmlElement& e, const AutomationView& v) {
    e.setAttribute("contraption-name", juce::String(v.organismName));
    e.setAttribute("target-type", "property");
    e.setAttribute("property-index", v.propertyIndex);
    e.setAttribute("property-name", juce::String(v.propertyName));
    e.setAttribute("index", v.index);
    e.setAttribute("height", v.height);
    e.setAttribute("snap-to", v.snapTo ? 1 : 0);
}

juce::String pluginStateTag(const std::string& kind) {
    if (kind == "vst3") return "vst3-class-info";
    if (kind == "lv2") return "lv2-class-info";
    if (kind == "au") return "au-state";
    return {};
}

void writeMidiSettings(juce::XmlElement& ce, const OrganismModel& c) {
    auto* vms = ce.createNewChildElement("vst-midi-settings");
    const char* mode = c.midiReceiveMode == OrganismModel::kMidiCordsOnly ? "off"
                     : c.midiReceiveMode == OrganismModel::kMidiChannel   ? "channel" : "omni";
    vms->setAttribute("receive-mode", mode);
    if (c.midiReceiveMode == OrganismModel::kMidiChannel)
        vms->setAttribute("receive-channel", c.midiReceiveChannel);
    vms->setAttribute("receive-port", c.midiReceivePort);
}

void writeOrganism(juce::XmlElement& ce, const OrganismModel& c) {
    ce.setAttribute("class", juce::String(c.classRaw.empty() ? c.displayClass : c.classRaw));
    ce.setAttribute("name", juce::String(c.name));
    if (c.internal) ce.setAttribute("internal", 1);
    auto* props = ce.createNewChildElement("properties");
    for (const auto& p : c.properties) {
        if (p.type == "pattern" && c.pattern.present) {
            auto* pe = props->createNewChildElement("property");
            pe->setAttribute("index", p.index);
            pe->setAttribute("name", juce::String(p.name));
            writePattern(*pe, c.pattern);
        } else {
            writeProperty(*props, p);
        }
    }
    if (const auto tag = pluginStateTag(c.kind); tag.isNotEmpty() && !c.pluginState.empty())
        props->createNewChildElement(tag)->addTextElement(juce::String(c.pluginState));
    writePresets(*ce.createNewChildElement("presets"), c);
    writeRollLocks(ce, c);
    auto* mod = ce.createNewChildElement("modulation-sources");
    writeAutomationLanes(*mod, c);
    writeMidiSources(*mod, c);
    writeOscSources(*mod, c);
    writeModSources(*mod, c);
    if (pluginStateTag(c.kind).isNotEmpty() || c.midiReceiveMode != OrganismModel::kMidiCordsOnly)
        writeMidiSettings(ce, c);
}

void writeRollLocks(juce::XmlElement& ce, const OrganismModel& c) {
    if (c.rollLocked.empty()) return;
    auto* nr = ce.createNewChildElement("no-random");
    for (const auto& name : c.rollLocked)
        nr->createNewChildElement("property")->setAttribute("name", juce::String(name));
}

void writeConnection(juce::XmlElement& e, const ConnectionModel& conn) {
    e.setAttribute("from", juce::String(conn.src));
    e.setAttribute("from-outlet", conn.srcOutlet);
    e.setAttribute("to", juce::String(conn.dst));
    e.setAttribute("to-inlet", conn.dstInlet);
    if (conn.midiChannel > 0) e.setAttribute("channel", conn.midiChannel);
}

void writeView(juce::XmlElement& ve, const OrganismView& v) {
    ve.setAttribute("contraption-name", juce::String(v.organismName));
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
}

void writeMetapad(juce::XmlElement& root, const MetapadModel& ms) {
    if (auto* o = root.getChildByName("document-snapshots")) root.removeChildElement(o, true);
    if (auto* o = root.getChildByName("metasurface")) root.removeChildElement(o, true);
    if (!ms.present) return;

    auto* me = root.createNewChildElement("metasurface");
    if (ms.temperature != 1.0) me->setAttribute("temperature", ms.temperature);
    auto* mask = me->createNewChildElement("document-snapshot-restore-mask");
    juce::XmlElement* cur = nullptr; std::string curName;
    for (const auto& e : ms.mask) {
        if (!cur || curName != e.organismName) {
            cur = mask->createNewChildElement("contraption-snapshot-restore-mask");
            cur->setAttribute("contraption-name", juce::String(e.organismName));
            curName = e.organismName;
        }
        auto* pe = cur->createNewChildElement("property-snapshot-restore-mask");
        pe->setAttribute("property-index", e.propertyIndex);
        pe->setAttribute("restore", e.restore ? 1 : 0);
    }
    auto* snaps = me->createNewChildElement("document-snapshots");
    for (const auto& s : ms.snapshots) {
        auto* se = snaps->createNewChildElement("document-snapshot");
        se->setAttribute("index", s.index);
        se->setAttribute("name", juce::String(s.name));
        se->setAttribute("colour", juce::String(s.colour));
        for (const auto& c : s.organisms) {
            auto* ce = se->createNewChildElement("contraption-snapshot");
            ce->setAttribute("contraption-name", juce::String(c.organismName));
            for (const auto& v : c.values) {
                auto* pe = ce->createNewChildElement("property-snapshot");
                pe->setAttribute("property-index", v.propertyIndex);
                if (v.type == "range") {
                    auto* re = pe->createNewChildElement("range");
                    re->createNewChildElement("min")->addTextElement(juce::String(v.value));
                    re->createNewChildElement("max")->addTextElement(juce::String(v.value2));
                } else {
                    pe->createNewChildElement(v.type.empty() ? "double" : juce::String(v.type))
                        ->addTextElement(juce::String(v.value));
                }
            }
        }
        for (const auto& sp : s.patterns) {
            auto* pe = se->createNewChildElement("pattern-snapshot");
            pe->setAttribute("contraption-name", juce::String(sp.organismName));
            writePattern(*pe, sp.pattern);
        }
    }
    auto* pts = me->createNewChildElement("metasurface-points");
    for (const auto& p : ms.points) {
        auto* pe = pts->createNewChildElement("metasurface-point");
        pe->setAttribute("snapshot-index", p.snapshotIndex);
        pe->setAttribute("x", p.x);
        pe->setAttribute("y", p.y);
    }
}

}
