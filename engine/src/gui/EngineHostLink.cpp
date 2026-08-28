#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AppSettings.h"
#include "gui/EngineHost.h"

namespace hum {

void EngineHost::setLinkEnabled(bool on) {
    if (on && juce::JUCEApplication::getInstance() == nullptr) return;
    if (on && link_ == nullptr) {
        const juce::ScopedLock sl(lock_);
        link_.reset(new LinkSync());
    }
    linkEnabled_.store(on, std::memory_order_relaxed);
    linkStartStop_.store(
        AppSettings::instance().getString("link.startstop", "off") == "on",
        std::memory_order_relaxed);
    if (link_) {
        link_->setStartStopSyncEnabled(on && linkStartStop_.load(std::memory_order_relaxed));
        link_->setEnabled(on);
        linkLastSessionPlaying_ = link_->sessionPlaying();
    }
    {
        const juce::ScopedLock sl(lock_);
        if (graph_) graph_->setExternalTempoMaster(on);
    }
    if (on) {
        link_->proposeTempo(tempo());
        linkAlign_.store(true, std::memory_order_relaxed);
    }
    AppSettings::instance().set("link.enabled", on ? "on" : "off");
}

void EngineHost::setLinkStartStopSync(bool on) {
    AppSettings::instance().set("link.startstop", on ? "on" : "off");
    linkStartStop_.store(on, std::memory_order_relaxed);
    if (link_) {
        link_->setStartStopSyncEnabled(on && linkEnabled_.load(std::memory_order_relaxed));
        linkLastSessionPlaying_ = link_->sessionPlaying();
    }
}

int EngineHost::linkPeers() const {
    return link_ != nullptr && linkEnabled_.load(std::memory_order_relaxed)
               ? link_->numPeers() : 0;
}

void EngineHost::applyLinkFromSettings() {
    if (AppSettings::instance().getString("link.enabled", "off") == "on")
        setLinkEnabled(true);
}

void EngineHost::linkApplyUi() {
    if (link_ == nullptr || !linkEnabled_.load(std::memory_order_relaxed)) return;
    const double bpm = link_->sessionTempo();
    if (bpm > 0.0 && std::abs(bpm - tempo()) > 0.05) setTempo(bpm);

    if (linkStartStop_.load(std::memory_order_relaxed)) {
        const bool sessionPlaying = link_->sessionPlaying();
        if (sessionPlaying != linkLastSessionPlaying_) {
            linkLastSessionPlaying_ = sessionPlaying;
            if (sessionPlaying && !isPlaying()) play();
            else if (!sessionPlaying && isPlaying()) stop();
        }
    }
}

}
