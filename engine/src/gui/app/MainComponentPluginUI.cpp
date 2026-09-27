// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"

#include "gui/video/WindowFloat.h"
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

void MainComponent::openVisualUI(const std::string& name, int width, int height) {
    auto* vn = dynamic_cast<VideoNode*>(host_.liveOrganism(name));
    if (vn == nullptr || dynamic_cast<VisualSource*>(host_.liveOrganism(name)) != nullptr)
        return;
    auto it = visualWindows_.find(name);
    if (it != visualWindows_.end()) {
        if (width > 0 && height > 0) it->second->openAt(width, height);
        it->second->toFront(true);
        return;
    }
    visualWindows_[name] = std::make_unique<VisualWindow>(
        host_, name, this, [this](const std::string& n) { closeVisualUI(n); }, width, height);
}

void MainComponent::reopenVisualOutputs() {
    const auto want = windowfloat::outputsToReopen(host_.model(), "Screen");
    if (want.empty()) return;
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<MainComponent>(this), want] {
        if (safe == nullptr) return;
        const int home = safe->displayHoldingMain();
        bool held = false;
        for (const auto& name : want) {
            safe->openVisualUI(name);
            const auto it = safe->visualWindows_.find(name);
            if (it == safe->visualWindows_.end() || home <= 0) continue;
            if ((int) safe->host_.liveParamValue(name, "Screen") != home) continue;
            it->second->guardDisplay(home);
            held = true;
        }
        if (held)
            safe->notifyError(tr("main-plugin-ui.output-windowed",
                            "A video output was set to fill the screen Humus is on, so it opened in a window. To fill it anyway, set its Screen to Window and back."));
    });
}

int MainComponent::displayHoldingMain() const {
    const auto* top = getTopLevelComponent();
    if (top == nullptr || !top->isShowing()) return 0;
    const auto& displays = juce::Desktop::getInstance().getDisplays().displays;
    const auto centre = top->getScreenBounds().getCentre();
    for (int i = 0; i < displays.size(); ++i)
        if (displays.getReference(i).totalArea.contains(centre)) return i + 1;
    return 0;
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
