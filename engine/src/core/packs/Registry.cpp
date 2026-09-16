// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/Registry.h"

#include <algorithm>
#include <array>
#include <cstring>

#include "core/packs/BuiltinPacks.h"
#include "core/packs/Catalogue.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PodModel.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Video.h"

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

class PodPort : public Organism {
public:
    explicit PodPort(int channels = 1) : ch_(channels) {}
    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }
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
    int ch_ = 1;
};

class PodMidiPort : public Organism, public MidiNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sr, int) override { sampleRate_ = sr; }
    void reset() override { count_ = 0; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent* events, int count) override {
        for (int i = 0; i < count && count_ < kMaxMidiEventsPerBlock; ++i)
            buf_[(size_t) count_++] = events[i];
    }
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(count_, capacity);
        for (int i = 0; i < n; ++i) out[i] = buf_[(size_t) i];
        count_ = 0;
        return n;
    }

private:
    std::array<MidiEvent, (size_t) kMaxMidiEventsPerBlock> buf_{};
    int count_ = 0;
};
class PodControlPort : public Organism, public ControlSource, public PinKinds {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sr, int) override { sampleRate_ = sr; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}
    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {pods::kControlPortValue, (float) params.get(pods::kControlPortParam, 0.0)};
        return 1;
    }
    bool controlOutlet(int) const override { return true; }
};
class PodVideoPort : public Organism, public VideoNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sr, int) override { sampleRate_ = sr; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    int numVideoInputs() const override { return 1; }
    int numVideoOutputs() const override { return 1; }
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

void registerBuiltinOrganisms() {
    static bool done = false;
    if (done) return;
    done = true;
    Registry::instance().registerPathResolver(
        [](const std::string& ref, const std::string& className) {
            return catalogue::resolve(ref, className);
        });
    for (const auto& pack : builtinPacks())
        pack.registerFactories(Registry::instance());
    PackRegistry::instance().loadBuiltinPacks();
    Registry::instance().registerClass("PodIn", [] { return std::make_unique<PodPort>(); });
    Registry::instance().registerClass("PodOut", [] { return std::make_unique<PodPort>(); });
    Registry::instance().registerClass("PodInlet", [] { return std::make_unique<PodPort>(); });
    Registry::instance().registerClass("PodOutlet", [] { return std::make_unique<PodPort>(); });
    Registry::instance().registerClass("SPodIn", [] { return std::make_unique<PodPort>(2); });
    Registry::instance().registerClass("SPodOut", [] { return std::make_unique<PodPort>(2); });
    Registry::instance().registerClass("SPodInlet", [] { return std::make_unique<PodPort>(2); });
    Registry::instance().registerClass("SPodOutlet", [] { return std::make_unique<PodPort>(2); });
    Registry::instance().registerClass("PodMidiIn", [] { return std::make_unique<PodMidiPort>(); });
    Registry::instance().registerClass("PodMidiOut", [] { return std::make_unique<PodMidiPort>(); });
    Registry::instance().registerClass("PodMidiInlet", [] { return std::make_unique<PodMidiPort>(); });
    Registry::instance().registerClass("PodMidiOutlet", [] { return std::make_unique<PodMidiPort>(); });
    Registry::instance().registerClass("PodVideoIn", [] { return std::make_unique<PodVideoPort>(); });
    Registry::instance().registerClass("PodVideoOut", [] { return std::make_unique<PodVideoPort>(); });
    Registry::instance().registerClass(pods::kControlInletClass, [] { return std::make_unique<PodControlPort>(); });
    Registry::instance().registerClass(pods::kControlOutletClass, [] { return std::make_unique<PodControlPort>(); });
}

}
