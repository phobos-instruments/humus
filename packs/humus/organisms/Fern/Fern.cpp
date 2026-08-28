#include "Fern/Fern.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr double kMaxSeconds = 4.0;
constexpr double kFadeMs = 30.0;
constexpr float kMaxRate = 0.25f;
constexpr double kEaseMs = 60.0;
constexpr double kDampHi = 18000.0, kDampLo = 700.0;
constexpr double kBodyLo = 20.0,    kBodyHi = 700.0;
constexpr double kQ = 0.7071;
}

void Fern::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    const int n = (int) (sampleRate * kMaxSeconds) + 8;
    chans_.assign((size_t) std::max(1, ch_), Chan{});
    for (auto& c : chans_) {
        c.line.prepare(n);
        c.line.setInterp(DelayLine::Interp::Cubic);
    }
    maxDelay_ = n - 8;
    fadeLen_ = std::max(1, (int) (kFadeMs * 0.001 * sampleRate));
    reset();
}

void Fern::reset() {
    for (auto& c : chans_) {
        c.line.clear();
        c.lp.reset();
        c.hp.reset();
        c.pos = -1.0f;
        c.tap[0] = c.tap[1] = 0.0f;
        c.cur = 0;
        c.xf = 1.0f;
        c.pending = -1.0f;
        c.last = 0.0f;
    }
}

float Fern::glideTape(Chan& c, float target) const {
    if (c.pos < 0.0f) { c.pos = target; return target; }
    if (std::abs(target - c.pos) < 1.0e-3f) { c.pos = target; return target; }
    const float ease = (float) smoothCoeff(kEaseMs, sampleRate_);
    const float want = ease * c.pos + (1.0f - ease) * target;
    c.pos += std::min(std::max(want - c.pos, -kMaxRate), kMaxRate);
    return c.pos;
}

float Fern::readFade(Chan& c, float target) {
    if (c.pos < 0.0f) {
        c.pos = target;
        c.tap[0] = c.tap[1] = target;
        c.cur = 0;
        c.xf = 1.0f;
    }
    if (std::abs(target - c.tap[c.cur]) > 1.0e-3f) {
        if (c.xf >= 1.0f) {
            c.cur ^= 1;
            c.tap[c.cur] = target;
            c.xf = 0.0f;
            c.pending = -1.0f;
        } else {
            c.pending = target;
        }
    }
    const float a = c.line.read(c.tap[c.cur ^ 1]);
    const float b = c.line.read(c.tap[c.cur]);
    const float y = a + c.xf * (b - a);
    if (c.xf < 1.0f) {
        c.xf = std::min(1.0f, c.xf + 1.0f / (float) fadeLen_);
        if (c.xf >= 1.0f && c.pending >= 0.0f) {
            const float p = c.pending;
            c.pending = -1.0f;
            c.cur ^= 1;
            c.tap[c.cur] = p;
            c.xf = 0.0f;
        }
    }
    c.pos = c.tap[c.cur];
    return y;
}

void Fern::process(const float* const* in, int numIn, float* const* out, int numOut,
                     int numSamples, const Transport& transport) {
    if (chans_.empty()) return;

    const bool sync = params.get("Sync", 0.0) >= 0.5;
    const bool pingPong = params.get("Mode", 0.0) >= 0.5;
    const bool fade = params.get("Glide", 0.0) >= 0.5;
    const float fb = (float) std::clamp(params.get("Feedback", 0.35), 0.0, 0.95);
    const float mix = (float) std::clamp(params.get("Mix", 0.35), 0.0, 1.0);
    const float drive = (float) std::clamp(params.get("Drive", 0.15), 0.0, 1.0);
    const double damp = std::clamp(params.get("Damp", 0.35), 0.0, 1.0);
    const double body = std::clamp(params.get("Body", 0.2), 0.0, 1.0);
    const float spread = (float) std::clamp(params.get("Spread", 0.0), -1.0, 1.0);

    sigFeedback_.store(fb, std::memory_order_relaxed);
    sigDamp_.store((float) damp, std::memory_order_relaxed);
    sigMix_.store(mix, std::memory_order_relaxed);

    double base;
    if (sync) {
        const double unit = transport.rhythmicUnitToSamples(params.getText("SyncUnit", "1/8"));
        base = std::max(1.0, params.get("SyncMultiplier", 1.0)) * unit;
    } else {
        base = std::clamp(params.get("Time", 375.0), 1.0, 4000.0) * 0.001 * sampleRate_;
    }
    base = std::clamp(base, 2.0, (double) maxDelay_);

    const double lpHz = kDampHi * std::pow(kDampLo / kDampHi, damp);
    const double hpHz = kBodyLo * std::pow(kBodyHi / kBodyLo, body);
    const double gLp = SvfTpt::gFor(lpHz, sampleRate_);
    const double gHp = SvfTpt::gFor(hpHz, sampleRate_);
    const double k = 1.0 / kQ;
    const float sat = 1.0f + drive * 8.0f;

    static constexpr int kMaxChans = 8;
    const int nc = std::min((int) chans_.size(), kMaxChans);
    for (int n = 0; n < numSamples; ++n) {
        float wet[kMaxChans] = {0.0f};
        for (int c = 0; c < nc; ++c) {
            auto& ch = chans_[(size_t) c];
            const double want = c == 1 ? base * (1.0 + spread * 0.5) : base;
            const float target = (float) std::clamp(want, 2.0, (double) maxDelay_);
            wet[c] = fade ? readFade(ch, target) : ch.line.read(glideTape(ch, target));
        }
        for (int c = 0; c < nc; ++c) {
            auto& ch = chans_[(size_t) c];
            const float x = (c < numIn && in[c]) ? in[c][n] : 0.0f;
            const int src = pingPong && nc > 1 ? (c ^ 1) : c;
            float s = wet[src];
            s = (float) ch.hp.process(s, gHp, k).hp;
            s = (float) ch.lp.process(s, gLp, k).lp;
            if (drive > 0.0f) {
                const float shaped = std::tanh(s * sat) / sat;
                s += drive * (shaped - s);
            }
            ch.line.write(x + fb * s);
            if (c < numOut && out[c]) out[c][n] = (1.0f - mix) * x + mix * wet[c];
        }
    }
    for (int c = (int) chans_.size(); c < numOut; ++c)
        if (out[c]) std::fill(out[c], out[c] + numSamples, 0.0f);
}

int Fern::sigil(Prim* out, int capacity, double t) {
    if (capacity <= 0) return 0;
    const float fb = std::clamp(sigFeedback_.load(std::memory_order_relaxed), 0.0f, 0.95f);
    const float damp = std::clamp(sigDamp_.load(std::memory_order_relaxed), 0.0f, 1.0f);
    const float mix = std::clamp(sigMix_.load(std::memory_order_relaxed), 0.0f, 1.0f);

    static constexpr float kCubics[3][4][2] = {
        {{4, 19}, {4, 8}, {9, 3}, {15, 4}},
        {{15, 4}, {19, 5}, {19, 9}, {16, 11}},
        {{16, 11}, {13, 13}, {11, 11}, {12, 9}},
    };
    const float sway = 0.05f * (float) std::sin(0.5 * t);
    const float curl = 0.75f + 0.4f * damp;
    float px[16], py[16];
    int np = 0;
    for (int cu = 0; cu < 3; ++cu) {
        const auto& C = kCubics[cu];
        for (int i = (cu == 0 ? 0 : 1); i <= 5; ++i) {
            const float u = (float) i / 5.0f, v = 1.0f - u;
            float x = v * v * v * C[0][0] + 3 * v * v * u * C[1][0]
                    + 3 * v * u * u * C[2][0] + u * u * u * C[3][0];
            float y = v * v * v * C[0][1] + 3 * v * v * u * C[1][1]
                    + 3 * v * u * u * C[2][1] + u * u * u * C[3][1];
            x /= 22.0f; y /= 22.0f;
            if (cu == 2) {
                x = 0.68f + (x - 0.68f) * curl;
                y = 0.45f + (y - 0.45f) * curl;
            }
            const float dx = x - 0.18f, dy = y - 0.86f;
            px[np] = 0.18f + dx - sway * dy;
            py[np] = 0.86f + dy + sway * dx;
            ++np;
        }
    }

    int c = 0;
    auto next = [&]() -> Prim* { return c < capacity ? &out[c++] : nullptr; };

    if (auto* p = next()) {
        p->kind = 0; p->role = 2; p->alpha = 0.95f; p->size = 0.05f;
        p->points = np;
        for (int i = 0; i < np; ++i) { p->pt[i][0] = px[i]; p->pt[i][1] = py[i]; }
    }
    const int leaves = 3 + (int) std::lround(fb / 0.95f * 4.0f);
    for (int l = 0; l < leaves; ++l) {
        auto* p = next();
        if (!p) break;
        const int at = 1 + l;
        if (at >= np - 6) break;
        const float side = (l % 2 == 0) ? 1.0f : -1.0f;
        const float tx = px[at + 1] - px[at - 1], ty = py[at + 1] - py[at - 1];
        const float len = std::sqrt(tx * tx + ty * ty) + 1e-6f;
        const float nx = -ty / len, ny = tx / len;
        const float reach = 0.10f + 0.03f * (float) (l % 3);
        p->kind = 0; p->role = 1; p->alpha = 0.65f; p->size = 0.035f; p->points = 2;
        p->pt[0][0] = px[at]; p->pt[0][1] = py[at];
        p->pt[1][0] = px[at] + side * nx * reach;
        p->pt[1][1] = py[at] + side * ny * reach;
    }
    if (auto* p = next()) {
        p->kind = 1; p->role = 3; p->alpha = 0.25f + 0.55f * mix; p->size = 0.05f;
        p->pt[0][0] = px[np - 1]; p->pt[0][1] = py[np - 1];
    }
    return c;
}

}
