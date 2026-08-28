#pragma once
#include <map>
#include <string>
#include <vector>

#include "hum/Parameter.h"
#include "hum/Pattern.h"

namespace hum {

struct NodeState {
    std::vector<Parameter> props;
    Pattern pattern;
};

inline bool sameParam(const Parameter& a, const Parameter& b) {
    return a.name == b.name && a.isRange == b.isRange && a.value == b.value
           && a.rangeMin == b.rangeMin && a.rangeMax == b.rangeMax && a.text == b.text;
}
inline bool samePattern(const Pattern& a, const Pattern& b) {
    if (a.present != b.present) return false;
    if (!a.present) return true;
    if (a.duration != b.duration || a.matrixResolution != b.matrixResolution
        || a.channels.size() != b.channels.size())
        return false;
    for (size_t i = 0; i < a.channels.size(); ++i) {
        const auto& ca = a.channels[i];
        const auto& cb = b.channels[i];
        if (ca.type != cb.type || ca.matrix != cb.matrix || ca.triggers != cb.triggers)
            return false;
    }
    return true;
}
inline bool sameState(const NodeState& a, const NodeState& b) {
    if (a.props.size() != b.props.size()) return false;
    for (size_t i = 0; i < a.props.size(); ++i)
        if (!sameParam(a.props[i], b.props[i])) return false;
    return samePattern(a.pattern, b.pattern);
}

class ParamHistory {
public:
    void commit(const std::string& node, const NodeState& cur) {
        auto& t = map_[node];
        if (!t.states.empty() && sameState(t.states[t.cursor], cur)) return;
        t.states.resize(t.states.empty() ? 0 : t.cursor + 1);
        t.states.push_back(cur);
        t.cursor = t.states.size() - 1;
    }

    const NodeState* back(const std::string& node, const NodeState& cur) {
        commit(node, cur);
        auto& t = map_[node];
        if (t.cursor == 0) return nullptr;
        return &t.states[--t.cursor];
    }

    const NodeState* forward(const std::string& node) {
        const auto it = map_.find(node);
        if (it == map_.end()) return nullptr;
        auto& t = it->second;
        if (t.cursor + 1 >= t.states.size()) return nullptr;
        return &t.states[++t.cursor];
    }

    void clear() { map_.clear(); }

private:
    struct Track {
        std::vector<NodeState> states;
        size_t cursor = 0;
    };
    std::map<std::string, Track> map_;
};

}
