// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include "gui/host/CordSplice.h"
#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/graph/CordReclaim.h"
#include "core/packs/PackManifest.h"
#include "core/plugins/PluginHost.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PodModel.h"
#include "core/packs/StripFamily.h"
#include "core/params/Randomize.h"
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "core/params/ParamSchema.h"
#include "core/packs/Roles.h"
#include "gui/properties/PresetLibrary.h"
#include "hum/Registry.h"

namespace hum {

std::string EngineHost::insertBefore(const std::string& name, const std::string& newClass) {
    if (!model_.byName(name)) return {};
    const int nameIns = inletsOf(name);

    std::vector<ConnectionModel> feeders;
    for (auto& c : model_.connections) if (c.dst == name) feeders.push_back(c);

    const std::string canon = canonicalClass(newClass);
    const auto* cm = PackRegistry::instance().classManifest(canon);
    const auto fam = strips::parse(canon);
    std::vector<std::string> sources;
    for (auto& f : feeders)
        if (std::find(sources.begin(), sources.end(), f.src) == sources.end())
            sources.push_back(f.src);

    if (cm != nullptr && cm->strips && fam.count > 0 && sources.size() >= 2) {
        const int channels = strips::sizeFor(fam, (int) sources.size());
        const std::string sized = strips::sized(fam, channels);
        const int width = std::max(1, classIO(sized).first / std::max(1, channels));

        beginTransaction();
        const auto pos = position(name);
        const auto nn = addOrganism(sized, {pos.x, juce::jmax(0, pos.y - 70)},
                                    pods::parentOf(name));
        for (int s = 0; s < (int) sources.size(); ++s) {
            const int ch = std::min(s, channels - 1);
            std::vector<int> outs;
            for (auto& f : feeders)
                if (f.src == sources[(size_t) s]
                    && std::find(outs.begin(), outs.end(), f.srcOutlet) == outs.end())
                    outs.push_back(f.srcOutlet);
            for (auto& f : feeders)
                if (f.src == sources[(size_t) s])
                    removeConnection(f.src, f.srcOutlet, name, f.dstInlet);
            for (int ci = 0; ci < width; ++ci) {
                const int o = outs.empty() ? 0 : outs[(size_t) std::min(ci, (int) outs.size() - 1)];
                connect(sources[(size_t) s], o, nn, ch * width + ci);
            }
        }
        for (int k = 0; k < std::min(width, nameIns); ++k) connect(nn, k, name, k);
        endTransaction();
        return nn;
    }

    const int midiIns = midiInletsOf(name);
    const int videoIns = videoInletsOf(name);
    beginTransaction();
    const auto pos = position(name);
    const auto nn = addOrganism(newClass, {pos.x, juce::jmax(0, pos.y - 70)},
                                pods::parentOf(name));
    auto cutAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        removeConnection(a, b, c, d);
    };
    auto joinAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        connect(a, b, c, d);
    };
    auto cutMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        removeMidiConnection(a, b, c, d);
    };
    auto joinMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        connectMidi(a, b, c, d);
    };
    auto cutVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        removeVideoConnection(a, b, c, d);
    };
    auto joinVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        connectVideo(a, b, c, d);
    };
    spliceBefore(model_.connections, name, nn, classIO(newClass), nameIns, cutAudio, joinAudio);
    spliceBefore(model_.midiConnections, name, nn, classMidiIO(newClass), midiIns, cutMidi,
                 joinMidi);
    spliceBefore(model_.videoConnections, name, nn, classVideoIO(newClass), videoIns, cutVideo,
                 joinVideo);
    endTransaction();
    return nn;
}

std::string EngineHost::insertAfter(const std::string& name, const std::string& newClass) {
    if (!model_.byName(name)) return {};
    const int nameOuts = outletsOf(name);
    const int midiOuts = midiOutletsOf(name);
    const int videoOuts = videoOutletsOf(name);
    beginTransaction();
    const auto pos = position(name);
    const auto nn = addOrganism(newClass, {pos.x, pos.y + 70},
                                pods::parentOf(name));
    auto cutAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        removeConnection(a, b, c, d);
    };
    auto joinAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        connect(a, b, c, d);
    };
    auto cutMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        removeMidiConnection(a, b, c, d);
    };
    auto joinMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        connectMidi(a, b, c, d);
    };
    auto cutVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        removeVideoConnection(a, b, c, d);
    };
    auto joinVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        connectVideo(a, b, c, d);
    };
    spliceAfter(model_.connections, name, nn, classIO(newClass), nameOuts, cutAudio, joinAudio);
    spliceAfter(model_.midiConnections, name, nn, classMidiIO(newClass), midiOuts, cutMidi,
                joinMidi);
    spliceAfter(model_.videoConnections, name, nn, classVideoIO(newClass), videoOuts, cutVideo,
                joinVideo);
    endTransaction();
    return nn;
}

std::string EngineHost::insertRecorderFor(const std::string& node) {
    if (!model_.byName(node)) return {};
    std::string tk;
    const auto track = classWithRole(role::kAudioTrack);
    if (outletsOf(node) > 0)      tk = insertAfter(node, track);
    else if (inletsOf(node) > 0)  tk = insertBefore(node, track);
    if (!tk.empty()) setParam(tk, "Record", 1.0);
    return tk;
}

std::string EngineHost::insertOnCord(const std::string& src, int outlet, const std::string& dst,
                                     int inlet, const std::string& newClass, juce::Point<int> at,
                                     const std::string& podScope) {
    beginTransaction();
    const auto nn = addOrganism(newClass, at, podScope);
    const auto io = classIO(newClass);
    removeConnection(src, outlet, dst, inlet);
    if (io.first  > 0) connect(src, outlet, nn, 0);
    if (io.second > 0) connect(nn, 0, dst, inlet);
    endTransaction();
    return nn;
}

}
