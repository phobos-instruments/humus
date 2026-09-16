// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/common/PerfLog.h"
#include "gui/properties/PropertiesPane.h"

#include "core/packs/ClassString.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "gui/editor/OrganismEditorFactory.h"
#include "hum/LayoutSpec.h"
#include "gui/properties/DeviceStripView.h"
#include "gui/help/HelpView.h"
#include "gui/style/LookAndFeel.h"
#include "gui/plugins/EmbeddedPluginView.h"
#include "gui/host/NodeRandomize.h"
#include "gui/editor/ParameterPanel.h"
#include "gui/plugins/PluginParamTable.h"
#include "gui/properties/PresetsView.h"
#include "gui/bricks/RootCollar.h"
#include "gui/common/Localisation.h"

namespace hum {

ParameterWindow::ParameterWindow(PropertiesHost& host, const std::string& name)
    : host_(host), name_(name) {
    viewSwitch_.onChange = [this](bool ui) {
        host_.setEditorMode(name_, ui ? 1 : 0);
        reloadValues();
        if (onContentResized) onContentResized();
    };
    addChildComponent(viewSwitch_);

    collapsed_ = host_.editorCollapsed(name);
    addAndMakeVisible(fold_);
    fold_.onClick = [this] { setCollapsed(!collapsed_); };

    buildContent();

    addAndMakeVisible(help_);
    help_.setTooltip(tr("parameter.help-for-this-organism", "Help for this organism"));
    help_.onClick = [this] {
        const auto* cm = host_.model().byName(name_);
        HelpView::show(cm ? cm->displayClass : name_, help_.getScreenBounds());
    };
    addChildComponent(rail_);
    rail_.onChanged = [this] { reloadValues(); };
    rail_.onOpenBrowser = [this] { openPresets(); };
    addAndMakeVisible(bypass_);
    bypass_.setTooltip(juce::String::fromUTF8(
        "Bypass - pass the signal straight through this organism "
        "(the cords stay put)"));
    bypass_.onClick = [this] {
        host_.setBypass(name_, !bypass_.isOn());
        refreshBypass();
        repaint();
    };
    bypass_.onRightClick = [this](juce::Point<int> at) {
        showAutomateMenu(host_, name_, kBypassParam, at, [this] { refreshBypass(); repaint(); });
    };
    refreshBypass();

    addAndMakeVisible(dice_);
    dice_.onClick = [this] {
        auto& hist = host_.paramHistory();
        hist.commit(name_, host_.captureNodeState(name_));
        host_.rollNode(name_);
        hist.commit(name_, host_.captureNodeState(name_));
        reloadValues();
    };
    dice_.onRightClick = [this](juce::Point<int> at) {
        showAutomateMenu(host_, name_, kRandomAction, at, nullptr, false);
    };
    updateDiceEnablement();

    addChildComponent(histBack_);
    addChildComponent(histFwd_);
    histBack_.setTooltip(juce::String::fromUTF8(
        "Undo - step back through THIS organism's settings; your current "
        "tweaks are kept as the newest entry"));
    histFwd_.setTooltip(juce::String::fromUTF8(
        "Redo - step forward through THIS organism's settings"));
    histBack_.onClick = [this] {
        if (const auto* s = host_.paramHistory().back(name_, host_.captureNodeState(name_))) {
            host_.applyNodeState(name_, *s);
            reloadValues();
        }
    };
    histFwd_.onClick = [this] {
        if (const auto* s = host_.paramHistory().forward(name_)) {
            host_.applyNodeState(name_, *s);
            reloadValues();
        }
    };
    histBack_.setVisible(historyApplies());
    histFwd_.setVisible(histBack_.isVisible());
    if (histBack_.isVisible())
        host_.paramHistory().commit(name_, host_.captureNodeState(name_));
    addAndMakeVisible(close_);
    close_.setTooltip(tr("parameter.close-this-editor", "Close this editor"));
    close_.onClick = [this] { if (onClose) onClose(name_); };
    addAndMakeVisible(detach_);
    detach_.setTooltip(tr("parameter.detach-into-its-own-window", "Detach into its own window"));
    detach_.onClick = [this] { if (onDetach) onDetach(name_); };
    addAndMakeVisible(up_);
    addAndMakeVisible(down_);
    up_.setTooltip(tr("parameter.move-up-the-rack", "Move up the rack"));
    down_.setTooltip(tr("parameter.move-down-the-rack", "Move down the rack"));
    up_.onClick = [this] { if (onMove) onMove(name_, -1); };
    down_.onClick = [this] { if (onMove) onMove(name_, 1); };

    pluginUi_.onClick = [this] { if (onOpenPluginUI) onOpenPluginUI(name_); };
    addChildComponent(pluginUi_);
    pluginUi_.setVisible(pluginHasUi_);

    {
        const int stored = host_.editorHalf(name);
        half_ = stored >= 0 ? stored != 0 : (preferredWidth() <= kRackHalfW);
    }
    const auto sz = host_.editorSize(name);
    userW_ = sz.x;
    userH_ = sz.y;
    for (auto* gp : {&gripR_, &gripB_, &gripC_}) {
        gp->onGesture = [this](int dw, int dh, bool done) { gripGesture(dw, dh, done); };
        addChildComponent(*gp);
    }
    updateGrips();

    syncRail();
    setSize(preferredWidth(), preferredHeight());
}

ParameterWindow::~ParameterWindow() = default;

void ParameterWindow::buildContent() {
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    const bool hadFocus = focused != nullptr && (focused == this || isParentOf(focused));

    buildContentImpl();
    if (editor_) editor_->setWearsCollar(false);
    setBufferedToImage(embedded_ == nullptr);

    if (hadFocus && juce::Component::getCurrentlyFocusedComponent() == nullptr)
        grabKeyboardFocus();
}

void ParameterWindow::buildContentImpl() {
    auto* pn = host_.pluginNodeFor(name_);
    pluginHasUi_ = pn && pn->hasEditor();
    const auto* cm = host_.model().byName(name_);
    const std::string cls = cm != nullptr ? cm->classRaw : std::string();
    const int mode = host_.editorMode(name_);
    const bool keepEmbedded = embedded_ != nullptr && !collapsed_ && pluginHasUi_
                              && (mode < 0 || mode == 1) && pn != nullptr
                              && pn->responding() && cls == builtClass_
                              && host_.missingClassNote(name_).empty();
    builtClass_ = cls;

    if (editor_) { removeChildComponent(editor_.get()); editor_.reset(); }
    if (embedded_ && !keepEmbedded) { removeChildComponent(embedded_.get()); embedded_.reset(); }
    if (strip_) { removeChildComponent(strip_.get()); strip_.reset(); }
    fold_.setCollapsed(collapsed_);
    fold_.setVisible(true);

    auto showStrip = [this] {
        strip_ = std::make_unique<DeviceStripView>(host_, name_);
        addAndMakeVisible(*strip_);
        viewSwitch_.setVisible(false);
        return strip_.get();
    };

    if (collapsed_) { showStrip(); return; }

    if (const auto* cm = host_.model().byName(name_);
        cm && isPluginKind(cm->kind) && (pn == nullptr || !pn->responding())) {
        showStrip()->setNote(
            pn == nullptr
                ? juce::String("Plugin not installed  -  passing through")
                : juce::String(tr("parameter.plugin-not-responding", "Plugin not responding")),
            pn == nullptr ? Palette::recordRed() : Palette::warnAmber());
        fold_.setVisible(false);
        return;
    }

    if (const auto note = host_.missingClassNote(name_); !note.empty()) {
        showStrip()->setNote(juce::String::fromUTF8(note.c_str())
                                 + juce::String("  -  passing through"),
                             Palette::warnAmber());
        fold_.setVisible(false);
        return;
    }

    if (pluginHasUi_ && mode == 0) {
        auto* t = new PluginParamTable(host_, name_);
        t->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
        editor_.reset(t);
        addAndMakeVisible(*editor_);
    } else if (pluginHasUi_ && (mode < 0 || mode == 1)) {
        if (!embedded_) {
            embedded_ = std::make_unique<EmbeddedPluginView>(host_, name_);
            embedded_->onLayoutChanged = [this] { if (onContentResized) onContentResized(); };
            embedded_->onWantsFloat    = [this] { if (onOpenPluginUI) onOpenPluginUI(name_); };
            addAndMakeVisible(*embedded_);
        }
    } else {
        editor_ = makeOrganismEditor(host_, name_);
        if (auto* pp = dynamic_cast<ParameterPanel*>(editor_.get());
            pp && pp->contentRows() == 0) {
            editor_.reset();
            showStrip();
            fold_.setVisible(false);
            return;
        }
        editor_->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
        addAndMakeVisible(*editor_);
    }
    viewSwitch_.set(embedded_ != nullptr);
    viewSwitch_.setVisible(pluginHasUi_);
    syncVeil();
}

void ParameterWindow::syncVeil() {
    const auto* cm = host_.model().byName(name_);
    const bool wanted = cm != nullptr && !collapsed_ && strip_ == nullptr
                        && caution::pending(cm->displayClass);
    if (wanted == (veil_ != nullptr)) return;
    if (!wanted) { veil_.reset(); repaint(); return; }
    veil_ = std::make_unique<caution::Veil>(cm->displayClass);
    veil_->onAcknowledged = [safe = juce::Component::SafePointer<ParameterWindow>(this)] {
        juce::MessageManager::callAsync([safe] {
            if (safe != nullptr) safe->syncVeil();
        });
    };
    addAndMakeVisible(*veil_);
    resized();
}

void ParameterWindow::setCollapsed(bool c) {
    if (collapsed_ == c) return;
    collapsed_ = c;
    host_.setEditorCollapsed(name_, c);
    reloadValues();
    if (onContentResized) onContentResized();
}

void ParameterWindow::releaseEmbedded() {
    if (embedded_) embedded_->release();
}
void ParameterWindow::setEmbeddedFloat(bool) {}

juce::Component* ParameterWindow::embeddedView() const { return embedded_.get(); }

void ParameterWindow::refreshBypass() {
    bypass_.setOn(host_.bypassed(name_));
}

void ParameterWindow::updateDiceEnablement() {
    const bool can = nodeSupportsRandom(host_, name_);
    dice_.setEnabled(can);
    dice_.setTooltip(can
        ? juce::String::fromUTF8(
              "Random - roll new settings for this organism (one undo step)")
        : juce::String::fromUTF8(
              "Random - this organism has nothing curated to roll"));
}

void ParameterWindow::syncRail() {
    rail_.setVisible(historyApplies());
    if (rail_.isVisible()) rail_.refresh();
}

void ParameterWindow::openPresets() {
    auto content = std::make_unique<PresetsView>(host_, name_, [this] {
        reloadValues();
        syncRail();
    });
    auto screen = rail_.isVisible() ? rail_.getScreenBounds() : getScreenBounds();
    juce::CallOutBox::launchAsynchronously(std::move(content), screen, nullptr);
}

void ParameterWindow::syncToModel() {
    const auto* cm = host_.model().byName(name_);
    if (cm == nullptr) return;
    if (cm->classRaw != builtClass_ || host_.editorCollapsed(name_) != collapsed_) {
        reloadValues();
        return;
    }
    refreshLiveValues();
    refreshPresetState();
}

void ParameterWindow::reloadValues() {
    collapsed_ = host_.editorCollapsed(name_);
    buildContent();
    pluginUi_.setVisible(pluginHasUi_);
    updateDiceEnablement();
    refreshBypass();
    histBack_.setVisible(historyApplies());
    histFwd_.setVisible(histBack_.isVisible());
    syncRail();
    setSize(preferredWidth(), preferredHeight());
    resized();
    repaint();
    if (onContentResized) onContentResized();
}

void ParameterWindow::setFloating(bool f) {
    floating_ = f;
    detach_.setVisible(!f);
    close_.setVisible(!f);
    fold_.setVisible(!f);
    if (f) rackMode_ = false;
    updateGrips();
    resized();
    repaint();
}

void ParameterWindow::paint(juce::Graphics& g) {
    perf::Scope scope("box.paint");
    const auto r = getLocalBounds().toFloat();
    const float rad = rackMode_ || floating_ ? 5.0f : 7.0f;
    g.setColour(juce::Colours::black.withAlpha(rackMode_ ? 0.14f : 0.20f));
    g.drawRoundedRectangle(r.translated(0.0f, 1.5f).reduced(1.0f), rad, rackMode_ ? 1.0f : 1.5f);
    g.setColour(Palette::panel);
    g.fillRoundedRectangle(r.reduced(0.5f), rad);
    juce::Path band;
    band.addRoundedRectangle(0.5f, 0.5f, r.getWidth() - 1.0f, (float) kTitle,
                             rad, rad, true, true, false, false);
    g.setColour(Palette::panelLight);
    g.fillPath(band);
    g.setColour(Palette::background.withAlpha(alpha::dim));
    g.fillRect(0.0f, (float) kTitle, r.getWidth(), (float) kBar);
    g.setColour(Palette::border.withAlpha(alpha::mid));
    g.drawHorizontalLine(chromeHeight(), 0.0f, r.getWidth());

    collar::paintBand(g, host_, name_, {5, 0, getWidth() - 10, kTitle},
19,kTitle - 5);
}

void ParameterWindow::paintOverChildren(juce::Graphics& g) {
    const auto r = getLocalBounds().toFloat();
    const float rad = rackMode_ || floating_ ? 5.0f : 7.0f;
    if (host_.bypassed(name_)) {
        g.setColour(Palette::background.withAlpha(alpha::mid));
        g.fillRect(getLocalBounds().withTrimmedTop(chromeHeight()));
    }
    g.setColour(selected_ ? Palette::accent : Palette::border);
    g.drawRoundedRectangle(r.reduced(selected_ ? 1.0f : 0.5f), rad, selected_ ? 2.0f : 1.0f);
    if (dropHot_) {
        g.setColour(Palette::accent.withAlpha(alpha::mist));
        g.fillRoundedRectangle(r.reduced(1.0f), rad);
        g.setColour(Palette::accent);
        g.drawRoundedRectangle(r.reduced(1.5f), rad, 2.0f);
    }
}

juce::String ParameterWindow::getTooltip() {
    if (getMouseXYRelative().y < kTitle && host_.midiOutletsOf(name_) > 0)
        return tr("parameter.drag-to-the-timeline-or", "Drag to the timeline (or right-click) to send its notes to a track");
    return {};
}

}
