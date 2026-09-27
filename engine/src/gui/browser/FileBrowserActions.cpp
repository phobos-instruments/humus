// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/FileBrowserView.h"

#include <string>
#include <vector>

#include "gui/browser/LibraryActions.h"

namespace hum::browser {

bool FileBrowserView::keyPressed(const juce::KeyPress& key) {
    const auto c = key.getTextCharacter();
    if (c >= '0' && c <= '5' && !key.getModifiers().isCommandDown()) {
        rateSelection(c - '0');
        return true;
    }
    if ((c == 'i' || c == 'I') && !key.getModifiers().isCommandDown()) {
        setInspectorOpen(!inspectorOpen_);
        return true;
    }
    if ((c == 'f' || c == 'F') && !key.getModifiers().isCommandDown()) {
        favouriteSelection();
        return true;
    }
    if (key == juce::KeyPress::spaceKey && audition_ != nullptr) {
        if (const auto* sound = focusedSound()) {
            if (audition_->current() == sound->path) audition_->toggle();
            else audition_->play(sound->path, sound->facts.bpm);
        }
        return true;
    }
    const bool up = key == juce::KeyPress::backspaceKey
        || key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier, 0);
    if (up && model_.place().type == Place::Type::Folder) {
        const auto parent = parentOf(model_.place().path);
        if (!parent.empty() && parent != model_.place().path) showPlace(folderPlace(parent));
        return true;
    }
    if (key == juce::KeyPress('r', juce::ModifierKeys::commandModifier, 0)) {
        refreshFromDisk();
        return true;
    }
    if (key == juce::KeyPress('f', juce::ModifierKeys::commandModifier, 0)) {
        search_.focus();
        return true;
    }
    if (key == juce::KeyPress::escapeKey && model_.pick().active()) {
        if (onCancelled) onCancelled();
        return true;
    }
    return false;
}

void FileBrowserView::rateSelection(int stars) {
    const auto paths = model_.selection();
    if (paths.empty()) return;
    index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.setRating(p, stars, index_.now()); });
    refreshNow();
}

void FileBrowserView::favouriteSelection() {
    const auto paths = model_.selection();
    if (paths.empty()) return;
    const bool on = index_.read([&](const FileIndex& i) {
        const auto* e = i.find(paths.front());
        return e == nullptr || !e->favourite;
    });
    index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.setFavourite(p, on, index_.now()); });
    refreshNow();
}

void FileBrowserView::open(int row) {
    if (row < 0 || row >= (int) model_.rows().size()) return;
    const auto& entry = model_.rows()[(size_t) row];
    if (forgetIfGone(entry.path)) return;
    if (entry.kind == Kind::Folder) {
        showPlace(folderPlace(entry.path));
        return;
    }
    if (model_.pick().active()) {
        model_.selectOnly(entry.path);
        confirmPick();
        return;
    }
    const auto path = entry.path;
    index_.edit([&](FileIndex& i) { i.noteUsed(path, index_.now()); });
    if (onOpen) onOpen(path);
}

bool FileBrowserView::pickable() const {
    const auto paths = model_.selection();
    if (paths.empty()) return false;
    for (const auto& p : paths) {
        const int row = model_.rowOf(p);
        if (row >= 0 && model_.rows()[(size_t) row].kind == Kind::Folder) return false;
    }
    return true;
}

void FileBrowserView::confirmPick() {
    const auto paths = model_.selection();
    for (const auto& p : paths)
        if (forgetIfGone(p)) return;
    if (!pickable()) {
        if (paths.size() == 1 && model_.rowOf(paths.front()) >= 0) open(model_.rowOf(paths.front()));
        return;
    }
    index_.edit([&](FileIndex& i) { for (const auto& p : paths) i.noteUsed(p, index_.now()); });
    if (onPicked) onPicked(paths);
}

bool FileBrowserView::isInterestedInFileDrag(const juce::StringArray& files) {
    if (dragging_) return false;
    for (const auto& f : files)
        if (importable(f.toStdString(), model_.roots().library)) return true;
    return false;
}

void FileBrowserView::filesDropped(const juce::StringArray& files, int, int) {
    std::vector<std::string> paths;
    for (const auto& f : files) paths.push_back(f.toStdString());
    const auto landed = importToLibrary(paths, model_.roots().library, index_);
    if (landed.empty()) return;
    refreshNow();
    const int row = model_.rowOf(landed.front());
    if (row >= 0) {
        model_.selectOnly(landed.front());
        table_.refresh();
        table_.scrollToSelection();
    }
}

void FileBrowserView::dragOut(const juce::StringArray& paths) {
    if (dragging_) return;
    dragging_ = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles(paths, false, this, [this] { dragging_ = false; });
}

}
