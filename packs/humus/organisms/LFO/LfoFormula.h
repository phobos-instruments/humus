// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "hum/dsp/Formula.h"
#include "hum/dsp/Prepared.h"

namespace hum {

class LfoFormula {
public:
    static constexpr const char* kFirst = "sin(tau*phase)";

    struct Moment {
        double phase = 0.0, input = 0.0, freq = 1.0, seconds = 0.0, beats = 0.0, bpm = 120.0;
        double sampleRate = 48000.0;
        float x = 0.5f, y = 0.5f, z = 0.5f, w = 0.5f;
    };

    void sync(const std::string& text);
    void publish(const std::string& text);
    void restart(const std::string& name);
    bool adopt();
    double at(const Moment& m);
    bool steps() const { return formulaSteps(prog_); }

private:
    static FormulaProgram compile(const std::string& text, const FormulaProgram& last);
    void clearState();

    FormulaProgram prog_;
    Prepared<FormulaProgram> pending_;
    std::string applied_;
    std::array<float, FormulaProgram::kStateSlots> state_{};
    std::uint32_t rng_ = 0x9e3779b9u, seed_ = 0x9e3779b9u;
    float prev_ = 0.0f;
};

}
