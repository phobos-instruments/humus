#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace hum {

struct Parameter {
    int index = -1;
    std::string name;
    std::string type;
    double value = 0.0;
    double rangeMin = 0.0;
    double rangeMax = 0.0;
    bool isRange = false;
    bool userEdited = false;

    std::string text;

    double minOr(double fallback) const { return isRange ? rangeMin : (value != 0.0 ? value : fallback); }
};

class ParameterSet {
public:
    void add(const Parameter& p) {
        if (!p.name.empty())
            if (auto it = byName_.find(p.name); it != byName_.end()) {
                const int keep = params_[(size_t) it->second].index;
                params_[(size_t) it->second] = p;
                params_[(size_t) it->second].index = keep;
                return;
            }
        byIndex_[p.index] = (int) params_.size();
        if (!p.name.empty()) byName_[p.name] = (int) params_.size();
        params_.push_back(p);
    }

    Parameter* byIndex(int i) {
        auto it = byIndex_.find(i);
        return it == byIndex_.end() ? nullptr : &params_[it->second];
    }
    Parameter* byName(const std::string& n) {
        auto it = byName_.find(n);
        return it == byName_.end() ? nullptr : &params_[it->second];
    }

    double get(const std::string& n, double fallback) const {
        auto it = byName_.find(n);
        if (it == byName_.end()) return fallback;
        const auto& p = params_[it->second];
        return p.isRange ? p.rangeMin : p.value;
    }
    double getMax(const std::string& n, double fallback) const {
        auto it = byName_.find(n);
        if (it == byName_.end()) return fallback;
        const auto& p = params_[it->second];
        return p.isRange ? p.rangeMax : p.value;
    }
    std::string getText(const std::string& n, const std::string& fallback = {}) const {
        auto it = byName_.find(n);
        return it == byName_.end() ? fallback : params_[it->second].text;
    }

    const std::vector<Parameter>& all() const { return params_; }

private:
    std::vector<Parameter> params_;
    std::unordered_map<int, int> byIndex_;
    std::unordered_map<std::string, int> byName_;
};

}
