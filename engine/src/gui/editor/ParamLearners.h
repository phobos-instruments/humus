// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/ControlGuess.h"
#include "core/midi/MidiControl.h"
#include "gui/editor/ControlDefaults.h"
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

    void arm(BrickHost& host, std::string organism, std::string param, double min, double max,
             bool showCard = true) {
        onCaptured = nullptr;
        host_ = &host; organism_ = std::move(organism); param_ = std::move(param);
        min_ = min; max_ = max;
        pendingNote_ = -1;
        pendingPort_ = kAnyMidiPort;
        pendingHeld_.clear();
        listening_ = -1;
        armedAt_ = juce::Time::getMillisecondCounter();
        host.midi().clearLastCC();
        if (onStatus)
            onStatus("MIDI Learn: move a controller or play a note for " + juce::String(param_)
                     + " (waiting " + juce::String((int) (kTimeoutMs / 1000)) + "s)");
        if (showCard) window_ = std::make_unique<QuickMapWindow>(
            "Quick Map MIDI Control",
            tr("organism-editor.learn-card-move", "Move a knob, fader, encoder or button on your MIDI device\nto control")
                + "\n" + juce::String(organism_) + " / " + juce::String(param_) + "\n\n"
                + tr("organism-editor.learn-card-combo", "For a combo, hold a pad or button first,\nthen move the control it unlocks."),
            [] { MidiLearner::instance().cancel(); });
        if (window_) window_->setCountdown((int) (kTimeoutMs / 1000));
        startTimerHz(20);
    }
    bool armed() const { return host_ != nullptr; }
    int secondsLeft() const {
        if (host_ == nullptr) return 0;
        const auto elapsed = juce::Time::getMillisecondCounter() - armedAt_;
        return elapsed >= kTimeoutMs ? 0 : (int) ((kTimeoutMs - elapsed) / 1000) + 1;
    }
    const std::string& armedOrganism() const { return organism_; }
    const std::string& armedParam() const { return param_; }

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
        if (listening_ >= 0) {
            if (juce::Time::getMillisecondCounterHiRes() - listenFrom_ < kListenMs) return;
            const auto guess = guessControl(listening_, host_->midi().recentValues(listening_, listenFrom_));
            capture(MidiSource(listening_, std::move(pendingHeld_), pendingPort_), guess);
            return;
        }
        const int cc = host_->midi().lastCC();
        if (cc >= 0) {
            host_->midi().clearLastCC();
            auto held = host_->midi().heldNotes(cc);
            if (isNoteSource(cc) && host_->midi().sourceValue(cc) > kHeldThreshold) {
                pendingNote_ = cc;
                pendingHeld_ = std::move(held);
                pendingPort_ = host_->midi().lastPort();
                if (window_)
                    window_->setMessage(tr("organism-editor.learn-holding", "Holding") + " "
                                        + juce::String(midiSourceLabel(cc)) + "\n"
                                        + tr("organism-editor.learn-holding-choice",
                                             "Release it to map the pad itself,\nor move another "
                                             "control to use this pad as its combo key."));
                return;
            }
            if (!isCcSource(cc)) {
                capture(MidiSource(cc, std::move(held), host_->midi().lastPort()), guessControl(cc, {}));
                return;
            }
            listening_ = cc;
            listenFrom_ = juce::Time::getMillisecondCounterHiRes() - kEchoMs;
            pendingHeld_ = std::move(held);
            pendingPort_ = host_->midi().lastPort();
            if (window_)
                window_->setMessage(tr("organism-editor.learn-listening", "Got") + " "
                                    + juce::String(midiSourceLabel(cc)) + "\n"
                                    + tr("organism-editor.learn-listening-more",
                                         "keep moving it for a moment\nso Humus can tell what kind of control it is"));
            return;
        }
        if (pendingNote_ >= 0 && host_->midi().sourceValue(pendingNote_) <= kHeldThreshold)
            capture(MidiSource(pendingNote_, std::move(pendingHeld_), pendingPort_),
                    guessControl(pendingNote_, {}));
    }

    void capture(const MidiSource& src, const ControlGuess& guess) {
        auto* host = host_;
        const auto org = organism_, prm = param_;
        const double lo = min_, hi = max_;
        const auto others = host->midi().map().usersOf(src, org, prm);
        auto status = onStatus;
        auto next = std::move(onCaptured);
        disarm();
        auto commit = [host, src, org, prm, lo, hi, status, next, guess](bool steal) {
            const auto stolen = host->midi().mapCC(src, org, prm, lo, hi, steal);
            const auto shape = learnedShape(*host, org, prm, guess);
            host->midi().setShape(src, org, prm, shape);
            if (status)
                status("MIDI: mapped " + juce::String(midiSourceLabel(src)) + " ("
                       + juce::String(modeLabel(shape.type)) + ") to "
                       + juce::String(org) + " / " + juce::String(prm)
                       + (!stolen.empty() ? " (reassigned from " + juce::String::fromUTF8(stolen.c_str()) + ")" : ""));
            if (next) next();
        };
        if (others.empty()) { commit(true); return; }
        mapconflict::ask("MIDI", juce::String(midiSourceLabel(src)),
                         juce::String(org) + " / " + juce::String(prm), others, commit);
    }

    BrickHost* host_ = nullptr;
    std::string organism_, param_;
    double min_ = 0.0, max_ = 1.0;
    static constexpr double kListenMs = 450.0;
    static constexpr double kEchoMs = 120.0;
    int pendingNote_ = -1, pendingPort_ = kAnyMidiPort;
    int listening_ = -1;
    double listenFrom_ = 0.0;
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

    bool armed() const { return host_ != nullptr; }

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
        const auto address = host_->osc().lastAddress();
        if (address.empty()) return;
        const auto addr = juce::String::fromUTF8(address.c_str());
        auto* host = host_;
        const auto org = organism_, prm = param_;
        const double lo = min_, hi = max_;
        const auto others = host->osc().map().usersOf(address, org, prm);
        auto status = onStatus;
        disarm();
        auto commit = [host, address, addr, org, prm, lo, hi, status](bool steal) {
            const auto stolen = host->osc().mapAddress(address, org, prm, lo, hi, steal);
            if (status)
                status("OSC: mapped " + addr + " to " + juce::String(org) + " / " + juce::String(prm)
                       + (!stolen.empty() ? " (reassigned from " + juce::String::fromUTF8(stolen.c_str()) + ")" : ""));
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
