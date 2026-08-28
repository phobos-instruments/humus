#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

#include "gui/SceneAssemble.h"
#include "gui/SceneCompile.h"
#include "gui/VideoPreviewStore.h"
#include "gui/VisualPlan.h"

namespace hum {

class GlCanvas : public juce::Component, public juce::OpenGLRenderer {
public:
    GlCanvas() {
        setOpaque(true);
        ctx_.setRenderer(this);
        ctx_.setComponentPaintingEnabled(false);
        ctx_.setContinuousRepainting(true);
        ctx_.attachTo(*this);
    }
    ~GlCanvas() override { ctx_.detach(); }

    void setPlan(const visual::Plan& p) {
        const juce::SpinLock::ScopedLockType sl(lock_);
        plan_ = p;
    }
    juce::String compileError() const {
        const juce::SpinLock::ScopedLockType sl(lock_);
        return error_;
    }

    void setPaused(bool paused) {
        if (paused == paused_) return;
        paused_ = paused;
        ctx_.setContinuousRepainting(!paused);
    }

    void pump() { ctx_.triggerRepaint(); }

    static const char* humPreamble() { return sceneHumPreamble(); }

    static const char* builtinScene() {
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

    void newOpenGLContextCreated() override {
        using namespace juce::gl;
        glGenBuffers(1, &quad_);
        glBindBuffer(GL_ARRAY_BUFFER, quad_);
        static const float verts[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                                      1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f, 1.0f};
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    }

    void openGLContextClosing() override {
        using namespace juce::gl;
        deckProgram_.reset();
        layerProgram_.reset();
        mixProgram_.reset();
        presentProgram_.reset();
        for (auto& [n, sp] : scenePrograms_) releaseSceneTextures(sp);
        scenePrograms_.clear();
        if (passFbo_ != 0) { glDeleteFramebuffers(1, &passFbo_); passFbo_ = 0; }
        destroyFbos();
        for (auto& [n, t] : deckTex_)
            if (t.tex != 0) glDeleteTextures(1, &t.tex);
        deckTex_.clear();
        if (quad_ != 0) { glDeleteBuffers(1, &quad_); quad_ = 0; }
        for (auto* t : {&texWave_, &texFft_, &texBlack_})
            if (*t != 0) { glDeleteTextures(1, t); *t = 0; }
        if (previewFbo_ != 0) { glDeleteFramebuffers(1, &previewFbo_); previewFbo_ = 0; }
        if (previewTex_ != 0) { glDeleteTextures(1, &previewTex_); previewTex_ = 0; }
    }

    void renderOpenGL() override {
        using namespace juce::gl;
        visual::Plan p;
        {
            const juce::SpinLock::ScopedLockType sl(lock_);
            p = plan_;
        }
        const float scale = (float) ctx_.getRenderingScale();
        const int w = juce::roundToInt(scale * (float) getWidth());
        const int h = juce::roundToInt(scale * (float) getHeight());

        ensurePrograms();
        ensureFbos((int) p.steps.size(), w, h);
        ensureBlackTex();

        juce::String firstError;
        for (int i = 0; i < (int) p.steps.size(); ++i) {
            auto& s = p.steps[(size_t) i];
            glBindFramebuffer(GL_FRAMEBUFFER, fbos_[(size_t) i].fbo);
            glViewport(0, 0, w, h);
            juce::OpenGLHelpers::clear(juce::Colours::black);
            switch (s.kind) {
                case visual::Step::Deck: renderDeck(s, p.wearClock); break;
                case visual::Step::Scene: renderScene(s, w, h, firstError); break;
                case visual::Step::Mix: renderMix(s); break;
                case visual::Step::Black: break;
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, w, h);
        juce::OpenGLHelpers::clear(juce::Colours::black);
        if (p.root >= 0 && p.root < (int) fbos_.size() && presentProgram_ != nullptr) {
            presentProgram_->use();
            const auto pid = presentProgram_->getProgramID();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fbos_[(size_t) p.root].tex);
            glUniform1i(glGetUniformLocation(pid, "tex"), 0);
            glUniform1f(glGetUniformLocation(pid, "fade"),
                        juce::jlimit(0.0f, 1.0f, p.masterFade));
            drawQuad(pid);
        }
        if (p.noSignal) drawNoSignal(w, h);
        publishPreview(p);
        {
            const juce::SpinLock::ScopedLockType sl(lock_);
            error_ = firstError;
        }
    }

    void publishPreview(const visual::Plan& p) {
        using namespace juce::gl;
        auto& store = VideoPreviewStore::instance();
        if (previewNode_.empty() || !store.wanted(previewNode_)) return;
        if ((previewTick_ ^= 1) == 0) return;

        if (previewFbo_ == 0) {
            glGenFramebuffers(1, &previewFbo_);
            glGenTextures(1, &previewTex_);
            glBindTexture(GL_TEXTURE_2D, previewTex_);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kPrevW, kPrevH, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, previewFbo_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, previewTex_, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, previewFbo_);
        glViewport(0, 0, kPrevW, kPrevH);
        juce::OpenGLHelpers::clear(juce::Colours::black);
        if (p.root >= 0 && p.root < (int) fbos_.size() && presentProgram_ != nullptr) {
            presentProgram_->use();
            const auto pid = presentProgram_->getProgramID();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fbos_[(size_t) p.root].tex);
            glUniform1i(glGetUniformLocation(pid, "tex"), 0);
            glUniform1f(glGetUniformLocation(pid, "fade"),
                        juce::jlimit(0.0f, 1.0f, p.masterFade));
            drawQuad(pid);
        }
        previewPixels_.resize((size_t) kPrevW * (size_t) kPrevH * 4u);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, kPrevW, kPrevH, GL_RGBA, GL_UNSIGNED_BYTE,
                     previewPixels_.data());
        for (int y = 0; y < kPrevH / 2; ++y) {
            auto* a = previewPixels_.data() + (size_t) y * kPrevW * 4u;
            auto* b = previewPixels_.data() + (size_t) (kPrevH - 1 - y) * kPrevW * 4u;
            std::swap_ranges(a, a + kPrevW * 4, b);
        }
        store.publish(previewNode_, kPrevW, kPrevH, previewPixels_.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void setPreviewNode(std::string n) { previewNode_ = std::move(n); }

private:
    struct Fbo { unsigned int fbo = 0, tex = 0; };
    struct DeckTex { unsigned int tex = 0; std::shared_ptr<const VideoLayer::Frame> uploaded; };
    struct SceneProg {
        std::unique_ptr<juce::OpenGLShaderProgram> program;
        visual::SceneSpec compiled;
        juce::String error;
        std::vector<HealedUniform> healed;
        struct Feedback { unsigned int tex[2] = {0, 0}; int cur = 0; };
        std::vector<Feedback> feedback;
        std::vector<std::unique_ptr<juce::OpenGLTexture>> imageTex;
        unsigned int under = 0;
        unsigned int finalTex = 0;
        int texW = 0, texH = 0;
    };

    static constexpr int kUnderUnit = 3;
    static constexpr int kFeedbackUnit0 = 4;

    void releaseSceneTextures(SceneProg& sp) {
        using namespace juce::gl;
        for (auto& fb : sp.feedback)
            for (auto& t : fb.tex)
                if (t != 0) { glDeleteTextures(1, &t); t = 0; }
        sp.feedback.clear();
        sp.imageTex.clear();
        if (sp.under != 0) { glDeleteTextures(1, &sp.under); sp.under = 0; }
        if (sp.finalTex != 0) { glDeleteTextures(1, &sp.finalTex); sp.finalTex = 0; }
        sp.texW = sp.texH = 0;
    }

    static unsigned int makeSceneTexture(int w, int h, bool floatFmt) {
        using namespace juce::gl;
        unsigned int t = 0;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, floatFmt ? (GLint) GL_RGBA16F : (GLint) GL_RGBA,
                     w, h, 0, GL_RGBA, floatFmt ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
        return t;
    }

    void ensureSceneTextures(SceneProg& sp, const visual::SceneSpec& spec, int w, int h) {
        using namespace juce::gl;
        if (sp.texW == w && sp.texH == h
            && (int) sp.feedback.size() == spec.buffers.size()
            && (spec.wantsUnder == (sp.under != 0))
            && (spec.wantsFinal == (sp.finalTex != 0)))
            return;
        releaseSceneTextures(sp);
        sp.texW = w; sp.texH = h;
        if (passFbo_ == 0) glGenFramebuffers(1, &passFbo_);
        GLint prevFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        auto cleared = [this](int tw, int th, bool floatFmt) {
            const auto t = makeSceneTexture(tw, th, floatFmt);
            glBindFramebuffer(GL_FRAMEBUFFER, passFbo_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, t, 0);
            juce::OpenGLHelpers::clear(juce::Colours::black);
            return t;
        };
        for (int i = 0; i < spec.buffers.size(); ++i) {
            SceneProg::Feedback fb;
            for (auto& t : fb.tex) t = cleared(w, h, true);
            sp.feedback.push_back(fb);
        }
        if (spec.wantsUnder) sp.under = cleared(w, h, false);
        if (spec.wantsFinal) sp.finalTex = cleared(w, h, false);
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint) prevFbo);
    }

    void drawNoSignal(int w, int h) {
        if (w <= 0 || h <= 0) return;
        std::unique_ptr<juce::LowLevelGraphicsContext> gl(
            juce::createOpenGLGraphicsContext(ctx_, w, h));
        if (gl == nullptr) return;
        juce::Graphics g(*gl);
        const auto r = juce::Rectangle<int>(0, 0, w, h);
        const float unit = (float) juce::jmin(w, h);
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.setFont(juce::Font(juce::FontOptions(unit * 0.075f).withStyle("Bold")));
        g.drawText("NO SIGNAL", r, juce::Justification::centred, false);
        g.setColour(juce::Colours::white.withAlpha(0.30f));
        g.setFont(juce::Font(juce::FontOptions(unit * 0.032f)));
        g.drawText("this output is bypassed",
                   r.translated(0, (int) (unit * 0.075f)),
                   juce::Justification::centred, false);
    }

    void drawQuad(unsigned int programId) {
        using namespace juce::gl;
        glBindBuffer(GL_ARRAY_BUFFER, quad_);
        const auto pos = glGetAttribLocation(programId, "pos");
        glEnableVertexAttribArray((GLuint) pos);
        glVertexAttribPointer((GLuint) pos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray((GLuint) pos);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    static const char* vertexSrc() { return sceneVertexSrc(); }

    std::unique_ptr<juce::OpenGLShaderProgram> build(const char* frag, juce::String* err) {
        auto p = std::make_unique<juce::OpenGLShaderProgram>(ctx_);
        if (p->addVertexShader(vertexSrc()) && p->addFragmentShader(frag) && p->link())
            return p;
        if (err != nullptr) *err = p->getLastError();
        return nullptr;
    }

    void ensurePrograms() {
        if (deckProgram_ == nullptr)
            deckProgram_ = build(
                "varying vec2 uv;\n"
                "uniform sampler2D tex;\n"
                "uniform float wear;\n"
                "uniform float wearTime;\n"
                "float vhsRand(vec2 co) {\n"
                "    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);\n"
                "}\n"
                "void main() {\n"
                "    vec2 p = vec2(uv.x, 1.0 - uv.y);\n"
                "    vec3 rgb;\n"
                "    if (wear > 0.001) {\n"
                "        float line = floor(p.y * 240.0);\n"
                "        float roll = fract(p.y - wearTime * 0.11);\n"
                "        float jit = (vhsRand(vec2(line, floor(wearTime * 24.0))) - 0.5)\n"
                "                    * 0.02 * wear\n"
                "                  + smoothstep(0.96, 1.0, roll) * 0.12 * wear;\n"
                "        p.x = clamp(p.x + jit, 0.0, 1.0);\n"
                "        float off = 0.006 * wear;\n"
                "        rgb = vec3(texture2D(tex, vec2(min(p.x + off, 1.0), p.y)).r,\n"
                "                   texture2D(tex, p).g,\n"
                "                   texture2D(tex, vec2(max(p.x - off, 0.0), p.y)).b);\n"
                "        float n = vhsRand(vec2(p.x * 320.0, line + floor(wearTime * 60.0)));\n"
                "        rgb = mix(rgb, vec3(n), smoothstep(0.955, 1.0, roll) * 0.8 * wear);\n"
                "        float g = dot(rgb, vec3(0.299, 0.587, 0.114));\n"
                "        rgb = mix(rgb, vec3(g), 0.25 * wear);\n"
                "    } else {\n"
                "        rgb = texture2D(tex, p).rgb;\n"
                "    }\n"
                "    gl_FragColor = vec4(rgb, 1.0);\n"
                "}\n", nullptr);
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
                "uniform float fade;\n"
                "void main() {\n"
                "    gl_FragColor = vec4(mix(texture2D(texA, uv).rgb,\n"
                "                            texture2D(texB, uv).rgb, fade), 1.0);\n"
                "}\n", nullptr);
        if (presentProgram_ == nullptr)
            presentProgram_ = build(
                "varying vec2 uv;\n"
                "uniform sampler2D tex;\n"
                "uniform float fade;\n"
                "void main() { gl_FragColor = vec4(texture2D(tex, uv).rgb * fade, 1.0); }\n",
                nullptr);
    }

    void destroyFbos() {
        using namespace juce::gl;
        for (auto& f : fbos_) {
            if (f.fbo != 0) glDeleteFramebuffers(1, &f.fbo);
            if (f.tex != 0) glDeleteTextures(1, &f.tex);
        }
        fbos_.clear();
    }

    void ensureFbos(int count, int w, int h) {
        using namespace juce::gl;
        if ((int) fbos_.size() == count && fboW_ == w && fboH_ == h) return;
        destroyFbos();
        fboW_ = w; fboH_ = h;
        for (int i = 0; i < count; ++i) {
            Fbo f;
            glGenTextures(1, &f.tex);
            glBindTexture(GL_TEXTURE_2D, f.tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, nullptr);
            glGenFramebuffers(1, &f.fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, f.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, f.tex, 0);
            fbos_.push_back(f);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void ensureBlackTex() {
        using namespace juce::gl;
        if (texBlack_ != 0) return;
        glGenTextures(1, &texBlack_);
        glBindTexture(GL_TEXTURE_2D, texBlack_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        const unsigned char px[4] = {0, 0, 0, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px);
    }

    unsigned int stepTex(int idx) const {
        return idx >= 0 && idx < (int) fbos_.size() ? fbos_[(size_t) idx].tex : texBlack_;
    }

    void renderDeck(const visual::Step& s, float wearClock) {
        using namespace juce::gl;
        auto& dt = deckTex_[s.node];
        if (s.frame != nullptr && s.frame != dt.uploaded) {
            if (dt.tex == 0) {
                glGenTextures(1, &dt.tex);
                glBindTexture(GL_TEXTURE_2D, dt.tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            } else {
                glBindTexture(GL_TEXTURE_2D, dt.tex);
            }
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s.frame->width,
                         s.frame->height, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                         s.frame->bgra.data());
            dt.uploaded = s.frame;
        }
        if (!s.active || dt.tex == 0 || deckProgram_ == nullptr) return;
        deckProgram_->use();
        const auto pid = deckProgram_->getProgramID();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dt.tex);
        glUniform1i(glGetUniformLocation(pid, "tex"), 0);
        glUniform1f(glGetUniformLocation(pid, "wear"),
                    juce::jlimit(0.0f, 1.0f, s.wear));
        glUniform1f(glGetUniformLocation(pid, "wearTime"), wearClock);
        drawQuad(pid);
    }

    void renderMix(const visual::Step& s) {
        using namespace juce::gl;
        if (mixProgram_ == nullptr) return;
        mixProgram_->use();
        const auto pid = mixProgram_->getProgramID();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, stepTex(s.mixA));
        glUniform1i(glGetUniformLocation(pid, "texA"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, stepTex(s.mixB));
        glUniform1i(glGetUniformLocation(pid, "texB"), 1);
        glUniform1f(glGetUniformLocation(pid, "fade"),
                    juce::jlimit(0.0f, 1.0f, s.mixFade));
        drawQuad(pid);
        glActiveTexture(GL_TEXTURE0);
    }

    void renderScene(const visual::Step& s, int w, int h, juce::String& firstError) {
        using namespace juce::gl;
        bool anyLayer = false;
        if (layerProgram_ != nullptr) {
            for (const auto& l : s.layers) {
                if (l.src < 0 || l.opacity <= 0.001f) continue;
                layerProgram_->use();
                const auto pid = layerProgram_->getProgramID();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, stepTex(l.src));
                glUniform1i(glGetUniformLocation(pid, "tex"), 0);
                glUniform1f(glGetUniformLocation(pid, "opacity"),
                            juce::jlimit(0.0f, 1.0f, l.opacity));
                glUniform1i(glGetUniformLocation(pid, "mode"), l.blend);
                if (anyLayer) {
                    glEnable(GL_BLEND);
                    switch (l.blend) {
                        case 1: glBlendFunc(GL_ONE, GL_ONE); break;
                        case 2: glBlendFunc(GL_DST_COLOR, GL_ZERO); break;
                        case 3: glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE); break;
                        default: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                } else {
                    glDisable(GL_BLEND);
                }
                drawQuad(pid);
                anyLayer = true;
            }
        }

        auto& sp = scenePrograms_[s.node];
        if (sp.program == nullptr || !(sp.compiled == s.scene)) {
            juce::String err;
            const auto builtin = juce::String(humPreamble()) + builtinScene();
            releaseSceneTextures(sp);
            sp.healed.clear();
            sp.program = s.scene.fragment.isNotEmpty()
                             ? compileScene(ctx_, s.scene.fragment, &err, nullptr,
                                            nullptr, &sp.healed)
                             : build(builtin.toRawUTF8(), &err);
            if (sp.program == nullptr) sp.program = build(builtin.toRawUTF8(), nullptr);
            sp.compiled = s.scene;
            sp.error = err;
        }
        if (sp.error.isNotEmpty() && firstError.isEmpty())
            firstError = juce::String(s.node) + ": " + sp.error;
        if (sp.program == nullptr) return;

        auto applyOutputBlend = [&] {
            glEnable(GL_BLEND);
            if (anyLayer) {
                glBlendColor(0.0f, 0.0f, 0.0f,
                             juce::jlimit(0.0f, 1.0f, s.sceneOpacity));
                if (s.sceneBlend == 1) glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
                else glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
            } else {
                glDisable(GL_BLEND);
            }
        };

        sp.program->use();
        const auto pid = sp.program->getProgramID();
        const auto loc = [pid](const char* n) {
            return juce::gl::glGetUniformLocation(pid, n);
        };
        const auto set1 = [&loc](const char* n, float v) {
            const auto l = loc(n);
            if (l >= 0) juce::gl::glUniform1f(l, v);
        };
        const auto set1i = [&loc](const char* n, int v) {
            const auto l = loc(n);
            if (l >= 0) juce::gl::glUniform1i(l, v);
        };
        for (const char* n : {"hum_Resolution", "RENDERSIZE", "a_size"}) {
            const auto l = loc(n);
            if (l >= 0) glUniform2f(l, (float) w, (float) h);
        }
        {
            const auto l = loc("hum_Bands");
            if (l >= 0) glUniform1fv(l, visual::kBands, s.bands.band);
        }
        set1("hum_Time", s.time);
        set1("hum_Beat", s.beat);
        set1("hum_BPM", s.bpm);
        set1("hum_OnBeat", s.onBeat);
        set1("hum_Level", s.bands.level);
        set1("hum_Brightness", s.brightness);
        set1("hum_Knob1", s.knob[0]);
        set1("hum_Knob2", s.knob[1]);
        set1("hum_Knob3", s.knob[2]);
        set1("hum_Knob4", s.knob[3]);
        set1("TIME", s.time);
        set1("TIMEDELTA", s.timeDelta);
        set1i("FRAMEINDEX", s.frameIndex);
        set1("FRAMECOUNT", (float) s.frameIndex);
        set1i("PASSINDEX", 0);
        {
            const auto l = loc("DATE");
            if (l >= 0) glUniform4f(l, s.date[0], s.date[1], s.date[2], s.date[3]);
        }
        for (const auto& hu : sp.healed) {
            const auto l = loc(hu.name.toRawUTF8());
            if (l < 0) continue;
            if (hu.comps == 1) glUniform1f(l, 0.5f);
            else if (hu.comps == 2) glUniform2f(l, 0.5f, 0.5f);
            else if (hu.comps == 3) glUniform3f(l, 0.5f, 0.5f, 0.5f);
            else glUniform4f(l, 0.5f, 0.5f, 0.5f, 1.0f);
        }
        for (const auto& u : s.extra) {
            const auto l = loc(u.name);
            if (l < 0) continue;
            if (u.isInt) glUniform1i(l, (int) u.v[0]);
            else if (u.comps == 1) glUniform1f(l, u.v[0]);
            else if (u.comps == 2) glUniform2f(l, u.v[0], u.v[1]);
            else if (u.comps == 3) glUniform3f(l, u.v[0], u.v[1], u.v[2]);
            else glUniform4f(l, u.v[0], u.v[1], u.v[2], u.v[3]);
        }
        if (s.scene.audioTex.isNotEmpty())
            uploadTex(pid, texWave_, s.wave, visual::kWaveTex, 1, s.scene.audioTex);
        if (s.scene.fftTex.isNotEmpty())
            uploadTex(pid, texFft_, s.fft, visual::kFftTex, 2, s.scene.fftTex);

        ensureSceneTextures(sp, s.scene, w, h);

        if (s.scene.wantsUnder && sp.under != 0) {
            glActiveTexture((GLenum) (GL_TEXTURE0 + kUnderUnit));
            glBindTexture(GL_TEXTURE_2D, sp.under);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
            const auto l = loc("u_texture");
            if (l >= 0) glUniform1i(l, kUnderUnit);
        }

        if (sp.imageTex.size() != s.scene.images.size()) {
            sp.imageTex.clear();
            for (const auto& im : s.scene.images) {
                auto t = std::make_unique<juce::OpenGLTexture>();
                t->loadImage(im.image);
                sp.imageTex.push_back(std::move(t));
            }
        }
        const int imageUnit0 = kFeedbackUnit0 + s.scene.buffers.size();
        for (int j = 0; j < (int) sp.imageTex.size(); ++j) {
            glActiveTexture((GLenum) (GL_TEXTURE0 + imageUnit0 + j));
            sp.imageTex[(size_t) j]->bind();
            const auto l = loc(s.scene.images[(size_t) j].name.toRawUTF8());
            if (l >= 0) glUniform1i(l, imageUnit0 + j);
        }

        const int finalUnit = imageUnit0 + (int) sp.imageTex.size();
        if (sp.finalTex != 0) {
            glActiveTexture((GLenum) (GL_TEXTURE0 + finalUnit));
            glBindTexture(GL_TEXTURE_2D, sp.finalTex);
            const auto l = loc("syn_FinalPass");
            if (l >= 0) glUniform1i(l, finalUnit);
        }

        for (int j = 0; j < (int) sp.feedback.size(); ++j) {
            glActiveTexture((GLenum) (GL_TEXTURE0 + kFeedbackUnit0 + j));
            glBindTexture(GL_TEXTURE_2D, sp.feedback[(size_t) j].tex[sp.feedback[(size_t) j].cur]);
            const auto l = loc(s.scene.buffers[j].toRawUTF8());
            if (l >= 0) glUniform1i(l, kFeedbackUnit0 + j);
        }
        glActiveTexture(GL_TEXTURE0);

        const int passes = juce::jmax(1, s.scene.passes);
        GLint stepFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &stepFbo);
        for (int pass = 0; pass < passes; ++pass) {
            set1("syn_PassIndex", (float) pass);
            const bool last = pass == passes - 1;
            if (last) {
                glBindFramebuffer(GL_FRAMEBUFFER, (GLuint) stepFbo);
                applyOutputBlend();
                drawQuad(pid);
                if (sp.finalTex != 0) {
                    glActiveTexture((GLenum) (GL_TEXTURE0 + finalUnit));
                    glBindTexture(GL_TEXTURE_2D, sp.finalTex);
                    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
                    glActiveTexture(GL_TEXTURE0);
                }
                continue;
            }
            if (pass >= (int) sp.feedback.size()) continue;
            auto& fb = sp.feedback[(size_t) pass];
            glBindFramebuffer(GL_FRAMEBUFFER, passFbo_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, fb.tex[1 - fb.cur], 0);
            glDisable(GL_BLEND);
            drawQuad(pid);
            fb.cur = 1 - fb.cur;
            glActiveTexture((GLenum) (GL_TEXTURE0 + kFeedbackUnit0 + pass));
            glBindTexture(GL_TEXTURE_2D, fb.tex[fb.cur]);
            glActiveTexture(GL_TEXTURE0);
        }
        glDisable(GL_BLEND);
    }

    void uploadTex(unsigned int programId, unsigned int& tex, const float* data,
                   int n, int unit, const juce::String& samplerName) {
        using namespace juce::gl;
        glActiveTexture((GLenum) (GL_TEXTURE0 + unit));
        if (tex == 0) {
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            glBindTexture(GL_TEXTURE_2D, tex);
        }
        unsigned char buf[4 * 512];
        for (int i = 0; i < n; ++i) {
            const float c = juce::jlimit(0.0f, 1.0f, data[i]);
            const auto b = (unsigned char) juce::roundToInt(c * 255.0f);
            buf[i * 4 + 0] = buf[i * 4 + 1] = buf[i * 4 + 2] = b;
            buf[i * 4 + 3] = 255;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, n, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        const auto l = glGetUniformLocation(programId, samplerName.toRawUTF8());
        if (l >= 0) glUniform1i(l, unit);
        const auto sz = glGetUniformLocation(
            programId, ("_" + samplerName + "_imgSize").toRawUTF8());
        if (sz >= 0) glUniform2f(sz, (float) n, 1.0f);
    }

    juce::OpenGLContext ctx_;
    std::unique_ptr<juce::OpenGLShaderProgram> deckProgram_, layerProgram_,
        mixProgram_, presentProgram_;
    std::map<std::string, SceneProg> scenePrograms_;
    std::map<std::string, DeckTex> deckTex_;
    std::vector<Fbo> fbos_;
    int fboW_ = 0, fboH_ = 0;
    unsigned int quad_ = 0, texWave_ = 0, texFft_ = 0, texBlack_ = 0;
    unsigned int passFbo_ = 0;

    static constexpr int kPrevW = 160, kPrevH = 90;
    std::string previewNode_;
    unsigned int previewFbo_ = 0, previewTex_ = 0;
    std::vector<std::uint8_t> previewPixels_;
    int previewTick_ = 0;

    bool paused_ = false;

    juce::SpinLock lock_;
    visual::Plan plan_;
    juce::String error_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlCanvas)
};

}
