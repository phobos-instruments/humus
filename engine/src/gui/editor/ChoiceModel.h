// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

#include "gui/editor/ControlArt.h"
#include "gui/editor/ControlModel.h"
#include "gui/editor/LayoutCondition.h"
#include "gui/host/ModelHost.h"
#include "hum/LayoutSpec.h"

namespace hum::choice {

enum class Kind { Combo, Buttons };

struct Option {
    std::string label;
    std::string tip;
};

struct Setup {
    std::string organism, param, source;
    Kind kind = Kind::Combo;
    std::vector<Option> options;
    int first = 0;
    bool idsAreValues = false;
    bool hasSteppers = false;
    bool waveIcons = false;
    ControlArt art;
};

struct State {
    int index = -1;
    std::string tip;
    bool enabled = true;
    bool dimmed = false;
    bool visible = true;
};

inline bool handles(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    return type == CT::Combo || type == CT::EnumButtons;
}

inline std::vector<std::string> tipsOf(const LayoutSpec::Control& control) {
    std::vector<std::string> out;
    const auto tips = control.extraOr("tips");
    if (tips.empty()) return out;
    for (size_t start = 0;;) {
        const auto bar = tips.find('|', start);
        out.push_back(tips.substr(start, bar == std::string::npos ? std::string::npos : bar - start));
        if (bar == std::string::npos) break;
        start = bar + 1;
    }
    return out;
}

inline Setup setupFor(const LayoutSpec::Control& control, const std::string& organism) {
    Setup s;
    s.organism = organism;
    s.param = control.param;
    s.kind = control.type == LayoutSpec::ControlType::EnumButtons ? Kind::Buttons : Kind::Combo;
    s.source = control.extraOr("source");
    s.idsAreValues = !s.source.empty();
    s.hasSteppers = !control.extraOr("steppers").empty();
    s.waveIcons = control.extraOr("icons") == "waves";
    if (s.kind == Kind::Buttons) s.first = std::atoi(control.extraOr("first", "0").c_str());
    const auto tips = tipsOf(control);
    for (size_t i = 0; i < control.options.size(); ++i) {
        Option o;
        o.label = control.options[i];
        if (i < tips.size()) o.tip = tips[i];
        s.options.push_back(std::move(o));
    }
    return s;
}

inline double valueForIndex(const Setup& setup, int index) {
    return (double) (setup.first + index);
}

inline int indexForValue(const Setup& setup, double value) {
    return (int) std::lround(value) - setup.first;
}

struct Offered {
    int index = 0;
    std::string text;
};

inline std::vector<std::string> offerOf(const LayoutSpec::Control& control) {
    std::vector<std::string> out;
    const std::string all = control.extraOr("offer");
    std::size_t from = 0;
    while (from <= all.size() && !all.empty()) {
        const auto to = all.find(';', from);
        const auto one = all.substr(from, to == std::string::npos ? std::string::npos : to - from);
        if (!one.empty()) out.push_back(one);
        if (to == std::string::npos) break;
        from = to + 1;
    }
    return out;
}

inline std::vector<Offered> offered(const std::vector<std::string>& options,
                                    const std::vector<std::string>& offer, int currentIndex) {
    std::vector<Offered> out;
    for (int i = 0; i < (int) options.size(); ++i) {
        bool listed = offer.empty() || i == currentIndex;
        for (const auto& o : offer) listed = listed || o == options[(size_t) i];
        if (listed) out.push_back({i, options[(size_t) i]});
    }
    return out;
}

inline constexpr int kZeroValueId = -1000000;

inline int comboIdForSourceValue(int value) { return value == 0 ? kZeroValueId : value; }

inline int sourceValueForComboId(int id) { return id == kZeroValueId ? 0 : id; }

inline int comboIdForValue(const Setup& setup, double value) {
    return setup.idsAreValues ? comboIdForSourceValue((int) std::lround(value)) : (int) std::lround(value) + 1;
}

inline double valueForComboId(const Setup& setup, int id) {
    return setup.idsAreValues ? (double) sourceValueForComboId(id) : (double) (id - 1);
}

inline int wrapped(int count, int current, bool forward) {
    if (count <= 0) return 0;
    return (current + (forward ? 1 : -1) + count) % count;
}

inline std::string optionTextFor(const Setup& setup, double value) {
    const int index = (int) std::lround(value);
    return index >= 0 && index < (int) setup.options.size()
               ? setup.options[(size_t) index].label
               : std::to_string(index);
}

inline std::string tipForIndex(const Setup& setup, int index) {
    if (index < 0 || index >= (int) setup.options.size()) return {};
    return setup.options[(size_t) index].tip;
}

inline State stateOf(ModelHost& host, const Setup& setup, const LayoutCondition& dimWhen,
                     const LayoutCondition& showWhen) {
    State st;
    const double value = control::paramValue(host, setup.organism, setup.param);
    st.index = setup.idsAreValues ? (int) std::lround(value) : indexForValue(setup, value);
    st.tip = setup.idsAreValues ? std::string() : tipForIndex(setup, st.index);
    st.dimmed = control::holds(host, setup.organism, dimWhen);
    st.enabled = !st.dimmed;
    st.visible = showWhen.empty() || control::holds(host, setup.organism, showWhen);
    return st;
}

inline void choose(ModelHost& host, const Setup& setup, int index) {
    host.editParam(setup.organism, setup.param, valueForIndex(setup, index));
}

}
