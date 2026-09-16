// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VisualGlCanvas.h"

#include <cstdint>
#include <cstring>

namespace hum {

void GlCanvas::requestGrab(int w, int h) {
    const auto asked = planStamp();
    const juce::SpinLock::ScopedLockType sl(grabLock_);
    grabW_ = w;
    grabH_ = h;
    grabReady_ = false;
    grabStamp_ = asked;
    grabWanted_ = w > 0 && h > 0;
}

bool GlCanvas::takeGrab(std::vector<std::uint8_t>& rgba) {
    const juce::SpinLock::ScopedLockType sl(grabLock_);
    if (!grabReady_) return false;
    rgba = std::move(grabbed_);
    grabReady_ = false;
    return true;
}

void GlCanvas::serveGrab(const visual::Plan& p, std::uint64_t stamp) {
    using namespace juce::gl;
    int want = 0, wantH = 0;
    {
        const juce::SpinLock::ScopedLockType sl(grabLock_);
        if (!grabAwaits(stamp)) return;
        want = grabW_;
        wantH = grabH_;
        grabWanted_ = false;
    }
    ensureTapTarget(grabTarget_, want, wantH);
    glBindFramebuffer(GL_FRAMEBUFFER, grabTarget_.fbo);
    glViewport(0, 0, want, wantH);
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
    std::vector<std::uint8_t> upside((size_t) want * (size_t) wantH * 4u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, want, wantH, GL_RGBA, GL_UNSIGNED_BYTE, upside.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    const size_t row = (size_t) want * 4u;
    std::vector<std::uint8_t> right(upside.size());
    for (int y = 0; y < wantH; ++y)
        std::memcpy(right.data() + (size_t) y * row,
                    upside.data() + (size_t) (wantH - 1 - y) * row, row);
    const juce::SpinLock::ScopedLockType sl(grabLock_);
    grabbed_ = std::move(right);
    grabReady_ = true;
}

void GlCanvas::publishTaps(const visual::Plan& p) {
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
        if (t.w <= 0 || t.h <= 0 || (t.sink == nullptr && !store.wanted(t.node))) continue;
        const auto key = t.sink != nullptr ? t.node + "/take" : t.node;
        live.insert(key);
        if (t.everyOther && previewTick_ == 0) continue;
        auto& rb = taps_[key];
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
        rb.pending[rb.next] = {t.w, t.h, p.beat, p.tempo, p.rolling};
        rb.next ^= 1;
        const auto ready = rb.pending[rb.next];
        if (ready.w > 0
            && rb.pboBytes[rb.next] == (size_t) ready.w * (size_t) ready.h * 4u) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, rb.pbo[rb.next]);
            if (const auto* mapped = (const std::uint8_t*) glMapBuffer(
                    GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)) {
                if (t.sink != nullptr)
                    t.sink->pushFrame(mapped, ready.w, ready.h, ready.beat, ready.tempo,
                                      ready.rolling);
                else
                    store.publishFlipped(t.node, ready.w, ready.h, mapped);
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

void GlCanvas::ensureTapTarget(TapTarget& rb, int w, int h) {
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
    rb.pending[0] = rb.pending[1] = {};
}

void GlCanvas::releaseTap(TapTarget& rb) {
    using namespace juce::gl;
    if (rb.fbo != 0) glDeleteFramebuffers(1, &rb.fbo);
    if (rb.tex != 0) glDeleteTextures(1, &rb.tex);
    if (rb.pbo[0] != 0) glDeleteBuffers(2, rb.pbo);
    rb = {};
}

void GlCanvas::setPreviewSize(int w, int h) {
    if (w > 0 && h > 0) {
        prevW_ = w;
        prevH_ = h;
    }
}

}
