// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiControl.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/host/ModHost.h"
#include "gui/host/OscHost.h"
#include "gui/editor/QuickMapWindow.h"
#include "io/PatchDocument.h"

namespace hum {

class MidiLearner : private juce::Timer {
public:
    static MidiLearner& instance() { static MidiLearner l; return l; }

    std::function<void(const juce::String&)> onStatus;

    std::function<void()> onCaptured;

    void arm(BrickHost& host, std::string organism, std::string param, double min, double max) {
        onCaptured = nullptr;
        host_ = &host; organism_ = std::move(organism); param_ = std::move(param);
        min_ = min; max_ = max;
        pendingNote_ = -1;
        pendingHeld_.clear();
        armedAt_ = juce::Time::getMillisecondCounter();
        host.midi().clearLastCC();
        if (onStatus)
            onStatus("MIDI Learn: move a controller or play a note for " + juce::String(param_)
                     + " (waiting " + juce::String((int) (kTimeoutMs / 1000)) + "s)");
        window_ = std::make_unique<QuickMapWindow>(
            "Quick Map MIDI Control",
            "Move a controller or play a note on your MIDI device\nto control\n"
                + juce::String(organism_) + " / " + juce::String(param_)
                + "\n\nHold a button while you do it for a shift combo.",
            [] { MidiLearner::instance().cancel(); });
        window_->setCountdown((int) (kTimeoutMs / 1000));
        startTimerHz(20);
    }
    bool armed() const { return host_ != nullptr; }

    void cancel() {
        if (host_ != nullptr && onStatus) onStatus(tr("organism-editor.midi-learn-cancelled", "MIDI Learn cancelled"));
        disarm();
    }

private:
    static constexpr juce::uint32 kTimeoutMs = 15000;

    void disarm() {
        onCaptured = nullptr;
        host_ = nullptr;
        stopTimer();
        window_.reset();
    }

    void timerCallback() override {
        if (!host_) { disarm(); return; }
        const auto elapsed = juce::Time::getMillisecondCounter() - armedAt_;
        if (elapsed > kTimeoutMs) {
            if (onStatus) onStatus(tr("organism-editor.midi-learn-timed-out-nothing", "MIDI Learn timed out (nothing received)"));
            disarm();
            return;
        }
        if (window_) window_->setCountdown((int) ((kTimeoutMs - elapsed) / 1000) + 1);
        const int cc = host_->midi().lastCC();
        if (cc >= 0) {
            host_->midi().clearLastCC();
            auto held = host_->midi().heldNotes(cc);
            if (isNoteSource(cc) && host_->midi().sourceValue(cc) > kHeldThreshold) {
                pendingNote_ = cc;
                pendingHeld_ = std::move(held);
                if (window_)
                    window_->setMessage("Holding " + juce::String(midiSourceLabel(cc))
                                        + ": release it to map it alone,\nor move another control"
                                          " to make it the shift.");
                return;
            }
            capture(MidiSource(cc, std::move(held)));
            return;
        }
        if (pendingNote_ >= 0 && host_->midi().sourceValue(pendingNote_) <= kHeldThreshold)
            capture(MidiSource(pendingNote_, std::move(pendingHeld_)));
    }

    void capture(const MidiSource& src) {
        auto* host = host_;
        const auto org = organism_, prm = param_;
        const double lo = min_, hi = max_;
        const auto others = host->midi().map().usersOf(src, org, prm);
        auto status = onStatus;
        auto next = std::move(onCaptured);
        disarm();
        auto commit = [host, src, org, prm, lo, hi, status, next](bool steal) {
            const auto stolen = host->midi().mapCC(src, org, prm, lo, hi, steal);
            if (status)
                status("MIDI: mapped " + juce::String(midiSourceLabel(src)) + " to "
                       + juce::String(org) + " / " + juce::String(prm)
                       + (stolen.isNotEmpty() ? " (reassigned from " + stolen + ")" : ""));
            if (next) next();
        };
        if (others.empty()) { commit(true); return; }
        mapconflict::ask("MIDI", juce::String(midiSourceLabel(src)),
                         juce::String(org) + " / " + juce::String(prm), others, commit);
    }

    BrickHost* host_ = nullptr;
    std::string organism_, param_;
    double min_ = 0.0, max_ = 1.0;
    int pendingNote_ = -1;
    std::vector<int> pendingHeld_;
    juce::uint32 armedAt_ = 0;
    std::unique_ptr<QuickMapWindow> window_;
};

class OscLearner : private juce::Timer {
public:
    static OscLearner& instance() { static OscLearner l; return l; }

    std::function<void(const juce::String&)> onStatus;

    void arm(BrickHost& host, std::string organism, std::string param, double min, double max) {
        host_ = &host; organism_ = std::move(organism); param_ = std::move(param);
        min_ = min; max_ = max;
        armedAt_ = juce::Time::getMillisecondCounter();
        host.osc().clearLastAddress();
        if (onStatus)
            onStatus("OSC Learn: move an OSC control for " + juce::String(param_)
                     + " (port " + juce::String(host.osc().port()) + ", waiting "
                     + juce::String((int) (kTimeoutMs / 1000)) + "s)");
        window_ = std::make_unique<QuickMapWindow>(
            "Quick Map OSC Control",
            "Move an OSC control (port " + juce::String(host.osc().port())
                + ") to control\n" + juce::String(organism_) + " / " + juce::String(param_),
            [] { OscLearner::instance().cancel(); });
        window_->setCountdown((int) (kTimeoutMs / 1000));
        startTimerHz(20);
    }

    void cancel() {
        if (host_ != nullptr && onStatus) onStatus(tr("organism-editor.osc-learn-cancelled", "OSC Learn cancelled"));
        disarm();
    }

private:
    static constexpr juce::uint32 kTimeoutMs = 15000;

    void disarm() {
        host_ = nullptr;
        stopTimer();
        window_.reset();
    }

    void timerCallback() override {
        if (!host_) { disarm(); return; }
        const auto elapsed = juce::Time::getMillisecondCounter() - armedAt_;
        if (elapsed > kTimeoutMs) {
            if (onStatus) onStatus(tr("organism-editor.osc-learn-timed-out-nothing", "OSC Learn timed out (nothing received)"));
            disarm();
            return;
        }
        if (window_) window_->setCountdown((int) ((kTimeoutMs - elapsed) / 1000) + 1);
        const auto addr = host_->osc().lastAddress();
        if (addr.isEmpty()) return;
        auto* host = host_;
        const auto org = organism_, prm = param_;
        const double lo = min_, hi = max_;
        const auto address = addr.toStdString();
        const auto others = host->osc().map().usersOf(address, org, prm);
        auto status = onStatus;
        disarm();
        auto commit = [host, address, addr, org, prm, lo, hi, status](bool steal) {
            const auto stolen = host->osc().mapAddress(address, org, prm, lo, hi, steal);
            if (status)
                status("OSC: mapped " + addr + " to " + juce::String(org) + " / " + juce::String(prm)
                       + (stolen.isNotEmpty() ? " (reassigned from " + stolen + ")" : ""));
        };
        if (others.empty()) { commit(true); return; }
        mapconflict::ask("OSC", addr, juce::String(org) + " / " + juce::String(prm), others, commit);
    }
    BrickHost* host_ = nullptr;
    std::string organism_, param_;
    double min_ = 0.0, max_ = 1.0;
    juce::uint32 armedAt_ = 0;
    std::unique_ptr<QuickMapWindow> window_;
};

}
