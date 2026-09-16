// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/Caution.h"
#include "gui/properties/BarButton.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/app/Dock.h"
#include "gui/host/PropertiesHost.h"
#include "gui/properties/PresetRail.h"
#include "gui/properties/SizeGrip.h"
#include "gui/app/ViewSwitch.h"
#include "gui/common/Localisation.h"
#include "gui/properties/ParameterWindow.h"
#include "gui/app/FloatingWindows.h"

namespace hum {

class EmbeddedPluginView;
class DeviceStripView;

class PropertiesPane : public juce::Component {
public:
    enum class LayoutMode { Rack, Blocks };

    explicit PropertiesPane(PropertiesHost& host) : host_(host) {
        addAndMakeVisible(viewport_);
        viewport_.setViewedComponent(&surface_, false);
        viewport_.setScrollBarsShown(false, true);
        viewport_.setScrollBarThickness(10);
        viewport_.onScrolled = [this] { map_.repaint(); flushVisibleLayouts(); };
        addAndMakeVisible(map_);
        surface_.addChildComponent(ghost_);
        buildModeStrip();
    }

    void setLayoutMode(LayoutMode m);
    LayoutMode layoutMode() const { return mode_; }

    void openFor(const std::string& name);
    void closeFor(const std::string& name);
    void expandFor(const std::string& name) {
        auto it = windows_.find(name);
        if (it != windows_.end()) it->second->setCollapsed(false);
    }
    void syncFromModel();
    void setDragLayout(bool on);
    void moveInOrder(const std::string& name, int delta);
    void autoArrangeFree();
    void closeFloatingForTest(const std::string& name);
    juce::Rectangle<int> floatingBoundsForTest(const std::string& name) const {
        auto it = floating_.find(name);
        return it == floating_.end() ? juce::Rectangle<int>() : it->second->getBounds();
    }
    juce::Rectangle<float> mapLensForTest() const { return map_.lens(); }
    juce::Rectangle<int> mapBoundsForTest() const { return map_.getBounds(); }
    void mapPressForTest(juce::Point<int> p) { map_.pressAt(p); }
    void mapDragForTest(juce::Point<int> p) { map_.dragAt(p); }
    void mapReleaseForTest(juce::Point<int> p, bool dragged) {
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent e(juce::Desktop::getInstance().getMainMouseSource(),
                                 p.toFloat(), juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                 &map_, &map_, now, p.toFloat(), now, 1, dragged);
        map_.mouseUp(e);
    }
    int viewYForTest() const { return viewport_.getViewPositionY(); }
    bool horizontalScrollShowingForTest() {
        return viewport_.getHorizontalScrollBar().isVisible();
    }
    juce::Point<int> surfaceWidthForTest() const {
        return {surface_.getWidth(), viewport_.getMaximumVisibleWidth()};
    }
    juce::Point<int> floatingMinimumForTest(const std::string& name) const {
        auto it = floating_.find(name);
        if (it == floating_.end()) return {};
        const auto* c = it->second->getConstrainer();
        return c == nullptr ? juce::Point<int>()
                            : juce::Point<int>(c->getMinimumWidth(), c->getMinimumHeight());
    }
    juce::Point<int> floatingWmMinimumForTest(const std::string& name) const {
        auto it = floating_.find(name);
        return it == floating_.end() ? juce::Point<int>()
                                     : windowManagerMinimum(*it->second);
    }
    void sizeFloatingForTest(const std::string& name, int w, int h) {
        auto it = floating_.find(name);
        if (it != floating_.end())
            it->second->setBoundsConstrained(it->second->getBounds().withSize(w, h));
    }
    void detach(const std::string& name);
    void redock(const std::string& name);
    bool isFloating(const std::string& name) const { return floating_.count(name) > 0; }
    std::function<bool(const juce::KeyPress&)> onFloatKey;
    std::function<bool(bool)> onFloatKeyState;
    void reload();
    void refreshLiveValues();
    void refreshTextEdits();
    void refreshPresetState();
    void reloadValuesFor(const std::string& name);
    void clearAll();
    void prune();

    void setSelected(const std::string& name);
    const std::string& selectedForTest() const { return selected_; }
    int revealYFor(const std::string& name);

    void releaseEmbeddedEditors();
    void releaseEmbedded(const std::string& name);
    void releaseEmbeddedEditorsWhere(const std::function<bool(const std::string&)>& doomed);

    OrganismEditor* editorFor(const std::string& name) const {
        auto it = windows_.find(name);
        return it != windows_.end() ? it->second->editor() : nullptr;
    }
    ParameterWindow* boxFor(const std::string& name) const {
        auto it = windows_.find(name);
        return it != windows_.end() ? it->second.get() : nullptr;
    }
    juce::Component* embeddedViewFor(const std::string& name) const {
        auto it = windows_.find(name);
        return it != windows_.end() ? it->second->embeddedView() : nullptr;
    }

    std::function<void(const std::string&)> onSelect;
    std::function<void(const std::string&)> onOpenPluginUI;
    std::function<void()> onAutomationChanged;
    std::function<void(const std::string&)> onAddOrganism;

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    static constexpr int kModeH = 22;
    static constexpr int kMapW  = 32;
    class Surface : public juce::Component {
    public:
        explicit Surface(PropertiesPane& o) : owner_(o) {}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
    private:
        PropertiesPane& owner_;
    };

    struct MapViewport : juce::Viewport, private juce::Timer {
        std::function<void()> onScrolled;
        void visibleAreaChanged(const juce::Rectangle<int>&) override {
            if (onScrolled) onScrolled();
        }
        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override {
            smoothWheel(w.deltaY, w.isReversed);
        }
        void seekTo(int x, int y) {
            cancelGlide();
            setViewPosition(x, y);
        }
        void glideTo(std::function<int()> target) {
            glideTarget_ = std::move(target);
            animating_ = true;
            if (!isTimerRunning()) startTimerHz(60);
        }
        void cancelGlide() {
            glideTarget_ = nullptr;
            animating_ = false;
            stopTimer();
        }
        void smoothWheel(float deltaY, bool reversed) {
            auto* vc = getViewedComponent();
            if (vc == nullptr) return;
            const int maxY = juce::jmax(0, vc->getHeight() - getViewHeight());
            if (maxY <= 0) return;
            const double d = reversed ? -deltaY : deltaY;
            const int from = animating_ ? targetY_ : getViewPositionY();
            glideTarget_ = nullptr;
            targetY_ = juce::jlimit(0, maxY,
                                    from - juce::roundToInt(d * getViewHeight() * 0.9));
            animating_ = true;
            if (!isTimerRunning()) startTimerHz(60);
        }
    private:
        void timerCallback() override {
            if (glideTarget_ != nullptr) {
                auto* vc = getViewedComponent();
                const int maxY = vc != nullptr
                                     ? juce::jmax(0, vc->getHeight() - getViewHeight()) : 0;
                targetY_ = juce::jlimit(0, maxY, glideTarget_());
            }
            const int cur = getViewPositionY();
            if (std::abs(targetY_ - cur) <= 1) {
                setViewPosition(getViewPositionX(), targetY_);
                glideTarget_ = nullptr;
                animating_ = false;
                stopTimer();
                return;
            }
            int step = juce::roundToInt((targetY_ - cur) * 0.3);
            if (step == 0) step = targetY_ > cur ? 1 : -1;
            setViewPosition(getViewPositionX(), cur + step);
        }
        int targetY_ = 0;
        bool animating_ = false;
        std::function<int()> glideTarget_;
    };

    class RackMap : public juce::Component, public juce::SettableTooltipClient {
    public:
        explicit RackMap(PropertiesPane& o) : owner_(o) {
            setTooltip(juce::String(tr("properties-pane.overview-map-drag-to-scroll", "Overview map - drag to scroll")));
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void pressAt(juce::Point<int>);
        void dragAt(juce::Point<int>);
        juce::Rectangle<float> lens() const;
    private:
        float scaleY() const;
        juce::Rectangle<float> toMap(juce::Rectangle<int> surfaceRect) const;
        void scrollBy(int mapDelta);
        PropertiesPane& owner_;
        int grabMapY_ = 0, grabViewY_ = 0;
        static constexpr int kInset = 3;
        static constexpr float kGrabSlack = 2.0f;
    };

    class GhostOverlay : public juce::Component {
    public:
        GhostOverlay() { setInterceptsMouseClicks(false, false); }
        void paint(juce::Graphics& g) override;
    };

    void updateSurfaceSize();
    void layoutBlocks();
    void flushVisibleLayouts();
    std::vector<std::string> unopenedInStoredOrder() const;
    bool dragLayout_ = false;
    std::map<std::string, std::unique_ptr<FloatingBoxWindow>> floating_;
    void ensureOrder();
    void reflowFromDrop();
    void buildModeStrip();
    std::map<std::string, juce::Rectangle<int>> computeLayout(
        const std::vector<std::string>& ord) const;
    std::vector<std::string> orderByPosition() const;
    void handleResizeGesture(const std::string&, int w, int h, bool done);
    void showDropPreview(const std::string& name);
    void droppedAt(const std::string& name);
    void showGhost(juce::Rectangle<int> r);
    void hideGhost();
    static juce::Point<int> snapFree(juce::Point<int> p);
    juce::Point<int> nearestFreePos(juce::Point<int> want, juce::Point<int> size,
                                    const std::string& exclude) const;
    void resolveFreeOverlaps(const std::string& fixedName);

    PropertiesHost& host_;
    LayoutMode mode_ = LayoutMode::Rack;
    ViewSwitch modeSwitch_{ false};
    juce::TextButton arrangeBtn_{tr("properties-pane.arrange", "Arrange")};
    MapViewport viewport_;
    Surface surface_{*this};
    RackMap map_{*this};
    GhostOverlay ghost_;
    std::map<std::string, std::unique_ptr<ParameterWindow>> windows_;
    std::vector<std::string> order_;
    std::set<std::string> placedFree_;
    std::string selected_;
    unsigned textStamp_ = (unsigned) -1;

    friend class Surface;
};

}
