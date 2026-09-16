// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include "core/plugins/BridgedPlugin.h"
#include "core/plugins/PluginHost.h"
#include "core/tuning/TuningProbe.h"
#include "core/tuning/TuningProbeStore.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"

namespace hum {

HostedPlugin* EngineHost::hostedPluginFor(const std::string& name) {
    if (!graph_) return nullptr;
    return dynamic_cast<HostedPlugin*>(graph_->find(name));
}

PluginNode* EngineHost::pluginNodeFor(const std::string& name) {
    if (!graph_) return nullptr;
    return dynamic_cast<PluginNode*>(graph_->find(name));
}

void EngineHost::pumpPluginTunings() {
    if (graph_ && graph_->takeLatencyChanges()) rebuild();
    const juce::ScopedLock sl(lock_);
    if (graph_) graph_->pumpPluginTunings();
}

static long long pluginMtimeMs(const std::string& classRaw) {
    const auto desc = PluginHost::instance().descriptionFor(classRaw);
    if (!desc) return 0;
    const juce::File f(desc->fileOrIdentifier);
    return f.existsAsFile() || f.isDirectory()
               ? f.getLastModificationTime().toMilliseconds() : 0;
}

void EngineHost::pollTuningProbes() {
    if (probeChild_ != nullptr) {
        if (probeChild_->isRunning()) {
            if (juce::Time::getMillisecondCounterHiRes() - probeStartMs_ > 90000) {
                probeChild_->kill();
                probeChild_.reset();
                probeOut_.deleteFile();
            }
            return;
        }
        TuningProbeVerdict v;
        if (parseTuningProbeVerdict(probeOut_.loadFileAsString().trim().toStdString(), v)
            && v != TuningProbeVerdict::Inconclusive) {
            tuningProbeStore::store(probeClass_, pluginMtimeMs(probeClass_), v);
            const juce::ScopedLock sl(lock_);
            if (graph_)
                graph_->setBendVerdict(probeClass_, v != TuningProbeVerdict::SpeaksMts);
        }
        probeOut_.deleteFile();
        probeChild_.reset();
    }

    std::vector<std::string> need;
    {
        const juce::ScopedLock sl(lock_);
        if (!graph_) return;
        need = graph_->pluginsNeedingVerdict();
    }
    for (const auto& cls : need) {
        TuningProbeVerdict v;
        if (tuningProbeStore::lookup(cls, pluginMtimeMs(cls), v)) {
            const juce::ScopedLock sl(lock_);
            if (graph_) graph_->setBendVerdict(cls, v != TuningProbeVerdict::SpeaksMts);
            continue;
        }
        probeClass_ = cls;
        probeOut_ = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("hum-probe-" + juce::Uuid().toString() + ".txt");
        auto child = std::make_unique<juce::ChildProcess>();
        juce::StringArray args;
        args.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                     .getFullPathName());
        args.add("--tuning-probe");
        args.add(probeOut_.getFullPathName());
        args.add(juce::String(cls));
        if (child->start(args, 0)) {
            probeChild_ = std::move(child);
            probeStartMs_ = juce::Time::getMillisecondCounterHiRes();
        }
        break;
    }
}

void EngineHost::pollBridges() {
    if (!graph_) return;
    for (int i = 0; i < graph_->nodeCount(); ++i)
        if (auto* bp = dynamic_cast<BridgedPlugin*>(graph_->organism(i)))
            bp->pollLifecycle();
}

void EngineHost::restartPluginNode(const std::string& name) {
    if (auto* bp = graph_ ? dynamic_cast<BridgedPlugin*>(graph_->find(name)) : nullptr)
        bp->restart();
}

void EngineHost::syncPluginStateToModel() {
    if (!graph_) return;
    for (auto& cm : model_.organisms)
        if (auto* hp = dynamic_cast<PluginNode*>(graph_->find(cm.name)))
            cm.pluginState = hp->getStateBase64();
}

}
