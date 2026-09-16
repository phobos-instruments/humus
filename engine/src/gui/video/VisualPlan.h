// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/video/VisualUniforms.h"
#include "gui/video/VideoLayer.h"
#include "gui/video/VideoTakeSink.h"
#include "hum/dsp/FadeLaw.h"

namespace hum::visual {

inline constexpr int kFftOrder = 10;
inline constexpr int kFft = 1 << kFftOrder;
inline constexpr int kWaveTex = 512;
inline constexpr int kFftTex = 256;
inline constexpr int kMaxSteps = 32;

struct SceneImage {
    juce::String name;
    juce::Image image;
    bool operator==(const SceneImage& o) const {
        return name == o.name && image == o.image;
    }
};

struct SceneSpec {
    juce::String fragment;
    juce::String audioTex, fftTex;
    int passes = 1;
    juce::StringArray buffers;
    bool wantsUnder = false;
    bool wantsFinal = false;
    std::vector<SceneImage> images;
    bool operator==(const SceneSpec& o) const {
        return fragment == o.fragment && audioTex == o.audioTex && fftTex == o.fftTex
               && passes == o.passes && buffers == o.buffers
               && wantsUnder == o.wantsUnder && wantsFinal == o.wantsFinal
               && images == o.images;
    }
};
struct UniformValue {
    char name[48] = {};
    int comps = 1;
    bool isInt = false;
    float v[4] = {};
};
struct LayerIn {
    int src = -1;
    float opacity = 1.0f;
    int blend = 0;
};
struct Tap {
    std::string node;
    int step = -1;
    int w = 0, h = 0;
    float fade = 1.0f;
    bool everyOther = false;
    std::shared_ptr<VideoTakeSink> sink;
};

struct FxParams {
    float posX = 0.0f, posY = 0.0f;
    float scale = 1.0f, rotate = 0.0f;
    float brightness = 1.0f, contrast = 1.0f, saturation = 1.0f, hue = 0.0f;
    float invert = 0.0f, pixelate = 0.0f;
    int mirror = 0;
};

struct Step {
    enum Kind { Black, Deck, Scene, Mix, Fx } kind = Black;
    std::string node;
    std::shared_ptr<const VideoLayer::Frame> frame;
    bool active = false;
    SceneSpec scene;
    visual::Bands bands;
    float time = 0.0f, beat = 0.0f, bpm = 120.0f, onBeat = 0.0f;
    float brightness = 1.0f;
    float knob[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    float timeDelta = 1.0f / 30.0f;
    int frameIndex = 0;
    float date[4] = {};
    std::vector<UniformValue> extra;
    float wave[kWaveTex] = {};
    float fft[kFftTex] = {};
    std::vector<LayerIn> layers;
    float sceneOpacity = 1.0f;
    int sceneBlend = 0;
    int mixA = -1, mixB = -1;
    float mixFade = 0.0f;
    float mixCurve = 0.0f;
    bool mixSum = false;
    FxParams fx;
};
inline std::pair<float, float> mixGains(const Step& s) {
    const float fade = std::min(1.0f, std::max(0.0f, s.mixFade));
    if (s.mixSum) return {1.0f, fade};
    return {fadeGain(1.0f - fade, s.mixCurve), fadeGain(fade, s.mixCurve)};
}

struct Plan {
    std::vector<Step> steps;
    std::vector<Tap> taps;
    int root = -1;
    float masterFade = 1.0f;
    bool noSignal = false;
    double beat = 0.0, tempo = 120.0;
    bool rolling = false;
};

}
