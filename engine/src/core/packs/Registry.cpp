// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/Registry.h"

#include <algorithm>
#include <array>
#include <cstring>

#include "core/packs/PackRegistry.h"

namespace hum {

namespace {
class PassThrough : public Organism {
public:
    int numAudioInputs() const override { return in_; }
    int numAudioOutputs() const override { return out_; }
    void configureChannels(int inlets, int outlets) override {
        in_ = std::max(0, inlets);
        out_ = std::max(1, outlets);
    }
    void prepare(double sr, int) override { sampleRate_ = sr; }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport&) override {
        for (int c = 0; c < numOut; ++c) {
            if (c < numIn && in && in[c]) std::memcpy(out[c], in[c], sizeof(float) * (size_t) numSamples);
            else std::fill(out[c], out[c] + numSamples, 0.0f);
        }
    }
private:
    int in_ = 2, out_ = 2;
};

}

Registry& Registry::instance() {
    static Registry r;
    return r;
}

std::vector<std::string> Registry::classNames() const {
    auto& packs = PackRegistry::instance();
    std::vector<std::string> names;
    names.reserve(factories_.size());
    for (auto& kv : factories_)
        if (packs.isClassEnabled(kv.first)) names.push_back(kv.first);
    std::sort(names.begin(), names.end());
    return names;
}

OrganismPtr Registry::create(const std::string& className) const {
    auto& packs = PackRegistry::instance();
    if (packs.isClassEnabled(className)) {
        auto it = factories_.find(className);
        if (it != factories_.end())
            if (auto c = it->second()) return c;
        for (const auto& [packId, fn] : patterns_) {
            auto* p = PackRegistry::instance().packById(packId);
            if (p && !p->enabled) continue;
            if (auto c = fn(className)) return c;
        }
    }
    return std::make_unique<PassThrough>();
}

std::string Registry::layoutJson(const std::string& genId, const std::string& className) const {
    for (const auto& [packId, fn] : layouts_) {
        auto* p = PackRegistry::instance().packById(packId);
        if (p && !p->enabled) continue;
        if (auto json = fn(genId, className); !json.empty()) return json;
    }
    return {};
}

}
