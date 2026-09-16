// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/library/BankLibrary.h"
#include "core/library/UserLibrary.h"
#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>
#include "core/graph/GraphLayout.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PerfBox.h"
#include "core/graph/PodModel.h"
#include "core/packs/Roles.h"
#include "hum/Registry.h"
#include "io/PatchWriter.h"
#include "io/ModRouteBuild.h"
#include "io/PatchLoader.h"
#include "gui/app/AppSettings.h"

namespace hum {

void EngineHost::newDocument(juce::Point<int> masterPos) {
    stopAudio();
    discardLiveGraph();
    beginTransaction();
    pushUndo();
    model_ = PatchDocumentModel{};
    model_.version = "1";
    model_.clock.tempo = 120.0;
    outputGain_.store(1.0f);
    limiterOn_.store(false);
    docPath_.clear();
    positions_.clear();
    original_.reset();
    if (const auto out = classWithRole(role::kMasterOut); !out.empty())
        addOrganism(out, masterPos);
    endTransaction();
    undo_.clear();
    redo_.clear();
    paramHistory_.clear();
    dirty_ = false;
}

bool EngineHost::loadFile(const std::string& path, std::string& error) {
    stopAudio();
    PatchDocumentModel m;
    std::unique_ptr<juce::XmlElement> raw;
    if (!parsePatchFile(path, m, error, &raw)) return false;
    discardLiveGraph();
    model_ = std::move(m);
    reconcilePropertyTypes(model_);
    for (auto& c : model_.organisms)
        for (auto& p : c.properties)
            if (p.name == "Record") p.value = 0.0;
    pods::resolvePodVideoCords(model_);
    original_ = std::move(raw);
    docPath_ = path;
    positions_.clear();
    midi().syncMapFromModel();
    osc().syncMapFromModel();
    mod().syncMapFromModel();
    perfbox::derive(model_);
    applyLoadedLayout();
    requestRebuild();
    undo_.clear();
    redo_.clear();
    paramHistory_.clear();
    dirty_ = false;
    if (link_ && linkEnabled_.load(std::memory_order_relaxed) && link_->numPeers() == 0)
        link_->proposeTempo(model_.clock.tempo);
    return true;
}

void EngineHost::applyLoadedLayout() {
    if (model_.views.empty() && !model_.organisms.empty()) {
        std::vector<LayoutNode> nodes;
        std::map<std::string, int> idx;
        for (auto& c : model_.organisms) {
            idx[c.name] = (int) nodes.size();
            const int ports = std::max(countInlets(model_, c.name), countOutlets(model_, c.name));
            nodes.push_back({std::max(116, 18 + ports * 12), 38});
        }
        std::vector<LayoutEdge> edges;
        auto add = [&](const std::vector<ConnectionModel>& cords) {
            for (auto& cn : cords) {
                const auto s = idx.find(cn.src), d = idx.find(cn.dst);
                if (s != idx.end() && d != idx.end() && s->second != d->second)
                    edges.push_back({s->second, d->second});
            }
        };
        add(model_.connections);
        add(model_.midiConnections);
        add(model_.videoConnections);
        const auto placed = layoutFlowGraph(nodes, edges);
        for (auto& c : model_.organisms) {
            const auto& p = placed[(size_t) idx[c.name]];
            positions_[c.name] = {p.first, p.second};
        }
        return;
    }

    std::map<std::string, const OrganismView*> byName;
    for (auto& v : model_.views) byName[v.organismName] = &v;

    constexpr int margin = 40;
    bool any = false;
    int minX = 0, minY = 0;
    for (auto& v : model_.views) {
        if (!any) { minX = v.patcherX; minY = v.patcherY; any = true; }
        else { minX = std::min(minX, v.patcherX); minY = std::min(minY, v.patcherY); }
    }
    const int offX = any ? margin - minX : 0;
    const int offY = any ? margin - minY : 0;

    int gridIdx = 0;
    for (auto& c : model_.organisms) {
        auto it = byName.find(c.name);
        if (it != byName.end()) {
            positions_[c.name] = {it->second->patcherX + offX, it->second->patcherY + offY};
        } else {
            positions_[c.name] = {60 + (gridIdx % 4) * 180, 60 + (gridIdx / 4) * 130};
            ++gridIdx;
        }
    }

    for (auto& v : model_.views)
        if (!model_.byName(v.organismName) && pods::isPod(model_, v.organismName))
            positions_[v.organismName] = {v.patcherX + offX, v.patcherY + offY};
}

void EngineHost::syncViewsFromPositions() {
    std::vector<OrganismView> merged;
    merged.reserve(model_.organisms.size());
    for (auto& c : model_.organisms) {
        OrganismView v;
        v.organismName = c.name;
        auto p = position(c.name);
        v.patcherX = p.x;
        v.patcherY = p.y;
        for (auto& old : model_.views)
            if (old.organismName == c.name) {
                v.hasEditor = old.hasEditor;
                v.editorVisible = old.editorVisible;
                v.editorX = old.editorX;
                v.editorY = old.editorY;
                v.editorMode = old.editorMode;
                v.editorW = old.editorW;
                v.editorH = old.editorH;
                v.editorHalf = old.editorHalf;
                v.editorCollapsed = old.editorCollapsed;
                v.editorFloating = old.editorFloating;
                v.floatX = old.floatX; v.floatY = old.floatY;
                v.floatW = old.floatW; v.floatH = old.floatH;
                break;
            }
        merged.push_back(std::move(v));
    }
    for (auto& [n, p] : positions_) {
        if (model_.byName(n) || !pods::isPod(model_, n)) continue;
        OrganismView v;
        v.organismName = n;
        v.patcherX = p.x;
        v.patcherY = p.y;
        merged.push_back(std::move(v));
    }
    model_.views = std::move(merged);
}

bool EngineHost::writeDocumentTo(const std::string& path, std::string& error,
                                 bool pullPluginState) {
    syncViewsFromPositions();
    midi().syncMapToModel();
    osc().syncMapToModel();
    mod().syncMapToModel();
    if (pullPluginState) syncPluginStateToModel();
    storeSessionAudioTo(path);
    return writePatchFile(path, model_, error, original_.get());
}

void EngineHost::storeSessionAudioTo(const std::string& path) {
    const juce::File doc(juce::String(juce::CharPointer_UTF8(path.c_str())));
    const auto dir = doc.getParentDirectory()
                        .getChildFile(doc.getFileNameWithoutExtension() + " Loops");
    for (auto& cm : model_.organisms) {
        auto* sa = dynamic_cast<SessionAudio*>(liveOrganism(cm.name));
        if (sa == nullptr) continue;
        const auto prefix = dir.getChildFile(juce::File::createLegalFileName(
            juce::String(juce::CharPointer_UTF8(cm.name.c_str()))));
        std::vector<std::pair<std::string, std::string>> vals;
        if (!sa->storeSessionAudio(prefix.getFullPathName().toStdString(), vals)) continue;
        for (const auto& [param, text] : vals) {
            Parameter* slot = nullptr;
            for (auto& p : cm.properties)
                if (p.name == param) slot = &p;
            if (slot == nullptr) {
                Parameter np;
                np.index = (int) cm.properties.size();
                np.name = param;
                np.type = "soundfile";
                cm.properties.push_back(np);
                slot = &cm.properties.back();
            }
            if (text.empty() && !slot->text.empty()) {
                const juce::File old(juce::String(juce::CharPointer_UTF8(slot->text.c_str())));
                if (old.isAChildOf(dir)) old.deleteFile();
            }
            slot->text = text;
            slot->userEdited = true;
            const auto forDsp = textForDsp(cm.name, text);
            {
                const juce::ScopedLock sl(lock_);
                if (graph_)
                    if (auto* o = graph_->find(cm.name)) {
                        if (auto* pp = o->params.byName(param)) pp->text = forDsp;
                        else {
                            Parameter np;
                            np.index = (int) o->params.all().size();
                            np.name = param;
                            np.text = forDsp;
                            o->params.add(np);
                        }
                    }
            }
            if (graph_)
                if (auto* o = graph_->find(cm.name)) o->onTextChanged(param, forDsp);
        }
    }
}

bool EngineHost::saveFile(const std::string& path, std::string& error) {
    if (!writeDocumentTo(path, error)) return false;
    dirty_ = false;
    for (auto& s : undo_) s.wasDirty = true;
    for (auto& s : redo_) s.wasDirty = true;
    docPath_ = path;
    return true;
}

bool EngineHost::saveCopy(const std::string& path, std::string& error) {
    const bool audible = audioRunning_ && playing_;
    return writeDocumentTo(path, error, !audible);
}

}
