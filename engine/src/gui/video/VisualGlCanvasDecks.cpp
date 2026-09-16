// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VisualGlCanvas.h"

namespace hum {

bool GlCanvas::bindDeckSurface(DeckTex& dt, const VideoLayer::Frame& f) {
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

void GlCanvas::uploadCompressedDeck(DeckTex& dt, const VideoLayer::Frame& f) {
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

void GlCanvas::renderDeck(const visual::Step& s) {
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

void GlCanvas::uploadTex(unsigned int programId, unsigned int& tex, const float* data, int n, int unit, const juce::String& samplerName) {
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

}
