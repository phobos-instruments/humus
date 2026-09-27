// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Math/Math.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum {

FormulaProgram MathNode::compileExpression(const std::string& source, const FormulaProgram& last,
                                           const FormulaInlets& inlets) {
    FormulaProgram prog = last;
    std::string text = source;
    while (!compileFormula(text.c_str(), prog, nullptr, &inlets) && !prog.valid() && text.size() > 1)
        text.pop_back();
    return prog;
}

void MathNode::setInletNames(const char (*names)[kNameChars], int count) {
    FormulaInlets next;
    next.count = std::clamp(count, 0, FormulaInlets::kMax);
    for (int k = 0; k < next.count; ++k)
        std::snprintf(next.names[k], FormulaInlets::kChars, "%s", names[k]);
    bool same = next.count == inlets_.count;
    for (int k = 0; same && k < next.count; ++k) same = std::strcmp(next.names[k], inlets_.names[k]) == 0;
    if (same) return;
    inlets_ = next;
    pendingProg_.publish(compileExpression(appliedText_, prog_, inlets_));
}

bool MathNode::readsName(const char* name) const {
    const std::string& text = appliedText_;
    const size_t len = std::strlen(name);
    auto identChar = [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; };
    for (size_t at = text.find(name); at != std::string::npos; at = text.find(name, at + 1)) {
        const bool startOk = at == 0 || !identChar(text[at - 1]);
        const bool endOk = at + len >= text.size() || !identChar(text[at + len]);
        if (startOk && endOk) return true;
    }
    return false;
}

const char* MathNode::knobForName(const char* name) const {
    static const char* const kKnobs[][2] = {{"x", "X"}, {"y", "Y"}, {"z", "Z"}, {"w", "W"}};
    for (const auto& k : kKnobs)
        if (std::strcmp(name, k[0]) == 0) return k[1];
    return nullptr;
}

int MathNode::inletNames(char (*out)[kNameChars], int capacity) const {
    const int n = std::min(capacity, inlets_.count);
    for (int k = 0; k < n; ++k) std::snprintf(out[k], kNameChars, "%s", inlets_.names[k]);
    return n;
}

void MathNode::syncExpression() {
    const std::string text = params.getText("Expression");
    if (text == appliedText_) return;
    appliedText_ = text;
    prog_ = compileExpression(text, prog_, inlets_);
    for (auto& st : state_) seedFormulaState(st.data(), prog_);
}

void MathNode::onTextChanged(const std::string& param, const std::string& text) {
    if (param != "Expression") return;
    appliedText_ = text;
    pendingProg_.publish(compileExpression(text, prog_, inlets_));
}

void MathNode::process(const float* const* in, int numIn, float* const* out, int numOut,
                       int numSamples, const Transport& transport) {
    if (numOut < 1) return;
    if (pendingProg_.adopt(prog_))
        for (auto& st : state_) seedFormulaState(st.data(), prog_);
    for (int c = 2; c < numOut; ++c)
        std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    float* dst = out[0];
    float* dstR = numOut > 1 ? out[1] : nullptr;
    std::sort(staged_.begin(), staged_.begin() + stagedCount_,
              [](const MidiEvent& a, const MidiEvent& b) { return a.sampleOffset < b.sampleOffset; });
    if (!prog_.valid()) {
        for (int i = 0; i < stagedCount_; ++i) held_.apply(staged_[(size_t) i]);
        stagedCount_ = 0;
        std::memset(dst, 0, sizeof(float) * (size_t) numSamples);
        if (dstR != nullptr) std::memset(dstR, 0, sizeof(float) * (size_t) numSamples);
        ctl_.store(0.5f, std::memory_order_relaxed);
        return;
    }
    const bool stereo = prog_.reads(fvCh);

    const std::array<float, 5> target = {(float) params.get("X", 0.5), (float) params.get("Y", 0.5),
                                         (float) params.get("Z", 0.5), (float) params.get("W", 0.5),
                                         (float) params.get("Freq", 220.0)};
    const bool autoPlay = params.get("Auto", 1.0) >= 0.5;
    if (!knobsPrimed_) {
        knobs_ = target;
        gateEnv_ = (autoPlay || held_.any()) ? 1.0f : 0.0f;
        knobsPrimed_ = true;
    }
    const float knobSlew = 1.0f - std::exp(-1.0f / (0.005f * (float) sampleRate_));
    const float gateSlew = 1.0f - std::exp(-1.0f / (0.003f * (float) sampleRate_));

    FormulaEnv env;
    env.rng = &rng_;
    env.dt = (float) (1.0 / sampleRate_);
    env.v[fvBpm] = (float) transport.tempo();
    env.v[fvSr] = (float) sampleRate_;
    for (int k = 0; k < FormulaInlets::kMax; ++k)
        env.v[fvIn0 + k] = inletValues_[(size_t) k].load(std::memory_order_relaxed);

    const float* a = (numIn > 0 && in != nullptr) ? in[0] : nullptr;
    const float* b = (numIn > 1 && in != nullptr) ? in[1] : nullptr;
    double t, beats;
    if (transport.playing()) { t = (double) transport.samplePosition() / sampleRate_;
                               beats = transport.beats(); }
    else                     { t = t_; beats = beat_; }
    const double dt = 1.0 / sampleRate_;
    const double beatsPerSample = transport.tempo() / kSecondsPerMinute / sampleRate_;

    int mi = 0;
    float last = 0.0f;
    for (int n = 0; n < numSamples; ++n) {
        while (mi < stagedCount_ && staged_[(size_t) mi].sampleOffset <= n)
            held_.apply(staged_[(size_t) mi++]);
        if (held_.any()) lastNote_ = held_.top();
        for (int k = 0; k < 5; ++k) knobs_[(size_t) k] += (target[(size_t) k] - knobs_[(size_t) k]) * knobSlew;
        const bool keyed = held_.any();
        const float gate = (autoPlay || keyed) ? 1.0f : 0.0f;
        gateEnv_ += (gate - gateEnv_) * gateSlew;

        env.v[fvA] = a != nullptr ? a[n] : 0.0f;
        env.v[fvB] = b != nullptr ? b[n] : 0.0f;
        env.v[fvX] = knobs_[0];
        env.v[fvY] = knobs_[1];
        env.v[fvZ] = knobs_[2];
        env.v[fvW] = knobs_[3];
        env.v[fvT] = (float) t;
        env.v[fvBeat] = (float) beats;
        if (autoPlay && !keyed) {
            env.v[fvFreq] = knobs_[4];
            env.v[fvNote] = (float) transport.tuning().midiNote(knobs_[4]);
            env.v[fvGate] = 1.0f;
            env.v[fvVel] = 1.0f;
        } else {
            env.v[fvNote] = (float) lastNote_;
            env.v[fvFreq] = (float) transport.tuning().hz(lastNote_);
            env.v[fvGate] = gate;
            env.v[fvVel] = (float) held_.lastVelocity / kMidiMaxF;
        }
        env.state = state_[0].data();
        env.v[fvCh] = 0.0f;
        env.v[fvPrev] = prev_[0];
        const float o = std::clamp(evalFormula(prog_, env), -16.0f, 16.0f);
        prev_[0] = o;
        float r = o;
        if (stereo) {
            env.state = state_[1].data();
            env.v[fvCh] = 1.0f;
            env.v[fvPrev] = prev_[1];
            r = std::clamp(evalFormula(prog_, env), -16.0f, 16.0f);
            prev_[1] = r;
        }
        dst[n] = o * gateEnv_;
        if (dstR != nullptr) dstR[n] = r * gateEnv_;
        last = o;
        t += dt;
        beats += beatsPerSample;
    }
    stagedCount_ = 0;
    t_ = t; beat_ = beats;
    if (numSamples > 0)
        ctl_.store(std::clamp((last + 1.0f) * 0.5f, 0.0f, 1.0f), std::memory_order_relaxed);
}

}
