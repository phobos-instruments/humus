#pragma once
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/BarButton.h"
#include "gui/OrganismEditor.h"
#include "gui/Dock.h"
#include "gui/EngineHost.h"
#include "gui/PresetRail.h"
#include "gui/SizeGrip.h"
#include "gui/ViewSwitch.h"

namespace hum {

class EmbeddedPluginView;
class DeviceStripView;

class ParameterWindow : public juce::Component, public juce::DragAndDropTarget,
                        public juce::TooltipClient {
public:
    ParameterWindow(EngineHost& host, const std::string& name);
    ~ParameterWindow() override;

    const std::string& organism() const { return name_; }
    int preferredWidth() const;
    int preferredHeight() const;

    void reloadValues();
    void syncToModel();
    void layoutForWidth(int w);
    void layoutNatural();
    int heightAtWidth(int w) const;
    int maxUsefulWidth() const;
    juce::Point<int> freeSize() const;
    void setUserSize(int w, int h) { userW_ = w; userH_ = h; }
    bool halfRack() const { return half_; }
    void setHalfRack(bool h) { half_ = h; }
    bool collapsed() const { return collapsed_; }
    void setCollapsed(bool c);
    static constexpr int kMinW = 180;
    void refreshLiveValues() {
        if (editor_) editor_->refreshAutomatedValues();
        const bool off = host_.bypassed(name_);
        if (off != bypass_.isOn()) { bypass_.setOn(off); repaint(); }
    }
    void reloadTextValues() {
        if (editor_) editor_->reloadTextValues();
    }
    void refreshPresetState() {
        const auto* c = host_.model().byName(name_);
        if (c == nullptr) return;
        const auto sig = std::make_tuple(c->currentPreset, c->presetDirty, c->presets.size());
        if (sig == presetSig_) return;
        presetSig_ = sig;
        rail_.refresh();
    }
    void releaseEmbedded();
    void setEmbeddedFloat(bool);
    std::function<void()> onContentResized;
    OrganismEditor* editor() const { return editor_.get(); }
    juce::Component* embeddedView() const;

    void setSelected(bool s) { if (selected_ != s) { selected_ = s; repaint(); } }
    bool isBeingDragged() const { return dragging_; }

    std::function<void(const std::string&)> onClose;
    std::function<void(const std::string&)> onOpenPluginUI;
    std::function<void(const std::string&, juce::Point<int>)> onMoved;
    std::function<void(const std::string&)> onSelect;
    std::function<void()> onAutomationChanged;
    std::function<void(const std::string&, int w, int h, bool done)> onResizeGesture;
    std::function<void(const std::string&)> onDragPreview;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    juce::String getTooltip() override;
    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDropped(const SourceDetails&) override;
    void itemDragEnter(const SourceDetails&) override { dropHot_ = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dropHot_ = false; repaint(); }

    static constexpr int kTitle = 30;
    static constexpr int kBar = 24;
    static constexpr int kRail = 22;
    static constexpr int kChrome = kTitle + kBar;
    int chromeHeight() const { return kChrome + (rail_.isVisible() ? kRail : 0); }
    bool railVisible() const { return rail_.isVisible(); }
    PresetRail::Geometry railGeometry() const { return rail_.geometry(); }
    bool railDrewDirty() const { return rail_.drewDirty(); }

private:
    void openPresets();
    bool historyApplies() const {
        const auto* cm = host_.model().byName(name_);
        return cm != nullptr && !cm->properties.empty();
    }
    void buildContent();
    void buildContentImpl();
    void layoutBar(juce::Rectangle<int> row);
    void syncRail();
    void refreshBypass();
    void updateDiceEnablement();
    int contentWidth() const;
    int contentHeightFor(int w) const;
    void gripGesture(int dw, int dh, bool done);
    void updateGrips();

    EngineHost& host_;
    std::string name_;
    bool pluginHasUi_ = false;
    std::string builtClass_;
    std::unique_ptr<OrganismEditor> editor_;
    std::unique_ptr<EmbeddedPluginView> embedded_;
    std::unique_ptr<DeviceStripView> strip_;
    FoldButton fold_;
    juce::TextButton help_{"?"};
    IconButton histBack_{IconGlyph::Undo, {}};
    IconButton histFwd_{IconGlyph::Redo, {}};
    BarButton bypass_{IconGlyph::Power, "BYPASS", {}};
    BarButton dice_{IconGlyph::Dice, "RANDOM", {}};
    PresetRail rail_{host_, name_};
    std::tuple<int, bool, size_t> presetSig_{-1, false, 0};
    ViewSwitch viewSwitch_;
    FloatButton pluginUi_;
    DockHeaderButton close_{DockHeaderButton::Close};
    SizeGrip gripR_{SizeGrip::Right}, gripB_{SizeGrip::Bottom}, gripC_{SizeGrip::Corner};
    juce::ComponentDragger dragger_;
    bool dragging_ = false, dropHot_ = false;
    juce::Point<int> dragStart_;
    bool selected_ = false;
    int userW_ = 0, userH_ = 0;
    bool half_ = false;
    bool collapsed_ = false;
    bool rackMode_ = true;
    juce::Point<int> gestureBase_;
    bool inGesture_ = false;
};

class PropertiesPane : public juce::Component {
public:
    enum class LayoutMode { Rack, Blocks };

    explicit PropertiesPane(EngineHost& host) : host_(host) {
        addAndMakeVisible(viewport_);
        viewport_.setViewedComponent(&surface_, false);
        viewport_.setScrollBarsShown(false, true);
        viewport_.setScrollBarThickness(10);
        viewport_.onScrolled = [this] { map_.repaint(); };
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
    void reload();
    void refreshLiveValues();
    void refreshTextEdits();
    void refreshPresetState();
    void reloadValuesFor(const std::string& name);
    void clearAll();
    void prune();

    void setSelected(const std::string& name);
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
            setTooltip(juce::String("Overview map - drag to scroll, click a block to select its box"));
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    private:
        float scaleY() const;
        juce::Rectangle<float> toMap(juce::Rectangle<int> surfaceRect) const;
        void seekTo(int mapY);
        std::string hitWindow(juce::Point<int>) const;
        PropertiesPane& owner_;
        std::string pressedOn_;
        static constexpr int kInset = 3;
    };

    class GhostOverlay : public juce::Component {
    public:
        GhostOverlay() { setInterceptsMouseClicks(false, false); }
        void paint(juce::Graphics& g) override;
    };

    void updateSurfaceSize();
    void layoutBlocks();
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

    EngineHost& host_;
    LayoutMode mode_ = LayoutMode::Rack;
    ViewSwitch modeSwitch_{ false};
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
