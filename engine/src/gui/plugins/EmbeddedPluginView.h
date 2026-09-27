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
#include "gui/properties/DeviceStripView.h"
#include "gui/host/PluginsHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/plugins/EmbeddedDebug.h"
#include "gui/plugins/PluginEditorTeardown.h"
#include "gui/plugins/PluginParamMirror.h"
#include "gui/common/Localisation.h"

#if JUCE_MAC
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

namespace hum {

class EmbeddedPluginView : public juce::Component, private juce::Timer {
public:
    EmbeddedPluginView(PluginsHost& host, std::string name)
        : host_(host), name_(std::move(name)) {
        setOpaque(false);
        panBar_.setAutoHide(false);
        panBar_.addListener(&panListener_);
        addChildComponent(panBar_);
        startTimerHz(30);
    }

    ~EmbeddedPluginView() override {
        stopTimer();
        onLayoutChanged = nullptr;
        onWantsFloat = nullptr;
        release();
    }

    int heightForWidth(int w) const {
        if (naturalW_ <= 0 || naturalH_ <= 0) return DeviceStripView::kHeight;
        if (fragile_ || !zoomable()) return naturalH_ + (naturalW_ > w ? kPanBarH : 0);
        return juce::jmax(24, (int) std::lround((double) naturalH_ * w / naturalW_));
    }
    int naturalWidth() const { return naturalW_ > 0 ? naturalW_ : 360; }

    void release() {
        if (!editor_) return;
#if JUCE_MAC
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
        naturalW_ = naturalH_ = 0;
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

    void paint(juce::Graphics& g) override {
        if (editor_) return;
        auto* hp = host_.hostedPluginFor(name_);
        const bool floatOwns = hp && hp->instance()->getActiveEditor();
        DeviceStripView::paintFace(
            g, host_, name_, getLocalBounds(),
            windowed_
                ? (floatOwns
                       ? juce::String(tr("embedded-plugin.ui-open-in-its-own", "UI open in its own window"))
                       : juce::String("This plugin brings its own window  -  click to open it"))
                : floatOwns
                    ? juce::String("UI in Float window  -  close it to embed here")
                    : juce::String::fromUTF8("Plugin UI  -  loading\xe2\x80\xa6"));
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
            && clipRegion == lastClip_ && std::abs(scaleNow - lastScale_) < 0.0005f) {
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
#if JUCE_MAC
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

    bool zoomable() const { return !actualSize_; }

    int panRange() const {
        return zoomable() ? 0 : juce::jmax(0, naturalW_ - getWidth());
    }

    float scaleFactor() const {
        if (!zoomable()) return 1.0f;
        return naturalW_ > 0 && getWidth() > 0
                   ? (float) getWidth() / (float) naturalW_ : 1.0f;
    }

    void layoutPanBar() {
        const int range = panRange();
        panBar_.setVisible(range > 0);
        if (range <= 0) { panX_ = 0; return; }
        panBar_.setBounds(getLocalBounds().removeFromBottom(kPanBarH));
        panBar_.setRangeLimits(0.0, (double) naturalW_, juce::dontSendNotification);
        panBar_.setCurrentRange((double) panX_, (double) getWidth(), juce::dontSendNotification);
        panX_ = juce::jlimit(0, range, panX_);
    }

    void timerCallback() override {
        auto* hp = host_.hostedPluginFor(name_);
        if (!hp) return;
        if (!editor_ && !hp->instance()->getActiveEditor()) acquire();
#if JUCE_MAC
        if (editor_ && !windowed_ && ++ownWindowPolls_ % 30 == 0
            && std::getenv("HUM_EMBED_NOWRAP") == nullptr)
            if (auto* peer = getPeer())
                if (embeddedEditorUsesOwnWindow(*peer, naturalW_, naturalH_)) {
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
            if (editor_->getWidth() != naturalW_ || editor_->getHeight() != naturalH_) {
                naturalW_ = editor_->getWidth();
                naturalH_ = editor_->getHeight();
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
        if (!embeddedViewResistsZoom(*editor_)) return;
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
        naturalW_ = ed->getWidth();
        naturalH_ = ed->getHeight();
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
    int naturalW_ = 0, naturalH_ = 0;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmbeddedPluginView)
};

}
