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

namespace hum {

class EmbeddedPluginView;
class DeviceStripView;

class ParameterWindow : public juce::Component, public juce::DragAndDropTarget,
                        public juce::TooltipClient {
public:
    ParameterWindow(PropertiesHost& host, const std::string& name);
    ~ParameterWindow() override;

    const std::string& organism() const { return name_; }
    int preferredWidth() const;
    int preferredHeight() const;

    void reloadValues();
    void syncToModel();
    void layoutForWidth(int w);
    void setFloating(bool f);
    bool floating() const { return floating_; }
    void setLayoutDeferred(bool d) { if (editor_) editor_->setLayoutDeferred(d); }
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
        syncVeil();
    }
    bool cautionShowing() const { return veil_ != nullptr; }
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
    std::function<void(const std::string&)> onDetach;
    std::function<void(const std::string&, int)> onMove;
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
    void syncVeil();
    void layoutBar(juce::Rectangle<int> row);
    void syncRail();
    void refreshBypass();
    void updateDiceEnablement();
    int contentWidth() const;
    int contentHeightFor(int w) const;
    void gripGesture(int dw, int dh, bool done);
    void updateGrips();

    PropertiesHost& host_;
    std::string name_;
    bool pluginHasUi_ = false;
    std::string builtClass_;
    std::unique_ptr<OrganismEditor> editor_;
    std::unique_ptr<EmbeddedPluginView> embedded_;
    std::unique_ptr<DeviceStripView> strip_;
    std::unique_ptr<caution::Veil> veil_;
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
    DockHeaderButton detach_{DockHeaderButton::Detach};
    DockHeaderButton up_{DockHeaderButton::Up}, down_{DockHeaderButton::Down};
    bool pressedTitle_ = false;
    juce::Point<int> pressAt_;
    SizeGrip gripR_{SizeGrip::Right}, gripB_{SizeGrip::Bottom}, gripC_{SizeGrip::Corner};
    juce::ComponentDragger dragger_;
    bool dragging_ = false, dropHot_ = false;
    juce::Point<int> dragStart_;
    bool selected_ = false;
    int userW_ = 0, userH_ = 0;
    bool half_ = false;
    bool floating_ = false;
    bool collapsed_ = false;
    bool rackMode_ = true;
    juce::Point<int> gestureBase_;
    bool inGesture_ = false;
};

}
