#pragma once
#include <vector>

#include <juce_core/juce_core.h>

#include "core/IsfParse.h"
#include "core/ShkShim.h"
#include "core/SsfShim.h"
#include "gui/VisualPlan.h"

namespace hum {

inline const char* sceneHumPreamble() {
    return
        "varying vec2 uv;\n"
        "uniform vec2 hum_Resolution;\n"
        "uniform float hum_Time, hum_Beat, hum_BPM, hum_OnBeat;\n"
        "uniform float hum_Level, hum_Brightness;\n"
        "uniform float hum_Bands[8];\n"
        "uniform float hum_Knob1, hum_Knob2, hum_Knob3, hum_Knob4;\n"
        "uniform sampler2D hum_Wave, hum_Spectrum;\n";
}

inline const char* sceneVertexSrc() {
    return
        "attribute vec2 pos;\n"
        "varying vec2 uv;\n"
        "void main() { uv = pos * 0.5 + 0.5; gl_Position = vec4(pos, 0.0, 1.0); }\n";
}

struct AssembledScene {
    visual::SceneSpec spec;
    std::vector<isf::Input> isfInputs;
    std::vector<ssf::Control> ssfControls;
    bool ssf = false;
    bool shk = false;
    juce::String parseError;
};

inline AssembledScene assembleScene(const juce::String& text) {
    AssembledScene a;
    if (text.isEmpty()) {
        a.spec = {{}, "hum_Wave", "hum_Spectrum"};
        return a;
    }
    if (isf::looksLikeIsf(text)) {
        auto sc = isf::parse(text);
        if (!sc.valid) {
            a.parseError = sc.error;
            return a;
        }
        a.isfInputs = std::move(sc.inputs);
        a.spec = {sc.fragment, sc.audioTex, sc.fftTex};
        return a;
    }
    if (ssf::looksLikeSsf(text)) {
        a.ssf = true;
        auto sc = ssf::wrap(text);
        a.spec = {sc.fragment, {}, "syn_Spectrum", sc.passes, sc.buffers};
        a.spec.wantsFinal = text.contains("syn_FinalPass");
        return a;
    }
    if (shk::looksLikeShk(text)) {
        a.shk = true;
        a.spec.fragment = shk::wrap(text);
        a.spec.wantsUnder = true;
        return a;
    }
    a.spec = {assembleGlsl(sceneHumPreamble(), text), "hum_Wave", "hum_Spectrum"};
    return a;
}

inline AssembledScene assembleSceneFile(const juce::File& f) {
    if (!f.isDirectory()) return assembleScene(f.loadFileAsString());
    AssembledScene a;
    const auto glsl = f.getChildFile("main.glsl").loadFileAsString();
    if (glsl.isEmpty()) {
        a.parseError = "no main.glsl inside " + f.getFileName();
        return a;
    }
    auto sc = ssf::wrapPackage(
        glsl, juce::JSON::parse(f.getChildFile("scene.json").loadFileAsString()));
    a.ssf = true;
    a.ssfControls = std::move(sc.controls);
    a.spec = {sc.fragment, {}, "syn_Spectrum", sc.passes, sc.buffers};
    a.spec.wantsFinal = glsl.contains("syn_FinalPass");
    for (const auto& im : sc.images) {
        auto img = juce::ImageFileFormat::loadFrom(f.getChildFile(im.path));
        if (img.isValid()) a.spec.images.push_back({im.name, std::move(img)});
    }
    return a;
}

}
