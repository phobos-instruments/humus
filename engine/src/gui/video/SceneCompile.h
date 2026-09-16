// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>

#include <juce_opengl/juce_opengl.h>

#include "gui/video/SceneAssemble.h"

namespace hum {

inline juce::StringArray undeclaredIdentifiers(const juce::String& log) {
    juce::StringArray names;
    for (const auto& line : juce::StringArray::fromLines(log)) {
        if (!line.contains("ndeclared identifier")) continue;
        const int a = line.indexOfChar('\'');
        const int b = a >= 0 ? line.indexOfChar(a + 1, '\'') : -1;
        if (b > a + 1) names.addIfNotAlreadyThere(line.substring(a + 1, b));
    }
    return names;
}

inline bool isAssignedTo(const juce::String& fragment, const juce::String& name) {
    auto ident = [](juce::juce_wchar c) {
        return juce::CharacterFunctions::isLetterOrDigit(c) || c == '_';
    };
    for (int at = fragment.indexOf(name); at >= 0;
         at = fragment.indexOf(at + name.length(), name)) {
        if (at > 0 && ident(fragment[at - 1])) continue;
        int i = at + name.length();
        if (i < fragment.length() && ident(fragment[i])) continue;
        while (i < fragment.length() && fragment[i] == ' ') ++i;
        if (i < fragment.length() - 1 && fragment[i] == '=' && fragment[i + 1] != '=')
            return true;
    }
    return false;
}

inline juce::String inferDeclaration(const juce::String& fragment, const juce::String& name) {
    if (!isAssignedTo(fragment, name)) {
        if (name == "PI") return "#define PI 3.14159265358979\n";
        if (name == "TAU") return "#define TAU 6.28318530718\n";
        if (name == "uv") return "#define uv (gl_FragCoord.xy / RENDERSIZE)\n";
        if (name == "time") return "#define time TIME\n";
        if (name == "iTime" || name == "iGlobalTime")
            return "#define " + name + " TIME\n";
        if (name == "iResolution") return "#define iResolution vec3(RENDERSIZE, 1.0)\n";
    }
    if (fragment.contains(name + "(")) return {};
    for (const char* kw : {"float ", "int ", "bool ", "vec2 ", "vec3 ", "vec4 "}) {
        const juce::String decl = juce::String(kw) + name;
        for (int at = fragment.indexOf(decl); at >= 0;
             at = fragment.indexOf(at + decl.length(), decl)) {
            const int nl = fragment.substring(0, at).lastIndexOfChar('\n');
            if (!fragment.substring(nl + 1, at).contains("//")) return {};
        }
    }
    if (fragment.contains("texture(" + name) || fragment.contains("texture2D(" + name))
        return "uniform sampler2D " + name + ";\n";
    juce::String type = "float";
    auto identCh = [](juce::juce_wchar c) {
        return juce::CharacterFunctions::isLetterOrDigit(c) || c == '_';
    };
    for (int at = fragment.indexOf("= " + name); at >= 0;
         at = fragment.indexOf(at + 2, "= " + name)) {
        int i = at - 1;
        while (i >= 0 && fragment[i] == ' ') --i;
        while (i >= 0 && identCh(fragment[i])) --i;
        while (i >= 0 && fragment[i] == ' ') --i;
        const int e = i + 1;
        while (i >= 0 && identCh(fragment[i])) --i;
        const auto ty = fragment.substring(i + 1, e);
        if (ty == "vec2" || ty == "vec3" || ty == "vec4") { type = ty; break; }
        if (ty == "float" || ty == "int" || ty == "bool") break;
    }
    if (isAssignedTo(fragment, name)) {
        const juce::String zero = type == "float" ? "0.0" : type + "(0.0)";
        return type + " " + name + " = " + zero + ";\n";
    }
    return "uniform " + type + " " + name + ";\n";
}

inline juce::String withDeclarations(const juce::String& fragment, const juce::String& decls) {
    auto lines = juce::StringArray::fromLines(fragment);
    if (!lines.isEmpty() && lines[0].trim().startsWith("#version"))
        return lines[0] + "\n" + decls + fragment.fromFirstOccurrenceOf("\n", false, false);
    return decls + fragment;
}

inline const char* intArgBridge() {
    return
        "float mix(int a, float b, float t) { return mix(float(a), b, t); }\n"
        "float mix(float a, int b, float t) { return mix(a, float(b), t); }\n"
        "float mix(int a, int b, float t) { return mix(float(a), float(b), t); }\n"
        "float min(int a, float b) { return min(float(a), b); }\n"
        "float min(float a, int b) { return min(a, float(b)); }\n"
        "float max(int a, float b) { return max(float(a), b); }\n"
        "float max(float a, int b) { return max(a, float(b)); }\n"
        "float pow(int a, float b) { return pow(float(a), b); }\n"
        "float pow(float a, int b) { return pow(a, float(b)); }\n"
        "float step(int e, float x) { return step(float(e), x); }\n"
        "float smoothstep(int a, int b, float x) { return smoothstep(float(a), float(b), x); }\n"
        "float smoothstep(int a, float b, float x) { return smoothstep(float(a), b, x); }\n"
        "float smoothstep(float a, int b, float x) { return smoothstep(a, float(b), x); }\n"
        "float clamp(float x, int lo, int hi) { return clamp(x, float(lo), float(hi)); }\n"
        "float clamp(float x, int lo, float hi) { return clamp(x, float(lo), hi); }\n"
        "float clamp(float x, float lo, int hi) { return clamp(x, lo, float(hi)); }\n"
        "float mod(float x, int y) { return mod(x, float(y)); }\n"
        "float mod(int x, float y) { return mod(float(x), y); }\n";
}

inline juce::String knownFunctionFor(const juce::String& name) {
    if (name == "modf")
        return "float modf(float x, out float ip) { ip = floor(x); return fract(x); }\n";
    return {};
}

struct HealedUniform {
    juce::String name;
    int comps = 1;
};

inline int healedComps(const juce::String& decl) {
    if (!decl.startsWith("uniform ")) return 0;
    if (decl.contains("sampler")) return 0;
    if (decl.contains("vec4")) return 4;
    if (decl.contains("vec3")) return 3;
    if (decl.contains("vec2")) return 2;
    return 1;
}

inline std::unique_ptr<juce::OpenGLShaderProgram>
compileScene(juce::OpenGLContext& ctx, const juce::String& fragment,
             juce::String* err, juce::StringArray* healed = nullptr,
             juce::String* finalSrc = nullptr,
             std::vector<HealedUniform>* healedUniforms = nullptr) {
    juce::String src = fragment;
    bool bridged = false;
    for (int attempt = 0; attempt < 4; ++attempt) {
        if (finalSrc != nullptr) *finalSrc = src;
        auto p = std::make_unique<juce::OpenGLShaderProgram>(ctx);
        if (p->addVertexShader(sceneVertexSrc())
            && p->addFragmentShader(src.toRawUTF8()) && p->link()) {
            if (err != nullptr) err->clear();
            return p;
        }
        const auto log = p->getLastError();
        if (err != nullptr) *err = log;
        juce::String decls;
        for (const auto& n : undeclaredIdentifiers(log)) {
            auto d = knownFunctionFor(n);
            if (d.isEmpty()) d = inferDeclaration(fragment, n);
            decls << d;
            if (d.isNotEmpty() && healed != nullptr) healed->addIfNotAlreadyThere(n);
            if (healedUniforms != nullptr)
                if (const int c = healedComps(d); c > 0)
                    healedUniforms->push_back({n, c});
        }
        if (!bridged
            && (log.contains("No matching function")
                || log.contains("Incompatible types")
                || log.contains("does not operate on"))) {
            bridged = true;
            decls << intArgBridge();
        }
        if (decls.isEmpty()) return nullptr;
        src = withDeclarations(src, decls);
    }
    return nullptr;
}

}
