// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <mutex>

#include <juce_core/juce_core.h>

namespace hum {

class AudioLock {
public:
    static constexpr double kGraphWaitShare = 0.25;
    static constexpr double kTapWaitShare = 0.1;

    struct Hold {
        double ms = 0.0;
        const char* file = "";
        int line = 0;
    };

    class Scope {
    public:
        explicit Scope(AudioLock& lock, const char* file = __builtin_FILE(), int line = __builtin_LINE())
            : lock_(lock), file_(file), line_(line) {
            lock_.section_.enter();
            since_ = juce::Time::getMillisecondCounterHiRes();
        }
        ~Scope() {
            lock_.note(juce::Time::getMillisecondCounterHiRes() - since_, file_, line_);
            lock_.section_.exit();
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        AudioLock& lock_;
        const char* file_;
        int line_;
        double since_ = 0.0;
    };

    class AudioThreadTry {
    public:
        AudioThreadTry(AudioLock& lock, double waitMs) : lock_(lock) {
            locked_ = lock_.section_.tryEnter();
            if (locked_ || waitMs <= 0.0) return;
            const double until = juce::Time::getMillisecondCounterHiRes() + waitMs;
            while (!locked_ && juce::Time::getMillisecondCounterHiRes() < until) locked_ = lock_.section_.tryEnter();
        }
        ~AudioThreadTry() {
            if (locked_) lock_.section_.exit();
        }
        AudioThreadTry(const AudioThreadTry&) = delete;
        AudioThreadTry& operator=(const AudioThreadTry&) = delete;
        bool isLocked() const { return locked_; }

    private:
        AudioLock& lock_;
        bool locked_ = false;
    };

    juce::CriticalSection& section() { return section_; }

    Hold takeLongestHold() {
        const std::lock_guard<std::mutex> held(recordLock_);
        const Hold out = longest_;
        longest_ = {};
        return out;
    }

private:
    void note(double ms, const char* file, int line) {
        const std::lock_guard<std::mutex> held(recordLock_);
        if (ms > longest_.ms) longest_ = {ms, file, line};
    }

    juce::CriticalSection section_;
    std::mutex recordLock_;
    Hold longest_;
};

}
