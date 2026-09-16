// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <optional>
#include <string>

#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"

namespace hum {

class UiWatchdog : public juce::Thread {
public:
    UiWatchdog(juce::File breadcrumb, juce::File log, int stallMs)
        : juce::Thread("hum-ui-watchdog"),
          breadcrumb_(std::move(breadcrumb)), log_(std::move(log)),
          stallMs_(juce::jmax(1, stallMs)) {}
    ~UiWatchdog() override {
        stopThread(2000);
        if (active_ == this) active_ = nullptr;
    }

    static juce::File configDir() { return appDataDir(); }
    static juce::File defaultBreadcrumb() { return configDir().getChildFile("ui-hang-pedal.txt"); }
    static juce::File defaultLog() { return configDir().getChildFile("ui-hang-log.txt"); }

    void heartbeat() { lastBeat_.store(juce::Time::getMillisecondCounter()); }

    void noteActivePlugin(const std::string& classRaw) {
        const juce::ScopedLock sl(pluginLock_);
        lastPlugin_ = classRaw;
    }

    static UiWatchdog* active() { return active_; }
    void makeActive() { active_ = this; }

    struct Suspend {
        explicit Suspend(UiWatchdog* w) : w_(w) { if (w_) ++w_->suspend_; }
        ~Suspend() {
            if (w_) { w_->heartbeat(); --w_->suspend_; }
        }
        Suspend(const Suspend&) = delete;
        Suspend& operator=(const Suspend&) = delete;
        UiWatchdog* w_;
    };

    struct HangReport { juce::String when; int stallMs = 0; juce::String pluginClassRaw; };
    static std::optional<HangReport> consumeBreadcrumb(const juce::File& f) {
        if (!f.existsAsFile()) return std::nullopt;
        juce::StringArray lines;
        lines.addLines(f.loadFileAsString());
        f.deleteFile();
        HangReport r;
        r.when = lines.size() > 0 ? lines[0] : juce::String();
        r.stallMs = lines.size() > 1 ? lines[1].getIntValue() : 0;
        r.pluginClassRaw = lines.size() > 2 ? lines[2] : juce::String();
        return r;
    }

    void run() override {
        heartbeat();
        while (!threadShouldExit()) {
            wait(juce::jmin(500, stallMs_ / 2 + 1));
            if (suspend_.load() > 0) continue;
            const juce::uint32 now = juce::Time::getMillisecondCounter();
            const bool stalled = (juce::int64) (now - lastBeat_.load()) > stallMs_;
            if (stalled && !written_) {
                writeBreadcrumb(now - lastBeat_.load());
                written_ = true;
            } else if (!stalled && written_) {
                recovered();
                written_ = false;
            }
        }
    }

private:
    void writeBreadcrumb(juce::uint32 stalledForMs) {
        juce::String plugin;
        {
            const juce::ScopedLock sl(pluginLock_);
            plugin = lastPlugin_;
        }
        breadcrumb_.getParentDirectory().createDirectory();
        breadcrumb_.replaceWithText(juce::Time::getCurrentTime().toISO8601(true) + "\n"
                                    + juce::String((int) stalledForMs) + "\n"
                                    + plugin + "\n");
    }
    void recovered() {
        if (auto rep = consumeBreadcrumb(breadcrumb_))
            log_.appendText("recovered " + rep->when + " stall " + juce::String(rep->stallMs)
                            + "ms plugin " + rep->pluginClassRaw + "\n");
    }

    inline static UiWatchdog* active_ = nullptr;

    juce::File breadcrumb_, log_;
    int stallMs_;
    std::atomic<juce::uint32> lastBeat_{0};
    std::atomic<int> suspend_{0};
    bool written_ = false;
    juce::CriticalSection pluginLock_;
    juce::String lastPlugin_;
};

}
