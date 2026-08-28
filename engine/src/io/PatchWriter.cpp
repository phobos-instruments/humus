#include "io/PatchWriter.h"

#include <cmath>
#include <set>

#include <juce_core/juce_core.h>

#include "io/PatchFormat.h"
#include "io/PatchParseInternal.h"
#include "io/PatchWriteInternal.h"

namespace hum {

namespace {

int indexOfChild(juce::XmlElement& parent, juce::XmlElement* child) {
    int i = 0;
    for (auto* c : parent.getChildIterator()) { if (c == child) return i; ++i; }
    return -1;
}

void writeNotes(juce::XmlElement& root, const PatchDocumentModel& doc) {
    if (auto* old = root.getChildByName("notes")) root.removeChildElement(old, true);
    if (doc.notes.empty()) return;
    root.createNewChildElement("notes")->addTextElement(
        juce::String(juce::CharPointer_UTF8(doc.notes.c_str())));
}

void writeMasterLevel(juce::XmlElement& root, const PatchDocumentModel& doc) {
    if (auto* old = root.getChildByName("master-level")) root.removeChildElement(old, true);
    if (std::abs(doc.masterLevel - 1.0) < 1e-6) return;
    root.createNewChildElement("master-level")->setAttribute("value", doc.masterLevel);
}

void writeMasterLimiter(juce::XmlElement& root, const PatchDocumentModel& doc) {
    if (auto* old = root.getChildByName("master-limiter")) root.removeChildElement(old, true);
    if (!doc.masterLimiter) return;
    root.createNewChildElement("master-limiter")->setAttribute("value", 1);
}

void writeGroove(juce::XmlElement& root, const PatchDocumentModel& doc) {
    if (auto* old = root.getChildByName("groove")) root.removeChildElement(old, true);
    if (doc.groove <= 0.0) return;
    auto* g = root.createNewChildElement("groove");
    g->setAttribute("value", doc.groove);
    g->setAttribute("unit", juce::String(doc.grooveUnit));
}

void writeClock(juce::XmlElement& clk, const ClockModel& c) {
    clk.setAttribute("tempo", c.tempo);
    clk.setAttribute("loop-start", c.loopStart);
    clk.setAttribute("loop-end", c.loopEnd);
    clk.setAttribute("loop-enabled", c.loopEnabled ? 1 : 0);
    if (c.songLength > 0.0) clk.setAttribute("song-length", c.songLength);
    else                    clk.removeAttribute("song-length");
    if (auto* old = clk.getChildByName("time-signature-timepoints"))
        clk.removeChildElement(old, true);
    if (!c.timeSignature.empty())
        clk.createNewChildElement("time-signature-timepoints")
            ->addTextElement(juce::String(c.timeSignature));
}

void buildFresh(juce::XmlElement& root, const PatchDocumentModel& doc) {
    root.setAttribute("version", doc.version.empty() ? "1" : juce::String(doc.version));

    auto* patch = root.createNewChildElement("patch");
    writeClock(*patch->createNewChildElement("clock"), doc.clock);

    for (const auto& c : doc.organisms)
        writeOrganism(*patch->createNewChildElement("contraption"), c);
    for (const auto& conn : doc.connections)
        writeConnection(*patch->createNewChildElement("audio-connection"), conn);
    for (const auto& conn : doc.midiConnections)
        writeConnection(*patch->createNewChildElement("midi-connection"), conn);
    for (const auto& conn : doc.videoConnections)
        writeConnection(*patch->createNewChildElement("video-connection"), conn);

    auto* cviews = root.createNewChildElement("contraption-views");
    for (const auto& v : doc.views)
        writeView(*cviews->createNewChildElement("contraption-view"), v);

    auto* aviews = root.createNewChildElement("automation-views");
    for (const auto& v : doc.automationViews)
        writeAutomationView(*aviews->createNewChildElement("automation-view"), v);

    auto* boxes = root.createNewChildElement("performance-boxes");
    for (const auto& b : doc.perfBoxes) {
        auto* be = boxes->createNewChildElement("performance-box");
        be->setAttribute("contraption", juce::String(b.organism));
        be->setAttribute("start", b.startBeat);
        be->setAttribute("end", b.endBeat);
    }

    writeMetapad(root, doc.metapad);
    writeNotes(root, doc);
    writeMasterLevel(root, doc);
    writeMasterLimiter(root, doc);
    writeGroove(root, doc);
}

void patchExisting(juce::XmlElement& root, const PatchDocumentModel& doc) {
    auto* patch = root.getChildByName("patch");
    if (!patch) { buildFresh(root, doc); return; }

    auto* clk = patch->getChildByName("clock");
    if (clk == nullptr) {
        clk = new juce::XmlElement("clock");
        patch->prependChildElement(clk);
    }
    writeClock(*clk, doc.clock);

    std::set<std::string> modelNames;
    for (const auto& c : doc.organisms) modelNames.insert(c.name);

    std::set<std::string> seen;
    std::vector<juce::XmlElement*> staleOrganisms, oldConnections;
    for (auto* ce : patch->getChildIterator()) {
        if (ce->hasTagName("audio-connection") || ce->hasTagName("midi-connection")
            || ce->hasTagName("video-connection")) {
            oldConnections.push_back(ce);
            continue;
        }
        if (!ce->hasTagName("contraption")) continue;
        auto nm = ce->getStringAttribute("name").toStdString();
        if (!modelNames.count(nm)) { staleOrganisms.push_back(ce); continue; }

        const OrganismModel* cm = doc.byName(nm);
        if (cm != nullptr) {
            const auto storedClass = ce->getStringAttribute("class").toStdString();
            const auto& modelClass = cm->classRaw.empty() ? cm->displayClass : cm->classRaw;
            if (storedClass != modelClass) { staleOrganisms.push_back(ce); continue; }
        }
        seen.insert(nm);

        if (cm) {
            if (auto* props = ce->getChildByName("properties"))
                for (auto* pe : props->getChildIterator()) {
                    if (!pe->hasTagName("property")) continue;
                    const auto pn = pe->getStringAttribute("name").toStdString();
                    const Parameter* mp = nullptr;
                    for (const auto& q : cm->properties)
                        if (q.name == pn) { mp = &q; break; }
                    if ((mp != nullptr && mp->type == "pattern")
                        || pe->getChildByName("pattern") != nullptr) {
                        if (!cm->pattern.present) continue;
                        pe->deleteAllChildElements();
                        writePattern(*pe, cm->pattern);
                        continue;
                    }
                    if (mp != nullptr && mp->userEdited) {
                        pe->deleteAllChildElements();
                        addValue(*pe, *mp);
                    }
                }
            if (auto* props = ce->getChildByName("properties"))
                for (const auto& mp : cm->properties) {
                    if (!mp.userEdited) continue;
                    bool present = false;
                    for (auto* pe : props->getChildIterator())
                        if (pe->hasTagName("property")
                            && pe->getStringAttribute("name") == juce::String(mp.name)) {
                            present = true;
                            break;
                        }
                    if (present) continue;
                    if (mp.type == "pattern") {
                        if (!cm->pattern.present) continue;
                        auto* pe = props->createNewChildElement("property");
                        pe->setAttribute("index", mp.index);
                        pe->setAttribute("name", juce::String(mp.name));
                        writePattern(*pe, cm->pattern);
                        continue;
                    }
                    writeProperty(*props, mp);
                }
            if (auto* oldLocks = ce->getChildByName("no-random"))
                ce->removeChildElement(oldLocks, true);
            writeRollLocks(*ce, *cm);
            if (auto* oldPresets = ce->getChildByName("presets")) ce->removeChildElement(oldPresets, true);
            auto* pres = new juce::XmlElement("presets");
            writePresets(*pres, *cm);
            int propIdx = -1;
            if (auto* props = ce->getChildByName("properties")) propIdx = indexOfChild(*ce, props);
            if (propIdx >= 0) ce->insertChildElement(pres, propIdx + 1);
            else              ce->addChildElement(pres);

            auto* mod = ce->getChildByName("modulation-sources");
            if (!mod) mod = ce->createNewChildElement("modulation-sources");
            std::vector<juce::XmlElement*> oldSources;
            for (auto* ps : mod->getChildIterator())
                if (ps->hasTagName("property-sources") &&
                    (ps->getChildByName("automation-controller") || ps->getChildByName("midi-controller")
                     || ps->getChildByName("osc-controller") || ps->getChildByName("mod-controller")))
                    oldSources.push_back(ps);
            for (auto* e : oldSources) mod->removeChildElement(e, true);
            writeAutomationLanes(*mod, *cm);
            writeMidiSources(*mod, *cm);
            writeOscSources(*mod, *cm);
            writeModSources(*mod, *cm);

            if (const auto tag = pluginStateTag(cm->kind); tag.isNotEmpty())
                if (auto* props = ce->getChildByName("properties")) {
                    if (auto* oldState = props->getChildByName(tag))
                        props->removeChildElement(oldState, true);
                    if (!cm->pluginState.empty())
                        props->createNewChildElement(tag)->addTextElement(
                            juce::String(cm->pluginState));
                }

            OrganismModel asStored;
            parseMidiSettings(*ce, asStored);
            if (asStored.midiReceiveMode != cm->midiReceiveMode ||
                asStored.midiReceiveChannel != cm->midiReceiveChannel ||
                asStored.midiReceivePort != cm->midiReceivePort) {
                if (auto* old = ce->getChildByName("vst-midi-settings"))
                    ce->removeChildElement(old, true);
                writeMidiSettings(*ce, *cm);
            }
        }
    }
    for (auto* e : staleOrganisms) patch->removeChildElement(e, true);
    for (auto* e : oldConnections)    patch->removeChildElement(e, true);

    for (const auto& c : doc.organisms)
        if (!seen.count(c.name))
            writeOrganism(*patch->createNewChildElement("contraption"), c);

    for (const auto& conn : doc.connections)
        writeConnection(*patch->createNewChildElement("audio-connection"), conn);
    for (const auto& conn : doc.midiConnections)
        writeConnection(*patch->createNewChildElement("midi-connection"), conn);
    for (const auto& conn : doc.videoConnections)
        writeConnection(*patch->createNewChildElement("video-connection"), conn);

    if (auto* old = root.getChildByName("contraption-views")) root.removeChildElement(old, true);
    auto* cviews = new juce::XmlElement("contraption-views");
    for (const auto& v : doc.views)
        writeView(*cviews->createNewChildElement("contraption-view"), v);
    int after = -1;
    if (auto* appView = root.getChildByName("application-view")) after = indexOfChild(root, appView);
    else if (auto* p = root.getChildByName("patch"))            after = indexOfChild(root, p);
    if (after >= 0) root.insertChildElement(cviews, after + 1);
    else            root.addChildElement(cviews);

    if (auto* old = root.getChildByName("automation-views")) root.removeChildElement(old, true);
    auto* aviews = new juce::XmlElement("automation-views");
    for (const auto& v : doc.automationViews)
        writeAutomationView(*aviews->createNewChildElement("automation-view"), v);
    root.insertChildElement(aviews, indexOfChild(root, cviews) + 1);

    if (auto* old = root.getChildByName("performance-boxes")) root.removeChildElement(old, true);
    auto* boxes = new juce::XmlElement("performance-boxes");
    for (const auto& b : doc.perfBoxes) {
        auto* be = boxes->createNewChildElement("performance-box");
        be->setAttribute("contraption", juce::String(b.organism));
        be->setAttribute("start", b.startBeat);
        be->setAttribute("end", b.endBeat);
    }
    root.insertChildElement(boxes, indexOfChild(root, aviews) + 1);

    writeMetapad(root, doc.metapad);

    writeNotes(root, doc);
    writeMasterLevel(root, doc);
    writeMasterLimiter(root, doc);
    writeGroove(root, doc);
}

void relativizeAudioClipPaths(juce::XmlElement& el, const juce::File& docDir) {
    if (el.hasTagName("pattern-channel") && el.hasAttribute("file")) {
        const auto s = el.getStringAttribute("file");
        if (juce::File::isAbsolutePath(s)) {
            const juce::File f(s);
            if (f.isAChildOf(docDir))
                el.setAttribute("file", toDocumentPath(f.getRelativePathFrom(docDir)));
        }
    }
    for (auto* c : el.getChildIterator()) relativizeAudioClipPaths(*c, docDir);
}

}

static std::unique_ptr<juce::XmlElement> buildTree(const PatchDocumentModel& doc,
                                                   const juce::XmlElement* original) {
    std::unique_ptr<juce::XmlElement> root;
    if (original) {
        root = std::make_unique<juce::XmlElement>(*original);
        patchExisting(*root, doc);
    } else {
        root = std::make_unique<juce::XmlElement>(kPatchRootTag);
        buildFresh(*root, doc);
    }
    return root;
}

bool writePatchFile(const std::string& path, const PatchDocumentModel& doc, std::string& error,
                  const juce::XmlElement* original) {
    auto root = buildTree(doc, original);
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    relativizeAudioClipPaths(*root, f.getParentDirectory());
    if (!root->writeTo(f, {})) { error = "could not write " + path; return false; }
    return true;
}

std::string writePatchString(const PatchDocumentModel& doc, const juce::XmlElement* original) {
    return buildTree(doc, original)->toString({}).toStdString();
}

}
