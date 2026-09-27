// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <utility>

#include "common/GestureVec.h"
#include "core/params/ParamSchema.h"
#include "gui/editor/LiveControls.h"
#include "gui/host/ModelHost.h"
#include "hum/caps/Video.h"
#include "hum/dsp/DspMath.h"
#include "io/PatchDocument.h"

namespace hum::grids {

class HandGestureModel {
public:
    static constexpr int kNeed = 30;
    using Features = std::array<float, GestureFeatureSource::kMaxFeatures>;

    struct Params {
        std::string gestures, tolerance, notePrefix, thresholdPrefix;
    };

    HandGestureModel(ModelHost& host, std::string organism, Params params)
        : host_(host), organism_(std::move(organism)), p_(std::move(params)) {}

    GestureFeatureSource* source() const {
        auto* src = live::source<GestureFeatureSource>(host_, organism_);
        if (src != nullptr) dims_ = std::clamp(src->gestureFeatureCount(), 1, gvec::kMaxDims);
        return src;
    }

    gvec::Set set() const { return gvec::decode(host_.liveParamText(organism_, p_.gestures).c_str(), dims_); }

    bool matches(float out[gvec::kSlots]) const {
        auto* src = source();
        Features cur{};
        const bool present = src != nullptr && src->gestureFeaturesLive(cur.data());
        const float tol = (float) host_.liveParamValue(organism_, p_.tolerance);
        if (present) gvec::matchAll(set(), cur.data(), std::max(0.05f, tol), out);
        return present;
    }

    int note(int slot) const {
        const int n = (int) paramOrDefault(p_.notePrefix + std::to_string(slot + 1), 60.0 + slot);
        return n < 0 ? 0 : (kMidiMax < n ? kMidiMax : n);
    }

    float threshold(int slot) const {
        return (float) std::clamp(paramOrDefault(p_.thresholdPrefix + std::to_string(slot + 1), 0.6), 0.05, 1.0);
    }

    void setNote(int slot, int n) {
        const int next = n < 0 ? 0 : (kMidiMax < n ? kMidiMax : n);
        host_.editParam(organism_, p_.notePrefix + std::to_string(slot + 1), (double) next);
    }

    void setThreshold(int slot, double fraction) {
        host_.editParam(organism_, p_.thresholdPrefix + std::to_string(slot + 1), std::clamp(fraction, 0.05, 1.0));
    }

    int learning() const { return learning_; }
    int samples() const { return samples_; }

    void startLearn(int slot, bool reinforce) {
        learning_ = slot;
        reinforcing_ = reinforce;
        samples_ = 0;
        acc_ = {};
    }

    void clear(int slot) {
        auto g = set();
        g.learned[(size_t) slot] = false;
        write(g);
    }

    void tick() {
        if (learning_ < 0) return;
        auto* src = source();
        if (src == nullptr) {
            learning_ = -1;
            return;
        }
        Features cur{};
        if (!src->gestureFeaturesLive(cur.data())) return;
        for (int i = 0; i < dims_; ++i) acc_[(size_t) i] += cur[(size_t) i];
        if (++samples_ < kNeed) return;
        Features cap{};
        for (int i = 0; i < dims_; ++i) cap[(size_t) i] = acc_[(size_t) i] / (float) kNeed;
        auto g = set();
        if (reinforcing_) {
            gvec::reinforce(g, learning_, cap.data());
        } else {
            g.tpl[(size_t) learning_] = cap;
            g.learned[(size_t) learning_] = true;
            g.count[(size_t) learning_] = 1;
            gvec::finalizeWeights(g);
        }
        write(g);
        learning_ = -1;
    }

private:
    double paramOrDefault(const std::string& name, double fallback) const {
        if (const auto* cm = host_.model().byName(organism_)) {
            for (const auto& pr : cm->properties)
                if (pr.name == name) return pr.value;
            for (const auto& d : schemaFor(cm->classRaw))
                if (d.name == name) return d.def;
        }
        return fallback;
    }

    void write(const gvec::Set& g) { host_.setParamText(organism_, p_.gestures, gvec::encode(g)); }

    ModelHost& host_;
    std::string organism_;
    Params p_;
    mutable int dims_ = 5;
    int learning_ = -1;
    bool reinforcing_ = false;
    int samples_ = 0;
    Features acc_{};
};

}
