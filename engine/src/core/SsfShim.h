#pragma once
#include <vector>

#include <juce_core/juce_core.h>

#include "core/IsfParse.h"

namespace hum {
namespace ssf {

inline bool looksLikeSsf(const juce::String& src) {
    return src.contains("renderMain") && !src.contains("void main(");
}

struct Control {
    juce::String name;
    int comps = 1;
    float def[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct SceneImage {
    juce::String name;
    juce::String path;
};

struct Scene {
    juce::String fragment;
    int passes = 1;
    juce::StringArray buffers;
    std::vector<Control> controls;
    std::vector<SceneImage> images;
};

inline int countPasses(const juce::String& src) {
    int maxIdx = 0;
    for (int at = src.indexOf("PASSINDEX"); at >= 0;
         at = src.indexOf(at + 9, "PASSINDEX")) {
        int i = at + 9;
        while (i < src.length() && (src[i] == ' ' || src[i] == '=')) ++i;
        int n = 0;
        bool any = false;
        while (i < src.length() && src[i] >= '0' && src[i] <= '9') {
            n = n * 10 + (src[i] - '0');
            any = true;
            ++i;
        }
        if (any) maxIdx = juce::jmax(maxIdx, n);
    }
    return maxIdx + 1;
}

inline juce::StringArray findBuffers(const juce::String& src) {
    juce::StringArray out;
    for (char c = 'A'; c <= 'Z'; ++c)
        if (src.contains("buff" + juce::String::charToString((juce::juce_wchar) c)))
            out.add("buff" + juce::String::charToString((juce::juce_wchar) c));
    return out;
}

inline Scene wrapWith(const juce::String& src, Scene sc) {
    juce::String pre;
    if (!src.contains("#version")) pre << "#version 120\n";
    pre <<
        "uniform vec2 RENDERSIZE;\n"
        "#define _uv (gl_FragCoord.xy / RENDERSIZE)\n"
        "#define _uvc ((_uv * 2.0 - 1.0) * vec2(RENDERSIZE.x / max(RENDERSIZE.y, 1.0), 1.0))\n"
        "#define _xy gl_FragCoord.xy\n"
        "#define texture texture2D\n"
        "uniform float TIME;\n"
        "uniform float TIMEDELTA;\n"
        "uniform float FRAMECOUNT;\n"
        "uniform float syn_Time;\n"
        "uniform float syn_BPM;\n"
        "uniform float syn_BeatTime;\n"
        "uniform float syn_OnBeat;\n"
        "uniform float syn_RandomOnBeat;\n"
        "uniform float syn_BPMSin;\n"
        "uniform float syn_BPMSin2;\n"
        "uniform float syn_BPMSin4;\n"
        "uniform float syn_BPMTri;\n"
        "uniform float syn_BPMTri2;\n"
        "uniform float syn_BPMTri4;\n"
        "uniform float syn_BPMTwitcher;\n"
        "uniform float syn_ToggleOnBeat;\n"
        "uniform float syn_FadeInOut;\n"
        "uniform float syn_Level;\n"
        "uniform float syn_BassLevel;\n"
        "uniform float syn_MidLevel;\n"
        "uniform float syn_MidHighLevel;\n"
        "uniform float syn_HighLevel;\n"
        "uniform float syn_BassHits;\n"
        "uniform float syn_MidHits;\n"
        "uniform float syn_MidHighHits;\n"
        "uniform float syn_HighHits;\n"
        "uniform float syn_Hits;\n"
        "uniform float syn_BassPresence;\n"
        "uniform float syn_MidPresence;\n"
        "uniform float syn_MidHighPresence;\n"
        "uniform float syn_HighPresence;\n"
        "uniform float syn_Intensity;\n"
        "uniform float syn_Presence;\n"
        "#define syn_BassTime syn_Time\n"
        "#define syn_MidTime syn_Time\n"
        "#define syn_MidHighTime syn_Time\n"
        "#define syn_HighTime syn_Time\n"
        "#define syn_CurvedTime syn_Time\n"
        "uniform float syn_PassIndex;\n"
        "#define PASSINDEX syn_PassIndex\n"
        "uniform sampler2D syn_Spectrum;\n"
        "uniform float syn_MediaType;\n"
        "uniform sampler2D syn_UserImage;\n"
        "uniform sampler2D syn_Media;\n"
        "uniform sampler2D syn_FinalPass;\n"
        "float _rand(vec2 p) {\n"
        "    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);\n"
        "}\n"
        "float _rand(float x) { return _rand(vec2(x, 0.0)); }\n"
        "float _noise(vec2 p) {\n"
        "    vec2 i = floor(p), f = fract(p);\n"
        "    vec2 u = f * f * (3.0 - 2.0 * f);\n"
        "    return mix(mix(_rand(i), _rand(i + vec2(1.0, 0.0)), u.x),\n"
        "               mix(_rand(i + vec2(0.0, 1.0)), _rand(i + vec2(1.0, 1.0)), u.x),\n"
        "               u.y);\n"
        "}\n"
        "float _noise(float x) { return _noise(vec2(x, 0.0)); }\n"
        "float _fbm(vec2 p) {\n"
        "    float v = 0.0, a = 0.5;\n"
        "    for (int i = 0; i < 5; ++i) { v += a * _noise(p); p *= 2.0; a *= 0.5; }\n"
        "    return v;\n"
        "}\n"
        "float _fbm(vec2 p, float oct) { return _fbm(p); }\n"
        "float _fbm(float x) { return _fbm(vec2(x, 0.0)); }\n"
        "float _noise(vec3 p) {\n"
        "    return mix(_noise(p.xy + vec2(17.0 * floor(p.z), 0.0)),\n"
        "               _noise(p.xy + vec2(17.0 * (floor(p.z) + 1.0), 0.0)),\n"
        "               smoothstep(0.0, 1.0, fract(p.z)));\n"
        "}\n"
        "float _fbm(vec3 p) {\n"
        "    float v = 0.0, a = 0.5;\n"
        "    for (int i = 0; i < 5; ++i) { v += a * _noise(p); p *= 2.0; a *= 0.5; }\n"
        "    return v;\n"
        "}\n"
        "float _statelessContinuousChaotic(float t) {\n"
        "    return sin(t) * 0.5 + sin(t * 0.351) * 0.3 + sin(t * 1.93) * 0.2;\n"
        "}\n"
        "vec2 _rotate(vec2 p, float a) {\n"
        "    float c = cos(a), s = sin(a);\n"
        "    return vec2(p.x * c - p.y * s, p.x * s + p.y * c);\n"
        "}\n"
        "float _pulse(float x, float p, float w) {\n"
        "    return smoothstep(p - w, p, x) * (1.0 - smoothstep(p, p + w, x));\n"
        "}\n"
        "float _sqPulse(float x, float p, float w) {\n"
        "    return step(p - w, x) * (1.0 - step(p + w, x));\n"
        "}\n"
        "vec3 _palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {\n"
        "    return a + b * cos(6.28318530718 * (c * t + d));\n"
        "}\n"
        "vec2 _pixelate(vec2 p, vec2 n) { return floor(p * n) / n; }\n"
        "vec2 _pixelate(vec2 p, float n) { return floor(p * n) / n; }\n"
        "vec3 _pixelate(vec3 p, float n) { return floor(p * n) / n; }\n"
        "float _pixelate(float x, float n) { return floor(x * n) / n; }\n"
        "vec3 _normalizeRGB(float r, float g, float b) { return vec3(r, g, b) / 255.0; }\n"
        "vec3 _rgb2hsv(vec3 c) {\n"
        "    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\n"
        "    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));\n"
        "    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));\n"
        "    float d = q.x - min(q.w, q.y);\n"
        "    float e = 1.0e-10;\n"
        "    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);\n"
        "}\n"
        "vec3 _hsv2rgb(vec3 c) {\n"
        "    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n"
        "    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n"
        "    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\n"
        "}\n"
        "float _luminance(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }\n"
        "float _map(float x, float a, float b, float c, float d) {\n"
        "    return c + (d - c) * (x - a) / (b - a);\n"
        "}\n"
        "float _nclamp(float x) { return clamp(x, 0.0, 1.0); }\n"
        "float _scale(float x, float lo, float hi) { return lo + x * (hi - lo); }\n"
        "float _nsin(float x) { return 0.5 + 0.5 * sin(x); }\n"
        "float _triWave(float x, float p) {\n"
        "    return abs(fract(x / max(p, 1.0e-6)) * 2.0 - 1.0) * 2.0 - 1.0;\n"
        "}\n"
        "float _triWave(float x) { return _triWave(x, 1.0); }\n"
        "vec3 _gamma(vec3 c, float g) { return pow(max(c, 0.0), vec3(g)); }\n"
        "vec4 _gamma(vec4 c, float g) { return vec4(pow(max(c.rgb, 0.0), vec3(g)), c.a); }\n"
        "vec4 _hueSaturationContrastLEGACY(vec4 c, float h, float sat, float con) {\n"
        "    vec3 hsv = _rgb2hsv(c.rgb);\n"
        "    hsv.x = fract(hsv.x + h);\n"
        "    hsv.y = clamp(hsv.y * (0.5 + sat), 0.0, 1.0);\n"
        "    vec3 rgb = (_hsv2rgb(hsv) - 0.5) * max(con, 0.0) + 0.5;\n"
        "    return vec4(clamp(rgb, 0.0, 1.0), c.a);\n"
        "}\n"
        "float tanh(float x) { float e = exp(2.0 * clamp(x, -20.0, 20.0)); return (e - 1.0) / (e + 1.0); }\n"
        "vec2 tanh(vec2 v) { return vec2(tanh(v.x), tanh(v.y)); }\n"
        "vec3 tanh(vec3 v) { return vec3(tanh(v.x), tanh(v.y), tanh(v.z)); }\n"
        "vec4 tanh(vec4 v) { return vec4(tanh(v.x), tanh(v.y), tanh(v.z), tanh(v.w)); }\n"
        "float sinh(float x) { return 0.5 * (exp(x) - exp(-x)); }\n"
        "float cosh(float x) { return 0.5 * (exp(x) + exp(-x)); }\n"
        "vec4 texture2D(sampler2D s, float x) { return texture2D(s, vec2(x, 0.5)); }\n"
        "vec2 _toPolar(vec2 p) { return vec2(atan(p.y, p.x), length(p)); }\n"
        "vec2 _toPolarTrue(vec2 p) { return vec2(atan(p.y, p.x), length(p)); }\n"
        "vec2 _uv2uvc(vec2 u) {\n"
        "    return (u * 2.0 - 1.0) * vec2(RENDERSIZE.x / max(RENDERSIZE.y, 1.0), 1.0);\n"
        "}\n"
        "vec2 _uvc2uv(vec2 c) {\n"
        "    return c / vec2(RENDERSIZE.x / max(RENDERSIZE.y, 1.0), 1.0) * 0.5 + 0.5;\n"
        "}\n"
        "vec4 texelFetch(sampler2D s, ivec2 p, int lod) {\n"
        "    return texture2D(s, (vec2(p) + 0.5) / RENDERSIZE);\n"
        "}\n"
        "vec2 textureSize(sampler2D s, int lod) { return RENDERSIZE; }\n"
        "bool _isMediaActive() { return false; }\n"
        "vec4 _loadMedia() { return vec4(0.0); }\n"
        "vec4 _loadMedia(vec2 p) { return vec4(0.0); }\n"
        "vec4 _loadMediaAsMask() { return vec4(0.0); }\n"
        "vec4 _textureMedia(vec2 p) { return vec4(0.0); }\n"
        "vec4 _textureMedia(vec2 p, float lod) { return vec4(0.0); }\n"
        "vec4 _textureMediaAsMask(vec2 p) { return vec4(0.0); }\n"
        "vec2 _correctMediaCoords(vec2 p) { return p; }\n"
        "vec2 _correctMediaCoords() { return _uv; }\n"
        "vec2 _flipMediaCoords(vec2 p) { return vec2(p.x, 1.0 - p.y); }\n"
        "vec4 _applyInvertMedia(vec4 c) { return c; }\n"
        "vec3 _applyInvertMedia(vec3 c) { return c; }\n"
        "vec4 _loadImage(sampler2D s) { return texture2D(s, _uv); }\n"
        "vec4 _loadImage(sampler2D s, vec2 off) { return texture2D(s, fract(_uv + off)); }\n"
        "#define IMG_NORM_PIXEL(s, p) texture2D(s, p)\n"
        "#define IMG_PIXEL(s, p) texture2D(s, (p) / RENDERSIZE)\n";
    for (const auto& b : sc.buffers)
        if (b.isNotEmpty()) pre << "uniform sampler2D " << b << ";\n";
    for (const auto& c : sc.controls)
        pre << "uniform "
            << (c.comps == 4 ? "vec4" : c.comps == 3 ? "vec3"
                : c.comps == 2 ? "vec2" : "float")
            << " " << c.name << ";\n";
    for (const auto& im : sc.images)
        pre << "uniform sampler2D " << im.name << ";\n";

    sc.fragment = assembleGlsl(pre,
                               src + "\nvoid main() { gl_FragColor = renderMain(); }\n");
    return sc;
}

inline Scene wrap(const juce::String& src) {
    Scene sc;
    sc.passes = countPasses(src);
    sc.buffers = findBuffers(src);
    return wrapWith(src, std::move(sc));
}

inline Scene wrapPackage(const juce::String& src, const juce::var& json) {
    Scene sc;
    if (const auto* ps = json["PASSES"].getArray()) {
        for (const auto& p : *ps) sc.buffers.add(p["TARGET"].toString().trim());
        sc.passes = ps->size() + 1;
    } else {
        sc.passes = countPasses(src);
        sc.buffers = findBuffers(src);
    }
    if (const auto* cs = json["CONTROLS"].getArray())
        for (const auto& cv : *cs) {
            Control c;
            c.name = cv["NAME"].toString().trim();
            if (c.name.isEmpty()) continue;
            const auto type = cv["TYPE"].toString();
            c.comps = type.startsWith("xy") ? 2 : type.startsWith("color") ? 3 : 1;
            if (const auto* arr = cv["DEFAULT"].getArray()) {
                for (int i = 0; i < 4 && i < arr->size(); ++i)
                    c.def[i] = (float) (double) (*arr)[i];
            } else if (!cv["DEFAULT"].isVoid()) {
                c.def[0] = (float) (double) cv["DEFAULT"];
            }
            sc.controls.push_back(std::move(c));
        }
    if (const auto* is = json["IMAGES"].getArray())
        for (const auto& iv : *is) {
            SceneImage im;
            im.name = iv["NAME"].toString().trim();
            im.path = iv["PATH"].toString().trim();
            if (im.name.isNotEmpty() && im.path.isNotEmpty())
                sc.images.push_back(std::move(im));
        }
    return wrapWith(src, std::move(sc));
}

}
}
