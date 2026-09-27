// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatchWriter.h"

#include <cmath>
#include <set>

#include <memory>
#include <vector>

#include "io/PatchParseInternal.h"
#include "io/PatchWriteInternal.h"

namespace hum {

namespace {

int indexOfChild(xml::Element& parent, xml::Element* child) {
    int i = 0;
    for (auto* c : parent.children()) { if (c == child) return i; ++i; }
    return -1;
}

void writeNotes(xml::Element& root, const PatchDocumentModel& doc) {
    if (auto* old = root.child("notes")) root.removeChild(old);
    if (doc.notes.empty()) return;
    root.addChild("notes")->addText(doc.notes);
}

void writeMasterLevel(xml::Element& root, const PatchDocumentModel& doc) {
    if (auto* old = root.child("master-level")) root.removeChild(old);
    if (std::abs(doc.masterLevel - 1.0) < 1e-6) return;
    root.addChild("master-level")->setAttribute("value", doc.masterLevel);
}

void writeMasterLimiter(xml::Element& root, const PatchDocumentModel& doc) {
    if (auto* old = root.child("master-limiter")) root.removeChild(old);
    if (!doc.masterLimiter) return;
    root.addChild("master-limiter")->setAttribute("value", 1);
}

void writeMidiModifiers(xml::Element& root, const PatchDocumentModel& doc) {
    if (auto* old = root.child("midi-modifiers")) root.removeChild(old);
    if (doc.midiModifiers.empty()) return;
    auto* mods = root.addChild("midi-modifiers");
    for (const auto& m : doc.midiModifiers) {
        auto* me = mods->addChild("modifier");
        me->setAttribute("number", m.source);
        me->setAttribute("latching", m.latching ? 1 : 0);
        me->setAttribute("own-action", m.ownAction ? 1 : 0);
    }
}

void writeGroove(xml::Element& root, const PatchDocumentModel& doc) {
    if (auto* old = root.child("groove")) root.removeChild(old);
    if (doc.groove <= 0.0 && doc.grooveUnit == "1/16") return;
    auto* g = root.addChild("groove");
    g->setAttribute("value", doc.groove);
    g->setAttribute("unit", doc.grooveUnit);
}

void writeClock(xml::Element& clk, const ClockModel& c) {
    clk.setAttribute("tempo", c.tempo);
    clk.setAttribute("loop-start", c.loopStart);
    clk.setAttribute("loop-end", c.loopEnd);
    clk.setAttribute("loop-enabled", c.loopEnabled ? 1 : 0);
    if (c.songLength > 0.0) clk.setAttribute("song-length", c.songLength);
    else                    clk.removeAttribute("song-length");
    if (auto* old = clk.child("time-signature-timepoints"))
        clk.removeChild(old);
    if (!c.timeSignature.empty())
        clk.addChild("time-signature-timepoints")
            ->addText(c.timeSignature);
}

void buildFresh(xml::Element& root, const PatchDocumentModel& doc) {
    root.setAttribute("version", doc.version.empty() ? std::string("1") : doc.version);

    auto* patch = root.addChild("patch");
    writeClock(*patch->addChild("clock"), doc.clock);

    for (const auto& c : doc.organisms)
        writeOrganism(*patch->addChild("contraption"), c);
    for (const auto& conn : doc.connections)
        writeConnection(*patch->addChild("audio-connection"), conn);
    for (const auto& conn : doc.midiConnections)
        writeConnection(*patch->addChild("midi-connection"), conn);
    for (const auto& conn : doc.videoConnections)
        writeConnection(*patch->addChild("video-connection"), conn);

    auto* cviews = root.addChild("contraption-views");
    for (const auto& v : doc.views)
        writeView(*cviews->addChild("contraption-view"), v);

    auto* aviews = root.addChild("automation-views");
    for (const auto& v : doc.automationViews)
        writeAutomationView(*aviews->addChild("automation-view"), v);

    auto* boxes = root.addChild("performance-boxes");
    for (const auto& b : doc.perfBoxes) {
        auto* be = boxes->addChild("performance-box");
        be->setAttribute("contraption", b.organism);
        be->setAttribute("start", b.startBeat);
        be->setAttribute("end", b.endBeat);
    }

    writeMetapad(root, doc.metapad);
    writeNotes(root, doc);
    writeMasterLevel(root, doc);
    writeMasterLimiter(root, doc);
    writeGroove(root, doc);
    writeMidiModifiers(root, doc);
}

void patchExisting(xml::Element& root, const PatchDocumentModel& doc) {
    auto* patch = root.child("patch");
    if (!patch) { buildFresh(root, doc); return; }

    auto* clk = patch->child("clock");
    if (clk == nullptr) clk = patch->prependChild(std::make_unique<xml::Element>("clock"));
    writeClock(*clk, doc.clock);

    std::set<std::string> modelNames;
    for (const auto& c : doc.organisms) modelNames.insert(c.name);

    std::set<std::string> seen;
    std::vector<xml::Element*> staleOrganisms, oldConnections;
    for (auto* ce : patch->children()) {
        if (ce->hasTag("audio-connection") || ce->hasTag("midi-connection")
            || ce->hasTag("video-connection")) {
            oldConnections.push_back(ce);
            continue;
        }
        if (!ce->hasTag("contraption")) continue;
        auto nm = ce->attribute("name");
        if (!modelNames.count(nm)) { staleOrganisms.push_back(ce); continue; }

        const OrganismModel* cm = doc.byName(nm);
        if (cm != nullptr) {
            const auto storedClass = ce->attribute("class");
            const auto& modelClass = cm->classRaw.empty() ? cm->displayClass : cm->classRaw;
            if (storedClass != modelClass) { staleOrganisms.push_back(ce); continue; }
        }
        seen.insert(nm);

        if (cm) {
            if (auto* props = ce->child("properties"))
                for (auto* pe : props->children()) {
                    if (!pe->hasTag("property")) continue;
                    const auto pn = pe->attribute("name");
                    const Parameter* mp = nullptr;
                    for (const auto& q : cm->properties)
                        if (q.name == pn) { mp = &q; break; }
                    if ((mp != nullptr && mp->type == "pattern")
                        || pe->child("pattern") != nullptr) {
                        if (!cm->pattern.present) continue;
                        pe->deleteChildren();
                        writePattern(*pe, cm->pattern);
                        continue;
                    }
                    if (mp != nullptr && mp->userEdited) {
                        pe->deleteChildren();
                        addValue(*pe, *mp);
                    }
                }
            if (auto* props = ce->child("properties"))
                for (const auto& mp : cm->properties) {
                    if (!mp.userEdited) continue;
                    bool present = false;
                    for (auto* pe : props->children())
                        if (pe->hasTag("property")
                            && pe->attribute("name") == mp.name) {
                            present = true;
                            break;
                        }
                    if (present) continue;
                    if (mp.type == "pattern") {
                        if (!cm->pattern.present) continue;
                        auto* pe = props->addChild("property");
                        pe->setAttribute("index", mp.index);
                        pe->setAttribute("name", mp.name);
                        writePattern(*pe, cm->pattern);
                        continue;
                    }
                    writeProperty(*props, mp);
                }
            if (auto* oldLocks = ce->child("no-random"))
                ce->removeChild(oldLocks);
            writeRollLocks(*ce, *cm);
            if (auto* oldEnds = ce->child("range-ends"))
                ce->removeChild(oldEnds);
            if (auto* oldModes = ce->child("range-mode"))
                ce->removeChild(oldModes);
            writeRangeModes(*ce, *cm);
            if (auto* oldInput = ce->child("track-input"))
                ce->removeChild(oldInput);
            writeTrackInput(*ce, *cm);
            if (auto* oldRow = ce->child("timeline-row"))
                ce->removeChild(oldRow);
            writeTimelineRow(*ce, *cm);
            if (auto* oldPresets = ce->child("presets")) ce->removeChild(oldPresets);
            auto pres = std::make_unique<xml::Element>("presets");
            writePresets(*pres, *cm);
            int propIdx = -1;
            if (auto* props = ce->child("properties")) propIdx = indexOfChild(*ce, props);
            if (propIdx >= 0) ce->insertChild(std::move(pres), propIdx + 1);
            else              ce->addChild(std::move(pres));

            auto* mod = ce->child("modulation-sources");
            if (!mod) mod = ce->addChild("modulation-sources");
            std::vector<xml::Element*> oldSources;
            for (auto* ps : mod->children())
                if (ps->hasTag("property-sources") &&
                    (ps->child("automation-controller") || ps->child("midi-controller")
                     || ps->child("osc-controller") || ps->child("mod-controller")))
                    oldSources.push_back(ps);
            for (auto* e : oldSources) mod->removeChild(e);
            writeAutomationLanes(*mod, *cm);
            writeMidiSources(*mod, *cm);
            writeOscSources(*mod, *cm);
            writeModSources(*mod, *cm);

            if (const auto tag = pluginStateTag(cm->kind); !tag.empty())
                if (auto* props = ce->child("properties")) {
                    if (auto* oldState = props->child(tag))
                        props->removeChild(oldState);
                    if (!cm->pluginState.empty())
                        props->addChild(tag)->addText(cm->pluginState);
                }

            OrganismModel asStored;
            parseMidiSettings(*ce, asStored);
            if (asStored.midiReceiveMode != cm->midiReceiveMode ||
                asStored.midiReceiveChannel != cm->midiReceiveChannel ||
                asStored.midiReceivePort != cm->midiReceivePort) {
                if (auto* old = ce->child("vst-midi-settings"))
                    ce->removeChild(old);
                writeMidiSettings(*ce, *cm);
            }
        }
    }
    for (auto* e : staleOrganisms) patch->removeChild(e);
    for (auto* e : oldConnections)    patch->removeChild(e);

    for (const auto& c : doc.organisms)
        if (!seen.count(c.name))
            writeOrganism(*patch->addChild("contraption"), c);

    for (const auto& conn : doc.connections)
        writeConnection(*patch->addChild("audio-connection"), conn);
    for (const auto& conn : doc.midiConnections)
        writeConnection(*patch->addChild("midi-connection"), conn);
    for (const auto& conn : doc.videoConnections)
        writeConnection(*patch->addChild("video-connection"), conn);

    if (auto* old = root.child("contraption-views")) root.removeChild(old);
    auto cviewsOwned = std::make_unique<xml::Element>("contraption-views");
    for (const auto& v : doc.views)
        writeView(*cviewsOwned->addChild("contraption-view"), v);
    int after = -1;
    if (auto* appView = root.child("application-view")) after = indexOfChild(root, appView);
    else if (auto* p = root.child("patch"))            after = indexOfChild(root, p);
    auto* cviews = after >= 0 ? root.insertChild(std::move(cviewsOwned), after + 1) : root.addChild(std::move(cviewsOwned));

    if (auto* old = root.child("automation-views")) root.removeChild(old);
    auto aviewsOwned = std::make_unique<xml::Element>("automation-views");
    for (const auto& v : doc.automationViews)
        writeAutomationView(*aviewsOwned->addChild("automation-view"), v);
    auto* aviews = root.insertChild(std::move(aviewsOwned), indexOfChild(root, cviews) + 1);

    if (auto* old = root.child("performance-boxes")) root.removeChild(old);
    auto boxes = std::make_unique<xml::Element>("performance-boxes");
    for (const auto& b : doc.perfBoxes) {
        auto* be = boxes->addChild("performance-box");
        be->setAttribute("contraption", b.organism);
        be->setAttribute("start", b.startBeat);
        be->setAttribute("end", b.endBeat);
    }
    root.insertChild(std::move(boxes), indexOfChild(root, aviews) + 1);

    writeMetapad(root, doc.metapad);

    writeNotes(root, doc);
    writeMasterLevel(root, doc);
    writeMasterLimiter(root, doc);
    writeGroove(root, doc);
    writeMidiModifiers(root, doc);
}


}

std::unique_ptr<xml::Element> buildPatchTree(const PatchDocumentModel& doc, const xml::Element* original) {
    std::unique_ptr<xml::Element> root;
    if (original) {
        root = original->copy();
        patchExisting(*root, doc);
    } else {
        root = std::make_unique<xml::Element>(kPatchRootTag);
        buildFresh(*root, doc);
    }
    return root;
}

std::string writePatchString(const PatchDocumentModel& doc, const xml::Element* original) {
    return xml::write(*buildPatchTree(doc, original));
}

}
