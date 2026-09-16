// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"
#include "gui/properties/PropertiesPane.h"
#include "gui/plugins/PluginEditorWindow.h"
#include "gui/video/VisualWindow.h"

#include "core/plugins/BridgeEditorControl.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "gui/plugins/PluginQuarantine.h"
#include "gui/common/Localisation.h"

namespace hum {

void MainComponent::openPluginUI(const std::string& name) {
    auto* pn = host_.pluginNodeFor(name);
    if (!pn || !pn->hasEditor()) return;
    if (quarantine::contains(pn->classRaw())) {
        auto* aw = new juce::AlertWindow(
            tr("main-plugin-ui.quarantined", "Quarantined plugin UI"),
            juce::String(pn->classRaw()).upToFirstOccurrenceOf("?", false, false)
                + " froze Humus before. Open its UI anyway?",
            juce::MessageBoxIconType::WarningIcon);
        aw->addButton(tr("main-plugin-ui.open-anyway", "Open anyway"), 1);
        aw->addButton(tr("main-plugin-ui.un-quarantine", "Un-quarantine"), 2);
        aw->addButton(tr("main-plugin-ui.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        aw->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, name, cls = pn->classRaw()](int r) {
                if (r == 0) return;
                if (r == 2) quarantine::remove(cls);
                openPluginUIImpl(name);
            }), true);
        return;
    }
    openPluginUIImpl(name);
}

void MainComponent::openPluginUIImpl(const std::string& name) {
    auto* pn = host_.pluginNodeFor(name);
    if (!pn || !pn->hasEditor()) return;
    watchdog_.noteActivePlugin(pn->classRaw());

    if (auto* be = dynamic_cast<BridgeEditorControl*>(pn)) {
        be->bridgeSetFloating(true);
        return;
    }

    auto it = pluginWindows_.find(name);
    if (it != pluginWindows_.end()) { it->second->toFront(true); return; }
    auto* hp = host_.hostedPluginFor(name);
    if (!hp) return;
    propsPane_->releaseEmbedded(name);
    pluginWindows_[name] = std::make_unique<PluginEditorWindow>(
        host_, name, *hp, this,
        [this](const std::string& n) { closePluginUI(n); });
}

void MainComponent::openVisualUI(const std::string& name) {
    auto* vn = dynamic_cast<VideoNode*>(host_.liveOrganism(name));
    if (vn == nullptr || dynamic_cast<VisualSource*>(host_.liveOrganism(name)) != nullptr)
        return;
    auto it = visualWindows_.find(name);
    if (it != visualWindows_.end()) { it->second->toFront(true); return; }
    visualWindows_[name] = std::make_unique<VisualWindow>(
        host_, name, this, [this](const std::string& n) { closeVisualUI(n); });
}

void MainComponent::closeVisualUI(const std::string& name) {
    juce::MessageManager::callAsync([this, name] { visualWindows_.erase(name); });
}

void MainComponent::closePluginUI(const std::string& name) {
    host_.syncPluginStateToModel();
    juce::MessageManager::callAsync([this, name] {
        pluginWindows_.erase(name);
#if JUCE_MAC
        if (auto* hp = host_.hostedPluginFor(name))
            if (auto* leaked = hp->instance()->getActiveEditor())
                hp->instance()->editorBeingDeleted(leaked);
#endif
    });
}

}
