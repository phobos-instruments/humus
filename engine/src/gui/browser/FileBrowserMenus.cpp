// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/FileBrowserView.h"

#include <algorithm>
#include <functional>

#include "core/app/AppPaths.h"
#include "core/browser/FolderScan.h"
#include "core/library/ExamplesTree.h"
#include "core/library/UserLibrary.h"
#include "gui/browser/LibraryActions.h"
#include "core/packs/Catalogue.h"
#include "gui/app/AppSettings.h"

namespace hum::browser {

namespace {

enum MenuId {
    kReveal = 1,
    kFavourite,
    kRemoveFromCollection,
    kNewCollectionWith,
    kAddTag,
    kStopWatching,
    kRenameCollection,
    kDeleteCollection,
    kMoveLibrary,
    kImport,
    kRemoveFromLibrary,
    kRemoveFromRecent,
    kPin,
    kUnpin,
    kRateBase = 100,
    kCollectionBase = 200,
};

void askText(juce::Component* owner, const juce::String& title, const juce::String& prompt, const juce::String& initial,
             std::function<void(const std::string&)> done) {
    auto* w = new juce::AlertWindow(title, prompt, juce::MessageBoxIconType::NoIcon);
    w->addTextEditor("text", initial);
    w->addButton(tr("browser.ok", "OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton(tr("browser.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe = juce::Component::SafePointer<juce::Component>(owner), w, done = std::move(done)](int r) {
            const auto text = w->getTextEditorContents("text").trim().toStdString();
            if (r == 1 && !text.empty() && safe != nullptr) done(text);
        }), true);
}

}

void FileBrowserView::showRowMenu(int row, juce::Point<int> at) {
    if (row < 0 || row >= (int) model_.rows().size()) return;
    const auto clicked = model_.rows()[(size_t) row].path;
    if (!model_.isSelected(row)) {
        model_.selectOnly(clicked);
        table_.refresh();
    }
    const auto paths = model_.selection();
    const auto collections = index_.read([](const FileIndex& i) { return i.collections(); });
    const auto* entry = &model_.rows()[(size_t) row];

    juce::PopupMenu rate;
    for (int s = 0; s <= kMaxRating; ++s)
        rate.addItem(kRateBase + s, s == 0 ? tr("browser.no-rating", "No rating") : juce::String(s) + " / " + juce::String(kMaxRating),
                     true, paths.size() == 1 && entry->rating == s);
    juce::PopupMenu into;
    for (int i = 0; i < (int) collections.size(); ++i)
        into.addItem(kCollectionBase + i, juce::String::fromUTF8(collections[(size_t) i].name.c_str()));
    if (!collections.empty()) into.addSeparator();
    into.addItem(kNewCollectionWith, tr("browser.new-collection-dots", "New Collection..."));

    juce::PopupMenu menu;
    menu.addItem(kFavourite, tr("browser.favourite", "Favourite"), true, paths.size() == 1 && entry->favourite);
    menu.addSubMenu(tr("browser.rate", "Rate"), rate);
    menu.addItem(kAddTag, tr("browser.add-tag-dots", "Add Tag..."));
    menu.addSubMenu(tr("browser.add-to-collection", "Add to Collection"), into);
    if (model_.place().type == Place::Type::Collection)
        menu.addItem(kRemoveFromCollection, tr("browser.remove-from-collection", "Remove from This Collection"));
    menu.addSeparator();
    const auto root = model_.roots().library;
    const bool canImport = std::any_of(paths.begin(), paths.end(), [&](const std::string& p) { return importable(p, root); });
    menu.addItem(kImport, tr("browser.import", "Import to Library"), canImport);
    menu.addItem(kRemoveFromLibrary, tr("browser.move-to-trash-dots", "Move to Trash..."), !trashable(paths, trashGuard()).empty());
    if (model_.place().type == Place::Type::Recent)
        menu.addItem(kRemoveFromRecent, tr("browser.remove-from-recent", "Remove from Recent"));
    menu.addSeparator();
    if (paths.size() == 1 && entry->kind == Kind::Folder) menu.addItem(kPin, tr("browser.pin", "Pin to Sidebar"));
    menu.addItem(kReveal, paint::revealLabel(), paths.size() == 1);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                       [this, paths, collections, entry = *entry](int r) {
        const auto now = index_.now();
        if (r == kReveal) fileAt(paths.front()).revealToUser();
        else if (r == kPin) pinFolders(paths);
        else if (r == kFavourite)
            index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.setFavourite(p, !entry.favourite, now); });
        else if (r >= kRateBase && r <= kRateBase + kMaxRating)
            index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.setRating(p, r - kRateBase, now); });
        else if (r == kAddTag) {
            askText(this, tr("browser.add-tag", "Add Tag"), tr("browser.tag-prompt", "Tag for the selected files"), {},
                    [this, paths](const std::string& tag) {
                        index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.addTag(p, tag, index_.now()); });
                        refreshNow();
                    });
        } else if (r >= kCollectionBase && r < kCollectionBase + (int) collections.size()) {
            const auto& name = collections[(size_t) (r - kCollectionBase)].name;
            index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.addToCollection(name, p); });
        } else if (r == kNewCollectionWith) {
            newCollection(paths);
        } else if (r == kImport) {
            importToLibrary(paths, model_.roots().library, index_);
        } else if (r == kRemoveFromLibrary) {
            confirmTrash(paths);
        } else if (r == kRemoveFromRecent) {
            index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.forgetUse(p); });
            if (onForgetRecent) for (const auto& p : paths) onForgetRecent(p);
        } else if (r == kRemoveFromCollection) {
            const auto name = model_.place().name;
            index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.removeFromCollection(name, p); });
        }
        refreshNow();
    });
}

void FileBrowserView::showPlaceMenu(const Place& place, juce::Point<int> at) {
    juce::PopupMenu menu;
    if (place.type == Place::Type::Watched) {
        menu.addItem(kReveal, paint::revealLabel());
        menu.addItem(kStopWatching, tr("browser.stop-watching", "Stop Watching"));
    } else if (place.type == Place::Type::Collection) {
        menu.addItem(kRenameCollection, tr("browser.rename", "Rename..."));
        menu.addItem(kDeleteCollection, tr("browser.delete-collection", "Delete Collection"));
    } else if (!place.path.empty()) {
        menu.addItem(kReveal, paint::revealLabel());
        if (place.type == Place::Type::Shelf)
            menu.addItem(kMoveLibrary, tr("browser.move-library", "Move Library Folder..."));
        if (pinned(place.path)) menu.addItem(kUnpin, tr("browser.unpin", "Remove from Sidebar"));
    } else {
        return;
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}), [this, place](int r) {
        if (r == kReveal) fileAt(place.path).revealToUser();
        else if (r == kMoveLibrary) moveLibrary();
        else if (r == kUnpin) index_.edit([&](FileIndex& i) { i.unpin(place.path); });
        else if (r == kStopWatching) {
            index_.edit([&](FileIndex& i) { i.unwatch(place.path); });
            index_.pruneOutsidePlaces();
        }
        else if (r == kDeleteCollection) index_.edit([&](FileIndex& i) { i.removeCollection(place.name); });
        else if (r == kRenameCollection)
            askText(this, tr("browser.rename-collection", "Rename Collection"), {}, juce::String::fromUTF8(place.name.c_str()),
                    [this, place](const std::string& name) {
                        index_.edit([&](FileIndex& i) { i.renameCollection(place.name, name); });
                        refreshNow();
                    });
        if ((r == kStopWatching || r == kDeleteCollection) && model_.place() == place) model_.setPlace(Place{});
        refreshNow();
    });
}

void FileBrowserView::moveLibrary() {
    chooser_ = std::make_unique<juce::FileChooser>(tr("library-pane.choose-the-library-folder", "Choose the library folder"),
                                                   library::root());
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this](const juce::FileChooser& fc) {
        const auto dir = fc.getResult();
        if (!dir.isDirectory()) return;
        AppSettings::instance().set("library.path", dir.getFullPathName());
        library::configuredRoot() = pathOf(dir);
        catalogue::plantUserFolders();
        auto roots = model_.roots();
        roots.library = pathOf(library::root());
        model_.setRoots(roots);
        index_.rescan();
        refreshNow();
    });
}

void FileBrowserView::addWatchedFolder() {
    chooser_ = std::make_unique<juce::FileChooser>(tr("browser.watch-folder", "Choose a folder to watch"));
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this](const juce::FileChooser& fc) {
        const auto dir = fc.getResult();
        if (!dir.isDirectory()) return;
        const auto path = pathOf(dir);
        index_.edit([&](FileIndex& i) { i.watch(path); });
        index_.rescan();
        Place p;
        p.type = Place::Type::Watched;
        p.path = path;
        showPlace(p);
    });
}

void FileBrowserView::confirmTrash(const std::vector<std::string>& paths) {
    const auto going = trashable(paths, trashGuard());
    if (going.empty()) return;
    const auto name = juce::String::fromUTF8(fileName(going.front()).c_str());
    const auto question = going.size() > 1
        ? trn("browser.trash-many", "Move {n} item to the Trash?", "Move these {n} items to the Trash?", (long long) going.size())
        : fileAt(going.front()).isDirectory()
            ? tr("browser.trash-folder", "Move \"{name}\" and everything in it to the Trash?", {{"name", name}})
            : tr("browser.trash-one", "Move \"{name}\" to the Trash?", {{"name", name}});
    trashQuestion_ = question;
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, tr("browser.move-to-trash", "Move to Trash"),
                                       question, tr("browser.move-to-trash", "Move to Trash"), tr("browser.cancel", "Cancel"), this,
                                       juce::ModalCallbackFunction::create([safe = juce::Component::SafePointer<FileBrowserView>(this), going](int ok) {
        if (ok == 0 || safe == nullptr) return;
        for (const auto& path : moveToTrash(going, safe->trashGuard(), safe->index_, safe->discard_))
            if (safe->onForgetRecent) safe->onForgetRecent(path);
        safe->refreshNow();
    }));
}

TrashGuard FileBrowserView::trashGuard() const {
    const auto& roots = model_.roots();
    TrashGuard guard;
    guard.libraryRoot = roots.library;
    guard.places = {roots.library, roots.patches, roots.recordings, roots.presets, roots.home};
    for (const auto& folder : roots.computer) guard.places.push_back(folder.path);
    index_.read([&](const FileIndex& i) {
        guard.places.insert(guard.places.end(), i.watched().begin(), i.watched().end());
        guard.places.insert(guard.places.end(), i.pinned().begin(), i.pinned().end());
        return 0;
    });
    for (const auto& root : examples::roots()) guard.shipped.push_back(pathOf(root));
    guard.shipped.push_back(pathOf(juce::File::getSpecialLocation(juce::File::currentApplicationFile)));
    guard.openDocument = openDocument ? openDocument() : std::string();
    return guard;
}

void FileBrowserView::tagSelection() {
    const auto paths = model_.selection();
    if (paths.empty()) return;
    askText(this, tr("browser.add-tag", "Add Tag"), tr("browser.tag-prompt", "Tag for the selected files"), {},
            [this, paths](const std::string& tag) {
                index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.addTag(p, tag, index_.now()); });
                refreshNow();
            });
}

void FileBrowserView::collectSelection() {
    const auto paths = model_.selection();
    if (paths.empty()) return;
    const auto collections = index_.read([](const FileIndex& i) { return i.collections(); });
    juce::PopupMenu menu;
    for (int i = 0; i < (int) collections.size(); ++i)
        menu.addItem(kCollectionBase + i, juce::String::fromUTF8(collections[(size_t) i].name.c_str()));
    if (!collections.empty()) menu.addSeparator();
    menu.addItem(kNewCollectionWith, tr("browser.new-collection-dots", "New Collection..."));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&batch_), [this, paths, collections](int r) {
        if (r == kNewCollectionWith) {
            newCollection(paths);
            return;
        }
        if (r < kCollectionBase || r >= kCollectionBase + (int) collections.size()) return;
        const auto& name = collections[(size_t) (r - kCollectionBase)].name;
        index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.addToCollection(name, p); });
        refreshNow();
    });
}

void FileBrowserView::newCollection(const std::vector<std::string>& adding) {
    askText(this, tr("browser.new-collection", "New Collection"), tr("browser.collection-prompt", "Name"), {},
            [this, adding](const std::string& name) {
                index_.edit([&](FileIndex& i) {
                    i.addCollection(name);
                    for (const auto& p : adding) i.addToCollection(name, p);
                });
                refreshNow();
            });
}

}
