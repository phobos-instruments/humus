// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/EditorHost.h"
#include "gui/host/EngineHostMetapad.h"
#include "gui/host/EngineHostPresets.h"
#include "gui/host/ParamHistory.h"
#include "gui/host/PluginsHost.h"
#include "io/PatchDocument.h"

namespace hum {

class PropertiesHost : public virtual EditorHost, public virtual PluginsHost {
public:
    ~PropertiesHost() override = default;

    virtual int  editorMode(const std::string& name) const = 0;
    virtual void setEditorMode(const std::string& name, int mode) = 0;
    virtual juce::Point<int> editorSize(const std::string& name) const = 0;
    virtual int editorHalf(const std::string& name) const = 0;
    virtual bool editorCollapsed(const std::string& name) const = 0;
    virtual void setEditorCollapsed(const std::string& name, bool collapsed) = 0;
    virtual void setBypass(const std::string& name, bool on) = 0;
    virtual std::string missingClassNote(const std::string& name) const = 0;
    virtual ParamHistory& paramHistory() = 0;
    virtual NodeState captureNodeState(const std::string& name) const = 0;
    virtual void applyNodeState(const std::string& name, const NodeState& s) = 0;
    virtual PresetHost& presets() = 0;
    virtual MetaEditor& metapad() = 0;
    virtual std::string metapadNodeName() = 0;
    virtual std::string metapadNodeNameIfAny() const = 0;
    virtual double metapadX() const = 0;
    virtual double metapadY() const = 0;
    virtual unsigned textStamp() const = 0;
    virtual juce::Point<int> editorPosition(const std::string& name) const = 0;
    virtual bool editorVisible(const std::string& name) const = 0;
    virtual void setEditorState(const std::string& name, juce::Point<int> pos, bool visible) = 0;
    virtual void setEditorSize(const std::string& name, juce::Point<int> size, int half) = 0;
    virtual bool editorFloating(const std::string& name) const = 0;
    virtual juce::Rectangle<int> editorFloatBounds(const std::string& name) const = 0;
    virtual void setEditorFloating(const std::string& name, bool floating, juce::Rectangle<int> bounds) = 0;
    virtual void presentCard(std::unique_ptr<juce::Component> card) = 0;
    virtual void rollNode(const std::string& name) = 0;
};

}
