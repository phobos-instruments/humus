#include "gui/EngineHost.h"

#include <cctype>
#include <map>

#include "core/Categories.h"
#include "core/PodModel.h"

namespace hum {

std::string EngineHost::importPatchAsPod(const std::string& path, juce::Point<int> at,
                                         const std::string& scope, std::string& error) {
    PatchDocumentModel doc;
    if (!parsePatchFile(path, doc, error, nullptr)) return {};

    std::string leaf;
    for (const auto ch : juce::File(juce::String(path)).getFileNameWithoutExtension()) {
        if (ch < 128 && (std::isalnum((int) ch) || ch == '_' || ch == '-'))
            leaf += (char) ch;
    }
    if (leaf.empty()) leaf = "Patch";
    const std::string parent = scope.empty() ? std::string() : scope + "/";
    std::string pod = parent + leaf;
    for (int n = 2; model_.byName(pod) || pods::isPod(model_, pod); ++n)
        pod = parent + leaf + "_" + std::to_string(n);

    beginTransaction();
    pushUndo();

    std::map<std::string, juce::Point<int>> viewPos;
    for (const auto& v : doc.views)
        viewPos[v.organismName] = {v.patcherX, v.patcherY};
    auto posOf = [&](const std::string& n, juce::Point<int> fallback) {
        auto it = viewPos.find(n);
        return it != viewPos.end() ? it->second : fallback;
    };

    std::map<std::string, std::string> renamed;
    std::map<std::string, std::array<std::string, 2>> inletFor;
    std::map<std::string, std::vector<std::string>> outletFor;
    std::map<std::string, std::string> midiInletFor;
    std::map<std::string, std::string> midiOutletFor;
    for (const auto& cm : doc.organisms) {
        const auto& cls = cm.displayClass;
        if (isHiddenOrganism(cls)) continue;
        if (cls == "MidiIn") {
            midiInletFor[cm.name] =
                addOrganism(pods::kMidiInletClass, posOf(cm.name, {40, 40}), pod);
            continue;
        }
        if (cls == "MidiOut") {
            midiOutletFor[cm.name] =
                addOrganism(pods::kMidiOutletClass, posOf(cm.name, {40, 420}), pod);
            continue;
        }
        if (cls == "SoundIn") {
            const auto p = posOf(cm.name, {40, 40});
            inletFor[cm.name] = {addOrganism(pods::kInletClass, p, pod),
                                 addOrganism(pods::kInletClass, {p.x + 90, p.y}, pod)};
            continue;
        }
        if (cls == "SoundOut") {
            int outs = 2;
            for (const auto& c : doc.connections)
                if (c.dst == cm.name) outs = std::max(outs, c.dstInlet + 1);
            const auto p = posOf(cm.name, {40, 420});
            auto& list = outletFor[cm.name];
            for (int k = 0; k < outs; ++k)
                list.push_back(addOrganism(pods::kOutletClass, {p.x + k * 90, p.y}, pod));
            continue;
        }
        OrganismModel copy = cm;
        copy.name = pod + "/" + cm.name;
        renamed[cm.name] = copy.name;
        positions_[copy.name] = posOf(cm.name, {60, 120});
        model_.organisms.push_back(std::move(copy));
    }

    for (const auto& c : doc.connections) {
        std::string src;
        int so = c.srcOutlet;
        if (auto it = inletFor.find(c.src); it != inletFor.end()) {
            src = it->second[(size_t) juce::jlimit(0, 1, c.srcOutlet)];
            so = 0;
        } else if (auto r = renamed.find(c.src); r != renamed.end()) {
            src = r->second;
        } else {
            continue;
        }
        std::string dst;
        int di = c.dstInlet;
        if (auto it = outletFor.find(c.dst); it != outletFor.end()) {
            if (c.dstInlet < 0 || c.dstInlet >= (int) it->second.size()) continue;
            dst = it->second[(size_t) c.dstInlet];
            di = 0;
        } else if (auto r = renamed.find(c.dst); r != renamed.end()) {
            dst = r->second;
        } else {
            continue;
        }
        model_.connections.push_back({src, so, dst, di});
    }
    for (const auto& c : doc.midiConnections) {
        std::string src;
        int so = c.srcOutlet;
        if (auto it = midiInletFor.find(c.src); it != midiInletFor.end()) { src = it->second; so = 0; }
        else if (auto r = renamed.find(c.src); r != renamed.end()) src = r->second;
        else continue;
        std::string dst;
        int di = c.dstInlet;
        if (auto it = midiOutletFor.find(c.dst); it != midiOutletFor.end()) { dst = it->second; di = 0; }
        else if (auto r = renamed.find(c.dst); r != renamed.end()) dst = r->second;
        else continue;
        model_.midiConnections.push_back({src, so, dst, di, c.midiChannel});
    }
    for (const auto& c : doc.videoConnections) {
        const auto rs = renamed.find(c.src), rd = renamed.find(c.dst);
        if (rs == renamed.end() || rd == renamed.end()) continue;
        model_.videoConnections.push_back({rs->second, c.srcOutlet, rd->second, c.dstInlet});
    }

    positions_[pod] = at;
    dirty_ = true;
    ++changeStamp_;
    midi().syncMapFromModel();
    osc().syncMapFromModel();
    mod().syncMapFromModel();
    rebuild();
    endTransaction();
    return pod;
}

}
