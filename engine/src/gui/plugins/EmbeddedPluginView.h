// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/plugins/HostedPlugin.h"
#include "gui/plugins/DragPicture.h"
#include "gui/plugins/EmbedFit.h"
#include "gui/properties/DeviceStripView.h"
#include "gui/host/PluginsHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/plugins/EmbeddedDebug.h"
#include "gui/plugins/PluginEditorTeardown.h"
#include "gui/plugins/PluginParamMirror.h"
#include "gui/common/Localisation.h"

#if JUCE_MAC || JUCE_WINDOWS
namespace hum {
bool embeddedClipUpdate(juce::Component& editor, juce::ComponentPeer& peer,
                        juce::Rectangle<int> clipRegionInPeer,
                        juce::Point<int> editorOriginInPeer,
                        float scale, juce::Colour background, void*& handle,
                        juce::Point<int> panInPlugin);
bool embeddedClipHide(void*& handle);
void embeddedClipRemove(juce::Component& editor, juce::ComponentPeer* peer, void*& handle);
}
#endif
#if JUCE_WINDOWS
namespace hum {
float embeddedClipScale(void* handle);
bool embeddedClipIntact(void* handle);
bool embeddedClipSnapshot(void* handle, juce::Image& picture, juce::Rectangle<int>& areaInPeer);
}
#endif

namespace hum {

class EmbeddedPluginView : public juce::Component, private juce::Timer {
public:
    EmbeddedPluginView(PluginsHost& host, std::string name)
        : host_(host), name_(std::move(name)) {
        setOpaque(false);
        panBar_.setAutoHide(false);
        panBar_.addListener(&panListener_);
        addChildComponent(panBar_);
#if JUCE_WINDOWS
        juce::Desktop::getInstance().addGlobalMouseListener(&gestureWatch_);
#endif
        startTimerHz(30);
    }

    ~EmbeddedPluginView() override {
#if JUCE_WINDOWS
        juce::Desktop::getInstance().removeGlobalMouseListener(&gestureWatch_);
#endif
        stopTimer();
        onLayoutChanged = nullptr;
        onWantsFloat = nullptr;
        release();
    }

    int heightForWidth(int w) const {
        if (fit_.naturalW() <= 0 || fit_.naturalH() <= 0) return DeviceStripView::kHeight;
        if (fragile_ || !zoomable()) return fit_.naturalH() + (fit_.naturalW() > w ? kPanBarH : 0);
        return fit_.heightFor(w);
    }
    int naturalWidth() const { return fit_.naturalW() > 0 ? fit_.naturalW() : 360; }

    void release() {
        if (!editor_) return;
#if JUCE_MAC || JUCE_WINDOWS
        {
            EditorOpGuard g(classRaw_, "plugins.fragileEditor", "unwrapping its embedded UI");
            embeddedClipRemove(*editor_, getPeer(), nativeClip_);
        }
#endif
        removeChildComponent(editor_.get());
#if JUCE_MAC
        auto* pn = host_.pluginNodeFor(name_);
        if (pn && shouldLeakEditor(pn->classRaw())) {
            auto* leaked = editor_.release();
            if (auto* hp = host_.hostedPluginFor(name_))
                hp->instance()->editorBeingDeleted(leaked);
        } else {
            EditorOpGuard g(classRaw_, "plugins.leakEditor", "closing its editor");
            editor_.reset();
        }
#else
        editor_.reset();
#endif
        fit_.reset({});
        picture_.clear();
        actualSize_ = false;
        judgeTicks_ = 0;
        panX_ = 0;
        geomCached_ = wrapSettled_ = false;
        if (onLayoutChanged) onLayoutChanged();
        repaint();
    }

    std::function<void()> onLayoutChanged;
    std::function<void()> onWantsFloat;

    void visibilityChanged() override {
        startTimerHz(isShowing() ? 30 : 4);
        updateEditorPosition();
    }

    void resized() override {
        layoutPanBar();
        updateEditorPosition();
    }

    void paintOverChildren(juce::Graphics& g) override {
        if (!picture_.shown) return;
        g.fillAll(Palette::panel);
        picture_.paint(g, getWidth());
    }

    void paint(juce::Graphics& g) override {
        if (editor_) return;
        auto* hp = host_.hostedPluginFor(name_);
        const bool floatOwns = hp && hp->instance()->getActiveEditor();
        DeviceStripView::paintFace(
            g, host_, name_, getLocalBounds(),
            windowed_
                ? (floatOwns
                       ? juce::String(tr("embedded-plugin.ui-open-in-its-own", "UI open in its own window"))
                       : tr("embedded-plugin.brings-its-own-window", "This plugin brings its own window  -  click to open it"))
                : floatOwns
                    ? tr("embedded-plugin.ui-in-float-window", "UI in Float window  -  close it to embed here")
                    : tr("embedded-plugin.loading", "Plugin UI  -  loading..."));
    }

    void mouseDown(const juce::MouseEvent&) override {
        if (!editor_ && onWantsFloat) onWantsFloat();
    }

    bool hasEditor() const { return editor_ != nullptr; }
    bool actualSize() const { return actualSize_; }

    void setActualSize(bool on) {
        if (classRaw_.empty()) return;
        if (on) embedsize::rememberActualSize(classRaw_);
        else    embedsize::rememberFit(classRaw_);
        applyActualSize(on);
    }

    bool scaledForTest() const { return zoomable(); }
    int panForTest() const { return panX_; }

private:

    struct PanListener : juce::ScrollBar::Listener {
        explicit PanListener(EmbeddedPluginView& o) : owner(o) {}
        void scrollBarMoved(juce::ScrollBar*, double start) override {
            const int want = juce::jlimit(0, owner.panRange(), (int) std::lround(start));
            if (want == owner.panX_) return;
            owner.panX_ = want;
            owner.geomCached_ = false;
            owner.updateEditorPosition();
        }
        EmbeddedPluginView& owner;
    };

    struct GestureWatch : juce::MouseListener {
        explicit GestureWatch(EmbeddedPluginView& o) : owner(o) {}
        void mouseDown(const juce::MouseEvent&) override { owner.takePicture(); }
        EmbeddedPluginView& owner;
    };

    struct PositionTracker : juce::ComponentMovementWatcher {
        explicit PositionTracker(EmbeddedPluginView& o)
            : juce::ComponentMovementWatcher(&o), owner(o) {}
        void componentMovedOrResized(bool, bool) override { owner.updateEditorPosition(); }
        void componentPeerChanged()               override { owner.updateEditorPosition(); }
        void componentVisibilityChanged()         override { owner.updateEditorPosition(); }
        EmbeddedPluginView& owner;
    };

    juce::Rectangle<int> pluginArea() const {
        return panBar_.isVisible() ? getLocalBounds().withTrimmedBottom(kPanBarH)
                                   : getLocalBounds();
    }

    void updateEditorPosition() {
        if (!editor_) return;
        if (editor_->getParentComponent() != this) return;
        auto* peer = getPeer();
        juce::Rectangle<int> vis, clipRegion;
        if (peer && isShowing()) {
            auto& pc = peer->getComponent();
            vis = clipRegion = pc.getLocalArea(this, pluginArea());
            if (auto* vp = enclosingViewport()) {
                clipRegion = pc.getLocalArea(vp->getViewedComponent(), vp->getViewArea());
                vis = vis.getIntersection(clipRegion);
            }
        }
        const float scaleNow = scaleFactor();
        if (geomCached_ && wrapSettled_ && peer == lastPeer_ && vis == lastVis_
            && clipRegion == lastClip_ && std::abs(scaleNow - lastScale_) < 0.0005f && nativeClipIntact()) {
            if (vis.isEmpty()) return;
            auto originNow = peer->getComponent().getLocalPoint(this, juce::Point<int>());
            if (originNow == lastOrigin_) return;
        }
        geomCached_ = true;
        lastPeer_ = peer;
        lastVis_ = vis;
        lastClip_ = clipRegion;
        lastScale_ = scaleNow;
        lastOrigin_ = peer && isShowing()
                          ? peer->getComponent().getLocalPoint(this, juce::Point<int>())
                          : juce::Point<int>();
        if (!vis.isEmpty()) {
            editor_->setVisible(true);
            editor_->setTopLeftPosition(0, 0);
            if (fragile_) {
                if (editor_->isTransformed()) editor_->setTransform({});
                wrapSettled_ = true;
                return;
            }
            const float s = scaleNow;
            bool native = false;
#if JUCE_MAC
            const auto origin = lastOrigin_;
            const bool zooming = std::abs(s - 1.0f) > 0.01f;
            if (!survivedFirstWrap_ || (zooming && !survivedFirstZoom_)) {
                EditorOpGuard g(classRaw_, "plugins.fragileEditor",
                                zooming ? "zooming its embedded UI" : "embedding its UI");
                native = embeddedClipUpdate(*editor_, *peer, clipRegion, origin, s,
                                            Palette::panel, nativeClip_, {panX_, 0});
                const bool done = !native || nativeClip_ != nullptr;
                if (done) { survivedFirstWrap_ = true; if (zooming) survivedFirstZoom_ = true; }
            } else {
                native = embeddedClipUpdate(*editor_, *peer, clipRegion, origin, s,
                                            Palette::panel, nativeClip_, {panX_, 0});
            }
#elif JUCE_WINDOWS
            if (picture_.wantedAt(s) && buttonHeld()) {
                if (!picture_.shown) embeddedClipHide(nativeClip_);
                picture_.shown = true;
                repaint();
                native = true;
            } else {
                native = embeddedClipUpdate(*editor_, *peer, vis, lastOrigin_, s,
                                            Palette::panel, nativeClip_, {panX_, 0});
            }
#endif
            wrapSettled_ = !native || nativeClip_ != nullptr;
#if JUCE_MAC
            if (std::getenv("HUM_EMBED_DEBUG") != nullptr)
                std::cerr << "[embed] " << name_ << " vis=" << vis.toString()
                          << " clip=" << clipRegion.toString()
                          << " scale=" << s << " native=" << (int) native
                          << " wrapped=" << (nativeClip_ != nullptr ? 1 : 0)
                          << " fragile=" << (int) fragile_ << "\n"
                          << embeddedDebugSuperviews(*editor_)
                          << embeddedDebugWindowChildren(*peer);
            if (std::getenv("HUM_EMBED_DEBUG") != nullptr && peer) {
                auto& pc = peer->getComponent();
                for (auto* p = getParentComponent(); p && p != &pc; p = p->getParentComponent())
                    std::cerr << "  ancestor " << typeid(*p).name() << " "
                              << pc.getLocalArea(p, p->getLocalBounds()).toString() << "\n";
                std::cerr << std::endl;
            }
#endif
            if (native) {
                editor_->setTransform({});
            } else {
                editor_->setTransform(std::abs(s - 1.0f) > 0.001f
                                          ? juce::AffineTransform::scale(s)
                                          : juce::AffineTransform());
            }
        } else {
#if JUCE_MAC || JUCE_WINDOWS
            if (embeddedClipHide(nativeClip_)) {
                editor_->setVisible(true);
                editor_->setTopLeftPosition(0, 0);
                wrapSettled_ = true;
                return;
            }
#endif
            editor_->setTopLeftPosition(-10000, -10000);
            editor_->setVisible(false);
            wrapSettled_ = true;
        }
    }

    static constexpr int kPanBarH = 10;
    static constexpr int kJudgeTicks = 45;

    static bool buttonHeld() { return juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown(); }

    void takePicture() {
#if JUCE_WINDOWS
        picture_.clear();
        auto* peer = getPeer();
        if (!editor_ || !zoomable() || peer == nullptr || !isShowing()) return;
        juce::Image image;
        juce::Rectangle<int> inPeer;
        if (!embeddedClipSnapshot(nativeClip_, image, inPeer)) {
            fit_.freezeAt(fit_.scale(getWidth()));
            return;
        }
        picture_.image = image;
        picture_.area = getLocalArea(&peer->getComponent(),
                                     inPeer.toFloat() / (float) peer->getPlatformScaleFactor());
        picture_.widthWhenTaken = getWidth();
        picture_.scaleWhenTaken = scaleFactor();
#endif
    }

    void dropPicture() {
        const bool wasShown = picture_.shown;
        picture_.clear();
        if (!wasShown) return;
        geomCached_ = false;
        repaint();
    }

    bool nativeClipIntact() const {
#if JUCE_WINDOWS
        return embeddedClipIntact(nativeClip_);
#else
        return true;
#endif
    }

    float outerScale() const {
#if JUCE_WINDOWS
        auto* peer = getPeer();
        if (peer == nullptr || getWidth() <= 0) return 1.0f;
        return peer->getComponent().getLocalArea(this, getLocalBounds().toFloat()).getWidth() / (float) getWidth();
#else
        return 1.0f;
#endif
    }

    float appliedEditorScale() const {
#if JUCE_WINDOWS
        return embeddedClipScale(nativeClip_);
#else
        return 1.0f;
#endif
    }


    bool zoomable() const { return !actualSize_; }

    int panRange() const {
        return zoomable() ? 0 : juce::jmax(0, fit_.naturalW() - getWidth());
    }

    float scaleFactor() const {
        return (zoomable() ? fit_.scale(getWidth()) : 1.0f) * outerScale();
    }

    void layoutPanBar() {
        const int range = panRange();
        panBar_.setVisible(range > 0);
        if (range <= 0) { panX_ = 0; return; }
        panBar_.setBounds(getLocalBounds().removeFromBottom(kPanBarH));
        panBar_.setRangeLimits(0.0, (double) fit_.naturalW(), juce::dontSendNotification);
        panBar_.setCurrentRange((double) panX_, (double) getWidth(), juce::dontSendNotification);
        panX_ = juce::jlimit(0, range, panX_);
    }

    void timerCallback() override {
        auto* hp = host_.hostedPluginFor(name_);
        if (!hp) return;
        if (picture_.taken() && !buttonHeld()) dropPicture();
        if (!editor_ && !hp->instance()->getActiveEditor()) acquire();
#if JUCE_MAC
        if (editor_ && !windowed_ && ++ownWindowPolls_ % 30 == 0
            && std::getenv("HUM_EMBED_NOWRAP") == nullptr)
            if (auto* peer = getPeer())
                if (embeddedEditorUsesOwnWindow(*peer, fit_.naturalW(), fit_.naturalH())) {
                    windowed_ = true;
                    enrollWindowedEditor(classRaw_);
                    release();
                    if (onWantsFloat) onWantsFloat();
                    return;
                }
#endif
        updateEditorPosition();
        if (editor_ && editor_->getX() == 0) {
            if (mirrorPluginParams(host_, name_, *hp))
                host_.pokeLiveRefresh();
            const bool buttonDown = buttonHeld();
            if (fit_.observe({editor_->getWidth(), editor_->getHeight()}, appliedEditorScale(), outerScale(),
                             buttonDown, fit_.scale(getWidth()))
                || fit_.letGo(buttonDown, getWidth())) {
                geomCached_ = false;
                if (onLayoutChanged) onLayoutChanged();
            }
        }
        judgeZoom();
        startTimerHz(isShowing() ? 30 : 4);
    }

    void applyActualSize(bool on) {
        if (on == actualSize_) return;
        actualSize_ = on;
        if (on) panX_ = 0;
        geomCached_ = false;
        layoutPanBar();
        if (onLayoutChanged) onLayoutChanged();
        updateEditorPosition();
    }

    void judgeZoom() {
        if (!editor_ || actualSize_ || embedsize::wantsFit(classRaw_)) return;
        if (judgeTicks_ >= kJudgeTicks) return;
        ++judgeTicks_;
        if (!embeddedViewResistsZoom(*editor_) && !fit_.ignoresZoom()) return;
        embedsize::rememberActualSize(classRaw_);
        applyActualSize(true);
    }

    void acquire() {
        auto* hp = host_.hostedPluginFor(name_);
        if (!hp) return;
        if (hp->instance()->getActiveEditor()) return;
        classRaw_ = hp->classRaw();
        windowed_ = isWindowedEditor(classRaw_);
        if (windowed_) { repaint(); return; }
        fragile_ = isFragileEditor(classRaw_);
        actualSize_ = embedsize::wantsActualSize(classRaw_);
        survivedFirstWrap_ = survivedFirstZoom_ = false;
        geomCached_ = wrapSettled_ = false;
        juce::AudioProcessorEditor* ed = nullptr;
        {
            EditorOpGuard g(classRaw_, "plugins.uiQuarantine", "opening its editor");
            ed = hp->instance()->createEditorIfNeeded();
        }
        if (!ed) return;
#if JUCE_MAC
        if (!fragile_ && embeddedViewIsRemote(*ed)) fragile_ = true;
#endif
        fit_.reset({ed->getWidth(), ed->getHeight()});
        editor_.reset(ed);
        addAndMakeVisible(editor_.get());
        judgeZoom();
        updateEditorPosition();
        if (onLayoutChanged) onLayoutChanged();
        repaint();
    }

    juce::Viewport* enclosingViewport() const {
        for (auto* p = getParentComponent(); p; p = p->getParentComponent())
            if (auto* vp = dynamic_cast<juce::Viewport*>(p)) return vp;
        return nullptr;
    }

    PluginsHost& host_;
    std::string name_;
    std::unique_ptr<juce::AudioProcessorEditor> editor_;
    std::string classRaw_;
    EmbedFit fit_;
    DragPicture picture_;
    bool fragile_ = false;
    bool windowed_ = false;
    int ownWindowPolls_ = 0;
    bool survivedFirstWrap_ = false;
    bool survivedFirstZoom_ = false;
    void* nativeClip_ = nullptr;
    bool geomCached_ = false;
    bool wrapSettled_ = false;
    juce::ComponentPeer* lastPeer_ = nullptr;
    juce::Rectangle<int> lastVis_, lastClip_;
    juce::Point<int> lastOrigin_;
    float lastScale_ = 0.0f;
    bool actualSize_ = false;
    int judgeTicks_ = 0;
    int panX_ = 0;
    juce::ScrollBar panBar_{false};
    PanListener panListener_{*this};
    PositionTracker tracker_{*this};
    GestureWatch gestureWatch_{*this};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmbeddedPluginView)
};

}
