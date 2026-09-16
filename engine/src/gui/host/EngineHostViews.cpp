// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/library/BankLibrary.h"
#include "core/library/UserLibrary.h"
#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>
#include "core/graph/GraphLayout.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PerfBox.h"
#include "core/graph/PodModel.h"
#include "core/packs/Roles.h"
#include "hum/Registry.h"
#include "io/PatchWriter.h"
#include "io/ModRouteBuild.h"
#include "io/PatchLoader.h"
#include "gui/app/AppSettings.h"

namespace hum {

juce::Point<int> EngineHost::position(const std::string& name) const {
    auto it = positions_.find(name);
    return it == positions_.end() ? juce::Point<int>{40, 40} : it->second;
}

juce::Point<int> EngineHost::editorPosition(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name && v.hasEditor) return {v.editorX, v.editorY};
    return {-1, -1};
}

bool EngineHost::editorVisible(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.hasEditor && v.editorVisible;
    return false;
}

void EngineHost::setEditorState(const std::string& name, juce::Point<int> pos, bool visible) {
    for (auto& v : model_.views)
        if (v.organismName == name) {
            v.hasEditor = true; v.editorVisible = visible; v.editorX = pos.x; v.editorY = pos.y;
            return;
        }
    OrganismView v;
    v.organismName = name;
    v.hasEditor = true; v.editorVisible = visible; v.editorX = pos.x; v.editorY = pos.y;
    model_.views.push_back(v);
}

juce::Point<int> EngineHost::editorSize(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return {v.editorW, v.editorH};
    return {0, 0};
}

int EngineHost::editorHalf(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.editorHalf;
    return -1;
}

void EngineHost::setEditorSize(const std::string& name, juce::Point<int> size, int half) {
    dirty_ = true;
    for (auto& v : model_.views)
        if (v.organismName == name) {
            v.editorW = size.x; v.editorH = size.y; v.editorHalf = half;
            return;
        }
    OrganismView v;
    v.organismName = name;
    v.editorW = size.x; v.editorH = size.y; v.editorHalf = half;
    model_.views.push_back(v);
}

bool EngineHost::editorCollapsed(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.editorCollapsed;
    return false;
}

bool EngineHost::editorFloating(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.editorFloating;
    return false;
}

juce::Rectangle<int> EngineHost::editorFloatBounds(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return {v.floatX, v.floatY, v.floatW, v.floatH};
    return {};
}

void EngineHost::setEditorFloating(const std::string& name, bool floating, juce::Rectangle<int> b) {
    dirty_ = true;
    for (auto& v : model_.views)
        if (v.organismName == name) {
            v.editorFloating = floating;
            v.floatX = b.getX(); v.floatY = b.getY(); v.floatW = b.getWidth(); v.floatH = b.getHeight();
            return;
        }
    OrganismView v;
    v.organismName = name;
    v.editorFloating = floating;
    v.floatX = b.getX(); v.floatY = b.getY(); v.floatW = b.getWidth(); v.floatH = b.getHeight();
    model_.views.push_back(v);
}

void EngineHost::setEditorCollapsed(const std::string& name, bool collapsed) {
    dirty_ = true;
    for (auto& v : model_.views)
        if (v.organismName == name) { v.editorCollapsed = collapsed; return; }
    OrganismView v;
    v.organismName = name;
    v.editorCollapsed = collapsed;
    model_.views.push_back(v);
}

int EngineHost::editorMode(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.editorMode;
    return -1;
}

void EngineHost::setEditorMode(const std::string& name, int mode) {
    dirty_ = true;
    for (auto& v : model_.views)
        if (v.organismName == name) { v.editorMode = mode; return; }
    OrganismView v;
    v.organismName = name;
    v.editorMode = mode;
    model_.views.push_back(v);
}

juce::Point<int> EngineHost::freeSpot(juce::Point<int> want) const {
    constexpr int kW = 150, kH = 64;
    auto taken = [&](juce::Point<int> at) {
        for (const auto& cm : model_.organisms) {
            const auto p = position(cm.name);
            if (std::abs(p.x - at.x) < kW && std::abs(p.y - at.y) < kH) return true;
        }
        return false;
    };
    for (int col = 0; col < 12; ++col)
        for (int rowN = 0; rowN < 24; ++rowN) {
            const juce::Point<int> at{want.x + col * kW, want.y + rowN * kH};
            if (!taken(at)) return at;
        }
    return want;
}

juce::Point<int> EngineHost::spotBelowPatch() const {
    const auto master = const_cast<EngineHost*>(this)->masterOutputName();
    int bottom = 60, left = 60, masterY = 0;
    bool any = false, haveMaster = false;
    for (const auto& cm : model_.organisms) {
        const auto p = position(cm.name);
        if (!master.empty() && cm.name == master) {
            masterY = p.y;
            haveMaster = true;
            continue;
        }
        if (!any) { left = p.x; any = true; }
        left = std::min(left, p.x);
        bottom = std::max(bottom, p.y);
    }
    int y = any ? bottom + 80 : 140;
    if (haveMaster) y = std::min(y, masterY - 80);
    return freeSpot({left, std::max(60, y)});
}

}
