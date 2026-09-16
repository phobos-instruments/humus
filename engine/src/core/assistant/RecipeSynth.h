// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

struct RecipeLayer {
    enum class Wave { Sine, Saw, Square, Triangle, Noise };
    Wave wave = Wave::Sine;
    double freq = 220.0;
    double freqEnd = 0.0;
    double level = 0.7;
    double attackMs = 3.0;
    double decayMs = 300.0;
    double lowpassHz = 0.0;
};

struct Recipe {
    std::string name;
    double durationSec = 1.0;
    std::vector<RecipeLayer> layers;
};

juce::String recipeSystemPrompt();

std::vector<Recipe> parseRecipes(const juce::String& reply);

std::vector<float> renderRecipe(const Recipe& r, double sampleRate);

}
