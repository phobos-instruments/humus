// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserPlaces.h"
#include "core/browser/FolderScan.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum::browser {

class BrowserSidebar : public juce::Component {
public:
    std::function<void(const Place&)> onChoose;
    std::function<void()> onAddFolder, onNewCollection;
    std::function<void(const Place&, juce::Point<int>)> onMenu;

    static constexpr int kRowH = 22, kHeadH = 24, kPad = 10;

    void setItems(std::vector<SidebarItem> items, const Place& current, const std::string& scanning) {
        items_ = std::move(items);
        current_ = current;
        scanning_ = scanning;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel);
        g.setColour(Palette::border.withAlpha(alpha::strong));
        g.drawVerticalLine(getWidth() - 1, 0.0f, (float) getHeight());
        for (const auto& row : layout()) {
            if (row.item == nullptr) {
                paintHead(g, row);
                continue;
            }
            paintItem(g, row);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int over = rowAt(e.getPosition());
        if (over != hover_) { hover_ = over; repaint(); }
    }
    void mouseExit(const juce::MouseEvent&) override { hover_ = -1; repaint(); }

    void mouseUp(const juce::MouseEvent& e) override {
        const auto rows = layout();
        const int i = rowAt(e.getPosition());
        if (i < 0) return;
        const auto& row = rows[(size_t) i];
        if (row.item == nullptr) {
            if (!row.plus) return;
            if (row.section == SidebarItem::Section::Watched && onAddFolder) onAddFolder();
            if (row.section == SidebarItem::Section::Collections && onNewCollection) onNewCollection();
            return;
        }
        if (e.mods.isPopupMenu()) {
            if (onMenu) onMenu(row.item->place, e.getScreenPosition());
            return;
        }
        if (!row.item->dimmed && onChoose) onChoose(row.item->place);
    }

private:
    struct Row {
        juce::Rectangle<int> bounds;
        const SidebarItem* item = nullptr;
        SidebarItem::Section section = SidebarItem::Section::Top;
        juce::String head;
        bool plus = false;
    };

    static juce::String headOf(SidebarItem::Section s) {
        switch (s) {
            case SidebarItem::Section::Library: return tr("browser.library", "LIBRARY");
            case SidebarItem::Section::Yours: return tr("browser.yours", "YOURS");
            case SidebarItem::Section::Computer: return tr("browser.computer", "COMPUTER");
            case SidebarItem::Section::Watched: return tr("browser.watched", "WATCHED FOLDERS");
            case SidebarItem::Section::Collections: return tr("browser.collections", "COLLECTIONS");
            case SidebarItem::Section::Top: break;
        }
        return {};
    }

    std::vector<Row> layout() const {
        std::vector<Row> rows;
        int y = 6;
        auto head = [&](SidebarItem::Section s) {
            Row r;
            r.section = s;
            r.head = headOf(s);
            r.plus = s == SidebarItem::Section::Watched || s == SidebarItem::Section::Collections;
            r.bounds = {0, y, getWidth(), kHeadH};
            rows.push_back(r);
            y += kHeadH;
        };
        auto section = SidebarItem::Section::Top;
        for (const auto s : {SidebarItem::Section::Top, SidebarItem::Section::Library, SidebarItem::Section::Yours,
                             SidebarItem::Section::Computer, SidebarItem::Section::Watched,
                             SidebarItem::Section::Collections}) {
            section = s;
            if (s != SidebarItem::Section::Top) head(s);
            const bool anyShown = std::any_of(items_.begin(), items_.end(), [&](const SidebarItem& item) {
                return item.section == section && !item.dimmed;
            });
            const bool picking = std::any_of(items_.begin(), items_.end(), [](const SidebarItem& i) { return i.dimmed; });
            if (s != SidebarItem::Section::Top && !anyShown && picking) {
                rows.pop_back();
                y -= kHeadH;
                continue;
            }
            for (const auto& item : items_) {
                if (item.section != section || item.dimmed) continue;
                Row r;
                r.item = &item;
                r.section = s;
                r.bounds = {0, y, getWidth(), kRowH};
                rows.push_back(r);
                y += kRowH;
            }
        }
        return rows;
    }

    const SidebarItem* nearestFolder() const {
        if (current_.type != Place::Type::Folder) return nullptr;
        const SidebarItem* best = nullptr;
        for (const auto& item : items_) {
            if (item.place.type != Place::Type::Folder) continue;
            const bool holds = current_.path == item.place.path || isUnder(current_.path, item.place.path);
            if (holds && (best == nullptr || item.place.path.size() > best->place.path.size())) best = &item;
        }
        return best;
    }

    int rowAt(juce::Point<int> p) const {
        const auto rows = layout();
        for (int i = 0; i < (int) rows.size(); ++i)
            if (rows[(size_t) i].bounds.contains(p)) return i;
        return -1;
    }

    void paintHead(juce::Graphics& g, const Row& row) const {
        auto r = row.bounds.reduced(kPad, 0).withTrimmedTop(8);
        g.setColour(Palette::textDim.withAlpha(alpha::strong));
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(row.head, r, juce::Justification::centredLeft, false);
        if (row.plus) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(14.0f));
            g.drawText("+", r, juce::Justification::centredRight, false);
        }
    }

    void paintItem(juce::Graphics& g, const Row& row) const {
        const auto& item = *row.item;
        const bool current = item.place == current_ || (&item == nearestFolder());
        auto r = row.bounds.reduced(4, 1);
        if (current) {
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(r.toFloat(), 4.0f);
            g.setColour(Palette::accent);
            g.fillRoundedRectangle(r.toFloat().withWidth(2.0f), 1.0f);
        } else if (hover_ >= 0 && rowAt(row.bounds.getCentre()) == hover_ && !item.dimmed) {
            g.setColour(Palette::panelLight.withAlpha(alpha::mid));
            g.fillRoundedRectangle(r.toFloat(), 4.0f);
        }
        g.setColour(item.dimmed ? Palette::textDim.withAlpha(alpha::muted) : current ? Palette::text : Palette::text.withAlpha(alpha::heavy));
        g.setFont(juce::FontOptions(12.5f));
        g.drawText(juce::String::fromUTF8(item.label.c_str()), r.withTrimmedLeft(kPad), juce::Justification::centredLeft, true);
        if (!scanning_.empty() && !item.place.path.empty() && scanning_ == item.place.path) {
            g.setColour(Palette::accent.withAlpha(alpha::mid));
            g.fillRect(r.getX() + kPad, r.getBottom() - 2, r.getWidth() / 3, 1);
        }
    }

    std::vector<SidebarItem> items_;
    Place current_;
    std::string scanning_;
    int hover_ = -1;
};

}
