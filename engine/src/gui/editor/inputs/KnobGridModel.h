// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/params/ParamSchema.h"
#include "core/params/UnitText.h"
#include "gui/editor/LiveControls.h"
#include "gui/host/ModelHost.h"
#include "hum/caps/Params.h"
#include "io/PatchDocument.h"

namespace hum::input {

class KnobGridModel {
public:
    static constexpr int kCell = 46;
    static constexpr int kHeader = 14;
    static constexpr int kLabelW = 40;

    struct Knob {
        std::string param;
        std::string field;
        double lo = 0.0, hi = 1.0;
        bool whole = false;
        Unit unit = Unit::None;
    };

    struct Metrics {
        double k;
        int cell, header, labelW;
    };

    KnobGridModel(ModelHost& host, std::string organism, const std::string& prefix, int rows,
                  std::vector<std::string> fields)
        : host_(host), organism_(std::move(organism)), rows_(rows), fields_(std::move(fields)) {
        const auto* cm = host_.model().byName(organism_);
        const auto& schema = schemaFor(cm != nullptr ? cm->classRaw : std::string{});
        for (int r = 1; r <= rows_; ++r) rowParams_.push_back(prefix + std::to_string(r) + "_On");
        for (int r = 1; r <= rows_; ++r)
            for (const auto& field : fields_) {
                Knob knob;
                knob.param = prefix + std::to_string(r) + "_" + field;
                knob.field = field;
                for (const auto& d : schema)
                    if (d.name == knob.param) {
                        knob.lo = d.min;
                        knob.hi = d.max;
                        knob.whole = d.isInt;
                        knob.unit = unitResolve(d.name, d.unit, d.min, d.max);
                        break;
                    }
                knobs_.push_back(std::move(knob));
            }
    }

    int rows() const { return rows_; }
    const std::vector<std::string>& fields() const { return fields_; }
    const std::vector<Knob>& knobs() const { return knobs_; }
    const std::string& rowParam(int row) const { return rowParams_[(size_t) row]; }

    double value(const std::string& param) const { return host_.liveParamValue(organism_, param); }
    bool rowOn(int row) const { return value(rowParam(row)) >= 0.5; }

    bool rowUnavailable(int row) const {
        auto* live = live::source<LiveParamRange>(host_, organism_);
        double lo = 0.0, hi = 0.0;
        return live != nullptr && live->liveParamRange(rowParam(row), lo, hi) && hi <= lo;
    }

    void setRow(int row, bool on) { host_.editParam(organism_, rowParam(row), on ? 1.0 : 0.0); }

    int preferredWidth() const { return kLabelW + (int) fields_.size() * kCell; }
    int preferredHeight() const { return kHeader + rows_ * kCell; }

    Metrics metrics(int width, int height) const {
        const double pw = preferredWidth();
        const double ph = preferredHeight();
        const double k = (pw <= 0.0 || ph <= 0.0) ? 1.0 : std::min({1.0, width / pw, height / ph});
        return {k, std::max(1, (int) std::lround(kCell * k)), std::max(1, (int) std::lround(kHeader * k)),
                std::max(1, (int) std::lround(kLabelW * k))};
    }

    static int cellX(int column, const Metrics& m) { return m.labelW + column * m.cell; }

private:
    ModelHost& host_;
    std::string organism_;
    int rows_;
    std::vector<std::string> fields_;
    std::vector<std::string> rowParams_;
    std::vector<Knob> knobs_;
};

}
