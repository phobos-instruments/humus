// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserModel.h"
#include "gui/browser/BrowserPaint.h"
#include "gui/browser/Loaders.h"
#include "gui/common/Localisation.h"

namespace hum::browser {

class BrowserTable : public juce::Component, private juce::TableListBoxModel {
public:
    std::function<void()> onSelection;
    std::function<void(int row)> onOpen;
    std::function<void(const std::string& path, int stars)> onRate;
    std::function<void(const std::string& path, bool on)> onFavourite;
    std::function<void(Column, bool ascending)> onSort;
    std::function<void(int row, juce::Point<int>)> onMenu;
    std::function<void(const juce::StringArray& paths)> onDragOut;

    static constexpr int kRowH = 26, kPad = 10, kStarsLeft = 8;

    explicit BrowserTable(BrowserModel& model) : model_(model), table_({}, this) {
        auto& h = table_.getHeader();
        using F = juce::TableHeaderComponent::ColumnPropertyFlags;
        const int fixed = F::visible | F::resizable | F::sortable;
        const int optional = fixed | F::appearsOnColumnMenu;
        h.addColumn(tr("browser.col-name", "Name"), id(Column::Name), 320, 120, -1, fixed);
        h.addColumn(tr("browser.col-rating", "Rating"), id(Column::Rating), 80, 76, 90, optional);
        h.addColumn(tr("browser.col-length", "Length"), id(Column::Length), 70, 50, 100, optional);
        h.addColumn({}, id(Column::Favourite), 28, 28, 28, F::visible);
        h.addColumn(tr("browser.col-kind", "Kind"), id(Column::Kind), 70, 50, 100, optional & ~F::visible);
        h.addColumn(tr("browser.col-bpm", "BPM"), id(Column::Bpm), 56, 40, 80, optional & ~F::visible);
        h.addColumn(tr("browser.col-key", "Key"), id(Column::Key), 48, 36, 70, optional & ~F::visible);
        h.addColumn(tr("browser.col-rate", "Rate"), id(Column::Rate), 64, 50, 90, optional & ~F::visible);
        h.addColumn(tr("browser.col-added", "Added"), id(Column::Added), 96, 70, 140, optional & ~F::visible);
        h.addColumn(tr("browser.col-size", "Size"), id(Column::Size), 70, 50, 100, optional & ~F::visible);
        h.addColumn(tr("browser.col-folder", "Folder"), id(Column::Folder), 160, 60, -1, optional);
        h.addColumn(tr("browser.col-loads-into", "Loads into"), id(Column::LoadsInto), 170, 60, -1, optional);
        h.addColumn(tr("browser.col-modified", "Modified"), id(Column::Modified), 96, 70, 140, optional);
        h.addColumn(tr("browser.col-created", "Created"), id(Column::Created), 96, 70, 140, optional & ~F::visible);
        h.moveColumn(id(Column::Folder), 1);
        h.setStretchToFitActive(true);
        table_.setHeaderHeight(22);
        table_.setRowHeight(kRowH);
        table_.setMultipleSelectionEnabled(true);
        restyle();
        addAndMakeVisible(table_);
    }

    void restyle() {
        auto& h = table_.getHeader();
        h.setColour(juce::TableHeaderComponent::backgroundColourId, Palette::panel);
        h.setColour(juce::TableHeaderComponent::textColourId, Palette::textDim);
        h.setColour(juce::TableHeaderComponent::outlineColourId, Palette::border.withAlpha(alpha::muted));
        table_.setColour(juce::ListBox::backgroundColourId, Palette::background);
        table_.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        table_.repaint();
    }
    void lookAndFeelChanged() override { restyle(); }

    static int id(Column c) { return (int) c + 1; }
    static bool opensOnDoubleClick(Column c) { return c != Column::Rating && c != Column::Favourite; }

    float cellX(const juce::MouseEvent& e, int row, int columnId) {
        const auto cell = table_.getCellPosition(columnId, row, true);
        return (float) (e.getEventRelativeTo(&table_).x - cell.getX());
    }
    static Column columnOf(int columnId) { return (Column) (columnId - 1); }

    std::string columnState() const { return table_.getHeader().toString().toStdString(); }
    void restoreColumns(const std::string& state) {
        if (!state.empty()) table_.getHeader().restoreFromString(juce::String(state));
    }

    void refresh() {
        juce::SparseSet<int> rows;
        for (int i = 0; i < (int) model_.rows().size(); ++i)
            if (model_.isSelected(i)) rows.addRange({i, i + 1});
        syncing_ = true;
        table_.updateContent();
        table_.setSelectedRows(rows, juce::dontSendNotification);
        syncing_ = false;
        table_.repaint();
    }

    void scrollToSelection() {
        const int row = model_.rowOf(model_.focused());
        if (row >= 0) table_.scrollToEnsureRowIsOnscreen(row);
    }

    void focusList() {
        table_.grabKeyboardFocus();
        if (table_.getNumSelectedRows() == 0 && getNumRows() > 0) table_.selectRow(0);
    }

    void resized() override { table_.setBounds(getLocalBounds()); }

private:
    int getNumRows() override { return (int) model_.rows().size(); }

    void paintRowBackground(juce::Graphics& g, int, int w, int h, bool selected) override {
        if (selected) {
            g.fillAll(Palette::panelLight);
            g.setColour(Palette::accent);
            g.fillRect(0, 0, 2, h);
        }
        g.setColour(Palette::border.withAlpha(alpha::mist));
        g.drawHorizontalLine(h - 1, 0.0f, (float) w);
    }

    void paintCell(juce::Graphics& g, int row, int columnId, int w, int h, bool selected) override {
        if (row < 0 || row >= getNumRows()) return;
        const auto& e = model_.rows()[(size_t) row];
        const bool hot = selected;
        const float cy = (float) h * 0.5f;
        auto text = [&](const juce::String& s, juce::Justification j, bool dim = true) {
            g.setColour(dim ? Palette::textDim : Palette::text);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(s, juce::Rectangle<int>(kPad, 0, w - 2 * kPad, h), j, true);
        };
        switch (columnOf(columnId)) {
            case Column::Name: {
                const float bw = paint::drawBadge(g, e.kind, (float) kPad, cy);
                g.setColour(Palette::text);
                g.setFont(juce::FontOptions(12.5f));
                g.drawText(juce::String::fromUTF8(fileName(e.path).c_str()),
                           juce::Rectangle<int>(kPad + (int) bw + 8, 0, w - kPad * 2 - (int) bw - 8, h),
                           juce::Justification::centredLeft, true);
                break;
            }
            case Column::Rating:
                if (e.kind != Kind::Folder) paint::drawStars(g, e.rating, (float) kStarsLeft, cy, hot);
                break;
            case Column::Favourite: paint::drawHeart(g, {0.0f, 0.0f, (float) w, (float) h}, e.favourite, hot); break;
            case Column::Length: text(paint::lengthText(e.facts.seconds), juce::Justification::centredRight); break;
            case Column::Kind: text(kindWord(e.kind), juce::Justification::centredLeft); break;
            case Column::Bpm: text(e.facts.bpm > 0.0 ? juce::String(e.facts.bpm, e.facts.bpm == std::floor(e.facts.bpm) ? 0 : 1) : juce::String(), juce::Justification::centredRight); break;
            case Column::Key: text(juce::String(e.facts.key), juce::Justification::centredLeft); break;
            case Column::Rate: text(paint::rateText(e.facts.sampleRate), juce::Justification::centredRight); break;
            case Column::Added: text(paint::dateText(e.added), juce::Justification::centredLeft); break;
            case Column::Created: text(paint::dateText(e.created), juce::Justification::centredLeft); break;
            case Column::Modified: text(paint::dateText(e.modified), juce::Justification::centredLeft); break;
            case Column::Size: text(paint::sizeText(e.size), juce::Justification::centredRight); break;
            case Column::LoadsInto: text(loadsInto(e), juce::Justification::centredLeft); break;
            case Column::Folder:
                text(juce::String::fromUTF8(whereText(e.path, model_.place(), model_.roots()).c_str()), juce::Justification::centredLeft);
                break;
        }
    }

    void cellClicked(int row, int columnId, const juce::MouseEvent& e) override {
        if (row < 0 || row >= getNumRows()) return;
        const auto& entry = model_.rows()[(size_t) row];
        if (e.mods.isPopupMenu()) {
            const auto at = e.getScreenPosition();
            later([this, row, at] { if (onMenu) onMenu(row, at); });
            return;
        }
        if (entry.kind == Kind::Folder) return;
        const auto path = entry.path;
        if (columnOf(columnId) == Column::Rating && onRate) {
            const int stars = paint::starsAt(cellX(e, row, columnId), (float) kStarsLeft);
            const int rating = stars == entry.rating ? 0 : stars;
            later([this, path, rating] { if (onRate) onRate(path, rating); });
        } else if (columnOf(columnId) == Column::Favourite && onFavourite) {
            const bool on = !entry.favourite;
            later([this, path, on] { if (onFavourite) onFavourite(path, on); });
        }
    }

    void cellDoubleClicked(int row, int columnId, const juce::MouseEvent&) override {
        if (!opensOnDoubleClick(columnOf(columnId))) return;
        later([this, row] { if (onOpen) onOpen(row); });
    }

    void returnKeyPressed(int row) override {
        later([this, row] { if (onOpen) onOpen(row); });
    }

    void selectedRowsChanged(int) override {
        if (syncing_) return;
        const auto rows = table_.getSelectedRows();
        model_.clearSelection();
        const int last = table_.getLastRowSelected();
        for (int i = 0; i < rows.size(); ++i)
            if (rows[i] != last) model_.select(rows[i], true, false);
        if (last >= 0) model_.select(last, true, false);
        later([this] { if (onSelection) onSelection(); });
    }

    void sortOrderChanged(int columnId, bool forwards) override {
        if (onSort) onSort(columnOf(columnId), forwards);
    }

    juce::var getDragSourceDescription(const juce::SparseSet<int>& rows) override {
        juce::StringArray paths;
        for (int i = 0; i < rows.size(); ++i)
            if (rows[i] >= 0 && rows[i] < getNumRows()) paths.add(juce::String::fromUTF8(model_.rows()[(size_t) rows[i]].path.c_str()));
        if (onDragOut && !paths.isEmpty()) onDragOut(paths);
        return {};
    }

    juce::String getCellTooltip(int row, int columnId) override {
        if (row < 0 || row >= getNumRows()) return {};
        if (columnOf(columnId) == Column::LoadsInto) {
            const auto classes = LoaderTable::shared().classesFor(model_.rows()[(size_t) row].path);
            return juce::String::fromUTF8(loadsIntoText(classes, (int) classes.size()).c_str());
        }
        if (columnOf(columnId) != Column::Name) return {};
        return juce::String::fromUTF8(model_.rows()[(size_t) row].path.c_str());
    }

    juce::String loadsInto(const Entry& e) const {
        if (e.kind == Kind::Folder || e.kind == Kind::Project || e.kind == Kind::Patch) return {};
        const auto ext = juce::File(juce::String::fromUTF8(e.path.c_str())).getFileExtension().toLowerCase();
        if (const auto it = loadsCache_.find(ext); it != loadsCache_.end()) return it->second;
        const auto text = juce::String::fromUTF8(loadsIntoText(LoaderTable::shared().classesFor(e.path)).c_str());
        loadsCache_[ext] = text;
        return text;
    }

    mutable std::map<juce::String, juce::String> loadsCache_;
    template <class Fn> void later(Fn fn) {
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<BrowserTable>(this), fn = std::move(fn)] {
            if (safe != nullptr) fn();
        });
    }

    BrowserModel& model_;
    juce::TableListBox table_;
    bool syncing_ = false;
};

}
