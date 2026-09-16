// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

inline juce::String assembleGlsl(const juce::String& preamble, const juce::String& body) {
    auto lines = juce::StringArray::fromLines(body);
    for (int i = 0; i < lines.size(); ++i) {
        const auto t = lines[i].trim();
        if (t.isEmpty() || t.startsWith("//")) continue;
        if (!t.startsWith("#version")) break;
        const auto directive = lines[i];
        lines.remove(i);
        return directive + "\n" + preamble + lines.joinIntoString("\n");
    }
    return preamble + body;
}

namespace isf {

enum class InputType { Float, Bool, Long, Event, Color, Point2D };

struct Input {
    juce::String name;
    InputType type = InputType::Float;
    double min = 0.0, max = 1.0;
    bool hasRange = false;
    float def[4] = {};
};

struct Scene {
    bool valid = false;
    juce::String error;
    juce::String fragment;
    std::vector<Input> inputs;
    juce::String audioTex;
    juce::String fftTex;
};

inline juce::var headerJson(const juce::String& src, int& bodyStart) {
    bodyStart = 0;
    const int open = src.indexOf("/*");
    if (open < 0) return {};
    const int close = src.indexOf(open + 2, "*/");
    if (close < 0) return {};
    bodyStart = close + 2;
    return juce::JSON::parse(src.substring(open + 2, close));
}

inline bool looksLikeIsf(const juce::String& src) {
    int bodyStart = 0;
    const auto v = headerJson(src, bodyStart);
    if (!v.isObject()) return false;
    return v.hasProperty("ISFVSN") || v.hasProperty("INPUTS") || v.hasProperty("PASSES")
        || v.hasProperty("CATEGORIES") || v.hasProperty("VSN");
}

inline Scene parse(const juce::String& src) {
    Scene s;
    int bodyStart = 0;
    const auto v = headerJson(src, bodyStart);
    if (!v.isObject()) {
        s.error = "no ISF JSON header";
        return s;
    }
    if (const auto* passes = v["PASSES"].getArray(); passes != nullptr && passes->size() > 1) {
        s.error = "multi-pass ISF scenes are not supported yet";
        return s;
    }

    juce::String decls;
    if (const auto* inputs = v["INPUTS"].getArray()) {
        for (const auto& in : *inputs) {
            const auto name = in["NAME"].toString();
            const auto type = in["TYPE"].toString();
            if (name.isEmpty()) continue;
            if (type == "image") {
                s.error = "image input '" + name
                          + "' needs a filter host - Lumen renders generator scenes";
                return s;
            }
            if (type == "audio" || type == "audioFFT") {
                (type == "audio" ? s.audioTex : s.fftTex) = name;
                decls << "uniform sampler2D " << name << ";\n"
                      << "uniform vec2 _" << name << "_imgSize;\n";
                continue;
            }
            Input p;
            p.name = name;
            const auto num = [&in](const char* k, double fallback) {
                return in.hasProperty(k) ? (double) in[k] : fallback;
            };
            if (type == "bool" || type == "event") {
                p.type = type == "bool" ? InputType::Bool : InputType::Event;
                p.def[0] = (float) num("DEFAULT", 0.0);
                decls << "uniform bool " << name << ";\n";
            } else if (type == "long") {
                p.type = InputType::Long;
                p.def[0] = (float) num("DEFAULT", 0.0);
                decls << "uniform int " << name << ";\n";
            } else if (type == "color") {
                p.type = InputType::Color;
                p.def[0] = p.def[1] = p.def[2] = p.def[3] = 1.0f;
                if (const auto* d = in["DEFAULT"].getArray())
                    for (int c = 0; c < juce::jmin(4, d->size()); ++c)
                        p.def[c] = (float) (double) (*d)[c];
                decls << "uniform vec4 " << name << ";\n";
            } else if (type == "point2D") {
                p.type = InputType::Point2D;
                if (const auto* d = in["DEFAULT"].getArray())
                    for (int c = 0; c < juce::jmin(2, d->size()); ++c)
                        p.def[c] = (float) (double) (*d)[c];
                decls << "uniform vec2 " << name << ";\n";
            } else {
                p.type = InputType::Float;
                p.hasRange = in.hasProperty("MIN") && in.hasProperty("MAX");
                p.min = num("MIN", 0.0);
                p.max = num("MAX", 1.0);
                p.def[0] = (float) num("DEFAULT", p.hasRange ? (p.min + p.max) * 0.5 : 0.5);
                decls << "uniform float " << name << ";\n";
            }
            s.inputs.push_back(std::move(p));
        }
    }

    juce::String body = src.substring(bodyStart);
    for (const auto& tex : {s.audioTex, s.fftTex})
        if (tex.isNotEmpty())
            body = body.replace("IMG_SIZE(" + tex + ")", "_" + tex + "_imgSize");

    s.fragment = assembleGlsl(
        juce::String("varying vec2 uv;\n"
                     "#define isf_FragNormCoord uv\n"
                     "#define vv_FragNormCoord uv\n"
                     "uniform vec2 RENDERSIZE;\n"
                     "uniform float TIME;\n"
                     "uniform float TIMEDELTA;\n"
                     "uniform int FRAMEINDEX;\n"
                     "uniform int PASSINDEX;\n"
                     "uniform vec4 DATE;\n"
                     "#define IMG_NORM_PIXEL(s,c) texture2D(s,c)\n"
                     "#define IMG_PIXEL(s,p) texture2D(s,(p)/RENDERSIZE)\n"
                     "#define IMG_THIS_NORM_PIXEL(s) texture2D(s,uv)\n"
                     "#define IMG_THIS_PIXEL(s) texture2D(s,uv)\n")
            + decls,
        body);
    s.valid = true;
    return s;
}

}
}
