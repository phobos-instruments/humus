// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/assistant/RecipeSynth.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hum {

juce::String recipeSystemPrompt() {
    return
        "You design SOUNDS for a sampler as synthesis recipes. Reply with ONLY a JSON "
        "object of this exact shape (no prose, no fences):\n"
        "{\"sounds\": [ {\"name\": \"snappy kick\", \"duration\": 0.6, \"layers\": [\n"
        "  {\"wave\": \"sine|saw|square|triangle|noise\", \"freq\": 120, \"freqEnd\": 45,\n"
        "   \"level\": 0.9, \"attack\": 2, \"decay\": 350, \"lowpass\": 0} ] } ] }\n"
        "Rules: 2-4 sounds, each 1-6 layers. duration in seconds (0.05-4). freq/freqEnd "
        "in Hz - freqEnd > 0 glides the pitch there exponentially (kicks, toms, zaps; "
        "omit or 0 for none). attack/decay in ms (the decay is exponential, to silence). "
        "lowpass in Hz filters that layer (0/omit = off); noise + lowpass makes hats, "
        "snares, wind. Layer levels 0-1; layers sum. Design deliberately: body layer + "
        "transient layer + noise layer is a good skeleton for percussive sounds.";
}

std::vector<Recipe> parseRecipes(const juce::String& reply) {
    std::vector<Recipe> out;
    const int open = reply.indexOfChar('{');
    const int close = reply.lastIndexOfChar('}');
    if (open < 0 || close <= open) return out;
    const juce::var parsed = juce::JSON::parse(reply.substring(open, close + 1));
    const auto* sounds = parsed["sounds"].getArray();
    if (!sounds) return out;

    for (const auto& sv : *sounds) {
        if (out.size() >= 4) break;
        Recipe r;
        r.name = sv["name"].toString().toStdString();
        if (r.name.empty()) r.name = "sound " + std::to_string(out.size() + 1);
        r.durationSec = sv.hasProperty("duration") ? (double) sv["duration"] : 1.0;
        r.durationSec = std::clamp(r.durationSec, 0.05, 4.0);
        if (const auto* layers = sv["layers"].getArray())
            for (const auto& lv : *layers) {
                if (r.layers.size() >= 6) break;
                RecipeLayer l;
                const auto w = lv["wave"].toString();
                l.wave = w == "saw"      ? RecipeLayer::Wave::Saw
                       : w == "square"   ? RecipeLayer::Wave::Square
                       : w == "triangle" ? RecipeLayer::Wave::Triangle
                       : w == "noise"    ? RecipeLayer::Wave::Noise
                                         : RecipeLayer::Wave::Sine;
                if (lv.hasProperty("freq")) l.freq = std::clamp((double) lv["freq"], 5.0, 18000.0);
                if (lv.hasProperty("freqEnd") && (double) lv["freqEnd"] > 0.0)
                    l.freqEnd = std::clamp((double) lv["freqEnd"], 5.0, 18000.0);
                if (lv.hasProperty("level")) l.level = std::clamp((double) lv["level"], 0.0, 1.0);
                if (lv.hasProperty("attack")) l.attackMs = std::clamp((double) lv["attack"], 0.0, 2000.0);
                if (lv.hasProperty("decay")) l.decayMs = std::clamp((double) lv["decay"], 5.0, 8000.0);
                if (lv.hasProperty("lowpass") && (double) lv["lowpass"] > 0.0)
                    l.lowpassHz = std::clamp((double) lv["lowpass"], 30.0, 18000.0);
                r.layers.push_back(l);
            }
        if (!r.layers.empty()) out.push_back(std::move(r));
    }
    return out;
}

std::vector<float> renderRecipe(const Recipe& r, double sampleRate) {
    const int n = (int) (std::clamp(r.durationSec, 0.05, 4.0) * sampleRate);
    std::vector<float> out((size_t) n, 0.0f);
    std::uint32_t rng = 0x1234567u;
    auto frand = [&rng] {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) (rng & 0xFFFFFF) / (float) 0x7FFFFF - 1.0f;
    };

    for (const auto& l : r.layers) {
        double phase = 0.0;
        float lpState = 0.0f;
        const float lpCoef = l.lowpassHz > 0.0
            ? (float) std::exp(-2.0 * juce::MathConstants<double>::pi * l.lowpassHz / sampleRate)
            : 0.0f;
        const double glideRatio = l.freqEnd > 0.0 ? l.freqEnd / l.freq : 1.0;
        const int attackN = std::max(1, (int) (l.attackMs * 0.001 * sampleRate));
        const double decayCoef = std::exp(-3.0 / (l.decayMs * 0.001 * sampleRate));
        double env = 0.0, envDecay = 1.0;

        for (int i = 0; i < n; ++i) {
            const double t = (double) i / n;
            const double freq = l.freq * std::pow(glideRatio, t);
            phase += freq / sampleRate;
            if (phase >= 1.0) phase -= 1.0;

            float s;
            switch (l.wave) {
                case RecipeLayer::Wave::Saw:      s = (float) (2.0 * phase - 1.0); break;
                case RecipeLayer::Wave::Square:   s = phase < 0.5 ? 1.0f : -1.0f; break;
                case RecipeLayer::Wave::Triangle: s = (float) (phase < 0.5 ? 4.0 * phase - 1.0
                                                                           : 3.0 - 4.0 * phase); break;
                case RecipeLayer::Wave::Noise:    s = frand(); break;
                default: s = (float) std::sin(2.0 * juce::MathConstants<double>::pi * phase); break;
            }
            if (lpCoef > 0.0f) { lpState = s + lpCoef * (lpState - s); s = lpState; }

            if (i < attackN) env = (double) i / attackN;
            else { envDecay *= decayCoef; env = envDecay; }
            out[(size_t) i] += (float) (s * env * l.level);
        }
    }

    float peak = 0.0f;
    for (float v : out) peak = std::max(peak, std::abs(v));
    if (peak > 1.0e-6f) {
        const float g = 0.9f / peak;
        for (float& v : out) v *= g;
    }
    return out;
}

}
