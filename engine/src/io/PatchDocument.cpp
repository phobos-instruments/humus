// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatchDocument.h"
#include "io/PatchMigrate.h"

#include "core/timeline/ClipOps.h"
#include "core/library/UserLibrary.h"

#include <juce_core/juce_core.h>

#include "core/packs/ClassString.h"
#include "io/PatchFormat.h"
#include "io/PatchParseInternal.h"

namespace hum {

namespace {

void parseClass(const juce::String& classRaw, OrganismModel& c) {
    c.classRaw = classRaw.toStdString();
    const auto id = parseClassString(c.classRaw);
    c.displayClass = id.display;
    c.kind = id.kind;
}

Parameter parseProperty(juce::XmlElement& p) {
    Parameter param;
    param.index = p.getIntAttribute("index", -1);
    param.name = p.getStringAttribute("name").toStdString();
    for (auto* v : p.getChildIterator()) {
        const juce::String tag = v->getTagName();
        param.type = tag.toStdString();
        if (tag == "double" || tag == "int") {
            param.value = v->getAllSubText().getDoubleValue();
        } else if (tag == "bool") {
            param.value = v->getAllSubText().trim() == "1" ? 1.0 : 0.0;
        } else if (tag == "enum") {
            param.value = v->getAllSubText().getDoubleValue();
        } else if (tag == "range") {
            param.isRange = true;
            if (auto* mn = v->getChildByName("min")) param.rangeMin = mn->getAllSubText().getDoubleValue();
            if (auto* mx = v->getChildByName("max")) param.rangeMax = mx->getAllSubText().getDoubleValue();
            param.value = param.rangeMin;
        } else if (tag == "rhythmic-unit" || tag == "soundfile") {
            param.text = library::resolve(
                juce::URL::removeEscapeChars(v->getAllSubText().trim()).toStdString());
        } else {
            param.text = v->getAllSubText().trim().toStdString();
        }
        break;
    }
    return param;
}

}

SnapshotValue parseSnapshotValue(juce::XmlElement& ps) {
    SnapshotValue v;
    v.propertyIndex = ps.getIntAttribute("property-index", -1);
    if (auto* c = ps.getFirstChildElement()) {
        v.type = c->getTagName().toStdString();
        if (v.type == "range") {
            if (auto* mn = c->getChildByName("min")) v.value = mn->getAllSubText().getDoubleValue();
            if (auto* mx = c->getChildByName("max")) v.value2 = mx->getAllSubText().getDoubleValue();
        } else {
            v.value = c->getAllSubText().getDoubleValue();
        }
    }
    return v;
}

void parseMetapad(juce::XmlElement& root, MetapadModel& ms) {
    auto* msEl = root.getChildByName("metasurface");
    auto* snapsEl = root.getChildByName("document-snapshots");
    if (!snapsEl && msEl) snapsEl = msEl->getChildByName("document-snapshots");
    if (auto* snaps = snapsEl) {
        ms.present = true;
        for (auto* se : snaps->getChildIterator()) {
            if (!se->hasTagName("document-snapshot")) continue;
            DocumentSnapshot s;
            s.index = se->getIntAttribute("index", 0);
            s.name = se->getStringAttribute("name").toStdString();
            s.colour = se->getStringAttribute("colour").toStdString();
            for (auto* ce : se->getChildIterator()) {
                if (ce->hasTagName("pattern-snapshot")) {
                    SnapshotPattern sp;
                    sp.organismName = ce->getStringAttribute("contraption-name").toStdString();
                    if (auto* pat = ce->getChildByName("pattern")) parsePattern(*pat, sp.pattern);
                    s.patterns.push_back(std::move(sp));
                    continue;
                }
                if (!ce->hasTagName("contraption-snapshot")) continue;
                SnapshotOrganism sc;
                sc.organismName = ce->getStringAttribute("contraption-name").toStdString();
                for (auto* pe : ce->getChildIterator())
                    if (pe->hasTagName("property-snapshot")) sc.values.push_back(parseSnapshotValue(*pe));
                s.organisms.push_back(std::move(sc));
            }
            ms.snapshots.push_back(std::move(s));
        }
    }
    if (!msEl) return;
    ms.present = true;
    ms.temperature = msEl->getDoubleAttribute("temperature", 1.0);
    if (auto* mask = msEl->getChildByName("document-snapshot-restore-mask"))
        for (auto* ce : mask->getChildIterator()) {
            if (!ce->hasTagName("contraption-snapshot-restore-mask")) continue;
            const auto cn = ce->getStringAttribute("contraption-name").toStdString();
            for (auto* pe : ce->getChildIterator())
                if (pe->hasTagName("property-snapshot-restore-mask"))
                    ms.mask.push_back({cn, pe->getIntAttribute("property-index", -1),
                                       pe->getIntAttribute("restore", 1) != 0});
        }
    if (auto* pts = msEl->getChildByName("metasurface-points"))
        for (auto* pe : pts->getChildIterator())
            if (pe->hasTagName("metasurface-point"))
                ms.points.push_back({pe->getIntAttribute("snapshot-index", 0),
                                     pe->getDoubleAttribute("x", 0.0), pe->getDoubleAttribute("y", 0.0)});
    if (auto* mp = msEl->getChildByName("morph-path"))
        for (auto* pe : mp->getChildIterator())
            if (pe->hasTagName("morph-point"))
                ms.morphPath.push_back({pe->getDoubleAttribute("beat", 0.0),
                                        pe->getDoubleAttribute("x", 0.5),
                                        pe->getDoubleAttribute("y", 0.5)});
}

bool parsePatchFile(const std::string& path, PatchDocumentModel& out, std::string& error,
                  std::unique_ptr<juce::XmlElement>* rawOut) {
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    if (!f.existsAsFile()) { error = "file not found: " + path; return false; }
    if (!parsePatchText(f.loadFileAsString().toStdString(), out, error, rawOut)) return false;
    const auto dir = f.getParentDirectory();
    for (auto& c : out.organisms)
        for (auto& ch : c.pattern.channels)
            if (!ch.audioFile.empty()
                && !juce::File::isAbsolutePath(juce::String(ch.audioFile)))
                ch.audioFile = dir.getChildFile(fromDocumentPath(juce::String(ch.audioFile)))
                                   .getFullPathName().toStdString();
    migrateLegacyControl(out);
    return true;
}

void migrateMorphPath(PatchDocumentModel& out) {
    if (out.metapad.morphPath.empty()) return;
    OrganismModel* meta = nullptr;
    for (auto& cm : out.organisms)
        if (isMetapadPseudo(cm.displayClass)) { meta = &cm; break; }
    if (meta == nullptr) {
        OrganismModel cm;
        cm.name = "Metapad";
        while (out.byName(cm.name) != nullptr) cm.name += "_";
        cm.classRaw = cm.displayClass = "MetasurfacePseudoSP";
        out.organisms.push_back(std::move(cm));
        meta = &out.organisms.back();
    }
    auto ensure = [&](const char* param) {
        for (auto& l : meta->automation) if (l.propertyName == param) return;
        AutomationLane nl;
        nl.propertyName = param;
        nl.propertyIndex = -1;
        nl.kind = "double";
        meta->automation.push_back(std::move(nl));
        AutomationView v;
        v.organismName = meta->name;
        v.propertyName = param;
        v.propertyIndex = -1;
        v.index = (int) out.automationViews.size();
        out.automationViews.push_back(v);
    };
    ensure(kMetaXParam);
    ensure(kMetaYParam);
    for (auto& l : meta->automation) {
        const bool isX = l.propertyName == kMetaXParam;
        if (!isX && l.propertyName != kMetaYParam) continue;
        for (const auto& p : out.metapad.morphPath)
            l.points.push_back({p.beat, isX ? p.x : p.y, isX ? p.x : p.y});
    }
    out.metapad.morphPath.clear();
}

bool parsePatchText(const std::string& xmlText, PatchDocumentModel& out, std::string& error,
                  std::unique_ptr<juce::XmlElement>* rawOut) {
    juce::XmlDocument doc{juce::String(juce::CharPointer_UTF8(xmlText.c_str()))};
    std::unique_ptr<juce::XmlElement> root(doc.getDocumentElement());
    if (!root) { error = doc.getLastParseError().toStdString(); return false; }
    if (!isPatchRootTag(root->getTagName().toStdString())) {
        error = "not a patch document: <" + root->getTagName().toStdString() + "> is not a document root";
        return false;
    }

    out.version = root->getStringAttribute("version").toStdString();
    out.newerFormat = root->getStringAttribute("version").getIntValue()
                      > PatchDocumentModel::kFormatVersion;
    out.applicationPath = root->getStringAttribute("application-path").toStdString();
    out.documentPath = root->getStringAttribute("document-path").toStdString();
    if (auto* notes = root->getChildByName("notes"))
        out.notes = notes->getAllSubText().toStdString();
    if (auto* ml = root->getChildByName("master-level"))
        out.masterLevel = juce::jlimit(0.0, 1.0, ml->getDoubleAttribute("value", 1.0));
    if (auto* lim = root->getChildByName("master-limiter"))
        out.masterLimiter = lim->getIntAttribute("value", 0) != 0;
    if (auto* gr = root->getChildByName("groove")) {
        out.groove = juce::jlimit(0.0, 1.0, gr->getDoubleAttribute("value", 0.0));
        const auto u = gr->getStringAttribute("unit", "1/16");
        if (u.isNotEmpty()) out.grooveUnit = u.toStdString();
    }
    if (auto* mods = root->getChildByName("midi-modifiers"))
        for (auto* me : mods->getChildIterator()) {
            if (!me->hasTagName("modifier")) continue;
            MidiModifierModel m;
            m.source = me->getIntAttribute("number", -1);
            m.latching = me->getIntAttribute("latching", 0) != 0;
            m.ownAction = me->getIntAttribute("own-action", 0) != 0;
            if (m.source >= 0) out.midiModifiers.push_back(m);
        }

    auto* patch = root->getChildByName("patch");
    if (!patch) { error = "no <patch>"; return false; }

    if (auto* clk = patch->getChildByName("clock")) {
        out.clock.tempo = clk->getDoubleAttribute("tempo", 120.0);
        out.clock.loopStart = clk->getDoubleAttribute("loop-start", 0.0);
        out.clock.loopEnd = clk->getDoubleAttribute("loop-end", 0.0);
        out.clock.loopEnabled = clk->getIntAttribute("loop-enabled", 0) != 0;
        out.clock.songLength = clk->getDoubleAttribute("song-length", 0.0);
        if (auto* ts = clk->getChildByName("time-signature-timepoints"))
            out.clock.timeSignature = ts->getAllSubText().toStdString();
    }

    for (auto* el : patch->getChildIterator()) {
        if (el->hasTagName("contraption")) {
            OrganismModel c;
            c.name = el->getStringAttribute("name").toStdString();
            c.internal = el->getIntAttribute("internal", 0) != 0;
            parseClass(el->getStringAttribute("class"), c);
            if (auto* props = el->getChildByName("properties")) {
                for (auto* p : props->getChildIterator()) {
                    if (p->hasTagName("property")) {
                        c.properties.push_back(parseProperty(*p));
                        if (auto* pat = p->getChildByName("pattern")) parsePattern(*pat, c.pattern);
                    } else if (p->hasTagName("au-class-info")) {
                        c.hasBlob = true;
                    } else if (p->hasTagName("vst3-class-info") || p->hasTagName("lv2-class-info")
                               || p->hasTagName("au-state")) {
                        c.pluginState = p->getAllSubText().trim().toStdString();
                        c.hasBlob = true;
                    }
                }
            }
            if (auto* nr = el->getChildByName("no-random"))
                for (auto* pe : nr->getChildIterator())
                    if (pe->hasTagName("property"))
                        c.rollLocked.insert(pe->getStringAttribute("name").toStdString());
            if (auto* presets = el->getChildByName("presets")) {
                c.currentPreset = presets->getIntAttribute("current-preset", 0);
                c.presetDirty = presets->getIntAttribute("current-preset-dirty", 0) != 0;
                c.currentPresetName =
                    presets->getStringAttribute("current-preset-name").toStdString();
                c.currentPresetSource =
                    presets->getStringAttribute("current-preset-source").toStdString();
                for (auto* pe : presets->getChildIterator()) {
                    if (!pe->hasTagName("preset")) continue;
                    PresetModel pm;
                    pm.number = pe->getIntAttribute("number", (int) c.presets.size() + 1);
                    pm.name = pe->getStringAttribute("name").toStdString();
                    for (auto* p : pe->getChildIterator())
                        if (p->hasTagName("property")) pm.properties.push_back(parseProperty(*p));
                    c.presets.push_back(std::move(pm));
                }
            }
            if (auto* mod = el->getChildByName("modulation-sources")) parseModulationSources(*mod, c);
            parseMidiSettings(*el, c);
            out.organisms.push_back(std::move(c));
        } else if (el->hasTagName("audio-connection") || el->hasTagName("midi-connection")
                   || el->hasTagName("video-connection")) {
            ConnectionModel m;
            m.src = el->getStringAttribute("from").toStdString();
            m.srcOutlet = el->getIntAttribute("from-outlet", 0);
            m.dst = el->getStringAttribute("to").toStdString();
            m.dstInlet = el->getIntAttribute("to-inlet", 0);
            m.midiChannel = el->getIntAttribute("channel", 0);
            (el->hasTagName("midi-connection")
                 ? out.midiConnections
                 : el->hasTagName("video-connection") ? out.videoConnections
                                                      : out.connections).push_back(m);
        }
    }

    if (auto* cviews = root->getChildByName("contraption-views"))
            for (auto* ve : cviews->getChildIterator()) {
                if (!ve->hasTagName("contraption-view")) continue;
                OrganismView v;
                v.organismName = ve->getStringAttribute("contraption-name").toStdString();
                v.patcherX = ve->getIntAttribute("patcher-x", 0);
                v.patcherY = ve->getIntAttribute("patcher-y", 0);
                v.hasEditor = ve->hasAttribute("editor-visible");
                v.editorVisible = ve->getIntAttribute("editor-visible", 0) != 0;
                v.editorX = ve->getIntAttribute("editor-x", 0);
                v.editorY = ve->getIntAttribute("editor-y", 0);
                v.editorMode = ve->getIntAttribute("editor-mode", -1);
                v.editorW = ve->getIntAttribute("editor-w", 0);
                v.editorH = ve->getIntAttribute("editor-h", 0);
                v.editorHalf = ve->hasAttribute("editor-half")
                                   ? (ve->getIntAttribute("editor-half", 0) != 0 ? 1 : 0)
                                   : -1;
                v.editorCollapsed = ve->getIntAttribute("editor-collapsed", 0) != 0;
                v.editorFloating = ve->getIntAttribute("editor-float", 0) != 0;
                v.floatX = ve->getIntAttribute("float-x", 0);
                v.floatY = ve->getIntAttribute("float-y", 0);
                v.floatW = ve->getIntAttribute("float-w", 0);
                v.floatH = ve->getIntAttribute("float-h", 0);
                out.views.push_back(std::move(v));
            }

    if (auto* aviews = root->getChildByName("automation-views"))
        for (auto* ae : aviews->getChildIterator()) {
            if (!ae->hasTagName("automation-view")) continue;
            AutomationView v;
            v.organismName = ae->getStringAttribute("contraption-name").toStdString();
            v.propertyName = ae->getStringAttribute("property-name").toStdString();
            v.propertyIndex = ae->getIntAttribute("property-index", -1);
            v.index = ae->getIntAttribute("index", 0);
            v.height = ae->getIntAttribute("height", 50);
            v.snapTo = ae->getIntAttribute("snap-to", 0) != 0;
            out.automationViews.push_back(std::move(v));
        }

    if (auto* boxes = root->getChildByName("performance-boxes"))
        for (auto* be : boxes->getChildIterator()) {
            if (!be->hasTagName("performance-box")) continue;
            PerformanceBox b;
            b.organism = be->getStringAttribute("contraption").toStdString();
            b.startBeat = be->getDoubleAttribute("start", 0.0);
            b.endBeat = be->getDoubleAttribute("end", 0.0);
            out.perfBoxes.push_back(std::move(b));
        }

    if (auto* av = root->getChildByName("application-view"))
        if (auto* mv = av->getChildByName("metasurface-view"))
            out.metapad.interpolateMode = mv->getIntAttribute("interpolate-mode", 0);

    parseMetapad(*root, out.metapad);
    migrateMorphPath(out);
    for (auto& cm : out.organisms) clipops::ensureClipIds(cm.pattern);

    if (rawOut) *rawOut = std::move(root);
    return true;
}

}
