// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include "hum/Tuning.h"

namespace hum {

class VoiceSource {
public:
    virtual ~VoiceSource() = default;
    virtual bool voiceParams(std::vector<std::pair<std::string, double>>& out) const = 0;
};

class SocketSources {
public:
    virtual ~SocketSources() = default;
    virtual void setSocketSource(const std::string& param, const std::string& sourceName) = 0;
};

class ReloadOnParam {
public:
    virtual ~ReloadOnParam() = default;
    virtual bool reloadsOn(const std::string& param) const = 0;
};

class TextVoiceSource {
public:
    virtual ~TextVoiceSource() = default;
    virtual bool takeVoiceTexts(std::vector<std::pair<std::string, std::string>>& out) = 0;
};

class LiveParamRange {
public:
    virtual ~LiveParamRange() = default;
    virtual bool liveParamRange(const std::string& param, double& lo, double& hi) const = 0;
    virtual bool rangeFollowsFile(const std::string&, const std::string&) const { return true; }
};

class PatchBank {
public:
    virtual ~PatchBank() = default;
    virtual int patchCount() const = 0;
    virtual std::string patchNameAt(int index) const = 0;
};

class RollListener {
public:
    virtual ~RollListener() = default;
    virtual void rolled() = 0;
};

class TuningProvider {
public:
    virtual ~TuningProvider() = default;
    virtual const Tuning& tuning() const = 0;
    virtual void refreshTuning() {}
};

inline std::atomic<unsigned>& renderSeedStore() {
    static std::atomic<unsigned> s{0};
    return s;
}
inline std::atomic<bool>& renderSeedSetStore() {
    static std::atomic<bool> s{false};
    return s;
}
inline void setRenderSeed(unsigned seed) {
    renderSeedStore().store(seed);
    renderSeedSetStore().store(true);
}
inline bool renderSeedIsSet() { return renderSeedSetStore().load(); }
inline unsigned renderSeed() { return renderSeedStore().load(); }

}
