#include "Mineral/Mineral.h"

#include <algorithm>
#include <cmath>

namespace hum {

double Mineral::geometricRatio(int facets) {
    const double n = std::max(3, facets);
    return std::clamp(2.0 * std::cos(3.14159265358979 / n), 1.01, 2.5);
}

void Mineral::process(const float* const*, int, float* const* out, int numOut,
                      int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];
    std::fill(L, L + numSamples, 0.0f);
    if (R != L) std::fill(R, R + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    const int facets = std::clamp((int) params.get("Facets", 5.0), 1, kMaxFacets);
    double ratio = std::max(1.01, params.get("Ratio", 1.62));
    if (params.get("Geometry", 0.0) >= 0.5) ratio = geometricRatio(facets);
    const double shine = std::clamp(params.get("Shine", 0.7), 0.1, 0.98);
    const double decay = std::max(1.0, params.get("Decay", 900.0)) * 0.001 * sr;
    const float strike = (float) params.get("Strike", 0.4);
    const double shimmer = params.get("Shimmer", 0.35);
    const float level = (float) params.get("Level", 0.8);

    MidiEvent ev[2 * MidiNode::kMaxMidiEventsPerBlock];
    int nEv = 0;
    {
        std::unique_lock<std::mutex> g(liveLock_, std::try_to_lock);
        if (g.owns_lock()) {
            for (int i = 0; i < liveCount_; ++i) { ev[nEv] = liveQ_[(size_t) i]; ev[nEv++].sampleOffset = 0; }
            liveCount_ = 0;
        }
    }
    for (int i = 0; i < stagedCount_; ++i) ev[nEv++] = staged_[(size_t) i];
    stagedCount_ = 0;

    auto strikeVoice = [&](int note, int vel) {
        Voice& v = voices_[(size_t) next_];
        next_ = (next_ + 1) % kVoices;
        v.t = 0;
        v.f0 = transport.tuning().hz(note);
        v.vel = (float) vel / 127.0f;
        v.phL.fill(0.0);
        v.phR.fill(0.0);
        sigHit_.store(1.0f, std::memory_order_relaxed);
    };
    for (int i = 0; i < nEv; ++i)
        if ((ev[i].data[0] & 0xF0) == 0x90 && ev[i].data[2] > 0)
            strikeVoice(ev[i].data[1], ev[i].data[2]);

    const long strikeLen = (long) (0.002 * sr);
    for (auto& v : voices_) {
        if (v.t < 0) continue;
        double active = 0.0;
        for (int i = 0; i < numSamples; ++i) {
            float sl = 0.0f, sr2 = 0.0f;
            double a = shine >= 0.97 ? 1.0 : 1.0;
            double facetAmp = 1.0;
            for (int p = 0; p < facets; ++p) {
                const double f = v.f0 * std::pow(ratio, p);
                if (f > sr * 0.45) break;
                const double env = std::exp(-(double) v.t / (decay / (1.0 + 0.8 * p)));
                const double det = shimmer * 0.004 * (p + 1);
                v.phL[(size_t) p] += f * (1.0 + det) / sr;
                v.phR[(size_t) p] += f * (1.0 - det) / sr;
                if (v.phL[(size_t) p] >= 1.0) v.phL[(size_t) p] -= 1.0;
                if (v.phR[(size_t) p] >= 1.0) v.phR[(size_t) p] -= 1.0;
                sl += (float) (std::sin(2.0 * 3.14159265358979 * v.phL[(size_t) p]) * env * facetAmp);
                sr2 += (float) (std::sin(2.0 * 3.14159265358979 * v.phR[(size_t) p]) * env * facetAmp);
                active = std::max(active, env * facetAmp);
                facetAmp *= shine;
            }
            if (v.t < strikeLen) {
                rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
                const float n = ((float) (rng_ & 0xFFFF) / 32768.0f - 1.0f)
                              * strike * (float) std::exp(-(double) v.t / (0.0006 * sr));
                sl += n;
                sr2 += n;
            }
            const float g = 0.35f * v.vel * level;
            L[i] += sl * g;
            if (R != L) R[i] += sr2 * g;
            ++v.t;
        }
        if (active < 1e-4) v.t = -1;
    }

    sigFacets_.store((float) facets, std::memory_order_relaxed);
    sigRatio_.store((float) ratio, std::memory_order_relaxed);
    sigShine_.store((float) shine, std::memory_order_relaxed);
    sigDecay_.store((float) params.get("Decay", 900.0), std::memory_order_relaxed);
    sigStrike_.store(strike, std::memory_order_relaxed);
    sigShimmer_.store((float) shimmer, std::memory_order_relaxed);
    const double dt = (double) numSamples / sr;
    float peak = 0.0f;
    for (int i = 0; i < numSamples; i += 4) peak = std::max(peak, std::abs(L[i]));
    const float fell = sigLevel_.load(std::memory_order_relaxed)
                     * (float) std::exp(-dt / 0.35);
    sigLevel_.store(std::min(1.0f, std::max(peak * 1.6f, fell)), std::memory_order_relaxed);
    sigHit_.store(sigHit_.load(std::memory_order_relaxed) * (float) std::exp(-dt / 0.18),
                  std::memory_order_relaxed);
}

int Mineral::sigil(Prim* out, int capacity, double t) {
    if (capacity <= 0) return 0;
    const int facets = std::clamp((int) sigFacets_.load(std::memory_order_relaxed), 1, kMaxFacets);
    const float ratio = sigRatio_.load(std::memory_order_relaxed);
    const float shine = sigShine_.load(std::memory_order_relaxed);
    const float decayN = std::clamp(sigDecay_.load(std::memory_order_relaxed) / 6000.0f, 0.0f, 1.0f);
    const float strike = sigStrike_.load(std::memory_order_relaxed);
    const float shimmer = std::clamp(sigShimmer_.load(std::memory_order_relaxed), 0.0f, 1.0f);
    const float level = sigLevel_.load(std::memory_order_relaxed);
    const float hit = sigHit_.load(std::memory_order_relaxed);

    const int n = facets + 2;
    const float elong = std::clamp((ratio - 1.01f) / 1.49f, 0.0f, 1.0f);
    const float base = 0.36f * (0.72f + 0.24f * level + 0.14f * hit);
    const float rx = base * (1.0f - 0.20f * elong);
    const float ry = base * (0.82f + 0.42f * elong);
    const double kTau = 6.283185307179586;
    double ang = 0.30 * t + 0.35 * hit;
    ang = std::floor(ang / kTau * 28.0) * (kTau / 28.0);

    auto vertex = [&](double a, float sx, float sy, float* xy) {
        xy[0] = 0.5f + rx * sx * (float) std::cos(a);
        xy[1] = 0.5f + ry * sy * (float) std::sin(a);
    };
    auto gem = [&](Prim& p, double a, float scale) {
        p.kind = 0; p.closed = true; p.points = n;
        for (int k = 0; k < n; ++k) vertex(a + kTau * k / n, scale, scale, p.pt[k]);
    };
    int c = 0;
    auto next = [&]() -> Prim* { return c < capacity ? &out[c++] : nullptr; };

    if (auto* p = next()) {
        gem(*p, ang - 0.35, 0.97f);
        p->role = 0; p->alpha = 0.08f + 0.30f * decayN; p->size = 0.02f;
    }
    if (hit > 0.02f) {
        if (auto* p = next()) {
            gem(*p, ang, 1.0f);
            p->role = 3; p->filled = true;
            p->alpha = hit * (0.20f + 0.55f * strike);
        }
    }
    if (shimmer > 0.01f) {
        if (auto* p = next()) {
            gem(*p, ang + 0.10 + 0.06 * std::sin(3.1 * t), 1.0f);
            p->role = 2; p->alpha = 0.10f + 0.30f * shimmer; p->size = 0.02f;
        }
    }
    if (auto* p = next()) {
        gem(*p, ang, 1.0f);
        p->role = 1; p->alpha = 0.95f; p->size = 0.028f;
    }
    for (int k = 0; k < facets; ++k) {
        auto* p = next();
        if (p == nullptr) break;
        p->kind = 0; p->points = 2; p->role = 0; p->alpha = 0.30f; p->size = 0.016f;
        p->pt[0][0] = 0.5f; p->pt[0][1] = 0.5f;
        vertex(ang + kTau * (k * n / facets % n) / n, 1.0f, 1.0f, p->pt[1]);
    }
    if (auto* p = next()) {
        const double ga = -0.7 * t;
        p->kind = 0; p->points = 2; p->role = 3; p->size = 0.05f;
        p->alpha = shine * (0.15f + 0.30f * (0.5f + 0.5f * (float) std::sin(0.9 * t + 1.0)));
        vertex(ga, 0.85f, 0.85f, p->pt[0]);
        vertex(ga + kTau * 0.18, 0.55f, 0.55f, p->pt[1]);
    }
    const int sparkles = (int) std::lround(shimmer * 4.0f);
    for (int k = 0; k < sparkles; ++k) {
        auto* p = next();
        if (p == nullptr) break;
        const float tw = 0.5f + 0.5f * (float) std::sin(5.0 * t + 2.1 * k);
        p->kind = 1; p->role = 3; p->alpha = tw * (0.25f + 0.55f * level);
        p->size = 0.02f + 0.015f * tw;
        vertex(ang + kTau * (k * 2 + 1) / (2.0 * n) * 2.0, 1.0f, 1.0f, p->pt[0]);
    }
    return c;
}

}
