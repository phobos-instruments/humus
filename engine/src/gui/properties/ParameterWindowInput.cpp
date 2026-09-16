// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/common/PerfLog.h"
#include "gui/properties/PropertiesPane.h"
#include "gui/host/EngineHostClips.h"
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
    gripR_.setVisible(!floating_);
    gripB_.setVisible(false);
    gripC_.setVisible(!rackMode_ && !floating_);
}

void ParameterWindow::mouseDown(const juce::MouseEvent& e) {
    toFront(true);
    if (onSelect) onSelect(name_);
    pressedTitle_ = (e.y < kTitle || strip_) && !floating_;
    pressAt_ = e.getPosition();
    if (pressedTitle_ && !rackMode_) { dragging_ = true; dragStart_ = getPosition(); dragger_.startDraggingComponent(this, e); }
}

void ParameterWindow::mouseDrag(const juce::MouseEvent& e) {
    if (rackMode_ && pressedTitle_ && host_.midiOutletsOf(name_) > 0
        && e.getPosition().getDistanceFrom(pressAt_) > 12) {
        if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            pressedTitle_ = false;
            if (onDragPreview) onDragPreview(name_);
            dnd->startDragging(juce::String("print:") + juce::String(juce::CharPointer_UTF8(name_.c_str())), this,
                               juce::ScaledImage(), true);
        }
        return;
    }
    if (!dragging_) return;
    auto* parent = getParentComponent();
    if (parent != nullptr && host_.midiOutletsOf(name_) > 0
        && !parent->getLocalBounds().contains(e.getEventRelativeTo(parent).getPosition())) {
        if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            dragging_ = false;
            setAlpha(1.0f);
            setTopLeftPosition(dragStart_);
            if (onDragPreview) onDragPreview(name_);
            dnd->startDragging(juce::String("print:") + juce::String(juce::CharPointer_UTF8(name_.c_str())), this,
                               juce::ScaledImage(), true);
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
    pressedTitle_ = false;
    setAlpha(1.0f);
    if (dragging_ && onMoved) onMoved(name_, getPosition());
    dragging_ = false;
}

}
