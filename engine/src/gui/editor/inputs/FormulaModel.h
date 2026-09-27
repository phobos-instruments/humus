// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "gui/host/BrickHost.h"
#include "hum/caps/Graph.h"
#include "hum/dsp/DspMath.h"
#include "hum/dsp/Formula.h"

namespace hum::input {

class FormulaModel {
public:
    static constexpr int kPlotN = 256;

    static double plotSpanSeconds(const FormulaProgram& p, double freq, double bpm) {
        const auto sh = formulaShape(p);
        const bool cycles = sh.role == FormulaRole::Voice || (sh.role == FormulaRole::Effect && !sh.timed);
        return cycles ? 4.0 / std::max(20.0, freq) : kSecondsPerMinute / std::max(1.0, bpm);
    }

    FormulaModel(BrickHost& host, std::string organism, std::string param, std::array<std::string, 5> knobs)
        : host_(host), organism_(std::move(organism)), param_(std::move(param)), knobs_(std::move(knobs)) {}

    std::string liveText() const { return host_.liveParamText(organism_, param_); }
    const std::string& cachedText() const { return cached_; }
    const FormulaProgram& program() const { return prog_; }
    const FormulaError& error() const { return err_; }
    bool hasError() const { return hasError_; }
    const float* output() const { return slots_; }
    const float* input() const { return inSlots_; }

    void pull(const std::string& text) {
        cached_ = text;
        liveEdit_ = false;
        hasError_ = !compileFormula(text.c_str(), prog_, &err_, &inlets_);
    }

    void commit(const std::string& typed) {
        const bool pushed = liveEdit_;
        liveEdit_ = false;
        if (typed == cached_) return;
        cached_ = typed;
        if (!pushed) host_.pushUndo();
        host_.setParamText(organism_, param_, typed);
    }

    void preview(const std::string& typed) {
        hasError_ = !compileFormula(typed.c_str(), prog_, &err_, &inlets_);
        if (hasError_ || typed == cached_) return;
        if (!liveEdit_) {
            host_.pushUndo();
            liveEdit_ = true;
        }
        cached_ = typed;
        host_.setParamText(organism_, param_, typed);
    }

    const FormulaInlets& inlets() const { return inlets_; }

    bool pollInlets() {
        FormulaInlets next;
        auto* named = dynamic_cast<NamedInlet*>(host_.liveOrganism(organism_));
        if (named != nullptr) {
            next.count = named->inletNames(next.names, FormulaInlets::kMax);
            for (int k = 0; k < next.count; ++k) inletValues_[(size_t) k] = named->inletValue(k);
        }
        bool same = next.count == inlets_.count;
        for (int k = 0; same && k < next.count; ++k) same = std::string(next.names[k]) == inlets_.names[k];
        if (same) return false;
        inlets_ = next;
        pull(cached_);
        return true;
    }

    bool pollKnobs() {
        bool moved = pollInlets();
        for (int k = 0; k < 5; ++k) {
            const double v = host_.liveParamValue(organism_, knobs_[(size_t) k]);
            if (std::abs(v - kx_[k]) > 1e-9) {
                kx_[k] = v;
                moved = true;
            }
        }
        return moved;
    }

    void plot() {
        FormulaEnv env;
        std::uint32_t rng = 0x9e3779b9u;
        float phase[FormulaProgram::kStateSlots] = {};
        env.rng = &rng;
        env.state = phase;
        seedFormulaState(phase, prog_);
        const double bpm = host_.tempo();
        const double span = plotSpanSeconds(prog_, kx_[4], bpm);
        env.dt = (float) (span / kPlotN);
        const auto sh = formulaShape(prog_);
        const double probeHz = sh.role == FormulaRole::Effect && sh.timed ? 8.0 / span : 2.5 * kx_[4];
        env.v[fvNote] = (float) hzToMidi(std::max(20.0, kx_[4]));
        env.v[fvFreq] = (float) kx_[4];
        env.v[fvGate] = 1.0f;
        env.v[fvVel] = 1.0f;
        env.v[fvX] = (float) kx_[0];
        env.v[fvY] = (float) kx_[1];
        env.v[fvZ] = (float) kx_[2];
        env.v[fvW] = (float) kx_[3];
        env.v[fvBpm] = (float) bpm;
        env.v[fvSr] = (float) kPlotN;
        env.v[fvA] = env.v[fvB] = 0.0f;
        for (int k = 0; k < inlets_.count; ++k) env.v[fvIn0 + k] = inletValues_[(size_t) k];
        for (int i = 0, n = std::min(16384, (int) std::ceil(1.0 / env.dt)); i < n; ++i)
            env.v[fvPrev] = evalFormula(prog_, env);
        for (int i = 0; i < kPlotN; ++i) {
            const float t = (float) (span * i / (kPlotN - 1));
            env.v[fvT] = t;
            env.v[fvBeat] = t * (float) (std::max(1.0, bpm) / kSecondsPerMinute);
            if (sh.role == FormulaRole::Effect) {
                inSlots_[i] = 0.8f * std::sin((float) (kTwoPi * probeHz * t));
                env.v[fvA] = inSlots_[i];
                env.v[fvB] = 0.8f * std::sin((float) (kTwoPi * probeHz * 0.5 * t));
            }
            const float o = evalFormula(prog_, env);
            slots_[i] = o;
            env.v[fvPrev] = o;
        }
    }

private:
    FormulaInlets inlets_;
    std::array<float, FormulaInlets::kMax> inletValues_{};
    BrickHost& host_;
    std::string organism_, param_;
    std::array<std::string, 5> knobs_;
    std::string cached_;
    FormulaProgram prog_;
    FormulaError err_;
    bool hasError_ = false;
    bool liveEdit_ = false;
    float slots_[kPlotN] = {};
    float inSlots_[kPlotN] = {};
    double kx_[5] = {0.5, 0.5, 0.5, 0.5, 220.0};
};

}
