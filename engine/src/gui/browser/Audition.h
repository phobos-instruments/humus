// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <juce_core/juce_core.h>

#include "core/audio/PreviewVoice.h"

namespace hum::browser {

class Audition {
public:
    static constexpr double kLongestSeconds = 120.0;
    static constexpr double kSilentRate = 1000.0;

    Audition(PreviewVoice& voice, std::function<double()> tempo) : voice_(voice), tempo_(std::move(tempo)) {}
    ~Audition();

    void play(const std::string& path, double fileBpm);
    void stop();
    void toggle();
    void pause() { voice_.setPaused(true); }
    void seek(double seconds) { voice_.seek(seconds); }
    void setLoop(bool on) { voice_.setLoop(on); }
    void setSync(bool on);
    void setGain(float g) { voice_.setGain(g); }

    bool loop() const { return voice_.loop(); }
    bool sync() const { return sync_; }
    float gain() const { return voice_.gain(); }
    bool playing() const { return voice_.playing(); }
    bool paused() const { return voice_.paused(); }
    bool holds(const std::string& path) const { return current() == path && (playing() || paused()); }
    int decodes() const { return decodes_.load(); }
    double position() const { return voice_.position(); }
    std::string current() const;
    double length() const;
    bool waitIdle(int ms);

private:
    void applySpeed();

    PreviewVoice& voice_;
    std::function<double()> tempo_;
    juce::ThreadPool pool_{1};
    mutable std::mutex lock_;
    std::string current_;
    double fileBpm_ = 0.0;
    std::shared_ptr<const PreviewSound> sound_;
    std::string soundPath_;
    std::int64_t soundStamp_ = 0;
    std::atomic<int> decodes_{0};
    std::atomic<unsigned> request_{0};
    bool sync_ = false;
};

}
