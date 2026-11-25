// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/patcher/OrganismTreeModel.h"
#include "gui/style/IconButton.h"

namespace hum {

class OrganismTreePanel : public juce::Component {
public:
    std::function<void(const std::string& cls)> onAdd;

    OrganismTreePanel();
    ~OrganismTreePanel() override;

    void refresh();
    void applyTexts();
    void resized() override;
    void paint(juce::Graphics& g) override;
    void visibilityChanged() override;

    int shownLeavesForTest() const { return organismtree::leafCount(shown_); }
    void setAllOpen(bool open);
    static organismtree::Leaf leafFor(const std::string& cls);
    static int paintLeafMark(juce::Graphics& g, const organismtree::Leaf& leaf, int height);
    void showFoldersForTest(std::vector<organismtree::Folder> folders) {
        all_ = std::move(folders);
        rebuildTree();
    }
    int openFoldersForTest() const;
    IconButton& unfoldAllForTest() { return unfoldAll_; }
    IconButton& foldAllForTest() { return foldAll_; }
    void searchForTest(const juce::String& text) {
        search_.setText(text, false);
        rebuildTree();
    }

private:
    void rebuildTree();
    std::vector<organismtree::Folder> collect() const;

    juce::TextEditor search_;
    IconButton unfoldAll_{IconGlyph::UnfoldAll, {}};
    IconButton foldAll_{IconGlyph::FoldAll, {}};
    juce::TreeView tree_;
    std::unique_ptr<juce::TreeViewItem> root_;
    std::vector<organismtree::Folder> all_, shown_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganismTreePanel)
};

}
