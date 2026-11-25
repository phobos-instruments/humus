// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "core/plugins/BridgeEditorControl.h"
#include "gui/host/EngineHost.h"
#include "gui/plugins/PluginEditorWindow.h"
#include "gui/plugins/PluginQuarantine.h"

namespace hum {

class PluginWindows {
public:
    explicit PluginWindows(EngineHost& host) : host_(host) {}
    ~PluginWindows() {
        *alive_ = false;
        windows_.clear();
    }

    std::function<void(const std::string&)> beforeOpen;

    void open(const std::string& name) {
        auto* node = host_.pluginNodeFor(name);
        if (node == nullptr || !node->hasEditor() || quarantine::contains(node->classRaw())) return;
        if (auto* bridged = dynamic_cast<BridgeEditorControl*>(node)) {
            bridged->bridgeSetFloating(true);
            return;
        }
        if (const auto it = windows_.find(name); it != windows_.end()) {
            it->second->toFront(true);
            return;
        }
        auto* hosted = host_.hostedPluginFor(name);
        if (hosted == nullptr) return;
        if (beforeOpen) beforeOpen(name);
        windows_[name] = std::make_unique<PluginEditorWindow>(host_, name, *hosted, nullptr,
                                                              [this](const std::string& n) { closed(n); });
    }

    template <class Doomed>
    void dropWhere(Doomed&& doomed) {
        for (auto it = windows_.begin(); it != windows_.end();)
            it = doomed(it->first) ? windows_.erase(it) : std::next(it);
    }

    bool isOpen(const std::string& name) const { return windows_.count(name) > 0; }

private:
    void closed(const std::string& name) {
        host_.syncPluginStateToModel();
        juce::MessageManager::callAsync([this, alive = alive_, name] {
            if (*alive) windows_.erase(name);
        });
    }

    EngineHost& host_;
    std::map<std::string, std::unique_ptr<PluginEditorWindow>> windows_;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

}
