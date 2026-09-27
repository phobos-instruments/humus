// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/BrowserModel.h"

#include <algorithm>
#include <cctype>

#include "core/browser/PlaceMemory.h"

namespace hum::browser {

namespace {

std::string folded(const std::string& s) {
    std::string out(s);
    for (auto& c : out) c = (char) std::tolower((unsigned char) c);
    return out;
}

template <class T> int order(const T& a, const T& b) { return a < b ? -1 : (b < a ? 1 : 0); }

int compare(const Entry& a, const Entry& b, Column column) {
    switch (column) {
        case Column::Rating: return order(a.rating, b.rating);
        case Column::Length: return order(a.facts.seconds, b.facts.seconds);
        case Column::Favourite: return order(a.favourite, b.favourite);
        case Column::Kind: return order((int) a.kind, (int) b.kind);
        case Column::Bpm: return order(a.facts.bpm, b.facts.bpm);
        case Column::Key: return order(a.facts.key, b.facts.key);
        case Column::Rate: return order(a.facts.sampleRate, b.facts.sampleRate);
        case Column::Added: return order(a.added, b.added);
        case Column::Size: return order(a.size, b.size);
        case Column::Folder: return order(parentOf(a.path), parentOf(b.path));
        case Column::Created: return order(a.created, b.created);
        case Column::Modified: return order(a.modified, b.modified);
        case Column::LoadsInto: case Column::Name: break;
    }
    return 0;
}

}

void BrowserModel::setPlace(const Place& place) {
    if (place == place_) return;
    place_ = place;
    selected_.clear();
}

Query BrowserModel::query() const {
    auto q = parseQuery(search_);
    if (pick_.active() && !pick_.kinds.empty()) q.lockKinds(pick_.kinds);
    return q;
}

void BrowserModel::sortBy(Column column, bool ascending) {
    sort_ = column;
    ascending_ = ascending;
    sortRows();
}

void BrowserModel::beginPick(const PickRequest& request, const FileIndex& index) {
    pick_ = request;
    place_ = request.resume && placeStillThere(*request.resume, request.kinds, roots_, index)
                 ? *request.resume
                 : homeFor(request.kinds, request.current, roots_, index);
    selected_.clear();
    if (!request.current.empty()) {
        selected_.insert(request.current);
        focus_ = request.current;
    }
}

void BrowserModel::refresh(const FileIndex& index) {
    const auto q = query();
    rows_.clear();
    if (place_.type == Place::Type::Folder) {
        const bool projectsWanted = !pick_.active() || pick_.kinds.empty()
            || std::find(pick_.kinds.begin(), pick_.kinds.end(), Kind::Project) != pick_.kinds.end();
        for (auto& e : listFolder(place_.path, index)) {
            if (e.kind == Kind::Project && !projectsWanted) e.kind = Kind::Folder;
            if (!fitsPick(e)) continue;
            if (q.empty() || q.matches(e)) rows_.push_back(std::move(e));
        }
    } else {
        for (const auto* e : entriesIn(index, place_, roots_))
            if (e != nullptr && fitsPick(*e) && (q.empty() || q.matches(*e))) rows_.push_back(*e);
    }
    if (place_.type != Place::Type::Recent) sortRows();
    for (auto it = selected_.begin(); it != selected_.end();)
        it = rowOf(*it) < 0 && *it != pick_.current ? selected_.erase(it) : std::next(it);
}

bool BrowserModel::fitsPick(const Entry& e) const {
    if (!pick_.active() || pick_.patterns.empty() || e.kind == Kind::Folder) return true;
    return matchesPatterns(e.path, pick_.patterns);
}

void BrowserModel::sortRows() {
    const auto column = sort_;
    const bool up = ascending_;
    std::stable_sort(rows_.begin(), rows_.end(), [&](const Entry& a, const Entry& b) {
        const bool fa = a.kind == Kind::Folder || a.kind == Kind::Project;
        const bool fb = b.kind == Kind::Folder || b.kind == Kind::Project;
        if (fa != fb) return fa;
        int c = compare(a, b, column);
        if (c == 0) c = order(folded(fileName(a.path)), folded(fileName(b.path)));
        if (c == 0) c = order(a.path, b.path);
        return up ? c < 0 : c > 0;
    });
}

int BrowserModel::rowOf(const std::string& path) const {
    for (int i = 0; i < (int) rows_.size(); ++i)
        if (rows_[(size_t) i].path == path) return i;
    return -1;
}

void BrowserModel::select(int row, bool add, bool range) {
    if (row < 0 || row >= (int) rows_.size()) return;
    const auto& path = rows_[(size_t) row].path;
    if (range && !focus_.empty() && rowOf(focus_) >= 0) {
        const int a = std::min(row, rowOf(focus_)), b = std::max(row, rowOf(focus_));
        if (!add) selected_.clear();
        for (int i = a; i <= b; ++i) selected_.insert(rows_[(size_t) i].path);
    } else if (add) {
        if (!selected_.erase(path)) selected_.insert(path);
    } else {
        selected_ = {path};
    }
    focus_ = path;
}

void BrowserModel::selectOnly(const std::string& path) {
    selected_ = {path};
    focus_ = path;
}

bool BrowserModel::isSelected(int row) const {
    return row >= 0 && row < (int) rows_.size() && selected_.count(rows_[(size_t) row].path) > 0;
}

}
