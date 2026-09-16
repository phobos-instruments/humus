// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VisualGlCanvas.h"

namespace hum {

const char* GlCanvas::builtinScene() {
    return
        "void main() {\n"
        "    vec2 p = uv * 2.0 - 1.0;\n"
        "    p.x *= hum_Resolution.x / max(hum_Resolution.y, 1.0);\n"
        "    float t = hum_Time * 0.5;\n"
        "    float r = length(p);\n"
        "    float a = atan(p.y, p.x);\n"
        "    float v = sin(r * (5.0 + 9.0 * hum_Knob1) - t * 2.5 + hum_Bands[1] * 7.0)\n"
        "            + sin(a * 3.0 + t + hum_Bands[4] * 5.0)\n"
        "            + sin((p.x + p.y) * (3.0 + 3.0 * hum_Knob3) + t * 1.7);\n"
        "    v *= 0.3333;\n"
        "    vec3 col = 0.5 + 0.5 * sin(v * 3.14159 + 6.28318 * hum_Knob2\n"
        "                               + vec3(0.0, 2.1, 4.2));\n"
        "    col *= (0.25 + 0.75 * hum_Level) * hum_Brightness;\n"
        "    col += vec3(0.9, 0.65, 0.3) * hum_OnBeat * 0.3 * hum_Brightness;\n"
        "    gl_FragColor = vec4(col, 1.0);\n"
        "}\n";
}

void GlCanvas::drawNoSignal(int w, int h) {
    if (w <= 0 || h <= 0) return;
    std::unique_ptr<juce::LowLevelGraphicsContext> gl(
        juce::createOpenGLGraphicsContext(ctx_, w, h));
    if (gl == nullptr) return;
    juce::Graphics g(*gl);
    const auto r = juce::Rectangle<int>(0, 0, w, h);
    const float unit = (float) juce::jmin(w, h);
    g.setColour(juce::Colours::white.withAlpha(alpha::mid));
    g.setFont(juce::Font(juce::FontOptions(unit * 0.075f).withStyle("Bold")));
    g.drawText(tr("visual-gl-canvas.no-signal", "NO SIGNAL"), r, juce::Justification::centred, false);
    g.setColour(juce::Colours::white.withAlpha(alpha::scrim));
    g.setFont(juce::Font(juce::FontOptions(unit * 0.032f)));
    g.drawText(tr("visual-gl-canvas.this-output-is-bypassed", "this output is bypassed"),
               r.translated(0, (int) (unit * 0.075f)),
               juce::Justification::centred, false);
}

void GlCanvas::drawQuad(unsigned int programId) {
    using namespace juce::gl;
    glBindBuffer(GL_ARRAY_BUFFER, quad_);
    const auto pos = glGetAttribLocation(programId, "pos");
    glEnableVertexAttribArray((GLuint) pos);
    glVertexAttribPointer((GLuint) pos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray((GLuint) pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::unique_ptr<juce::OpenGLShaderProgram> GlCanvas::build(const char* frag, juce::String* err) {
    auto p = std::make_unique<juce::OpenGLShaderProgram>(ctx_);
    if (p->addVertexShader(vertexSrc()) && p->addFragmentShader(frag) && p->link())
        return p;
    if (err != nullptr) *err = p->getLastError();
    return nullptr;
}

std::unique_ptr<juce::OpenGLShaderProgram> GlCanvas::buildDeck(bool rect, bool ycocg) {
    juce::String prologue =
        rect ? "#extension GL_ARB_texture_rectangle : require\n"
               "#define SAMPLER sampler2DRect\n"
               "uniform vec2 texSize;\n"
               "#define RAWSAMPLE(t, p) texture2DRect(t, (p) * texSize)\n"
             : "#define SAMPLER sampler2D\n"
               "#define RAWSAMPLE(t, p) texture2D(t, p)\n";
    prologue += ycocg ? "vec4 hapYcocg(vec4 c) {\n"
                        "    float scale = c.b * (255.0 / 8.0) + 1.0;\n"
                        "    float Co = (c.r - 0.50196078) / scale;\n"
                        "    float Cg = (c.g - 0.50196078) / scale;\n"
                        "    return vec4(c.a + Co - Cg, c.a + Cg,\n"
                        "                c.a - Co - Cg, 1.0);\n"
                        "}\n"
                        "#define SAMPLE(t, p) hapYcocg(RAWSAMPLE(t, p))\n"
                      : "#define SAMPLE(t, p) RAWSAMPLE(t, p)\n";
    const juce::String src =
        prologue
        + "varying vec2 uv;\n"
          "uniform SAMPLER tex;\n"
          "void main() {\n"
          "    vec2 p = vec2(uv.x, 1.0 - uv.y);\n"
          "    gl_FragColor = vec4(SAMPLE(tex, p).rgb, 1.0);\n"
          "}\n";
    return build(src.toRawUTF8(), nullptr);
}

void GlCanvas::ensurePrograms() {
    if (deckProgram_ == nullptr) deckProgram_ = buildDeck(false);
    if (deckYCoCgProgram_ == nullptr) {
        deckYCoCgProgram_ = buildDeck(false, true);
        if (deckYCoCgProgram_ == nullptr)
            std::fprintf(stderr, "deck ycocg shader failed to build\n");
    }
#if JUCE_MAC
    if (deckRectProgram_ == nullptr) deckRectProgram_ = buildDeck(true);
#endif
    if (layerProgram_ == nullptr)
        layerProgram_ = build(
            "varying vec2 uv;\n"
            "uniform sampler2D tex;\n"
            "uniform float opacity;\n"
            "uniform int mode;\n"
            "void main() {\n"
            "    vec4 c = texture2D(tex, uv);\n"
            "    if (mode == 2) gl_FragColor = vec4(mix(vec3(1.0), c.rgb, opacity), 1.0);\n"
            "    else if (mode == 1 || mode == 3)\n"
            "        gl_FragColor = vec4(c.rgb * opacity, 1.0);\n"
            "    else gl_FragColor = vec4(c.rgb, opacity);\n"
            "}\n", nullptr);
    if (mixProgram_ == nullptr)
        mixProgram_ = build(
            "varying vec2 uv;\n"
            "uniform sampler2D texA, texB;\n"
            "uniform float gainA, gainB;\n"
            "void main() {\n"
            "    gl_FragColor = vec4(clamp(texture2D(texA, uv).rgb * gainA\n"
            "                              + texture2D(texB, uv).rgb * gainB,\n"
            "                              0.0, 1.0), 1.0);\n"
            "}\n", nullptr);
    if (fxProgram_ == nullptr)
        fxProgram_ = build(
            "varying vec2 uv;\n"
            "uniform sampler2D tex;\n"
            "uniform vec2 pos;\n"
            "uniform float scale, rotate, aspect;\n"
            "uniform float brightness, contrast, saturation, hue, invert, pixelate;\n"
            "uniform int mirror;\n"
            "vec3 hueShift(vec3 c, float a) {\n"
            "    const vec3 k = vec3(0.57735);\n"
            "    float s = sin(a), co = cos(a);\n"
            "    return c * co + cross(k, c) * s + k * dot(k, c) * (1.0 - co);\n"
            "}\n"
            "void main() {\n"
            "    vec2 p = uv - 0.5 - pos * 0.5;\n"
            "    p.x *= aspect;\n"
            "    float s = sin(rotate), c = cos(rotate);\n"
            "    p = mat2(c, -s, s, c) * p;\n"
            "    p.x /= aspect;\n"
            "    p = p / max(scale, 1.0e-4) + 0.5;\n"
            "    if (mirror == 1 || mirror == 3) p.x = p.x < 0.5 ? p.x : 1.0 - p.x;\n"
            "    if (mirror == 2 || mirror == 3) p.y = p.y < 0.5 ? p.y : 1.0 - p.y;\n"
            "    if (pixelate > 0.0) {\n"
            "        float n = mix(512.0, 8.0, pixelate);\n"
            "        p = (floor(p * vec2(n, n / aspect)) + 0.5) / vec2(n, n / aspect);\n"
            "    }\n"
            "    bool inside = p.x >= 0.0 && p.x <= 1.0 && p.y >= 0.0 && p.y <= 1.0;\n"
            "    vec3 col = inside ? texture2D(tex, p).rgb : vec3(0.0);\n"
            "    col = (col - 0.5) * contrast + 0.5;\n"
            "    col *= brightness;\n"
            "    float l = dot(col, vec3(0.299, 0.587, 0.114));\n"
            "    col = mix(vec3(l), col, saturation);\n"
            "    col = hueShift(col, hue);\n"
            "    col = mix(col, 1.0 - col, invert);\n"
            "    gl_FragColor = vec4(clamp(col, 0.0, 1.0), 1.0);\n"
            "}\n", nullptr);
    if (presentProgram_ == nullptr)
        presentProgram_ = build(
            "varying vec2 uv;\n"
            "uniform sampler2D tex;\n"
            "uniform float fade;\n"
            "void main() { gl_FragColor = vec4(texture2D(tex, uv).rgb * fade, 1.0); }\n",
            nullptr);
}

}
