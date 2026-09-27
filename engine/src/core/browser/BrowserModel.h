// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "core/browser/BrowserPlaces.h"
#include "core/browser/BrowserQuery.h"

namespace hum::browser {

enum class Column { Name, Rating, Length, Favourite, Kind, Bpm, Key, Rate, Added, Size, Folder, LoadsInto, Created, Modified };

struct PickRequest {
    std::string title;
    std::vector<Kind> kinds;
    std::string current;
    bool offerOther = false;
    std::string patterns;
    std::string remember;
    std::optional<Place> resume;
    bool active() const { return !title.empty(); }
};

class BrowserModel {
public:
    void setRoots(PlaceRoots roots) { roots_ = std::move(roots); }
    const PlaceRoots& roots() const { return roots_; }

    void setPlace(const Place& place);
    const Place& place() const { return place_; }

    void setSearch(const std::string& text) { search_ = text; }
    const std::string& search() const { return search_; }
    Query query() const;

    void sortBy(Column column, bool ascending);
    Column sortColumn() const { return sort_; }
    bool sortAscending() const { return ascending_; }

    void beginPick(const PickRequest& request, const FileIndex& index);
    void endPick() { pick_ = {}; }
    const PickRequest& pick() const { return pick_; }

    void refresh(const FileIndex& index);
    const std::vector<Entry>& rows() const { return rows_; }
    int rowOf(const std::string& path) const;
    std::vector<SidebarItem> sidebar(const FileIndex& index) const { return sidebarFor(index, roots_, pick_.kinds); }

    void select(int row, bool add, bool range);
    void selectOnly(const std::string& path);
    void clearSelection() { selected_.clear(); }
    bool isSelected(int row) const;
    std::vector<std::string> selection() const { return {selected_.begin(), selected_.end()}; }
    std::string focused() const { return focus_; }

private:
    void sortRows();
    bool fitsPick(const Entry& e) const;

    PlaceRoots roots_;
    Place place_;
    std::string search_;
    Column sort_ = Column::Name;
    bool ascending_ = true;
    PickRequest pick_;
    std::vector<Entry> rows_;
    std::set<std::string> selected_;
    std::string focus_;
};

}
