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
    if (detach_.isVisible()) detach_.setBounds(h.removeFromRight(kTitle));
    up_.setVisible(rackMode_ && !floating_);
    down_.setVisible(rackMode_ && !floating_);
    if (down_.isVisible()) { down_.setBounds(h.removeFromRight(kTitle)); up_.setBounds(h.removeFromRight(kTitle)); }
    layoutBar(a.removeFromTop(kBar));
    if (rail_.isVisible()) rail_.setBounds(a.removeFromTop(kRail));
    if (editor_) editor_->setBounds(a);
    if (embedded_) embedded_->setBounds(a);
    if (strip_) strip_->setBounds(a);
    if (veil_) { veil_->setBounds(a); veil_->toFront(false); }
    const int chrome = chromeHeight();
    gripR_.setBounds(getWidth() - 6, chrome, 6, getHeight() - chrome);
    gripB_.setBounds(0, getHeight() - 6, getWidth() - 16, 6);
    gripC_.setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    for (auto* gp : {&gripR_, &gripB_, &gripC_}) gp->toFront(false);
}

}
