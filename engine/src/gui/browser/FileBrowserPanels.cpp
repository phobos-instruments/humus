// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/FileBrowserView.h"

#include <algorithm>

#include "core/app/AppPaths.h"
#include "gui/app/AppSettings.h"

namespace hum::browser {

namespace {

constexpr const char* kInspectorKey = "browser.inspector";
constexpr const char* kProjectsViewKey = "browser.projectsView";

}

bool FileBrowserView::placeLoading() const {
    const auto& place = model_.place();
    switch (place.type) {
        case Place::Type::Projects: return index_.anyPlacePending();
        case Place::Type::Shelf:
        case Place::Type::Watched:
        case Place::Type::Recordings:
        case Place::Type::Presets: return index_.placePending(place.path);
        default: return false;
    }
}

bool FileBrowserView::showsCards() const {
    return model_.place().type == Place::Type::Projects && projectsView_ != kList;
}

void FileBrowserView::setProjectsView(int view) {
    projectsView_ = std::clamp(view, (int) kCards, (int) kList);
    AppSettings::instance().set(kProjectsViewKey, projectsView_);
    viewCards_.setToggleState(projectsView_ == kCards, juce::dontSendNotification);
    viewSmall_.setToggleState(projectsView_ == kSmall, juce::dontSendNotification);
    viewList_.setToggleState(projectsView_ == kList, juce::dontSendNotification);
    cards_.setCompact(projectsView_ == kSmall);
    refreshNow();
    resized();
}

void FileBrowserView::wirePanels() {
    refresh_.setTooltip(tr("browser.refresh-tip", "Look again for new, moved and deleted files (Cmd+R)"));
    refresh_.onClick = [this] { refreshFromDisk(); };
    addAndMakeVisible(refresh_);
    viewCards_.setTooltip(tr("browser.view-cards", "Cards"));
    viewSmall_.setTooltip(tr("browser.view-small", "Small"));
    viewList_.setTooltip(tr("browser.view-list", "List"));
    const int views[] = {kCards, kSmall, kList};
    int k = 0;
    for (auto* b : {&viewCards_, &viewSmall_, &viewList_}) {
        b->setClickingTogglesState(false);
        const int view = views[k++];
        b->onClick = [this, view] { setProjectsView(view); };
        addChildComponent(*b);
    }
    addChildComponent(crumbs_);
    crumbs_.onChoose = [this](const std::string& dir) { showPlace(folderPlace(dir)); };
    cardsView_.setViewedComponent(&cards_, false);
    cardsView_.setScrollBarsShown(true, false);
    addChildComponent(cardsView_);
    cards_.onSelection = [this] { selectionMoved(); };
    cards_.onOpen = [this](int row) { open(row); };
    cards_.onMenu = [this](int row, juce::Point<int> at) { showRowMenu(row, at); };
    cards_.onRate = [this](const std::string& path, int stars) {
        index_.edit([&](FileIndex& i) { i.setRating(path, stars, index_.now()); });
        refreshNow();
    };
    addChildComponent(veil_);
    addChildComponent(inspector_);
    addChildComponent(batch_);
    addAndMakeVisible(info_);
    info_.setButtonText("i");
    info_.setTooltip(tr("browser.inspector-tip", "Details, tags and collections (I)"));
    info_.setClickingTogglesState(true);
    info_.onClick = [this] { setInspectorOpen(info_.getToggleState()); };

    auto focusedPath = [this] { return model_.focused(); };
    inspector_.onRate = [this, focusedPath](int stars) {
        index_.edit([&](FileIndex& i) { i.setRating(focusedPath(), stars, index_.now()); });
        refreshNow();
    };
    inspector_.onFavourite = [this, focusedPath](bool on) {
        index_.edit([&](FileIndex& i) { i.setFavourite(focusedPath(), on, index_.now()); });
        refreshNow();
    };
    inspector_.onAddTag = [this, focusedPath](const std::string& tag) {
        index_.edit([&](FileIndex& i) { i.addTag(focusedPath(), tag, index_.now()); });
        refreshNow();
    };
    inspector_.onRemoveTag = [this, focusedPath](const std::string& tag) {
        index_.edit([&](FileIndex& i) { i.removeTag(focusedPath(), tag); });
        refreshNow();
    };
    inspector_.onCollection = [this, focusedPath](const std::string& name, bool in) {
        index_.edit([&](FileIndex& i) {
            if (in) i.addToCollection(name, focusedPath());
            else i.removeFromCollection(name, focusedPath());
        });
        refreshNow();
    };
    inspector_.onReveal = [focusedPath] { fileAt(focusedPath()).revealToUser(); };

    batch_.onRate = [this](int stars) {
        const auto paths = model_.selection();
        index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.setRating(p, stars, index_.now()); });
        refreshNow();
    };
    batch_.onTag = [this] { tagSelection(); };
    batch_.onCollect = [this] { collectSelection(); };
    batch_.onClear = [this] {
        const auto keep = model_.focused();
        model_.selectOnly(keep);
        table_.refresh();
        selectionMoved();
    };
    restyle();
    setInspectorOpen(AppSettings::instance().getInt(kInspectorKey, 0) != 0);
    setProjectsView(AppSettings::instance().getInt(kProjectsViewKey, kCards));
}

void FileBrowserView::restyle() {
    title_.setColour(juce::Label::textColourId, Palette::text);
    open_.setColour(juce::TextButton::buttonColourId, Palette::accent);
    open_.setColour(juce::TextButton::textColourOffId, paint::onAccent());
    for (auto* b : {&cancel_, &other_, &info_}) {
        b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        b->setColour(juce::TextButton::textColourOffId, Palette::text);
    }
    info_.setColour(juce::TextButton::buttonOnColourId, Palette::accent);
    info_.setColour(juce::TextButton::textColourOnId, paint::onAccent());
    repaint();
}

void FileBrowserView::refreshFromDisk() {
    const int dropped = index_.pruneGone();
    index_.rescan();
    flash_ = dropped > 0 ? juce::String(dropped) + " " + tr("browser.gone-removed", "missing files removed")
                         : tr("browser.refreshed", "Looking for changes");
    flashUntil_ = juce::Time::getMillisecondCounterHiRes() + kFlashMs;
    refreshNow();
}

bool FileBrowserView::forgetIfGone(const std::string& path) {
    if (!BrowserIndex::gone(path)) return false;
    index_.edit([&](FileIndex& i) { i.forget(path); });
    flash_ = juce::String::fromUTF8(fileName(path).c_str()) + " " + tr("browser.is-gone", "is no longer there, so it was removed");
    flashUntil_ = juce::Time::getMillisecondCounterHiRes() + kFlashMs;
    refreshNow();
    return true;
}

void FileBrowserView::setInspectorOpen(bool open) {
    inspectorOpen_ = open;
    AppSettings::instance().set(kInspectorKey, open ? 1 : 0);
    info_.setToggleState(open, juce::dontSendNotification);
    inspector_.setVisible(open);
    syncPanels();
    resized();
}

void FileBrowserView::syncPanels() {
    const int picked = (int) model_.selection().size();
    const bool wasBatch = batch_.isVisible();
    batch_.setCount(picked);
    batch_.setVisible(picked > 1);
    if (inspectorOpen_) {
        const int row = model_.rowOf(model_.focused());
        const Entry* e = row >= 0 ? &model_.rows()[(size_t) row] : nullptr;
        std::vector<std::string> names, holding;
        index_.read([&](const FileIndex& i) {
            for (const auto& c : i.collections()) names.push_back(c.name);
            if (e != nullptr) holding = i.collectionsHolding(e->path);
            return 0;
        });
        const auto where = e != nullptr ? juce::String::fromUTF8(whereText(e->path, Place{}, model_.roots()).c_str()) : juce::String();
        inspector_.show(e, names, holding, where);
    }
    if (wasBatch != batch_.isVisible()) resized();
}

}
