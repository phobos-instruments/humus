// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
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

#include "gui/style/Colours.h"
#include "gui/video/SceneAssemble.h"
#include "gui/video/SceneCompile.h"
#include "gui/video/VideoPreviewStore.h"
#include "gui/video/VisualPlan.h"
#include "gui/common/Localisation.h"

namespace hum {

class GlCanvas : public juce::Component, public juce::OpenGLRenderer {
public:
    GlCanvas();
    ~GlCanvas() override { ctx_.detach(); }

    void setPlan(const visual::Plan& p) { setPlan(std::make_shared<const visual::Plan>(p)); }
    void setPlan(visual::Plan&& p);
    void setPlan(std::shared_ptr<const visual::Plan> fresh);

    std::uint64_t planStamp() const {
        const juce::SpinLock::ScopedLockType sl(lock_);
        return planStamp_;
    }
    juce::String compileError() const {
        const juce::SpinLock::ScopedLockType sl(lock_);
        return error_;
    }

    void setPaused(bool paused);

    void pump() { ctx_.triggerRepaint(); }

    void requestGrab(int w, int h);

    bool wouldServeForTest(std::uint64_t stamp) const {
        const juce::SpinLock::ScopedLockType sl(grabLock_);
        return grabAwaits(stamp);
    }

    bool takeGrab(std::vector<std::uint8_t>& rgba);

    static const char* humPreamble() { return sceneHumPreamble(); }

    static const char* builtinScene();

    void newOpenGLContextCreated() override;

    void openGLContextClosing() override;

    void renderOpenGL() override;

    bool grabAwaits(std::uint64_t stamp) const { return grabWanted_ && stamp >= grabStamp_; }

    void serveGrab(const visual::Plan& p, std::uint64_t stamp);

    void publishTaps(const visual::Plan& p);

    juce::SpinLock grabLock_;
    std::vector<std::uint8_t> grabbed_;
    int grabW_ = 0, grabH_ = 0;
    std::uint64_t grabStamp_ = 0;
    bool grabWanted_ = false, grabReady_ = false;

    struct PendingRead {
        int w = 0, h = 0;
        double beat = 0.0, tempo = 120.0;
        bool rolling = false;
    };

    struct TapTarget {
        unsigned int fbo = 0, tex = 0, pbo[2] = {0, 0};
        int w = 0, h = 0, next = 0;
        size_t pboBytes[2] = {0, 0};
        PendingRead pending[2];
    };

    void ensureTapTarget(TapTarget& rb, int w, int h);

    void releaseTap(TapTarget& rb);

    void setPreviewNode(std::string n) { previewNode_ = std::move(n); }

    unsigned renderCount() const { return renders_.load(); }

    void setPreviewSize(int w, int h);

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

    void releaseSceneTextures(SceneProg& sp);

    static unsigned int makeSceneTexture(int w, int h, bool floatFmt);

    void ensureSceneTextures(SceneProg& sp, const visual::SceneSpec& spec, int w, int h);

    void drawNoSignal(int w, int h);

    void drawQuad(unsigned int programId);

    static const char* vertexSrc() { return sceneVertexSrc(); }

    std::unique_ptr<juce::OpenGLShaderProgram> build(const char* frag, juce::String* err);

    std::unique_ptr<juce::OpenGLShaderProgram> buildDeck(bool rect, bool ycocg = false);

    void ensurePrograms();

    void destroyFbos();

    void ensureFbos(int count, int w, int h);

    void ensureBlackTex();

    unsigned int stepTex(int idx) const {
        return idx >= 0 && idx < (int) fbos_.size() ? fbos_[(size_t) idx].tex : texBlack_;
    }

    bool bindDeckSurface(DeckTex& dt, const VideoLayer::Frame& f);

    void uploadCompressedDeck(DeckTex& dt, const VideoLayer::Frame& f);

    void renderDeck(const visual::Step& s);

    void renderMix(const visual::Step& s);

    void renderFx(const visual::Step& s, int w, int h);

    void renderScene(const visual::Step& s, int w, int h, juce::String& firstError);

    void uploadTex(unsigned int programId, unsigned int& tex, const float* data, int n, int unit, const juce::String& samplerName);

    juce::OpenGLContext ctx_;
    std::unique_ptr<juce::OpenGLShaderProgram> deckProgram_, deckRectProgram_,
        deckYCoCgProgram_, layerProgram_,
        mixProgram_, fxProgram_, presentProgram_;
    std::map<std::string, SceneProg> scenePrograms_;
    std::map<std::string, DeckTex> deckTex_;
    std::vector<Fbo> fbos_;
    int fboW_ = 0, fboH_ = 0;
    unsigned int quad_ = 0, texWave_ = 0, texFft_ = 0, texBlack_ = 0;
    unsigned int passFbo_ = 0;

    int prevW_ = 160, prevH_ = 90;
    std::string previewNode_;
    std::map<std::string, TapTarget> taps_;
    TapTarget grabTarget_;
    std::atomic<unsigned> renders_{0};
    int previewTick_ = 0;

    bool paused_ = false;

    juce::SpinLock lock_;
    std::shared_ptr<const visual::Plan> plan_;
    std::uint64_t planStamp_ = 0;
    juce::String error_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlCanvas)
};

}
