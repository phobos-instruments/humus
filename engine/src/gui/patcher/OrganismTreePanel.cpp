// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/OrganismTreePanel.h"

#include <utility>

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "gui/common/CategoryText.h"
#include "gui/common/Localisation.h"
#include "gui/bricks/RootCollar.h"
#include "gui/patcher/ClassPickerMenu.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

namespace {

constexpr int kRowH = 22, kSearchH = 26, kPad = 6, kBadgeW = 32;

constexpr int kMarkSize = 18;

int paintBadge(juce::Graphics& g, const std::string& badge, int height) {
    const float cy = (float) height * 0.5f;
    const juce::Rectangle<float> r(4.5f, cy - 6.5f, (float) kBadgeW, 13.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r, 3.0f, 1.0f);
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.drawText(juce::String(badge), r, juce::Justification::centred, false);
    return kBadgeW + 5;
}

int paintGrownMark(juce::Graphics& g, const std::string& display, int height) {
    const auto& mark = collar::grownMark(collar::speciesNameFor(display), Family::Utility);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(mark, juce::Rectangle<float>(2.0f, (float) (height - kMarkSize) * 0.5f, (float) kMarkSize, (float) kMarkSize));
    return kMarkSize + 2;
}

void paintRow(juce::Graphics& g, int width, int height, const juce::String& text, bool strong, bool selected,
              const organismtree::Leaf* leaf = nullptr) {
    if (selected) {
        g.setColour(Palette::accent.withAlpha(alpha::scrim));
        g.fillRect(0, 0, width, height);
    }
    const int x = 4 + (leaf != nullptr ? OrganismTreePanel::paintLeafMark(g, *leaf, height) : 0);
    g.setColour(strong ? Palette::text : Palette::textDim);
    g.setFont(juce::FontOptions(strong ? 12.5f : 12.0f, strong ? juce::Font::bold : juce::Font::plain));
    g.drawText(text, x, 0, width - x - 4, height, juce::Justification::centredLeft, true);
}

class LeafItem : public juce::TreeViewItem {
public:
    LeafItem(organismtree::Leaf leaf, const OrganismTreePanel& panel) : leaf_(std::move(leaf)), panel_(panel) {}
    bool mightContainSubItems() override { return false; }
    int getItemHeight() const override { return kRowH; }
    void paintItem(juce::Graphics& g, int w, int h) override {
        paintRow(g, w, h, juce::String::fromUTF8(leaf_.display.c_str()), false, isSelected(), &leaf_);
    }
    void itemDoubleClicked(const juce::MouseEvent&) override {
        if (panel_.onAdd) panel_.onAdd(leaf_.cls);
    }
    juce::var getDragSourceDescription() override { return juce::String(kClassDragPrefix) + juce::String(leaf_.cls); }
    juce::String getTooltip() override { return tr("organism-tree.leaf-tip", "Drag onto the patcher, or double-click to add"); }

private:
    organismtree::Leaf leaf_;
    const OrganismTreePanel& panel_;
};

class FolderItem : public juce::TreeViewItem {
public:
    FolderItem(juce::String text, bool strong) : text_(std::move(text)), strong_(strong) {}
    bool mightContainSubItems() override { return true; }
    int getItemHeight() const override { return kRowH; }
    void paintItem(juce::Graphics& g, int w, int h) override { paintRow(g, w, h, text_, strong_, false); }
    void itemClicked(const juce::MouseEvent&) override { setOpen(!isOpen()); }

private:
    juce::String text_;
    bool strong_;
};

class RootItem : public juce::TreeViewItem {
public:
    bool mightContainSubItems() override { return true; }
    void paintItem(juce::Graphics&, int, int) override {}
};

void addFolder(juce::TreeViewItem& parent, const organismtree::Folder& folder, bool strong, bool open,
               const OrganismTreePanel& panel) {
    auto* item = new FolderItem(juce::String::fromUTF8(folder.name.c_str()), strong);
    parent.addSubItem(item);
    for (const auto& child : folder.folders) addFolder(*item, child, false, open, panel);
    for (const auto& leaf : folder.leaves) item->addSubItem(new LeafItem(leaf, panel));
    item->setOpen(open || strong);
}

void openAll(juce::TreeViewItem& item, bool open) {
    for (int i = 0; i < item.getNumSubItems(); ++i)
        if (auto* child = item.getSubItem(i); child->mightContainSubItems()) {
            openAll(*child, open);
            child->setOpen(open);
        }
}

int countOpen(const juce::TreeViewItem& item) {
    int n = 0;
    for (int i = 0; i < item.getNumSubItems(); ++i)
        if (const auto* child = item.getSubItem(i); child->getNumSubItems() > 0)
            n += (child->isOpen() ? 1 : 0) + countOpen(*child);
    return n;
}

juce::String originText(const std::string& origin) {
    if (origin == "Plugins") return tr("organism-tree.plugins", "Plug-ins");
    return juce::String::fromUTF8(origin.c_str());
}

}

OrganismTreePanel::OrganismTreePanel() {
    applyTexts();
    unfoldAll_.onClick = [this] { setAllOpen(true); };
    foldAll_.onClick = [this] { setAllOpen(false); };
    addAndMakeVisible(unfoldAll_);
    addAndMakeVisible(foldAll_);
    search_.setFont(juce::FontOptions(13.5f));
    search_.onTextChange = [this] { rebuildTree(); };
    search_.onEscapeKey = [this] { search_.clear(); rebuildTree(); };
    addAndMakeVisible(search_);
    tree_.setRootItemVisible(false);
    tree_.setDefaultOpenness(false);
    tree_.setIndentSize(14);
    tree_.setColour(juce::TreeView::linesColourId, Palette::textDim);
    tree_.setColour(juce::TreeView::backgroundColourId, Palette::background);
    addAndMakeVisible(tree_);
}

OrganismTreePanel::~OrganismTreePanel() { tree_.setRootItem(nullptr); }

std::vector<organismtree::Folder> OrganismTreePanel::collect() const {
    organismtree::Folder root;
    for (const auto& [origin, groups] : classPickerGroups()) {
        auto& section = organismtree::childOf(root, originText(origin).toStdString());
        for (const auto& [category, classes] : groups) {
            std::vector<std::string> path;
            for (const auto& segment : categorySegments(category)) path.push_back(categoryText(segment).toStdString());
            for (const auto& cls : classes) organismtree::place(section, path, leafFor(cls));
        }
    }
    return root.folders;
}

int OrganismTreePanel::paintLeafMark(juce::Graphics& g, const organismtree::Leaf& leaf, int height) {
    return leaf.badge.empty() ? paintGrownMark(g, leaf.display, height) : paintBadge(g, leaf.badge, height);
}

organismtree::Leaf OrganismTreePanel::leafFor(const std::string& cls) {
    const auto id = parseClassString(cls);
    return {cls, id.display, pluginBadge(id.kind)};
}

void OrganismTreePanel::setAllOpen(bool open) {
    if (root_ != nullptr) openAll(*root_, open);
}

int OrganismTreePanel::openFoldersForTest() const { return root_ != nullptr ? countOpen(*root_) : 0; }

void OrganismTreePanel::applyTexts() {
    search_.setTextToShowWhenEmpty(tr("organism-tree.search", "Search..."), Palette::textDim);
    unfoldAll_.setTooltip(tr("organism-tree.expand-all", "Expand all"));
    foldAll_.setTooltip(tr("organism-tree.collapse-all", "Collapse all"));
    if (isVisible()) refresh();
}

void OrganismTreePanel::refresh() {
    all_ = collect();
    rebuildTree();
}

void OrganismTreePanel::visibilityChanged() {
    if (isVisible()) refresh();
}

void OrganismTreePanel::rebuildTree() {
    const auto query = search_.getText().toStdString();
    shown_ = organismtree::filtered(all_, query);
    tree_.setRootItem(nullptr);
    root_ = std::make_unique<RootItem>();
    const bool searching = !query.empty();
    for (const auto& section : shown_) addFolder(*root_, section, true, searching, *this);
    tree_.setRootItem(root_.get());
}

void OrganismTreePanel::resized() {
    auto r = getLocalBounds().reduced(kPad);
    auto top = r.removeFromTop(kSearchH);
    foldAll_.setBounds(top.removeFromRight(kSearchH));
    top.removeFromRight(2);
    unfoldAll_.setBounds(top.removeFromRight(kSearchH));
    top.removeFromRight(kPad);
    search_.setBounds(top);
    r.removeFromTop(kPad);
    tree_.setBounds(r);
}

void OrganismTreePanel::paint(juce::Graphics& g) { g.fillAll(Palette::background); }

}
