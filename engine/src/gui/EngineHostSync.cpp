#include <cmath>

#include "gui/AppSettings.h"
#include "gui/EngineHost.h"

namespace hum {

struct EngineHost::ClockTimer : juce::HighResolutionTimer {
    explicit ClockTimer(EngineHost& h) : host(h) {}
    ~ClockTimer() override { stopTimer(); }

    void hiResTimerCallback() override {
        if (!host.playing_.load(std::memory_order_relaxed)) { gen.reset(); return; }
        const int n = gen.ticksFor(host.positionBeats());
        const juce::ScopedLock sl(lock);
        if (out == nullptr) return;
        for (int i = 0; i < n; ++i)
            out->sendMessageNow(juce::MidiMessage::midiClock());
    }

    EngineHost& host;
    MidiClockGenerator gen;
    juce::CriticalSection lock;
    juce::MidiOutput* out = nullptr;
};

void EngineHost::ClockTimerDeleter::operator()(ClockTimer* t) const { delete t; }

void EngineHost::setMidiSyncMode(int mode) {
    syncMode_.store(mode, std::memory_order_relaxed);
    AppSettings::instance().set("midi.sync",
                                juce::String(mode == kSyncGenerate ? "generate"
                                             : mode == kSyncChase ? "chase" : "off"));
    if (mode != kSyncOff && !midi().enabled()) midi().setEnabled(true);

    if (mode == kSyncGenerate) {
        if (!clockTimer_) clockTimer_.reset(new ClockTimer(*this));
        syncSetOutput(midiOuts_[0].get());
        clockTimer_->startTimer(2);
    } else if (clockTimer_) {
        clockTimer_->stopTimer();
    }
    chaseCmd_.store(0);
    chaseSpp_.store(-1.0);
    chaseBpm_.store(0.0);
}

void EngineHost::applyMidiSyncFromSettings() {
    const auto s = AppSettings::instance().getString("midi.sync", "off");
    const int mode = s == "generate" ? kSyncGenerate : s == "chase" ? kSyncChase : kSyncOff;
    if (mode != kSyncOff) setMidiSyncMode(mode);
}

void EngineHost::syncSetOutput(juce::MidiOutput* out) {
    if (!clockTimer_) return;
    const juce::ScopedLock sl(clockTimer_->lock);
    clockTimer_->out = out;
}

void EngineHost::syncTransportEvent(bool starting) {
    if (syncMode_.load(std::memory_order_relaxed) != kSyncGenerate || midiOuts_[0] == nullptr)
        return;
    if (!starting) {
        midiOuts_[0]->sendMessageNow(juce::MidiMessage::midiStop());
        return;
    }
    const double beats = positionBeats();
    if (beats < 1.0e-6) {
        midiOuts_[0]->sendMessageNow(juce::MidiMessage::midiStart());
    } else {
        midiOuts_[0]->sendMessageNow(juce::MidiMessage::songPositionPointer(
            juce::jlimit(0, 16383, (int) std::lround(beats * 4.0))));
        midiOuts_[0]->sendMessageNow(juce::MidiMessage::midiContinue());
    }
}

bool EngineHost::syncHandleMidi(const juce::MidiMessage& m) {
    if (syncMode_.load(std::memory_order_relaxed) != kSyncChase) return false;
    if (m.isMidiClock()) {
        const double bpm = chase_.onClock(juce::Time::getMillisecondCounterHiRes());
        if (bpm > 0.0) chaseBpm_.store(bpm, std::memory_order_relaxed);
        return true;
    }
    if (m.isMidiStart())    { chase_.reset(); chaseCmd_.store(1); return true; }
    if (m.isMidiContinue()) { chaseCmd_.store(2); return true; }
    if (m.isMidiStop())     { chaseCmd_.store(3); return true; }
    if (m.isSongPositionPointer()) {
        chaseSpp_.store(m.getSongPositionPointerMidiBeat() / 4.0);
        return true;
    }
    return false;
}

void EngineHost::syncApplyChase() {
    if (syncMode_.load(std::memory_order_relaxed) != kSyncChase) return;
    if (linkEnabled_.load(std::memory_order_relaxed)) {
        chaseCmd_.store(0);
        chaseSpp_.store(-1.0);
        chaseBpm_.store(0.0, std::memory_order_relaxed);
        return;
    }
    switch (chaseCmd_.exchange(0)) {
        case 1: playFromStart(); break;
        case 2: play(); break;
        case 3: stop(); break;
        default: break;
    }
    const double spp = chaseSpp_.exchange(-1.0);
    if (spp >= 0.0) setPositionBeats(spp);
    const double bpm = chaseBpm_.load(std::memory_order_relaxed);
    if (bpm > 0.0 && std::abs(bpm - tempo()) > 0.05) setTempo(bpm);
}

}
