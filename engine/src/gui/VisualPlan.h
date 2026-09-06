#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/VisualUniforms.h"
#include "gui/VideoLayer.h"

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
};

struct Step {
    enum Kind { Black, Deck, Scene, Mix } kind = Black;
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
};
struct Plan {
    std::vector<Step> steps;
    std::vector<Tap> taps;
    int root = -1;
    float masterFade = 1.0f;
    bool noSignal = false;
};

}
