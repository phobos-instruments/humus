#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

#include "hum/Number.h"
#include "hum/dsp/DspMath.h"

namespace hum {

enum class FOp : std::uint8_t {
    PushK, PushVar,
    Neg, Add, Sub, Mul, Div, Mod,
    Lt, Gt, Le, Ge, Eq, Ne,
    Sin, Cos, Tan, Tanh, Asin, Exp, Log, Log2, Sqrt, Abs, Floor, Ceil, Fract,
    Saw, Tri, Sqr,
    Pow, Min, Max, Step, Pulse,
    Clamp, Mix, Smoothstep, If,
    Noise, Ph, Harm, Env,
};

enum FVar : std::uint8_t {
    fvA, fvB,
    fvX, fvY, fvZ, fvW,
    fvT, fvBeat, fvBpm, fvSr,
    fvPrev,
    fvNote, fvFreq, fvGate, fvVel,
    fvCh,
    fvCount
};

struct FormulaProgram {
    static constexpr int kMaxOps = 128;
    static constexpr int kMaxStack = 32;
    static constexpr int kMaxDepth = 32;
    struct Op {
        FOp code = FOp::PushK;
        std::uint8_t var = 0;
        float k = 0.0f;
    };
    Op ops[kMaxOps];
    int n = 0;
    bool valid() const { return n > 0; }
    bool reads(FVar v) const {
        for (int i = 0; i < n; ++i) if (ops[i].code == FOp::PushVar && ops[i].var == v) return true;
        return false;
    }
};

struct FormulaError {
    const char* msg = nullptr;
    int pos = -1;
};

struct FormulaEnv {
    float v[fvCount] = {};
    std::uint32_t* rng = nullptr;
    float* state = nullptr;
    float dt = 0.0f;
};

inline float formulaNoise(std::uint32_t& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (float) (s & 0xffffffu) * (2.0f / 16777215.0f) - 1.0f;
}

inline float evalFormula(const FormulaProgram& p, const FormulaEnv& env) {
    float st[FormulaProgram::kMaxStack];
    int sp = 0;
    for (int i = 0; i < p.n; ++i) {
        const auto& op = p.ops[i];
        switch (op.code) {
            case FOp::PushK:   st[sp++] = op.k; break;
            case FOp::PushVar: st[sp++] = env.v[op.var]; break;
            case FOp::Neg:     st[sp - 1] = -st[sp - 1]; break;
            case FOp::Add:     --sp; st[sp - 1] += st[sp]; break;
            case FOp::Sub:     --sp; st[sp - 1] -= st[sp]; break;
            case FOp::Mul:     --sp; st[sp - 1] *= st[sp]; break;
            case FOp::Div:     --sp; st[sp - 1] = st[sp] != 0.0f ? st[sp - 1] / st[sp] : 0.0f; break;
            case FOp::Mod:     --sp; st[sp - 1] = st[sp] != 0.0f ? std::fmod(st[sp - 1], st[sp]) : 0.0f; break;
            case FOp::Lt:      --sp; st[sp - 1] = st[sp - 1] <  st[sp] ? 1.0f : 0.0f; break;
            case FOp::Gt:      --sp; st[sp - 1] = st[sp - 1] >  st[sp] ? 1.0f : 0.0f; break;
            case FOp::Le:      --sp; st[sp - 1] = st[sp - 1] <= st[sp] ? 1.0f : 0.0f; break;
            case FOp::Ge:      --sp; st[sp - 1] = st[sp - 1] >= st[sp] ? 1.0f : 0.0f; break;
            case FOp::Eq:      --sp; st[sp - 1] = st[sp - 1] == st[sp] ? 1.0f : 0.0f; break;
            case FOp::Ne:      --sp; st[sp - 1] = st[sp - 1] != st[sp] ? 1.0f : 0.0f; break;
            case FOp::Sin:     st[sp - 1] = std::sin(st[sp - 1]); break;
            case FOp::Cos:     st[sp - 1] = std::cos(st[sp - 1]); break;
            case FOp::Tan:     st[sp - 1] = std::tan(st[sp - 1]); break;
            case FOp::Tanh:    st[sp - 1] = std::tanh(st[sp - 1]); break;
            case FOp::Asin:    st[sp - 1] = std::asin(st[sp - 1]); break;
            case FOp::Exp:     st[sp - 1] = std::exp(st[sp - 1]); break;
            case FOp::Log:     st[sp - 1] = std::log(st[sp - 1]); break;
            case FOp::Log2:    st[sp - 1] = std::log2(st[sp - 1]); break;
            case FOp::Sqrt:    st[sp - 1] = std::sqrt(st[sp - 1]); break;
            case FOp::Abs:     st[sp - 1] = std::abs(st[sp - 1]); break;
            case FOp::Floor:   st[sp - 1] = std::floor(st[sp - 1]); break;
            case FOp::Ceil:    st[sp - 1] = std::ceil(st[sp - 1]); break;
            case FOp::Fract:   st[sp - 1] = st[sp - 1] - std::floor(st[sp - 1]); break;
            case FOp::Saw: {
                const float f = st[sp - 1] - std::floor(st[sp - 1]);
                st[sp - 1] = 2.0f * f - 1.0f;
                break;
            }
            case FOp::Tri: {
                const float f = st[sp - 1] - std::floor(st[sp - 1]);
                st[sp - 1] = 1.0f - 4.0f * std::abs(f - 0.5f);
                break;
            }
            case FOp::Sqr: {
                const float f = st[sp - 1] - std::floor(st[sp - 1]);
                st[sp - 1] = f < 0.5f ? 1.0f : -1.0f;
                break;
            }
            case FOp::Pow:     --sp; st[sp - 1] = std::pow(st[sp - 1], st[sp]); break;
            case FOp::Min:     --sp; st[sp - 1] = std::min(st[sp - 1], st[sp]); break;
            case FOp::Max:     --sp; st[sp - 1] = std::max(st[sp - 1], st[sp]); break;
            case FOp::Step:    --sp; st[sp - 1] = st[sp] >= st[sp - 1] ? 1.0f : 0.0f; break;
            case FOp::Pulse: {
                --sp;
                const float f = st[sp - 1] - std::floor(st[sp - 1]);
                st[sp - 1] = f < st[sp] ? 1.0f : -1.0f;
                break;
            }
            case FOp::Clamp: {
                sp -= 2;
                const float lo = st[sp], hi = st[sp + 1];
                st[sp - 1] = std::min(std::max(st[sp - 1], lo), hi);
                break;
            }
            case FOp::Mix: {
                sp -= 2;
                st[sp - 1] = st[sp - 1] + (st[sp] - st[sp - 1]) * st[sp + 1];
                break;
            }
            case FOp::Smoothstep: {
                sp -= 2;
                const float e0 = st[sp - 1], e1 = st[sp], x = st[sp + 1];
                float u = e1 != e0 ? (x - e0) / (e1 - e0) : 0.0f;
                u = std::min(std::max(u, 0.0f), 1.0f);
                st[sp - 1] = u * u * (3.0f - 2.0f * u);
                break;
            }
            case FOp::If: {
                sp -= 2;
                st[sp - 1] = st[sp - 1] != 0.0f ? st[sp] : st[sp + 1];
                break;
            }
            case FOp::Noise:
                st[sp++] = env.rng != nullptr ? formulaNoise(*env.rng) : 0.0f;
                break;
            case FOp::Ph: {
                if (env.state == nullptr) { st[sp - 1] = 0.0f; break; }
                float& ph = env.state[op.var];
                const float next = ph + st[sp - 1] * env.dt;
                ph = std::isfinite(next) ? next - std::floor(next) : 0.0f;
                st[sp - 1] = ph;
                break;
            }
            case FOp::Harm: {
                sp -= 2;
                const float hz = st[sp - 1];
                const float nf = std::min(32.0f, std::max(1.0f, st[sp]));
                const int n = (int) std::ceil(nf - 1.0e-6f);
                const float tilt = st[sp + 1];
                if (env.state == nullptr || env.dt <= 0.0f) { st[sp - 1] = 0.0f; break; }
                float& ph = env.state[op.var];
                const float next = ph + hz * env.dt;
                ph = std::isfinite(next) ? next - std::floor(next) : 0.0f;
                const float nyquist = 0.5f / env.dt;
                float sum = 0.0f, norm = 0.0f;
                for (int k = 1; k <= n; ++k) {
                    const float amp = std::pow((float) k, -tilt) * std::min(1.0f, nf - (float) (k - 1));
                    norm += amp * amp;
                    if ((float) k * std::abs(hz) < nyquist)
                        sum += amp * std::sin(kTwoPiF * (float) k * ph);
                }
                st[sp - 1] = norm > 0.0f ? sum / std::sqrt(norm) : 0.0f;
                break;
            }
            case FOp::Env: {
                sp -= 2;
                const float gate = st[sp - 1];
                if (env.state == nullptr || env.dt <= 0.0f) { st[sp - 1] = 0.0f; break; }
                float& lvl = env.state[op.var];
                const float secs = std::max(1.0e-4f, gate > lvl ? st[sp] : st[sp + 1]);
                lvl += (gate - lvl) * (1.0f - std::exp(-env.dt / secs));
                if (!std::isfinite(lvl)) lvl = 0.0f;
                st[sp - 1] = lvl;
                break;
            }
        }
    }
    const float r = sp > 0 ? st[sp - 1] : 0.0f;
    return std::isfinite(r) ? r : 0.0f;
}

namespace formula_detail {

struct Fn { const char* name; FOp op; int arity; };
inline const Fn* functions(int& count) {
    static const Fn k[] = {
        {"sin", FOp::Sin, 1},   {"cos", FOp::Cos, 1},     {"tan", FOp::Tan, 1},
        {"tanh", FOp::Tanh, 1}, {"asin", FOp::Asin, 1},   {"exp", FOp::Exp, 1},
        {"log", FOp::Log, 1},   {"log2", FOp::Log2, 1},   {"sqrt", FOp::Sqrt, 1},
        {"abs", FOp::Abs, 1},   {"floor", FOp::Floor, 1}, {"ceil", FOp::Ceil, 1},
        {"fract", FOp::Fract, 1},
        {"saw", FOp::Saw, 1},   {"tri", FOp::Tri, 1},     {"sqr", FOp::Sqr, 1},
        {"pow", FOp::Pow, 2},   {"min", FOp::Min, 2},     {"max", FOp::Max, 2},
        {"step", FOp::Step, 2}, {"pulse", FOp::Pulse, 2},
        {"clamp", FOp::Clamp, 3}, {"mix", FOp::Mix, 3},
        {"smoothstep", FOp::Smoothstep, 3}, {"if", FOp::If, 3},
        {"noise", FOp::Noise, 0}, {"ph", FOp::Ph, 1}, {"harm", FOp::Harm, 3},
        {"env", FOp::Env, 3},
    };
    count = (int) (sizeof(k) / sizeof(k[0]));
    return k;
}

struct Var { const char* name; FVar var; };
inline const Var* variables(int& count) {
    static const Var k[] = {
        {"a", fvA}, {"b", fvB}, {"x", fvX}, {"y", fvY}, {"z", fvZ}, {"w", fvW},
        {"t", fvT}, {"beat", fvBeat}, {"bpm", fvBpm}, {"sr", fvSr}, {"prev", fvPrev},
        {"note", fvNote}, {"freq", fvFreq}, {"gate", fvGate}, {"vel", fvVel}, {"ch", fvCh},
    };
    count = (int) (sizeof(k) / sizeof(k[0]));
    return k;
}

struct Konst { const char* name; float k; };
inline const Konst* constants(int& count) {
    static const Konst k[] = {
        {"pi", kPiF},
        {"tau", kTwoPiF},
        {"e", 2.71828182845904523536f},
    };
    count = (int) (sizeof(k) / sizeof(k[0]));
    return k;
}

struct Parser {
    const char* s;
    int i = 0;
    int depth = 0;
    FormulaProgram prog;
    int stack = 0;
    const char* err = nullptr;
    int errPos = -1;

    void fail(const char* m) {
        if (err == nullptr) { err = m; errPos = i; }
    }
    void skip() {
        while (s[i] == ' ' || s[i] == '\t') ++i;
    }
    bool emit(FOp code, std::uint8_t var, float k, int stackEffect) {
        if (err != nullptr) return false;
        if (prog.n >= FormulaProgram::kMaxOps) { fail("expression too long"); return false; }
        prog.ops[prog.n++] = {code, var, k};
        stack += stackEffect;
        if (stack > FormulaProgram::kMaxStack) { fail("expression too deep"); return false; }
        return true;
    }

    bool ident(char* buf, int cap) {
        skip();
        if (!(s[i] >= 'a' && s[i] <= 'z')) return false;
        int n = 0;
        while ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9')) {
            if (n + 1 >= cap) { fail("name too long"); return false; }
            buf[n++] = s[i++];
        }
        buf[n] = 0;
        return true;
    }

    void primary() {
        skip();
        const char c = s[i];
        if (c == '(') {
            ++i;
            expr();
            skip();
            if (s[i] == ')') ++i;
            else fail("expected )");
            return;
        }
        if ((c >= '0' && c <= '9') || c == '.') {
            const char* end = nullptr;
            const float k = (float) scanDouble(s + i, &end);
            if (end == s + i) { fail("bad number"); return; }
            i = (int) (end - s);
            emit(FOp::PushK, 0, k, +1);
            return;
        }
        char name[16];
        if (ident(name, sizeof(name))) {
            skip();
            if (s[i] == '(') {
                int fnCount = 0;
                const Fn* fns = functions(fnCount);
                for (int f = 0; f < fnCount; ++f)
                    if (std::strcmp(fns[f].name, name) == 0) {
                        ++i;
                        int args = 0;
                        skip();
                        if (s[i] == ')') { ++i; }
                        else {
                            for (;;) {
                                expr();
                                ++args;
                                skip();
                                if (s[i] == ',') { ++i; continue; }
                                if (s[i] == ')') { ++i; break; }
                                fail("expected , or )");
                                return;
                            }
                        }
                        if (err != nullptr) return;
                        if (args != fns[f].arity) { fail("wrong number of arguments"); return; }
                        emit(fns[f].op, (std::uint8_t) prog.n, 0.0f, 1 - fns[f].arity);
                        return;
                    }
            }
            int varCount = 0;
            const Var* vars = variables(varCount);
            for (int v = 0; v < varCount; ++v)
                if (std::strcmp(vars[v].name, name) == 0) {
                    emit(FOp::PushVar, (std::uint8_t) vars[v].var, 0.0f, +1);
                    return;
                }
            int kCount = 0;
            const Konst* ks = constants(kCount);
            for (int v = 0; v < kCount; ++v)
                if (std::strcmp(ks[v].name, name) == 0) {
                    emit(FOp::PushK, 0, ks[v].k, +1);
                    return;
                }
            fail(s[i] == '(' ? "unknown function" : "unknown name");
            return;
        }
        fail("expected a value");
    }

    void unary() {
        skip();
        if (s[i] == '-') {
            ++i;
            unary();
            emit(FOp::Neg, 0, 0.0f, 0);
            return;
        }
        primary();
    }

    void mul() {
        unary();
        for (;;) {
            skip();
            const char c = s[i];
            if (c == '*' || c == '/' || c == '%') {
                ++i;
                unary();
                emit(c == '*' ? FOp::Mul : c == '/' ? FOp::Div : FOp::Mod, 0, 0.0f, -1);
                if (err != nullptr) return;
                continue;
            }
            if ((c >= 'a' && c <= 'z') || c == '(') {
                primary();
                emit(FOp::Mul, 0, 0.0f, -1);
                if (err != nullptr) return;
                continue;
            }
            return;
        }
    }

    void add() {
        mul();
        for (;;) {
            skip();
            const char c = s[i];
            if (c != '+' && c != '-') return;
            ++i;
            mul();
            emit(c == '+' ? FOp::Add : FOp::Sub, 0, 0.0f, -1);
            if (err != nullptr) return;
        }
    }

    void rel() {
        add();
        for (;;) {
            skip();
            const char c = s[i];
            if (c != '<' && c != '>') return;
            const bool orEq = s[i + 1] == '=';
            i += orEq ? 2 : 1;
            add();
            emit(c == '<' ? (orEq ? FOp::Le : FOp::Lt) : (orEq ? FOp::Ge : FOp::Gt),
                 0, 0.0f, -1);
            if (err != nullptr) return;
        }
    }

    void expr() {
        if (++depth > FormulaProgram::kMaxDepth) { fail("expression too nested"); --depth; return; }
        rel();
        for (;;) {
            skip();
            const bool eq = s[i] == '=' && s[i + 1] == '=';
            const bool ne = s[i] == '!' && s[i + 1] == '=';
            if (!eq && !ne) break;
            i += 2;
            rel();
            emit(eq ? FOp::Eq : FOp::Ne, 0, 0.0f, -1);
            if (err != nullptr) break;
        }
        --depth;
    }
};

}

enum class FormulaRole { Modulator, Voice, Effect };
struct FormulaShape { FormulaRole role = FormulaRole::Modulator; bool timed = false; };
inline FormulaShape formulaShape(const FormulaProgram& p) {
    bool inlet = false, osc = false, midi = false, timed = false;
    for (int i = 0; i < p.n; ++i) {
        const auto& op = p.ops[i];
        osc |= op.code == FOp::Ph || op.code == FOp::Harm;
        if (op.code != FOp::PushVar) continue;
        inlet |= op.var == fvA || op.var == fvB;
        midi |= op.var == fvFreq || op.var == fvNote || op.var == fvGate || op.var == fvVel;
        timed |= op.var == fvT || op.var == fvBeat;
    }
    return {inlet ? FormulaRole::Effect : (osc || midi) ? FormulaRole::Voice : FormulaRole::Modulator, timed};
}
inline const char* formulaRoleName(FormulaRole r) {
    return r == FormulaRole::Effect ? "EFFECT" : r == FormulaRole::Voice ? "VOICE" : "MODULATOR";
}

inline bool compileFormula(const char* src, FormulaProgram& out, FormulaError* err = nullptr) {
    formula_detail::Parser p{src != nullptr ? src : ""};
    p.skip();
    if (p.s[p.i] == 0) {
        if (err != nullptr) *err = {"empty expression", 0};
        return false;
    }
    p.expr();
    p.skip();
    if (p.err == nullptr && p.s[p.i] != 0) p.fail("unexpected text after expression");
    if (p.err == nullptr && p.stack != 1) p.fail("incomplete expression");
    if (p.err != nullptr) {
        if (err != nullptr) *err = {p.err, p.errPos};
        return false;
    }
    out = p.prog;
    if (err != nullptr) *err = {};
    return true;
}

}
