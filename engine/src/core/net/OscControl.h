// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "core/net/ControlShape.h"

namespace hum {

struct OscMapEntry {
    std::string address;
    std::string organism;
    std::string param;
    double min = 0.0;
    double max = 1.0;
    ControlShape shape;
    ControlShapeState state;
    double pendingT = -1.0;
};

struct OscParamUpdate {
    std::string organism;
    std::string param;
    double value = 0.0;
};

class OscControlMap {
public:
    void set(const std::string& address, const std::string& organism,
             const std::string& param, double min, double max) {
        for (auto& e : entries_)
            if (e.address == address && e.organism == organism && e.param == param) {
                e.min = min; e.max = max; return;
            }
        OscMapEntry e;
        e.address = address; e.organism = organism; e.param = param;
        e.min = min; e.max = max;
        entries_.push_back(std::move(e));
    }

    void setShape(const std::string& address, const std::string& organism,
                  const std::string& param, const ControlShape& shape) {
        for (auto& e : entries_)
            if (e.address == address && e.organism == organism && e.param == param) {
                e.shape = shape;
                e.state = ControlShapeState{};
                return;
            }
    }
    const ControlShape* shapeOf(const std::string& address, const std::string& organism,
                                const std::string& param) const {
        for (auto& e : entries_)
            if (e.address == address && e.organism == organism && e.param == param)
                return &e.shape;
        return nullptr;
    }

    std::vector<std::pair<std::string, std::string>>
    steal(const std::string& address, const std::string& keepOrganism,
          const std::string& keepParam) {
        std::vector<std::pair<std::string, std::string>> stolen;
        for (const auto& e : entries_)
            if (e.address == address
                && !(e.organism == keepOrganism && e.param == keepParam))
                stolen.push_back({e.organism, e.param});
        erase([&](const OscMapEntry& e) {
            return e.address == address
                && !(e.organism == keepOrganism && e.param == keepParam);
        });
        return stolen;
    }

    void clear(const std::string& address, const std::string& organism,
               const std::string& param) {
        erase([&](const OscMapEntry& e) {
            return e.address == address && e.organism == organism && e.param == param;
        });
    }
    void clearOrganism(const std::string& organism) {
        erase([&](const OscMapEntry& e) { return e.organism == organism; });
    }
    void renameOrganism(const std::string& oldName, const std::string& newName) {
        for (auto& e : entries_) if (e.organism == oldName) e.organism = newName;
    }
    void clearAll() { entries_.clear(); }

    bool empty() const { return entries_.empty(); }
    std::vector<std::pair<std::string, std::string>>
    usersOf(const std::string& address, const std::string& exceptOrganism,
            const std::string& exceptParam) const {
        std::vector<std::pair<std::string, std::string>> out;
        for (const auto& e : entries_)
            if (e.address == address && !(e.organism == exceptOrganism && e.param == exceptParam))
                out.push_back({e.organism, e.param});
        return out;
    }

    const std::vector<OscMapEntry>& entries() const { return entries_; }

    std::vector<OscParamUpdate> resolve(const std::string& address, double value01) const {
        std::vector<OscParamUpdate> out;
        const double t = value01 < 0.0 ? 0.0 : (value01 > 1.0 ? 1.0 : value01);
        for (const auto& e : entries_)
            if (e.address == address)
                out.push_back({e.organism, e.param, e.min + (e.max - e.min) * t});
        return out;
    }

    std::vector<OscParamUpdate> deliver(const std::string& address, double value01) {
        std::vector<OscParamUpdate> out;
        for (auto& e : entries_) {
            if (e.address != address) continue;
            if (e.shape.smoothing > 1e-6) { e.pendingT = value01; continue; }
            const double shaped = advanceControlShape(e.shape, e.state, value01, 0.0);
            if (shaped >= 0.0)
                out.push_back({e.organism, e.param, shapedToRange(e.shape, e.min, e.max, shaped)});
        }
        return out;
    }
    std::vector<OscParamUpdate> tick(double dt) {
        std::vector<OscParamUpdate> out;
        for (auto& e : entries_) {
            if (e.pendingT < 0.0) continue;
            const double shaped = advanceControlShape(e.shape, e.state, e.pendingT, dt);
            if (shaped >= 0.0)
                out.push_back({e.organism, e.param, shapedToRange(e.shape, e.min, e.max, shaped)});
        }
        return out;
    }

private:
    template <typename Pred>
    void erase(Pred p) {
        std::vector<OscMapEntry> kept;
        for (auto& e : entries_) if (!p(e)) kept.push_back(e);
        entries_.swap(kept);
    }
    std::vector<OscMapEntry> entries_;
};

}
