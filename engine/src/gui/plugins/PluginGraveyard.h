// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace hum {

class PluginGraveyard : private juce::Timer {
public:
    static constexpr int kGraceMs = 2000;

    explicit PluginGraveyard(int graceMs = kGraceMs);
    ~PluginGraveyard() override;

    void bury(std::unique_ptr<juce::AudioPluginInstance> instance, const std::string& classRaw);
    void flush();
    int waiting() const;

private:
    struct Grave {
        std::unique_ptr<juce::AudioPluginInstance> instance;
        std::string classRaw;
        double buriedAtMs = 0.0;
    };

    void timerCallback() override;
    void wake();
    void release(bool all);

    const int graceMs_;
    mutable std::mutex lock_;
    std::vector<Grave> graves_;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

}
