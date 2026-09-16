// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VisualGlCanvas.h"

namespace hum {

void GlCanvas::releaseSceneTextures(SceneProg& sp) {
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

unsigned int GlCanvas::makeSceneTexture(int w, int h, bool floatFmt) {
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

void GlCanvas::ensureSceneTextures(SceneProg& sp, const visual::SceneSpec& spec, int w, int h) {
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

void GlCanvas::renderScene(const visual::Step& s, int w, int h, juce::String& firstError) {
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

}
