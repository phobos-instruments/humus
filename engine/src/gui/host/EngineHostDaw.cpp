// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <string>
#include <utility>
#include <vector>

namespace hum {

void EngineHost::serviceTick() {
    if (daw_ != nullptr) daw_->drainSeen([this](const juce::MidiMessage& m) { noteMidiControl(m, 1); });
    pollMidiControl();
    pollBridges();
    pumpPluginTunings();
    pollTuningProbes();
    serviceCountIn();
    pumpOscOut();
    files().pollRecorders();
}

void EngineHost::addDawControlTarget(int slot, const std::string& organism, const std::string& param) {
    if (daw_ == nullptr) return;
    bool first = false;
    {
        const AudioLock::Scope sl(lock_);
        first = daw_->add(slot, {organism, param}, graph_.get(), model_);
    }
    if (first) daw_->noteMappedAt(slot, liveParamValue(organism, param));
}

void EngineHost::setDawControlTargets(int slot, const std::vector<DawMacroTarget>& targets) {
    if (daw_ == nullptr) return;
    {
        const AudioLock::Scope sl(lock_);
        daw_->setTargets(slot, targets, graph_.get(), model_);
    }
    const auto now = daw_->targets(slot);
    if (!now.empty()) daw_->noteMappedAt(slot, liveParamValue(now.front().organism, now.front().param));
}

void EngineHost::removeDawControlTarget(const std::string& organism, const std::string& param) {
    if (daw_ == nullptr) return;
    const AudioLock::Scope sl(lock_);
    daw_->remove(organism, param);
}

void EngineHost::setDawControlRange(const std::string& organism, const std::string& param, double from, double to) {
    if (daw_ == nullptr) return;
    const AudioLock::Scope sl(lock_);
    daw_->setRange(organism, param, from, to);
}

}
