// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"

#include "gui/browser/Loaders.h"
#include "gui/common/Localisation.h"

namespace hum {

namespace {

std::vector<std::string> pathsOf(const juce::StringArray& files) {
    std::vector<std::string> out;
    for (const auto& f : files) out.push_back(f.toStdString());
    return out;
}

}

bool PatcherCanvas::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& f : files)
        if (!browser::LoaderTable::shared().classesFor(f.toStdString()).empty()) return true;
    return false;
}

void PatcherCanvas::dropNewOrganisms(const std::string& className, const std::vector<std::string>& paths, juce::Point<int> at) {
    const auto made = browser::loadIntoNew(host_, className, paths, at.x, at.y, scope_);
    refresh();
    if (!made.empty()) select(made.back());
}

void PatcherCanvas::filesDropped(const juce::StringArray& files, int x, int y) {
    const auto paths = pathsOf(files);
    const auto at = modelPos(juce::Point<int>(x, y));
    if (const auto node = hitNode(at); !node.empty() && !browser::loadInto(host_, node, paths.front()).empty()) {
        select(node);
        return;
    }
    const auto classes = browser::LoaderTable::shared().classesFor(paths.front());
    if (classes.empty()) return;
    if (classes.size() == 1) {
        dropNewOrganisms(classes.front(), paths, at);
        return;
    }
    juce::PopupMenu menu;
    menu.addSectionHeader(tr("patcher-drop.load-into-new", "Load into a new..."));
    for (int i = 0; i < (int) classes.size(); ++i) menu.addItem(i + 1, juce::String::fromUTF8(classes[(size_t) i].c_str()));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(juce::Rectangle<int>(x, y, 1, 1))),
                       [safe = juce::Component::SafePointer<PatcherCanvas>(this), classes, paths, at](int r) {
        if (safe != nullptr && r >= 1 && r <= (int) classes.size()) safe->dropNewOrganisms(classes[(size_t) (r - 1)], paths, at);
    });
}

}
