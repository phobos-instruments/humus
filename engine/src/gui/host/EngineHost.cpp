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

EngineHost::EngineHost() {
    for (int i = 0; i < kMidiSourceCount; ++i) { midiState_.ccValue[i].store(-1); midiState_.ccLastApplied[i] = -1; }
    for (int i = 0; i < kMaxDeviceChannels; ++i) { inMap_[i] = i; outMap_[i] = i; }
    seedDeviceFormatFromSettings();
    midiDeviceListConn_ = juce::MidiDeviceListConnection::make([this] {
        if (midiState_.enabled) midi().refreshDevices();
    });
    if (AppSettings::instance().getInt("osc.enabled", 0) != 0) osc_.setEnabled(true);
    osc_.applySerialSettings();
    if (const auto lib = AppSettings::instance().getString("library.path"); lib.isNotEmpty())
        library::configuredRoot() = lib.toStdString();
    latch_ = AppSettings::instance().getInt("automation.latch", 0) != 0;
    registerBuiltinOrganisms();
    newDocument();
}

EngineHost::~EngineHost() {
    *hostAlive_ = false;
    midi().setEnabled(false);
    stopAudio();
}

void EngineHost::requestRebuild() {
    if (inTxn_) { rebuildDue_ = true; return; }
    rebuild();
}

void EngineHost::rebuild() {
    rebuildDue_ = false;
    ++rebuilds_;
    ++laneStamp_;
    syncMidiTrackTargets();

    outputGain_.store((float) juce::jlimit(0.0, 1.0, model_.masterLevel));
    limiterOn_.store(model_.masterLimiter);
    const bool canAdopt =
        graph_ != nullptr && sampleRate_ == preparedSampleRate_ && block_ == preparedBlock_;
    if (onBeforeRebuild)
        onBeforeRebuild([&](const std::string& name) {
            if (!canAdopt) return false;
            const auto* cm = model_.byName(name);
            if (cm == nullptr) return false;
            const Organism* live = graph_->find(name);
            return live != nullptr && !live->matchToken().empty()
                   && live->matchToken() == cm->classRaw;
        });
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        midiState_.targets.clear();
        liveMidiIns_.clear();
        midiMonitors_.clear();
        namedLiveIns_.clear();
        midiDrains_.clear();
    }

    std::function<const Organism*(const OrganismModel&)> reuse;
    if (canAdopt)
        reuse = [this](const OrganismModel& cm) -> const Organism* {
            const Organism* live = graph_->find(cm.name);
            return live != nullptr && !live->matchToken().empty()
                        && live->matchToken() == cm.classRaw ? live : nullptr;
        };
    auto g = std::make_unique<AudioGraph>();
    std::string err;
    if (!buildGraph(model_, *g, err, reuse)) {
        juce::Logger::writeToLog("rebuild failed, the previous graph keeps running: " + juce::String(err));
        if (onBuildFailed) onBuildFailed(err);
        return;
    }
    if (canAdopt) g->prepare(sampleRate_, block_, model_.clock.tempo, *graph_);
    else g->prepare(sampleRate_, block_, model_.clock.tempo);

    const bool audible = audioRunning_ && fadeGainPub_.load() > 0.0005f;
    if (audible) {
        fadeTarget_.store(0.0f);
        for (int i = 0; i < 60 && fadeGainPub_.load() > 0.0005f; ++i)
            juce::Thread::sleep(1);
    }

    std::unique_ptr<AudioGraph> retired;
    {
        const juce::ScopedLock sl(lock_);
        const int64_t pos = graph_ ? graph_->transport().samplePosition() : 0;
        const double beats = graph_ ? graph_->transport().beats() : 0.0;
        g->transport().setPlaying(playing_);
        g->transport().restorePosition(pos, beats);
        g->setExternalTempoMaster(linkEnabled_.load(std::memory_order_relaxed));
        if (canAdopt) g->adoptMatchingNodes(*graph_);
        std::vector<std::pair<MasterTap*, int>> taps;
        std::vector<std::pair<HardwareOut*, int>> auxes;
        for (int i = 0; i < g->nodeCount(); ++i) {
            if (auto* s = dynamic_cast<MasterTap*>(g->organism(i))) taps.emplace_back(s, i);
            else if (auto* a = dynamic_cast<HardwareOut*>(g->organism(i))) auxes.emplace_back(a, i);
        }
        retired = std::move(graph_);
        graph_ = std::move(g);
        graph_->setModRoutes([&] {
            std::vector<ModRoute> routes;
            for (const auto& e : mod_.map().entries()) {
                const auto* target = model_.byName(e.organism);
                if (target == nullptr || !routeLivesInEngine(*target, e.param)) continue;
                ModRoute r;
                r.dstParam = e.param;
                if (fillModRoute(r, *target, model_.byName(e.source), e.value, e.min, e.max, e.shape,
                                 [&](const std::string& n) { return graph_->indexOf(n); }))
                    routes.push_back(std::move(r));
            }
            return routes;
        }());
        soundOut_ = taps.empty() ? nullptr : taps.front().first;
        masterTaps_ = std::move(taps);
        auxOuts_ = std::move(auxes);
        recorders_.clear();
        for (auto& c : model_.organisms)
            if (auto* fr = dynamic_cast<Recorder*>(graph_->find(c.name)))
                recorders_.emplace_back(c.name, fr);
        {
            const juce::ScopedLock ml(midiState_.targetsLock);
            for (auto& c : model_.organisms) {
                auto* node = graph_->find(c.name);
                if (!node) continue;
                if (auto* hp = dynamic_cast<PluginNode*>(node))
                    midiState_.targets.push_back({hp, c.midiReceiveMode, c.midiReceiveChannel});
                if (auto* in = dynamic_cast<LiveMidiIn*>(node)) {
                    if (in->monitorsAllPorts()) midiMonitors_.push_back(in);
                    else if (in->liveMidiPort() >= 0) liveMidiIns_.push_back(in);
                    namedLiveIns_.push_back({c.name, in});
                }
                if (auto* out = dynamic_cast<PendingMidiOut*>(node))
                    midiDrains_.push_back(out);
            }
        }
        publishClock();
    }
    retired.reset();

    preparedSampleRate_ = sampleRate_;
    preparedBlock_ = block_;
    if (modelSwapped_) {
        modelSwapped_ = false;
        std::vector<Organism*> clipNodes;
        {
            const juce::ScopedLock sl(lock_);
            for (const auto& cm : model_.organisms)
                if (auto* live = graph_ ? graph_->find(cm.name) : nullptr) {
                    live->loadFrom(OrganismState{cm.properties, cm.pattern});
                    clipNodes.push_back(live);
                }
        }
        for (auto* live : clipNodes) {
            if (auto* cr = dynamic_cast<ClipRecorder*>(live)) cr->ensureClipsLoaded();
            if (auto* sa = dynamic_cast<SessionAudio*>(live)) sa->loadSessionAudio();
        }
    }
    syncAutomation();
    for (const auto& cm : model_.organisms) syncNodeTrack(cm.name);
    applySolo();
    for (const auto& cm : model_.organisms)
        for (const auto& p : cm.properties) {
            if (p.name.rfind("File", 0) != 0) continue;
            if (p.text.rfind(kAssetScheme, 0) != 0
                && p.text.rfind(banks::kLegacyPrefix, 0) != 0) continue;
            if (auto* fl = dynamic_cast<FileLoader*>(graph_ ? graph_->find(cm.name) : nullptr))
                fl->loadFromFile(banks::resolve(p.text, cm.displayClass));
        }
    if (audioRunning_) requestFadeIn();
    scheduleAuxChannelCheck();
}

void EngineHost::discardLiveGraph() {
    if (onBeforeRebuild) onBeforeRebuild([](const std::string&) { return false; });
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        midiState_.targets.clear();
        liveMidiIns_.clear();
        midiMonitors_.clear();
        namedLiveIns_.clear();
        midiDrains_.clear();
    }
    std::unique_ptr<AudioGraph> retired;
    {
        const juce::ScopedLock sl(lock_);
        retired = std::move(graph_);
        soundOut_ = nullptr;
        masterTaps_.clear();
        auxOuts_.clear();
        recorders_.clear();
        publishClock();
    }
    retired.reset();
}

}

