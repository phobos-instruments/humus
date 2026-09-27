// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/params/ParamSchema.h"
#include "core/params/ValueText.h"
#include "gui/editor/LiveControls.h"
#include "gui/host/ModelHost.h"
#include "io/PatchDocument.h"

namespace hum::input {

inline double limit(double lo, double hi, double v) { return v < lo ? lo : (hi < v ? hi : v); }

struct ParamRange {
    double lo = 0.0, hi = 1.0;
    bool integer = true;
};

inline ParamRange rangeFor(ModelHost& host, const std::string& organism, const std::string& param,
                           ParamRange fallback = {}) {
    if (const auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return {d.min, d.max, d.isInt || d.isEnum};
    return fallback;
}

class NumberInput {
public:
    NumberInput(ModelHost& host, std::string organism, std::string param, std::string shows = {})
        : host_(host), organism_(std::move(organism)), param_(std::move(param)), shows_(std::move(shows)),
          range_(rangeFor(host_, organism_, param_)), last_(shown()) {}

    const ParamRange& range() const { return range_; }
    const std::string& param() const { return param_; }

    double setting() const { return host_.liveParamValue(organism_, param_); }

    double shown() const;
    bool set(double v);
    bool poll();

    void beginDrag() { host_.beginParamDrag(organism_, param_); }
    void endDrag() { host_.endParamDrag(); }

    double fieldDragPerPixel(bool fine, double fineFactor) const;

    double fieldWheelStep() const { return range_.integer ? 1.0 : (range_.hi - range_.lo) / 100.0; }

    std::string fieldText(double v) const {
        return range_.integer ? std::to_string((int) std::llround(v)) : decimalText(v, 2);
    }

private:
    ModelHost& host_;
    std::string organism_, param_, shows_;
    ParamRange range_;
    double last_ = 0.0;
};

class NumberBox {
public:
    NumberBox(ModelHost& host, std::string organism, std::string param, int decimals, double step,
              std::string shows = {})
        : number_(host, std::move(organism), std::move(param), std::move(shows)),
          decimals_(std::clamp(decimals, 0, 6)),
          step_(step > 0.0 ? step : (number_.range().integer ? 1.0 : std::pow(10.0, -decimals_))) {}

    NumberInput& number() { return number_; }
    const NumberInput& number() const { return number_; }
    double step(bool fine, double fineFactor) const { return fine ? step_ / fineFactor : step_; }

    std::string text(double v) const;

private:
    NumberInput number_;
    const int decimals_;
    const double step_;
};

class NoteInput {
public:
    NoteInput(ModelHost& host, std::string organism, std::string param, int lowest, int highest);

    int note() const { return (int) host_.liveParamValue(organism_, param_); }
    int lowest() const { return lo_; }
    int highest() const { return hi_; }

    bool step(int direction);
    bool poll();

private:
    ModelHost& host_;
    std::string organism_, param_;
    int lo_ = 0, hi_ = 0, last_ = 0;
};

class TextInput {
public:
    TextInput(ModelHost& host, std::string organism, std::string param)
        : host_(host), organism_(std::move(organism)), param_(std::move(param)) {}

    std::string text() const { return host_.liveParamText(organism_, param_); }
    void commit(const std::string& text) { host_.setParamText(organism_, param_, text); }

private:
    ModelHost& host_;
    std::string organism_, param_;
};

class RhythmicUnit {
public:
    RhythmicUnit(ModelHost& host, std::string organism, std::string multiplierParam, std::string unitParam)
        : host_(host), organism_(std::move(organism)), multiplier_(std::move(multiplierParam)),
          unit_(std::move(unitParam)) {}

    static const std::vector<std::string>& units();
    static constexpr const char* kDefaultUnit = "1/16";

    double multiplier() const { return host_.liveParamValue(organism_, multiplier_); }
    std::string unit() const;

    void commitMultiplier(double m) { host_.setParam(organism_, multiplier_, std::max(m, 0.001)); }
    void commitUnit(const std::string& u) { host_.setParamText(organism_, unit_, u); }

private:
    ModelHost& host_;
    std::string organism_, multiplier_, unit_;
};

}
