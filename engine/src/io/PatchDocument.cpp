// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatchDocument.h"
#include "io/PatchMigrate.h"

#include "core/timeline/ClipOps.h"
#include "core/library/UserLibrary.h"

#include <algorithm>

#include "core/packs/ClassString.h"
#include "io/PatchFormat.h"
#include "io/PatchParseInternal.h"
#include "io/XmlText.h"

namespace hum {

namespace {

void parseClass(const std::string& classRaw, OrganismModel& c) {
    c.classRaw = classRaw;
    const auto id = parseClassString(c.classRaw);
    c.displayClass = id.display;
    c.kind = id.kind;
}

Parameter parseProperty(xml::Element& p) {
    Parameter param;
    param.index = p.intAttribute("index", -1);
    param.name = p.attribute("name");
    for (auto* v : p.children()) {
        const std::string tag = v->tag();
        param.type = tag;
        if (tag == "double" || tag == "int") {
            param.value = xmltext::textDouble(*v);
        } else if (tag == "bool") {
            param.value = xmltext::trimmed(v->allSubText()) == "1" ? 1.0 : 0.0;
        } else if (tag == "enum") {
            param.value = xmltext::textDouble(*v);
        } else if (tag == "range") {
            param.isRange = true;
            if (auto* mn = v->child("min")) param.rangeMin = xmltext::textDouble(*mn);
            if (auto* mx = v->child("max")) param.rangeMax = xmltext::textDouble(*mx);
            param.value = param.rangeMin;
        } else if (tag == "rhythmic-unit" || tag == "soundfile") {
            param.text = library::resolve(
                xmltext::urlUnescaped(xmltext::trimmed(v->allSubText())));
        } else {
            param.text = xmltext::trimmed(v->allSubText());
        }
        break;
    }
    return param;
}

}

SnapshotValue parseSnapshotValue(xml::Element& ps) {
    SnapshotValue v;
    v.propertyIndex = ps.intAttribute("property-index", -1);
    if (auto* c = ps.childAt(0)) {
        v.type = c->tag();
        if (v.type == "range") {
            if (auto* mn = c->child("min")) v.value = xmltext::textDouble(*mn);
            if (auto* mx = c->child("max")) v.value2 = xmltext::textDouble(*mx);
        } else {
            v.value = xmltext::textDouble(*c);
        }
    }
    return v;
}

void parseMetapad(xml::Element& root, MetapadModel& ms) {
    auto* msEl = root.child("metasurface");
    auto* snapsEl = root.child("document-snapshots");
    if (!snapsEl && msEl) snapsEl = msEl->child("document-snapshots");
    if (auto* snaps = snapsEl) {
        ms.present = true;
        for (auto* se : snaps->children()) {
            if (!se->hasTag("document-snapshot")) continue;
            DocumentSnapshot s;
            s.index = se->intAttribute("index", 0);
            s.name = se->attribute("name");
            s.colour = se->attribute("colour");
            for (auto* ce : se->children()) {
                if (ce->hasTag("pattern-snapshot")) {
                    SnapshotPattern sp;
                    sp.organismName = ce->attribute("contraption-name");
                    if (auto* pat = ce->child("pattern")) parsePattern(*pat, sp.pattern);
                    s.patterns.push_back(std::move(sp));
                    continue;
                }
                if (!ce->hasTag("contraption-snapshot")) continue;
                SnapshotOrganism sc;
                sc.organismName = ce->attribute("contraption-name");
                for (auto* pe : ce->children())
                    if (pe->hasTag("property-snapshot")) sc.values.push_back(parseSnapshotValue(*pe));
                s.organisms.push_back(std::move(sc));
            }
            ms.snapshots.push_back(std::move(s));
        }
    }
    if (!msEl) return;
    ms.present = true;
    ms.temperature = msEl->doubleAttribute("temperature", 1.0);
    if (auto* mask = msEl->child("document-snapshot-restore-mask"))
        for (auto* ce : mask->children()) {
            if (!ce->hasTag("contraption-snapshot-restore-mask")) continue;
            const auto cn = ce->attribute("contraption-name");
            for (auto* pe : ce->children())
                if (pe->hasTag("property-snapshot-restore-mask"))
                    ms.mask.push_back({cn, pe->intAttribute("property-index", -1),
                                       pe->intAttribute("restore", 1) != 0});
        }
    if (auto* pts = msEl->child("metasurface-points"))
        for (auto* pe : pts->children())
            if (pe->hasTag("metasurface-point"))
                ms.points.push_back({pe->intAttribute("snapshot-index", 0),
                                     pe->doubleAttribute("x", 0.0), pe->doubleAttribute("y", 0.0)});
    if (auto* mp = msEl->child("morph-path"))
        for (auto* pe : mp->children())
            if (pe->hasTag("morph-point"))
                ms.morphPath.push_back({pe->doubleAttribute("beat", 0.0),
                                        pe->doubleAttribute("x", 0.5),
                                        pe->doubleAttribute("y", 0.5)});
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
                    std::unique_ptr<xml::Element>* rawOut) {
    auto root = xml::parse(xmlText);
    if (!root) { error = "not a readable XML document"; return false; }
    if (!isPatchRootTag(root->tag())) {
        error = "not a patch document: <" + root->tag() + "> is not a document root";
        return false;
    }

    out.version = root->attribute("version");
    out.newerFormat = xml::intValue(root->attribute("version")) > PatchDocumentModel::kFormatVersion;
    out.applicationPath = root->attribute("application-path");
    out.documentPath = root->attribute("document-path");
    if (auto* notes = root->child("notes"))
        out.notes = notes->allSubText();
    if (auto* ml = root->child("master-level"))
        out.masterLevel = std::clamp(ml->doubleAttribute("value", 1.0), 0.0, 1.0);
    if (auto* lim = root->child("master-limiter"))
        out.masterLimiter = lim->intAttribute("value", 0) != 0;
    if (auto* gr = root->child("groove")) {
        out.groove = std::clamp(gr->doubleAttribute("value", 0.0), 0.0, 1.0);
        const auto u = gr->attribute("unit", "1/16");
        if (!u.empty()) out.grooveUnit = u;
    }
    if (auto* mods = root->child("midi-modifiers"))
        for (auto* me : mods->children()) {
            if (!me->hasTag("modifier")) continue;
            MidiModifierModel m;
            m.source = me->intAttribute("number", -1);
            m.latching = me->intAttribute("latching", 0) != 0;
            m.ownAction = me->intAttribute("own-action", 0) != 0;
            if (m.source >= 0) out.midiModifiers.push_back(m);
        }

    auto* patch = root->child("patch");
    if (!patch) { error = "no <patch>"; return false; }

    if (auto* clk = patch->child("clock")) {
        out.clock.tempo = clk->doubleAttribute("tempo", 120.0);
        out.clock.loopStart = clk->doubleAttribute("loop-start", 0.0);
        out.clock.loopEnd = clk->doubleAttribute("loop-end", 0.0);
        out.clock.loopEnabled = clk->intAttribute("loop-enabled", 0) != 0;
        out.clock.songLength = clk->doubleAttribute("song-length", 0.0);
        if (auto* ts = clk->child("time-signature-timepoints"))
            out.clock.timeSignature = ts->allSubText();
    }

    for (auto* el : patch->children()) {
        if (el->hasTag("contraption")) {
            OrganismModel c;
            c.name = el->attribute("name");
            c.internal = el->intAttribute("internal", 0) != 0;
            parseClass(el->attribute("class"), c);
            if (auto* props = el->child("properties")) {
                for (auto* p : props->children()) {
                    if (p->hasTag("property")) {
                        c.properties.push_back(parseProperty(*p));
                        if (auto* pat = p->child("pattern")) parsePattern(*pat, c.pattern);
                    } else if (p->hasTag("au-class-info")) {
                        c.hasBlob = true;
                    } else if (p->hasTag("vst3-class-info") || p->hasTag("lv2-class-info")
                               || p->hasTag("au-state")) {
                        c.pluginState = xmltext::trimmed(p->allSubText());
                        c.hasBlob = true;
                    }
                }
            }
            if (auto* nr = el->child("no-random"))
                for (auto* pe : nr->children())
                    if (pe->hasTag("property"))
                        c.rollLocked.insert(pe->attribute("name"));
            if (auto* re = el->child("range-ends"))
                for (auto* pe : re->children())
                    if (pe->hasTag("property"))
                        c.rangeModes[pe->attribute("name")] = "ends";
            if (auto* rm = el->child("range-mode"))
                for (auto* pe : rm->children())
                    if (pe->hasTag("property"))
                        c.rangeModes[pe->attribute("name")] = pe->attribute("mode", "span");
            if (auto* ti = el->child("track-input")) {
                const auto port = ti->attribute("port", "");
                if (port == "all") c.trackInput = OrganismModel::kTrackInputAll;
                else if (port == "none") c.trackInput = OrganismModel::kTrackInputNone;
                else if (const int n = ti->intAttribute("port", 0); n >= 1 && n <= 8) c.trackInput = n - 1;
            }
            if (auto* placed = el->child("timeline-row"))
                c.timelineRow = std::max(-1, placed->intAttribute("index", -1));
            if (auto* presets = el->child("presets")) {
                c.currentPreset = presets->intAttribute("current-preset", 0);
                c.presetDirty = presets->intAttribute("current-preset-dirty", 0) != 0;
                c.currentPresetName =
                    presets->attribute("current-preset-name");
                c.currentPresetSource =
                    presets->attribute("current-preset-source");
                for (auto* pe : presets->children()) {
                    if (!pe->hasTag("preset")) continue;
                    PresetModel pm;
                    pm.number = pe->intAttribute("number", (int) c.presets.size() + 1);
                    pm.name = pe->attribute("name");
                    for (auto* p : pe->children())
                        if (p->hasTag("property")) pm.properties.push_back(parseProperty(*p));
                    c.presets.push_back(std::move(pm));
                }
            }
            if (auto* mod = el->child("modulation-sources")) parseModulationSources(*mod, c);
            parseMidiSettings(*el, c);
            out.organisms.push_back(std::move(c));
        } else if (el->hasTag("audio-connection") || el->hasTag("midi-connection")
                   || el->hasTag("video-connection")) {
            ConnectionModel m;
            m.src = el->attribute("from");
            m.srcOutlet = el->intAttribute("from-outlet", 0);
            m.dst = el->attribute("to");
            m.dstInlet = el->intAttribute("to-inlet", 0);
            m.midiChannel = el->intAttribute("channel", 0);
            (el->hasTag("midi-connection")
                 ? out.midiConnections
                 : el->hasTag("video-connection") ? out.videoConnections
                                                      : out.connections).push_back(m);
        }
    }

    if (auto* cviews = root->child("contraption-views"))
            for (auto* ve : cviews->children()) {
                if (!ve->hasTag("contraption-view")) continue;
                OrganismView v;
                v.organismName = ve->attribute("contraption-name");
                v.patcherX = ve->intAttribute("patcher-x", 0);
                v.patcherY = ve->intAttribute("patcher-y", 0);
                v.hasEditor = ve->hasAttribute("editor-visible");
                v.editorVisible = ve->intAttribute("editor-visible", 0) != 0;
                v.editorX = ve->intAttribute("editor-x", 0);
                v.editorY = ve->intAttribute("editor-y", 0);
                v.editorMode = ve->intAttribute("editor-mode", -1);
                v.editorW = ve->intAttribute("editor-w", 0);
                v.editorH = ve->intAttribute("editor-h", 0);
                v.editorHalf = ve->hasAttribute("editor-half")
                                   ? (ve->intAttribute("editor-half", 0) != 0 ? 1 : 0)
                                   : -1;
                v.editorCollapsed = ve->intAttribute("editor-collapsed", 0) != 0;
                v.editorFloating = ve->intAttribute("editor-float", 0) != 0;
                v.floatX = ve->intAttribute("float-x", 0);
                v.floatY = ve->intAttribute("float-y", 0);
                v.floatW = ve->intAttribute("float-w", 0);
                v.floatH = ve->intAttribute("float-h", 0);
                out.views.push_back(std::move(v));
            }

    if (auto* aviews = root->child("automation-views"))
        for (auto* ae : aviews->children()) {
            if (!ae->hasTag("automation-view")) continue;
            AutomationView v;
            v.organismName = ae->attribute("contraption-name");
            v.propertyName = ae->attribute("property-name");
            v.propertyIndex = ae->intAttribute("property-index", -1);
            v.index = ae->intAttribute("index", 0);
            v.height = ae->intAttribute("height", 50);
            v.snapTo = ae->intAttribute("snap-to", 0) != 0;
            out.automationViews.push_back(std::move(v));
        }

    if (auto* boxes = root->child("performance-boxes"))
        for (auto* be : boxes->children()) {
            if (!be->hasTag("performance-box")) continue;
            PerformanceBox b;
            b.organism = be->attribute("contraption");
            b.startBeat = be->doubleAttribute("start", 0.0);
            b.endBeat = be->doubleAttribute("end", 0.0);
            out.perfBoxes.push_back(std::move(b));
        }

    if (auto* av = root->child("application-view"))
        if (auto* mv = av->child("metasurface-view"))
            out.metapad.interpolateMode = mv->intAttribute("interpolate-mode", 0);

    parseMetapad(*root, out.metapad);
    migrateMorphPath(out);
    for (auto& cm : out.organisms) clipops::ensureClipIds(cm.pattern);

    if (rawOut) *rawOut = std::move(root);
    return true;
}

}
