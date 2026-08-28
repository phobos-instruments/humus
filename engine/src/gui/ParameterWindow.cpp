#include "gui/PropertiesPane.h"

#include "core/ClassString.h"
#include "core/HostedPlugin.h"
#include "core/PluginNode.h"
#include "gui/OrganismEditorFactory.h"
#include "gui/DeviceStripView.h"
#include "gui/HelpView.h"
#include "gui/LookAndFeel.h"
#include "gui/EmbeddedPluginView.h"
#include "gui/NodeRandomize.h"
#include "gui/ParameterPanel.h"
#include "gui/PluginParamTable.h"
#include "gui/PresetsView.h"
#include "gui/RootCollar.h"

namespace hum {

ParameterWindow::ParameterWindow(EngineHost& host, const std::string& name)
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
    help_.setTooltip("Help for this organism");
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
        randomizeNode(host_, name_);
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
    close_.setTooltip("Close this editor");
    close_.onClick = [this] { if (onClose) onClose(name_); };

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
                : juce::String("Plugin not responding"),
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

int ParameterWindow::preferredWidth() const {
    if (embedded_) return juce::jlimit(280, 1200, embedded_->naturalWidth());
    return editor_ ? editor_->preferredContentWidth() : 280;
}

int ParameterWindow::contentWidth() const { return preferredWidth(); }

int ParameterWindow::contentHeightFor(int w) const {
    if (strip_) return DeviceStripView::kHeight;
    if (embedded_) return embedded_->heightForWidth(w);
    return editor_ ? editor_->preferredContentHeight(w) : 100;
}

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

int ParameterWindow::preferredHeight() const {
    return chromeHeight() + contentHeightFor(preferredWidth());
}

int ParameterWindow::heightAtWidth(int w) const { return chromeHeight() + contentHeightFor(w); }

juce::Point<int> ParameterWindow::freeSize() const {
    const int w = userW_ > 0 ? juce::jlimit((int) kMinW, maxUsefulWidth(), userW_)
                             : preferredWidth();
    return {w, heightAtWidth(w)};
}

int ParameterWindow::maxUsefulWidth() const {
    const int hFlat = contentHeightFor(4000);
    if (contentHeightFor(kMinW) == hFlat) return 4000;
    int lo = kMinW, hi = 4000;
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (contentHeightFor(mid) == hFlat) hi = mid;
        else lo = mid + 1;
    }
    return juce::jmax(lo, preferredWidth());
}

void ParameterWindow::layoutForWidth(int w) {
    rackMode_ = true;
    updateGrips();
    setSize(w, heightAtWidth(w));
}

void ParameterWindow::layoutNatural() {
    rackMode_ = false;
    updateGrips();
    const auto s = freeSize();
    setSize(s.x, s.y);
}

void ParameterWindow::gripGesture(int dw, int dh, bool done) {
    if (!inGesture_) {
        gestureBase_ = {getWidth(), getHeight()};
        inGesture_ = true;
    }
    if (onResizeGesture)
        onResizeGesture(name_, gestureBase_.x + dw, gestureBase_.y + dh, done);
    if (done) inGesture_ = false;
}

void ParameterWindow::updateGrips() {
    gripR_.setVisible(true);
    gripB_.setVisible(false);
    gripC_.setVisible(!rackMode_);
}

void ParameterWindow::paint(juce::Graphics& g) {
    const auto r = getLocalBounds().toFloat();
    constexpr float rad = 7.0f;
    g.setColour(juce::Colours::black.withAlpha(0.20f));
    g.fillRoundedRectangle(r.translated(0.0f, 1.5f).reduced(0.5f), rad);
    g.setColour(Palette::panel);
    g.fillRoundedRectangle(r.reduced(0.5f), rad);
    juce::Path band;
    band.addRoundedRectangle(0.5f, 0.5f, r.getWidth() - 1.0f, (float) kTitle,
                             rad, rad, true, true, false, false);
    g.setColour(Palette::panelLight);
    g.fillPath(band);
    g.setColour(Palette::background.withAlpha(0.45f));
    g.fillRect(0.0f, (float) kTitle, r.getWidth(), (float) kBar);
    g.setColour(Palette::border.withAlpha(0.6f));
    g.drawHorizontalLine(chromeHeight(), 0.0f, r.getWidth());

    collar::paintBand(g, host_, name_, {5, 0, getWidth() - 10, kTitle},
19,kTitle - 5);
}

void ParameterWindow::paintOverChildren(juce::Graphics& g) {
    const auto r = getLocalBounds().toFloat();
    constexpr float rad = 7.0f;
    if (host_.bypassed(name_)) {
        g.setColour(Palette::background.withAlpha(0.62f));
        g.fillRect(getLocalBounds().withTrimmedTop(chromeHeight()));
    }
    g.setColour(selected_ ? Palette::accent : Palette::border);
    g.drawRoundedRectangle(r.reduced(selected_ ? 1.0f : 0.5f), rad, selected_ ? 2.0f : 1.0f);
    if (dropHot_) {
        g.setColour(Palette::accent.withAlpha(0.12f));
        g.fillRoundedRectangle(r.reduced(1.0f), rad);
        g.setColour(Palette::accent);
        g.drawRoundedRectangle(r.reduced(1.5f), rad, 2.0f);
    }
}

juce::String ParameterWindow::getTooltip() {
    if (getMouseXYRelative().y < kTitle && host_.midiOutletsOf(name_) > 0)
        return "Drag to the timeline (or right-click) to send its notes to a track";
    return {};
}

void ParameterWindow::layoutBar(juce::Rectangle<int> row) {
    row.reduce(4, 2);
    auto right = row;
    auto place = [&](juce::Component& c, int w, int shrinkY) {
        c.setBounds(right.removeFromRight(w).reduced(0, shrinkY));
        right.removeFromRight(2);
    };
    if (help_.isVisible()) place(help_, 22, 0);
    if (histFwd_.isVisible()) { place(histFwd_, 20, 0); place(histBack_, 20, 0); }
    if (pluginUi_.isVisible()) place(pluginUi_, 24, 0);
    if (viewSwitch_.isVisible()) place(viewSwitch_, 54, 0);

    auto left = row.withRight(right.getRight());
    constexpr int kGap = 10;
    const int full = bypass_.widthFor(false) + kGap + dice_.widthFor(false);
    const bool compact = full > left.getWidth();
    bypass_.setCompact(compact);
    dice_.setCompact(compact);
    bypass_.setBounds(left.removeFromLeft(bypass_.widthFor(compact)));
    left.removeFromLeft(kGap);
    dice_.setBounds(left.removeFromLeft(dice_.widthFor(compact)));
}

void ParameterWindow::resized() {
    auto a = getLocalBounds();
    auto h = a.removeFromTop(kTitle);
    fold_.setBounds(h.removeFromLeft(22));
    close_.setBounds(h.removeFromRight(kTitle));
    layoutBar(a.removeFromTop(kBar));
    if (rail_.isVisible()) rail_.setBounds(a.removeFromTop(kRail));
    if (editor_) editor_->setBounds(a);
    if (embedded_) embedded_->setBounds(a);
    if (strip_) strip_->setBounds(a);
    const int chrome = chromeHeight();
    gripR_.setBounds(getWidth() - 6, chrome, 6, getHeight() - chrome);
    gripB_.setBounds(0, getHeight() - 6, getWidth() - 16, 6);
    gripC_.setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    for (auto* gp : {&gripR_, &gripB_, &gripC_}) gp->toFront(false);
}

void ParameterWindow::mouseDown(const juce::MouseEvent& e) {
    toFront(true);
    if (onSelect) onSelect(name_);
    if (e.y < kTitle || strip_) { dragging_ = true; dragStart_ = getPosition(); dragger_.startDraggingComponent(this, e); }
}

void ParameterWindow::mouseDrag(const juce::MouseEvent& e) {
    if (!dragging_) return;
    auto* parent = getParentComponent();
    if (parent != nullptr && host_.midiOutletsOf(name_) > 0
        && !parent->getLocalBounds().contains(e.getEventRelativeTo(parent).getPosition())) {
        if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            dragging_ = false;
            setAlpha(1.0f);
            setTopLeftPosition(dragStart_);
            if (onDragPreview) onDragPreview(name_);
            dnd->startDragging(juce::String("print:") + juce::String(juce::CharPointer_UTF8(name_.c_str())), this);
            return;
        }
    }
    setAlpha(0.85f);
    dragger_.dragComponent(this, e, nullptr);
    if (auto* parent = getParentComponent()) {
        int x = juce::jlimit(0, juce::jmax(0, parent->getWidth() - getWidth()), getX());
        int y = juce::jlimit(0, juce::jmax(0, parent->getHeight() - getHeight()), getY());
        setTopLeftPosition(x, y);
    }
    if (onDragPreview) onDragPreview(name_);
}

bool ParameterWindow::isInterestedInDragSource(const SourceDetails& d) {
    return d.description.toString().startsWith("noteclip:") && host_.clips().ownsPattern(name_);
}

void ParameterWindow::itemDropped(const SourceDetails& d) {
    dropHot_ = false;
    repaint();
    const auto parts = juce::StringArray::fromTokens(d.description.toString(), ":", {});
    if (parts.size() < 3) return;
    host_.pushUndo();
    if (host_.clips().adoptNotes(parts[1].toStdString(), parts[2].getIntValue(), name_) && editor_)
        editor_->reloadValues();
}

void ParameterWindow::mouseUp(const juce::MouseEvent&) {
    setAlpha(1.0f);
    if (dragging_ && onMoved) onMoved(name_, getPosition());
    dragging_ = false;
}

}
