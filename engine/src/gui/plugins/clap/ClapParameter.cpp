// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/clap/ClapParameter.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hum::claphost {

ClapParameter::ClapParameter(const clap_param_info_t& info, double plainValue, Text text, Parse parse,
                     std::atomic<bool>& anyPending, std::function<void()> onEdit)
    : id_(info.id), cookie_(info.cookie), flags_(info.flags), name_(juce::String::fromUTF8(info.name)),
      min_(info.min_value), max_(std::max(info.max_value, info.min_value)), default_(info.default_value),
      text_(std::move(text)), parse_(std::move(parse)), anyPending_(anyPending), onEdit_(std::move(onEdit)) {
    value_.store(normalised(plainValue), std::memory_order_relaxed);
}

double ClapParameter::plain(float normalisedValue) const {
    const double v = min_ + (double) juce::jlimit(0.0f, 1.0f, normalisedValue) * (max_ - min_);
    return stepped() ? std::round(v) : v;
}

float ClapParameter::normalised(double plainValue) const {
    if (max_ <= min_) return 0.0f;
    return (float) juce::jlimit(0.0, 1.0, (plainValue - min_) / (max_ - min_));
}

void ClapParameter::showPluginValue(double plainValue) {
    value_.store(normalised(plainValue), std::memory_order_relaxed);
}

void ClapParameter::setValue(float newValue) {
    if (readOnly()) return;
    value_.store(juce::jlimit(0.0f, 1.0f, newValue), std::memory_order_relaxed);
    pending_.store(true, std::memory_order_release);
    anyPending_.store(true, std::memory_order_release);
    if (onEdit_) onEdit_();
}

bool ClapParameter::takePending(double& plainOut) {
    if (!pending_.exchange(false, std::memory_order_acquire)) return false;
    plainOut = plainValue();
    return true;
}

juce::String ClapParameter::getText(float normalisedValue, int maximumLength) const {
    const double v = plain(normalisedValue);
    const auto shown = text_ ? text_(id_, v) : juce::String();
    return (shown.isNotEmpty() ? shown : juce::String(v, 2)).substring(0, maximumLength);
}

float ClapParameter::getValueForText(const juce::String& text) const {
    double v = 0.0;
    if (parse_ && parse_(id_, text, v)) return normalised(v);
    return normalised(text.getDoubleValue());
}

int ClapParameter::getNumSteps() const {
    if (!stepped()) return juce::AudioProcessor::getDefaultNumParameterSteps();
    return juce::jmax(2, (int) std::lround(max_ - min_) + 1);
}

}
