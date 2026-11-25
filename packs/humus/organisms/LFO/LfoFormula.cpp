// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "LFO/LfoFormula.h"

#include <algorithm>

namespace hum {

FormulaProgram LfoFormula::compile(const std::string& source, const FormulaProgram& last) {
    FormulaProgram prog = last;
    std::string text = source.empty() ? std::string(kFirst) : source;
    while (!compileFormula(text.c_str(), prog) && !prog.valid() && text.size() > 1) text.pop_back();
    return prog;
}

void LfoFormula::sync(const std::string& text) {
    if (text == applied_ && prog_.valid()) return;
    applied_ = text;
    prog_ = compile(text, prog_);
    clearState();
}

void LfoFormula::publish(const std::string& text) {
    applied_ = text;
    pending_.publish(compile(text, prog_));
}

void LfoFormula::restart(const std::string& name) {
    seed_ = formulaSeedFor(name);
    clearState();
}

bool LfoFormula::adopt() {
    if (!pending_.adopt(prog_)) return false;
    clearState();
    return true;
}

void LfoFormula::clearState() {
    state_.fill(0.0f);
    seedFormulaState(state_.data(), prog_);
    rng_ = seed_;
    prev_ = 0.0f;
}

double LfoFormula::at(const Moment& m) {
    if (!prog_.valid()) return 0.0;
    FormulaEnv env;
    env.rng = &rng_;
    env.state = state_.data();
    env.dt = (float) (1.0 / m.sampleRate);
    env.v[fvA] = (float) m.input;
    env.v[fvX] = m.x;
    env.v[fvY] = m.y;
    env.v[fvZ] = m.z;
    env.v[fvW] = m.w;
    env.v[fvT] = (float) m.seconds;
    env.v[fvBeat] = (float) m.beats;
    env.v[fvBpm] = (float) m.bpm;
    env.v[fvSr] = (float) m.sampleRate;
    env.v[fvFreq] = (float) m.freq;
    env.v[fvRate] = (float) m.freq;
    env.v[fvGate] = 1.0f;
    env.v[fvVel] = 1.0f;
    env.v[fvPhase] = (float) m.phase;
    env.v[fvPrev] = prev_;
    prev_ = std::clamp(evalFormula(prog_, env), -4.0f, 4.0f);
    return prev_;
}

}
