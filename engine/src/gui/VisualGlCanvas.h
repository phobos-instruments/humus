#pragma once
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

#if JUCE_MAC
#include <CoreVideo/CoreVideo.h>
#include <IOSurface/IOSurface.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLIOSurface.h>
#endif

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
        deckRectProgram_.reset();
        deckYCoCgProgram_.reset();
        layerProgram_.reset();
        mixProgram_.reset();
        presentProgram_.reset();
        for (auto& [n, sp] : scenePrograms_) releaseSceneTextures(sp);
        scenePrograms_.clear();
        if (passFbo_ != 0) { glDeleteFramebuffers(1, &passFbo_); passFbo_ = 0; }
        destroyFbos();
        for (auto& [n, t] : deckTex_) {
            if (t.tex != 0) glDeleteTextures(1, &t.tex);
            if (t.rectTex != 0) glDeleteTextures(1, &t.rectTex);
        }
        deckTex_.clear();
        if (quad_ != 0) { glDeleteBuffers(1, &quad_); quad_ = 0; }
        for (auto* t : {&texWave_, &texFft_, &texBlack_})
            if (*t != 0) { glDeleteTextures(1, t); *t = 0; }
        for (auto& [n, rb] : taps_) releaseTap(rb);
        taps_.clear();
    }

    void renderOpenGL() override {
        using namespace juce::gl;
        renders_.fetch_add(1);
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
                case visual::Step::Deck: renderDeck(s); break;
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
        publishTaps(p);
        {
            const juce::SpinLock::ScopedLockType sl(lock_);
            error_ = firstError;
        }
    }

    void publishTaps(const visual::Plan& p) {
        using namespace juce::gl;
        auto& store = VideoPreviewStore::instance();
        auto taps = p.taps;
        if (!previewNode_.empty()) {
            visual::Tap t;
            t.node = previewNode_;
            t.step = p.root;
            t.w = prevW_;
            t.h = prevH_;
            t.fade = p.masterFade;
            t.everyOther = true;
            taps.push_back(std::move(t));
        }
        previewTick_ ^= 1;
        std::set<std::string> live;
        for (const auto& t : taps) {
            if (t.w <= 0 || t.h <= 0 || !store.wanted(t.node)) continue;
            live.insert(t.node);
            if (t.everyOther && previewTick_ == 0) continue;
            auto& rb = taps_[t.node];
            ensureTapTarget(rb, t.w, t.h);
            glBindFramebuffer(GL_FRAMEBUFFER, rb.fbo);
            glViewport(0, 0, t.w, t.h);
            juce::OpenGLHelpers::clear(juce::Colours::black);
            if (t.step >= 0 && t.step < (int) fbos_.size()
                && presentProgram_ != nullptr) {
                presentProgram_->use();
                const auto pid = presentProgram_->getProgramID();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fbos_[(size_t) t.step].tex);
                glUniform1i(glGetUniformLocation(pid, "tex"), 0);
                glUniform1f(glGetUniformLocation(pid, "fade"),
                            juce::jlimit(0.0f, 1.0f, t.fade));
                drawQuad(pid);
            }
            const size_t bytes = (size_t) t.w * (size_t) t.h * 4u;
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, rb.pbo[rb.next]);
            if (rb.pboBytes[rb.next] != bytes) {
                glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr) bytes, nullptr,
                             GL_STREAM_READ);
                rb.pboBytes[rb.next] = bytes;
            }
            glReadPixels(0, 0, t.w, t.h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            rb.pending[rb.next] = {t.w, t.h};
            rb.next ^= 1;
            const auto ready = rb.pending[rb.next];
            if (ready.first > 0
                && rb.pboBytes[rb.next]
                       == (size_t) ready.first * (size_t) ready.second * 4u) {
                glBindBuffer(GL_PIXEL_PACK_BUFFER, rb.pbo[rb.next]);
                if (const auto* mapped = (const std::uint8_t*) glMapBuffer(
                        GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)) {
                    store.publishFlipped(t.node, ready.first, ready.second, mapped);
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                }
            }
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        for (auto it = taps_.begin(); it != taps_.end();)
            it = live.count(it->first) ? std::next(it) : (releaseTap(it->second),
                                                          taps_.erase(it));
    }

    struct TapTarget {
        unsigned int fbo = 0, tex = 0, pbo[2] = {0, 0};
        int w = 0, h = 0, next = 0;
        size_t pboBytes[2] = {0, 0};
        std::pair<int, int> pending[2] = {{0, 0}, {0, 0}};
    };

    void ensureTapTarget(TapTarget& rb, int w, int h) {
        using namespace juce::gl;
        if (rb.fbo != 0 && rb.w == w && rb.h == h) return;
        if (rb.fbo == 0) {
            glGenFramebuffers(1, &rb.fbo);
            glGenTextures(1, &rb.tex);
            glGenBuffers(2, rb.pbo);
        }
        glBindTexture(GL_TEXTURE_2D, rb.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, rb.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, rb.tex, 0);
        rb.w = w;
        rb.h = h;
        rb.pending[0] = rb.pending[1] = {0, 0};
    }

    void releaseTap(TapTarget& rb) {
        using namespace juce::gl;
        if (rb.fbo != 0) glDeleteFramebuffers(1, &rb.fbo);
        if (rb.tex != 0) glDeleteTextures(1, &rb.tex);
        if (rb.pbo[0] != 0) glDeleteBuffers(2, rb.pbo);
        rb = {};
    }

    void setPreviewNode(std::string n) { previewNode_ = std::move(n); }

    unsigned renderCount() const { return renders_.load(); }

    void setPreviewSize(int w, int h) {
        if (w > 0 && h > 0) {
            prevW_ = w;
            prevH_ = h;
        }
    }

private:
    struct Fbo { unsigned int fbo = 0, tex = 0; };
    struct DeckTex {
        unsigned int tex = 0;
        int w = 0, h = 0;
        unsigned int glFmt = 0;
        unsigned int rectTex = 0;
        int rectW = 0, rectH = 0;
        bool rect = false, ycocg = false;
        std::shared_ptr<const VideoLayer::Frame> uploaded;
    };
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

    std::unique_ptr<juce::OpenGLShaderProgram> buildDeck(bool rect, bool ycocg = false) {
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

    void ensurePrograms() {
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

    bool bindDeckSurface(DeckTex& dt, const VideoLayer::Frame& f) {
#if JUCE_MAC
        using namespace juce::gl;
        auto* pb = (CVPixelBufferRef) f.native;
        if (pb == nullptr) return false;
        IOSurfaceRef surf = CVPixelBufferGetIOSurface(pb);
        if (surf == nullptr) return false;
        if (dt.rectTex == 0) glGenTextures(1, &dt.rectTex);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, dt.rectTex);
        const auto w = (GLsizei) IOSurfaceGetWidth(surf);
        const auto h = (GLsizei) IOSurfaceGetHeight(surf);
        if (CGLTexImageIOSurface2D(CGLGetCurrentContext(), GL_TEXTURE_RECTANGLE_ARB,
                                   GL_RGBA, w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                                   surf, 0)
            != kCGLNoError)
            return false;
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        dt.rectW = (int) w;
        dt.rectH = (int) h;
        return true;
#else
        juce::ignoreUnused(dt, f);
        return false;
#endif
    }

    void uploadCompressedDeck(DeckTex& dt, const VideoLayer::Frame& f) {
        using namespace juce::gl;
        constexpr unsigned int kDxt1 = 0x83F0, kDxt5 = 0x83F3;
        const unsigned int fmt = f.fmt == VideoLayer::Frame::DXT1 ? kDxt1 : kDxt5;
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
        if (dt.w == f.width && dt.h == f.height && dt.glFmt == fmt) {
            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, f.width, f.height, fmt,
                                      (GLsizei) f.blocks.size(), f.blocks.data());
        } else {
            dt.w = f.width;
            dt.h = f.height;
            dt.glFmt = fmt;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, fmt, f.width, f.height, 0,
                                   (GLsizei) f.blocks.size(), f.blocks.data());
        }
        dt.rect = false;
        dt.ycocg = f.fmt == VideoLayer::Frame::YCoCgDXT5;
    }

    void renderDeck(const visual::Step& s) {
        using namespace juce::gl;
        auto& dt = deckTex_[s.node];
        if (s.frame != nullptr && s.frame != dt.uploaded) {
            if (s.frame->fmt != VideoLayer::Frame::BGRA && !s.frame->blocks.empty()) {
                uploadCompressedDeck(dt, *s.frame);
                dt.uploaded = s.frame;
            } else if (bindDeckSurface(dt, *s.frame)) {
                dt.rect = true;
                dt.ycocg = false;
                dt.uploaded = s.frame;
            } else if (!s.frame->bgra.empty()) {
                dt.rect = false;
                dt.ycocg = false;
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
                if (dt.w == s.frame->width && dt.h == s.frame->height
                    && dt.glFmt == 0) {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dt.w, dt.h, GL_BGRA,
                                    GL_UNSIGNED_BYTE, s.frame->bgra.data());
                } else {
                    dt.w = s.frame->width;
                    dt.h = s.frame->height;
                    dt.glFmt = 0;
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dt.w, dt.h, 0, GL_BGRA,
                                 GL_UNSIGNED_BYTE, s.frame->bgra.data());
                }
                dt.uploaded = s.frame;
            }
        }
        auto* program = dt.rect ? deckRectProgram_.get()
                      : dt.ycocg ? deckYCoCgProgram_.get()
                                 : deckProgram_.get();
        const auto bound = dt.rect ? dt.rectTex : dt.tex;
        if (!s.active || bound == 0 || program == nullptr) return;
        program->use();
        const auto pid = program->getProgramID();
        glActiveTexture(GL_TEXTURE0);
#if JUCE_MAC
        if (dt.rect) {
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bound);
            glUniform2f(glGetUniformLocation(pid, "texSize"), (float) dt.rectW,
                        (float) dt.rectH);
        } else
#endif
            glBindTexture(GL_TEXTURE_2D, bound);
        glUniform1i(glGetUniformLocation(pid, "tex"), 0);
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
    std::unique_ptr<juce::OpenGLShaderProgram> deckProgram_, deckRectProgram_,
        deckYCoCgProgram_, layerProgram_,
        mixProgram_, presentProgram_;
    std::map<std::string, SceneProg> scenePrograms_;
    std::map<std::string, DeckTex> deckTex_;
    std::vector<Fbo> fbos_;
    int fboW_ = 0, fboH_ = 0;
    unsigned int quad_ = 0, texWave_ = 0, texFft_ = 0, texBlack_ = 0;
    unsigned int passFbo_ = 0;

    int prevW_ = 160, prevH_ = 90;
    std::string previewNode_;
    std::map<std::string, TapTarget> taps_;
    std::atomic<unsigned> renders_{0};
    int previewTick_ = 0;

    bool paused_ = false;

    juce::SpinLock lock_;
    visual::Plan plan_;
    juce::String error_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlCanvas)
};

}
