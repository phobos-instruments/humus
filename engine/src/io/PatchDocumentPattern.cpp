#include "io/PatchParseInternal.h"

namespace hum {

namespace {
void parseTimepoints(const juce::String& text, const std::string& kind, AutomationLane& lane) {
    auto toks = juce::StringArray::fromTokens(text, " \t\n\r", "");
    toks.removeEmptyStrings();
    const int stride = (kind == "range") ? 3 : (kind == "trigger") ? 1 : 2;
    for (int i = 0; i + stride <= toks.size(); i += stride) {
        AutomationBreakpoint bp;
        bp.beat = toks[i].getDoubleValue();
        if (kind == "trigger") {
            bp.value = bp.valueMax = 1.0;
        } else if (kind == "range") {
            bp.value = toks[i + 1].getDoubleValue();
            bp.valueMax = toks[i + 2].getDoubleValue();
        } else {
            bp.value = bp.valueMax = toks[i + 1].getDoubleValue();
        }
        lane.points.push_back(bp);
    }
}
}

void parsePattern(juce::XmlElement& pe, Pattern& pat) {
    pat.present = true;
    pat.duration = pe.getIntAttribute("duration", 0);
    pat.matrixResolution = pe.getStringAttribute("matrix-resolution", "1/16").toStdString();
    for (auto* ce : pe.getChildIterator()) {
        if (!ce->hasTagName("pattern-channel")) continue;
        PatternChannel ch;
        ch.type = ce->getStringAttribute("channel-type", "trigger-timepoints").toStdString();
        ch.snap = ce->getStringAttribute("snap", "").toStdString();
        ch.swing = ce->getDoubleAttribute("swing", -1.0);
        ch.startTick = ce->getIntAttribute("start", -1);
        ch.lengthTicks = ce->getIntAttribute("length", 0);
        ch.loopClip = ce->getIntAttribute("loop", 0) != 0;
        ch.name = ce->getStringAttribute("clip-name", "").toStdString();
        ch.color = ce->getIntAttribute("color", 0);
        ch.id = ce->getIntAttribute("clip-id", 0);
        ch.fadeInTicks = ce->getIntAttribute("fade-in", 0);
        ch.fadeOutTicks = ce->getIntAttribute("fade-out", 0);
        if (ch.type == "audio-clip") {
            ch.audioFile = ce->getStringAttribute("file", "").toStdString();
            ch.audioOffset = ce->getStringAttribute("offset", "0").getLargeIntValue();
            ch.audioGain = ce->getDoubleAttribute("clip-gain", 1.0);
            ch.sourceBpm = ce->getDoubleAttribute("source-bpm", 0.0);
            ch.warpMode = ce->getIntAttribute("warp", 0);
            ch.audioReverse = ce->getIntAttribute("reverse", 0) != 0;
            ch.audioPitch = ce->getDoubleAttribute("pitch", 0.0);
            pat.channels.push_back(std::move(ch));
            continue;
        }
        if (ch.type == "time-signatures") {
            if (auto* tp = ce->getChildByName("time-signature-timepoints")) {
                auto toks = juce::StringArray::fromTokens(tp->getAllSubText(), " \t\n\r|", "");
                toks.removeEmptyStrings();
                for (int i = 0; i + 2 < toks.size(); i += 3) {
                    PatternTimeSig ts;
                    ts.tick = toks[i].getIntValue();
                    ts.numerator = toks[i + 1].getIntValue();
                    ts.denominator = toks[i + 2].getIntValue();
                    ch.timeSignatures.push_back(ts);
                }
            }
        } else if (auto* tp = ce->getChildByName("trigger-timepoints")) {
            auto toks = juce::StringArray::fromTokens(tp->getAllSubText(), " \t\n\r", "");
            toks.removeEmptyStrings();
            for (auto& t : toks) ch.triggers.push_back(t.getIntValue());
        } else {
            ch.matrix = ce->getAllSubText().toStdString();
        }
        pat.channels.push_back(std::move(ch));
    }
}

void parseModulationSources(juce::XmlElement& mod, OrganismModel& c) {
    for (auto* ps : mod.getChildIterator()) {
        if (!ps->hasTagName("property-sources")) continue;
        if (auto* mc = ps->getChildByName("midi-controller")) {
            MidiControllerSource src;
            src.propertyName = ps->getStringAttribute("property-name").toStdString();
            src.propertyIndex = ps->getIntAttribute("property-index", -1);
            src.mapMin = mc->getDoubleAttribute("map-minimum", 0.0);
            src.mapMax = mc->getDoubleAttribute("map-maximum", 1.0);
            src.smoothing = mc->getDoubleAttribute("smoothing", 0.0);
            if (mc->hasAttribute("threshold")) {
                src.isSwitch = true;
                src.inverted = mc->getIntAttribute("invert", 0) != 0;
                src.toggle = mc->getIntAttribute("toggle", 0) != 0;
                src.threshold = mc->getDoubleAttribute("threshold", 8192.0) / 16383.0;
            }
            if (auto* curve = mc->getChildByName("midi-mapping-curve")) {
                for (auto* pt : curve->getChildIterator())
                    if (pt->hasTagName("mapping-point"))
                        src.curve.push_back({pt->getDoubleAttribute("in", 0.0) / 16383.0,
                                             pt->getDoubleAttribute("out", 0.0)});
                const bool identity = src.curve.size() == 2
                    && src.curve[0] == std::make_pair(0.0, 0.0)
                    && src.curve[1] == std::make_pair(1.0, 1.0);
                if (identity || src.curve.size() < 2) src.curve.clear();
            }
            if (auto* spec = mc->getChildByName("midi-message-spec")) {
                src.cc = spec->getIntAttribute("number", 0);
                src.port = spec->getIntAttribute("port", 0);
                src.channel = spec->getIntAttribute("channel", 0);
                src.specType = spec->getStringAttribute("type", "7-bit-control-change").toStdString();
            }
            c.midiSources.push_back(std::move(src));
            continue;
        }
        if (auto* oc = ps->getChildByName("osc-controller")) {
            OscControllerSource src;
            src.propertyName = ps->getStringAttribute("property-name").toStdString();
            src.propertyIndex = ps->getIntAttribute("property-index", -1);
            src.address = oc->getStringAttribute("address").toStdString();
            src.mapMin = oc->getDoubleAttribute("map-minimum", 0.0);
            src.mapMax = oc->getDoubleAttribute("map-maximum", 1.0);
            src.smoothing = oc->getDoubleAttribute("smoothing", 0.0);
            if (oc->hasAttribute("threshold")) {
                src.isSwitch = true;
                src.inverted = oc->getIntAttribute("invert", 0) != 0;
                src.toggle = oc->getIntAttribute("toggle", 0) != 0;
                src.threshold = oc->getDoubleAttribute("threshold", 8192.0) / 16383.0;
            }
            if (auto* curve = oc->getChildByName("osc-mapping-curve")) {
                for (auto* pt : curve->getChildIterator())
                    if (pt->hasTagName("mapping-point"))
                        src.curve.push_back({pt->getDoubleAttribute("in", 0.0) / 16383.0,
                                             pt->getDoubleAttribute("out", 0.0)});
                if (src.curve.size() < 2) src.curve.clear();
            }
            c.oscSources.push_back(std::move(src));
            continue;
        }
        if (auto* mo = ps->getChildByName("mod-controller")) {
            ModControllerSource src;
            src.propertyName = ps->getStringAttribute("property-name").toStdString();
            src.propertyIndex = ps->getIntAttribute("property-index", -1);
            src.sourceOrganism = mo->getStringAttribute("source-organism").toStdString();
            src.sourceValue = mo->getStringAttribute("source-value").toStdString();
            src.mapMin = mo->getDoubleAttribute("map-minimum", 0.0);
            src.mapMax = mo->getDoubleAttribute("map-maximum", 1.0);
            src.smoothing = mo->getDoubleAttribute("smoothing", 0.0);
            if (mo->hasAttribute("threshold")) {
                src.isSwitch = true;
                src.inverted = mo->getIntAttribute("invert", 0) != 0;
                src.toggle = mo->getIntAttribute("toggle", 0) != 0;
                src.threshold = mo->getDoubleAttribute("threshold", 8192.0) / 16383.0;
            }
            if (auto* curve = mo->getChildByName("mod-mapping-curve")) {
                for (auto* pt : curve->getChildIterator())
                    if (pt->hasTagName("mapping-point"))
                        src.curve.push_back({pt->getDoubleAttribute("in", 0.0) / 16383.0,
                                             pt->getDoubleAttribute("out", 0.0)});
                if (src.curve.size() < 2) src.curve.clear();
            }
            c.modSources.push_back(std::move(src));
            continue;
        }
        auto* ac = ps->getChildByName("automation-controller");
        if (!ac) continue;
        AutomationLane lane;
        lane.propertyName = ps->getStringAttribute("property-name").toStdString();
        lane.propertyIndex = ps->getIntAttribute("property-index", -1);
        lane.mute = ac->getIntAttribute("mute", 0) != 0;
        lane.record = ac->getIntAttribute("record", 0) != 0;
        if (auto* tp = ac->getChildByName("double-timepoints")) {
            lane.kind = "double"; parseTimepoints(tp->getAllSubText(), "double", lane);
        } else if (auto* tp = ac->getChildByName("range-timepoints")) {
            lane.kind = "range"; parseTimepoints(tp->getAllSubText(), "range", lane);
        } else if (auto* tp = ac->getChildByName("trigger-timepoints")) {
            lane.kind = "trigger"; parseTimepoints(tp->getAllSubText(), "trigger", lane);
        }
        if (auto* cv = ac->getChildByName("curve-timepoints")) {
            auto toks = juce::StringArray::fromTokens(cv->getAllSubText(), " \t\n\r", "");
            toks.removeEmptyStrings();
            for (int k = 0; k < toks.size() && k < (int) lane.points.size(); ++k)
                lane.points[(size_t) k].curve = toks[k].getDoubleValue();
        }
        c.automation.push_back(std::move(lane));
    }
}

void parseMidiSettings(juce::XmlElement& organismEl, OrganismModel& c) {
    auto* vms = organismEl.getChildByName("vst-midi-settings");
    if (!vms) return;
    const auto mode = vms->getStringAttribute("receive-mode", "off");
    if (mode == "omni") c.midiReceiveMode = OrganismModel::kMidiOmni;
    else if (mode == "channel") c.midiReceiveMode = OrganismModel::kMidiChannel;
    else c.midiReceiveMode = OrganismModel::kMidiCordsOnly;
    c.midiReceiveChannel = juce::jlimit(1, 16, vms->getIntAttribute("receive-channel", 1));
    c.midiReceivePort = vms->getIntAttribute("receive-port", 0);
}

}
