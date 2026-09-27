// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/midi/MidiSource.h"
#include "io/ControlShapeXml.h"
#include "io/PatchParseInternal.h"
#include "io/XmlText.h"

#include <algorithm>

namespace hum {

namespace {
void parseTimepoints(const std::string& text, const std::string& kind, AutomationLane& lane) {
    const auto toks = xmltext::tokens(text, " \t\n\r");
    const int stride = (kind == "range") ? 3 : (kind == "trigger") ? 1 : 2;
    for (size_t i = 0; i + (size_t) stride <= toks.size(); i += (size_t) stride) {
        AutomationBreakpoint bp;
        bp.beat = xml::doubleValue(toks[i]);
        if (kind == "trigger") {
            bp.value = bp.valueMax = 1.0;
        } else if (kind == "range") {
            bp.value = xml::doubleValue(toks[i + 1]);
            bp.valueMax = xml::doubleValue(toks[i + 2]);
        } else {
            bp.value = bp.valueMax = xml::doubleValue(toks[i + 1]);
        }
        lane.points.push_back(bp);
    }
}
}

void parsePattern(xml::Element& pe, Pattern& pat) {
    pat.present = true;
    pat.duration = pe.intAttribute("duration", 0);
    pat.matrixResolution = pe.attribute("matrix-resolution", "1/16");
    for (auto* ce : pe.children()) {
        if (!ce->hasTag("pattern-channel")) continue;
        PatternChannel ch;
        ch.type = ce->attribute("channel-type", "trigger-timepoints");
        ch.snap = ce->attribute("snap", "");
        ch.swing = ce->doubleAttribute("swing", -1.0);
        ch.startTick = ce->intAttribute("start", -1);
        ch.lengthTicks = ce->intAttribute("length", 0);
        ch.loopClip = ce->intAttribute("loop", 0) != 0;
        ch.name = ce->attribute("clip-name", "");
        ch.color = ce->intAttribute("color", 0);
        ch.id = ce->intAttribute("clip-id", 0);
        ch.fadeInTicks = ce->intAttribute("fade-in", 0);
        ch.fadeOutTicks = ce->intAttribute("fade-out", 0);
        ch.fadeInCurve = ce->doubleAttribute("fade-in-curve", 0.0);
        ch.fadeOutCurve = ce->doubleAttribute("fade-out-curve", 0.0);
        if (ch.type == "audio-clip" || ch.type == "video-clip") {
            ch.audioFile = ce->attribute("file", "");
            ch.audioOffset = xmltext::largeInt(ce->attribute("offset", "0"));
            ch.audioGain = ce->doubleAttribute("clip-gain", 1.0);
            ch.sourceBpm = ce->doubleAttribute("source-bpm", 0.0);
            ch.warpMode = ce->intAttribute("warp", 0);
            ch.audioReverse = ce->intAttribute("reverse", 0) != 0;
            ch.audioPitch = ce->doubleAttribute("pitch", 0.0);
            pat.channels.push_back(std::move(ch));
            continue;
        }
        if (ch.type == "time-signatures") {
            if (auto* tp = ce->child("time-signature-timepoints")) {
                const auto toks = xmltext::tokens(tp->allSubText(), " \t\n\r|");
                for (size_t i = 0; i + 2 < toks.size(); i += 3) {
                    PatternTimeSig ts;
                    ts.tick = xml::intValue(toks[i]);
                    ts.numerator = xml::intValue(toks[i + 1]);
                    ts.denominator = xml::intValue(toks[i + 2]);
                    ch.timeSignatures.push_back(ts);
                }
            }
        } else if (auto* tp = ce->child("trigger-timepoints")) {
            for (const auto& t : xmltext::tokens(tp->allSubText(), " \t\n\r")) ch.triggers.push_back(xml::intValue(t));
        } else {
            ch.matrix = ce->allSubText();
        }
        pat.channels.push_back(std::move(ch));
    }
}

void parseModulationSources(xml::Element& mod, OrganismModel& c) {
    for (auto* ps : mod.children()) {
        if (!ps->hasTag("property-sources")) continue;
        if (auto* mc = ps->child("midi-controller")) {
            MidiControllerSource src;
            src.propertyName = ps->attribute("property-name");
            src.propertyIndex = ps->intAttribute("property-index", -1);
            src.mapMin = mc->doubleAttribute("map-minimum", 0.0);
            src.mapMax = mc->doubleAttribute("map-maximum", 1.0);
            src.shape = controlxml::readBehaviour(*mc);
            controlxml::readCurve(*mc, "midi-mapping-curve", src.shape, true);
            if (auto* spec = mc->child("midi-message-spec")) {
                const auto type = parseMode(spec->attribute("type", ""), MidiMessageType::ControlChange);
                const int number = spec->intAttribute("number", 0);
                src.cc = type == MidiMessageType::ControlChange ? number : sourceFromMessage(type, number);
                for (auto* h : spec->children())
                    if (h->hasTag("held")) src.held.push_back(h->intAttribute("number", -1));
                src.held = normalizedHeld(std::move(src.held), src.cc);
                src.port = spec->intAttribute("port", 0);
                src.device = spec->attribute("device", "");
                src.legacyChannel = spec->intAttribute("channel", 0);
                src.channel = midiChannelSlot(spec->intAttribute("midi-channel", kAnyMidiChannel));
            }
            c.midiSources.push_back(std::move(src));
            continue;
        }
        if (auto* oc = ps->child("osc-controller")) {
            OscControllerSource src;
            src.propertyName = ps->attribute("property-name");
            src.propertyIndex = ps->intAttribute("property-index", -1);
            src.address = oc->attribute("address");
            src.mapMin = oc->doubleAttribute("map-minimum", 0.0);
            src.mapMax = oc->doubleAttribute("map-maximum", 1.0);
            src.shape = controlxml::readBehaviour(*oc);
            controlxml::readCurve(*oc, "osc-mapping-curve", src.shape, false);
            c.oscSources.push_back(std::move(src));
            continue;
        }
        if (auto* mo = ps->child("mod-controller")) {
            ModControllerSource src;
            src.propertyName = ps->attribute("property-name");
            src.propertyIndex = ps->intAttribute("property-index", -1);
            src.sourceOrganism = mo->attribute("source-organism");
            src.sourceValue = mo->attribute("source-value");
            src.mapMin = mo->doubleAttribute("map-minimum", 0.0);
            src.mapMax = mo->doubleAttribute("map-maximum", 1.0);
            src.shape = controlxml::readBehaviour(*mo);
            controlxml::readCurve(*mo, "mod-mapping-curve", src.shape, false);
            c.modSources.push_back(std::move(src));
            continue;
        }
        auto* ac = ps->child("automation-controller");
        if (!ac) continue;
        AutomationLane lane;
        lane.propertyName = ps->attribute("property-name");
        lane.propertyIndex = ps->intAttribute("property-index", -1);
        lane.mute = ac->intAttribute("mute", 0) != 0;
        lane.record = ac->intAttribute("record", 0) != 0;
        for (const char* kind : {"double", "range", "trigger"})
            if (auto* tp = ac->child(std::string(kind) + "-timepoints")) {
                lane.kind = kind;
                parseTimepoints(tp->allSubText(), kind, lane);
                break;
            }
        if (lane.kind == "double" && ac->intAttribute("hold", 0) != 0) lane.kind = "step";
        if (auto* cv = ac->child("curve-timepoints")) {
            const auto toks = xmltext::tokens(cv->allSubText(), " \t\n\r");
            for (size_t k = 0; k < toks.size() && k < lane.points.size(); ++k)
                lane.points[k].curve = xml::doubleValue(toks[k]);
        }
        c.automation.push_back(std::move(lane));
    }
}

void parseMidiSettings(xml::Element& organismEl, OrganismModel& c) {
    auto* vms = organismEl.child("vst-midi-settings");
    if (!vms) return;
    const auto mode = vms->attribute("receive-mode", "off");
    if (mode == "omni") c.midiReceiveMode = OrganismModel::kMidiOmni;
    else if (mode == "channel") c.midiReceiveMode = OrganismModel::kMidiChannel;
    else c.midiReceiveMode = OrganismModel::kMidiCordsOnly;
    c.midiReceiveChannel = std::clamp(vms->intAttribute("receive-channel", 1), 1, 16);
    c.midiReceivePort = vms->intAttribute("receive-port", 0);
}

}
