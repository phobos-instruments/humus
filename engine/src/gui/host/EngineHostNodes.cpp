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
#include "hum/caps/Audio.h"
#include "hum/caps/Files.h"
#include "hum/caps/Video.h"

namespace hum {

int EngineHost::countInlets(const PatchDocumentModel& m, const std::string& name) {
    int n = 0;
    for (auto& c : m.connections) if (c.dst == name) n = std::max(n, c.dstInlet + 1);
    return n;
}

int EngineHost::countOutlets(const PatchDocumentModel& m, const std::string& name) {
    int n = 0;
    for (auto& c : m.connections) if (c.src == name) n = std::max(n, c.srcOutlet + 1);
    return n;
}

const Organism* EngineHost::liveNode(const std::string& name) {
    if (!graph_) return nullptr;
    if (auto* c = graph_->find(name)) return c;
    if (!rebuildDue_) return nullptr;
    rebuild();
    return graph_ ? graph_->find(name) : nullptr;
}

int EngineHost::inletsOf(const std::string& name) {
    if (auto* c = liveNode(name)) return std::max(c->numAudioInputs(), countInlets(model_, name));
    return countInlets(model_, name);
}

int EngineHost::outletsOf(const std::string& name) {
    if (auto* c = liveNode(name)) return std::max(c->numAudioOutputs(), countOutlets(model_, name));
    return countOutlets(model_, name);
}

void EngineHost::nodeActivity(const std::string& name, float& audioPeak, float& midiCount) {
    audioPeak = 0.0f;
    midiCount = 0.0f;
    if (!graph_) return;
    const int i = graph_->indexOf(name);
    if (i < 0) return;
    audioPeak = graph_->nodeAudioActivity(i);
    midiCount = graph_->nodeMidiActivity(i);
}

bool EngineHost::armNodeCapture(int samples) {
    const juce::ScopedLock sl(lock_);
    if (!graph_) return false;
    graph_->armCapture(samples);
    return true;
}

void EngineHost::disarmNodeCapture() {
    const juce::ScopedLock sl(lock_);
    if (graph_) graph_->disarmCapture();
}

int EngineHost::copyNodeCapture(const std::string& name, std::vector<float>& out) {
    const juce::ScopedLock sl(lock_);
    out.clear();
    if (!graph_) return 0;
    const int node = graph_->indexOf(name);
    const int n = graph_->captureFill(node);
    const float* d = graph_->captureData(node);
    if (d == nullptr || n <= 0) return 0;
    out.assign(d, d + n);
    return n;
}

int EngineHost::nodeMeter(const std::string& name, float* levels, int maxCh) {
    auto* src = dynamic_cast<LevelMeterSource*>(liveOrganism(name));
    if (src == nullptr) return 0;
    const int n = std::min(src->meterChannels(), maxCh);
    const bool live = audioAlive() && !bypassed(name);
    for (int c = 0; c < n; ++c) levels[c] = live ? src->meterLevel(c) : 0.0f;
    return n;
}

std::string EngineHost::masterOutputName() {
    if (graph_)
        for (int i = 0; i < graph_->nodeCount(); ++i)
            if (auto* c = graph_->organism(i))
                if (dynamic_cast<MasterTap*>(c)) return c->name();
    return {};
}

std::vector<std::string> EngineHost::arrangeableNodes() {
    std::vector<std::string> out;
    if (graph_ == nullptr) return out;
    for (int i = 0; i < graph_->nodeCount(); ++i) {
        auto* c = graph_->organism(i);
        if (c == nullptr) continue;
        if (nodeIsInternal(c->name())) continue;
        if (dynamic_cast<ClipRecorder*>(c) != nullptr || nodeIsNoteTrack(c->name())
            || dynamic_cast<VideoTimelineSource*>(c) != nullptr)
            out.push_back(c->name());
    }
    return out;
}

bool EngineHost::nodeRecordsAudio(const std::string& name) {
    if (!graph_) return false;
    return dynamic_cast<ClipRecorder*>(graph_->find(name)) != nullptr;
}

bool EngineHost::nodeArrangesVideo(const std::string& name) {
    if (!graph_) return false;
    return dynamic_cast<VideoTimelineSource*>(graph_->find(name)) != nullptr;
}

bool EngineHost::nodeRecordsVideo(const std::string& name) {
    if (!graph_) return false;
    auto* live = graph_->find(name);
    auto* vn = dynamic_cast<VideoNode*>(live);
    return dynamic_cast<VideoTimelineSource*>(live) != nullptr && vn != nullptr
           && vn->numVideoInputs() > 0;
}

}
