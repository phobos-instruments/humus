// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>

#include <clap/ext/params.h>

namespace hum::claphost {

class ClapParameter : public juce::AudioPluginInstance::HostedParameter {
public:
    using Text = std::function<juce::String(clap_id, double)>;
    using Parse = std::function<bool(clap_id, const juce::String&, double&)>;

    ClapParameter(const clap_param_info_t& info, double plainValue, Text text, Parse parse,
              std::atomic<bool>& anyPending, std::function<void()> onEdit);

    clap_id id() const { return id_; }
    void* cookie() const { return cookie_; }
    bool readOnly() const { return (flags_ & CLAP_PARAM_IS_READONLY) != 0; }
    double plain(float normalised) const;
    float normalised(double plain) const;

    void showPluginValue(double plain);
    bool takePending(double& plain);
    double plainValue() const { return plain(value_.load(std::memory_order_relaxed)); }

    float getValue() const override { return value_.load(std::memory_order_relaxed); }
    void setValue(float newValue) override;
    float getDefaultValue() const override { return normalised(default_); }
    juce::String getName(int maximumLength) const override { return name_.substring(0, maximumLength); }
    juce::String getLabel() const override { return {}; }
    juce::String getText(float normalisedValue, int maximumLength) const override;
    float getValueForText(const juce::String& text) const override;
    int getNumSteps() const override;
    bool isDiscrete() const override { return stepped(); }
    bool isBoolean() const override { return stepped() && max_ - min_ == 1.0; }
    bool isAutomatable() const override { return (flags_ & CLAP_PARAM_IS_AUTOMATABLE) != 0; }
    juce::String getParameterID() const override { return juce::String((juce::uint32) id_); }

private:
    bool stepped() const { return (flags_ & CLAP_PARAM_IS_STEPPED) != 0; }

    clap_id id_;
    void* cookie_;
    uint32_t flags_;
    juce::String name_;
    double min_, max_, default_;
    Text text_;
    Parse parse_;
    std::atomic<bool>& anyPending_;
    std::function<void()> onEdit_;
    std::atomic<float> value_{0.0f};
    std::atomic<bool> pending_{false};
};

}
