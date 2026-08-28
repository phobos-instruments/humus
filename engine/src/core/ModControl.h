#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "core/ControlShape.h"

namespace hum {

struct ModMapEntry {
    std::string source;
    std::string value;
    std::string organism;
    std::string param;
    double min = 0.0;
    double max = 1.0;
    ControlShape shape;
    ControlShapeState state;
};

struct ModParamUpdate {
    std::string organism;
    std::string param;
    double value = 0.0;
};

class ModControlMap {
public:
    void set(const std::string& source, const std::string& value,
             const std::string& organism, const std::string& param, double min, double max) {
        for (auto& e : entries_)
            if (e.source == source && e.value == value && e.organism == organism
                && e.param == param) {
                e.min = min; e.max = max; return;
            }
        ModMapEntry e;
        e.source = source; e.value = value; e.organism = organism; e.param = param;
        e.min = min; e.max = max;
        entries_.push_back(std::move(e));
    }

    void setShape(const std::string& source, const std::string& value,
                  const std::string& organism, const std::string& param,
                  const ControlShape& shape) {
        for (auto& e : entries_)
            if (e.source == source && e.value == value && e.organism == organism
                && e.param == param) {
                e.shape = shape;
                e.state = ControlShapeState{};
                return;
            }
    }
    const ControlShape* shapeOf(const std::string& source, const std::string& value,
                                const std::string& organism, const std::string& param) const {
        for (auto& e : entries_)
            if (e.source == source && e.value == value && e.organism == organism
                && e.param == param)
                return &e.shape;
        return nullptr;
    }

    void clear(const std::string& source, const std::string& value,
               const std::string& organism, const std::string& param) {
        erase([&](const ModMapEntry& e) {
            return e.source == source && e.value == value && e.organism == organism
                && e.param == param;
        });
    }
    void clearOrganism(const std::string& organism) {
        erase([&](const ModMapEntry& e) {
            return e.organism == organism || e.source == organism;
        });
    }
    void renameOrganism(const std::string& oldName, const std::string& newName) {
        for (auto& e : entries_) {
            if (e.organism == oldName) e.organism = newName;
            if (e.source == oldName) e.source = newName;
        }
    }
    void clearAll() { entries_.clear(); }

    bool empty() const { return entries_.empty(); }
    const std::vector<ModMapEntry>& entries() const { return entries_; }

    std::vector<ModParamUpdate>
    tick(const std::function<bool(const std::string& source, const std::string& value,
                                  float& out)>& read,
         double dt) {
        std::vector<ModParamUpdate> out;
        for (auto& e : entries_) {
            float v = 0.0f;
            if (!read(e.source, e.value, v)) continue;
            const double shaped = advanceControlShape(e.shape, e.state, v, dt);
            if (shaped >= 0.0)
                out.push_back({e.organism, e.param, shapedToRange(e.shape, e.min, e.max, shaped)});
        }
        return out;
    }

private:
    template <typename Pred>
    void erase(Pred p) {
        std::vector<ModMapEntry> kept;
        for (auto& e : entries_) if (!p(e)) kept.push_back(e);
        entries_.swap(kept);
    }
    std::vector<ModMapEntry> entries_;
};

}
