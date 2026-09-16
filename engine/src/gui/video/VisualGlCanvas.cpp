// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VisualGlCanvas.h"

#include <cstdint>

namespace hum {

GlCanvas::GlCanvas() {
    setOpaque(true);
    ctx_.setRenderer(this);
    ctx_.setComponentPaintingEnabled(false);
    ctx_.setContinuousRepainting(true);
    ctx_.attachTo(*this);
}

void GlCanvas::setPlan(visual::Plan&& p) {
    setPlan(std::make_shared<const visual::Plan>(std::move(p)));
}

void GlCanvas::setPlan(std::shared_ptr<const visual::Plan> fresh) {
    std::shared_ptr<const visual::Plan> retired;
    {
        const juce::SpinLock::ScopedLockType sl(lock_);
        retired = std::move(plan_);
        plan_ = std::move(fresh);
        ++planStamp_;
    }
}

void GlCanvas::setPaused(bool paused) {
    if (paused == paused_) return;
    paused_ = paused;
    ctx_.setContinuousRepainting(!paused);
}

void GlCanvas::newOpenGLContextCreated() {
    using namespace juce::gl;
    glGenBuffers(1, &quad_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_);
    static const float verts[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                                  1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f, 1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
}

void GlCanvas::openGLContextClosing() {
    using namespace juce::gl;
    deckProgram_.reset();
    deckRectProgram_.reset();
    deckYCoCgProgram_.reset();
    layerProgram_.reset();
    mixProgram_.reset();
    fxProgram_.reset();
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
    releaseTap(grabTarget_);
}

void GlCanvas::renderOpenGL() {
    using namespace juce::gl;
    renders_.fetch_add(1);
    std::shared_ptr<const visual::Plan> held;
    std::uint64_t stamp = 0;
    {
        const juce::SpinLock::ScopedLockType sl(lock_);
        held = plan_;
        stamp = planStamp_;
    }
    static const visual::Plan kEmptyPlan;
    const visual::Plan& p = held ? *held : kEmptyPlan;
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
            case visual::Step::Fx: renderFx(s, w, h); break;
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
    serveGrab(p, stamp);
    {
        const juce::SpinLock::ScopedLockType sl(lock_);
        error_ = firstError;
    }
}

void GlCanvas::destroyFbos() {
    using namespace juce::gl;
    for (auto& f : fbos_) {
        if (f.fbo != 0) glDeleteFramebuffers(1, &f.fbo);
        if (f.tex != 0) glDeleteTextures(1, &f.tex);
    }
    fbos_.clear();
}

void GlCanvas::ensureFbos(int count, int w, int h) {
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

void GlCanvas::ensureBlackTex() {
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

void GlCanvas::renderMix(const visual::Step& s) {
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
    const auto gains = visual::mixGains(s);
    glUniform1f(glGetUniformLocation(pid, "gainA"), gains.first);
    glUniform1f(glGetUniformLocation(pid, "gainB"), gains.second);
    drawQuad(pid);
    glActiveTexture(GL_TEXTURE0);
}

void GlCanvas::renderFx(const visual::Step& s, int w, int h) {
    using namespace juce::gl;
    if (fxProgram_ == nullptr) return;
    fxProgram_->use();
    const auto pid = fxProgram_->getProgramID();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, stepTex(s.mixA));
    glUniform1i(glGetUniformLocation(pid, "tex"), 0);
    const auto& f = s.fx;
    glUniform2f(glGetUniformLocation(pid, "pos"), f.posX, f.posY);
    glUniform1f(glGetUniformLocation(pid, "scale"), f.scale);
    glUniform1f(glGetUniformLocation(pid, "rotate"), f.rotate);
    glUniform1f(glGetUniformLocation(pid, "aspect"), h > 0 ? (float) w / (float) h : 1.0f);
    glUniform1f(glGetUniformLocation(pid, "brightness"), f.brightness);
    glUniform1f(glGetUniformLocation(pid, "contrast"), f.contrast);
    glUniform1f(glGetUniformLocation(pid, "saturation"), f.saturation);
    glUniform1f(glGetUniformLocation(pid, "hue"), f.hue);
    glUniform1f(glGetUniformLocation(pid, "invert"), f.invert);
    glUniform1f(glGetUniformLocation(pid, "pixelate"), juce::jlimit(0.0f, 1.0f, f.pixelate));
    glUniform1i(glGetUniformLocation(pid, "mirror"), f.mirror);
    drawQuad(pid);
}

}
