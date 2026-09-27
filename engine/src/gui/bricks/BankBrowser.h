// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "core/library/BankLibrary.h"
#include "core/tuning/ScaleLibrary.h"
#include "gui/app/AppSettings.h"
#include "gui/bricks/RowMarquee.h"
#include "gui/common/Localisation.h"
#include "gui/editor/files/BankRows.h"
#include "gui/editor/files/BankSlotModels.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class BankBrowser : public juce::Component {
public:
    using Row = files::BankRow;
    using Bucket = files::BankBucket;
    static constexpr const char* kScalesKind = "Scales";

    static juce::String recentKey(const std::string& className) {
        return juce::String("banks.recent.")
             + juce::String(className);
    }
    static std::vector<std::string> recentRefs(const std::string& className) {
        std::vector<std::string> out;
        for (const auto& line : juce::StringArray::fromLines(
                 AppSettings::instance().getString(recentKey(className))))
            if (const auto ref = line.trim(); ref.isNotEmpty()) out.push_back(ref.toStdString());
        return out;
    }
    static void remember(const std::string& ref, const std::string& className) {
        juce::StringArray keep;
        keep.add(juce::String(ref));
        for (const auto& line : juce::StringArray::fromLines(
                 AppSettings::instance().getString(recentKey(className)))) {
            const auto t = line.trim();
            if (t.isNotEmpty() && !keep.contains(t)) keep.add(t);
        }
        while (keep.size() > kRecents) keep.remove(keep.size() - 1);
        AppSettings::instance().set(recentKey(className), keep.joinIntoString("\n"));
    }
    static void forget(const std::string& ref, const std::string& className) {
        const juce::String drop(juce::CharPointer_UTF8(ref.c_str()));
        juce::StringArray keep;
        for (const auto& line : juce::StringArray::fromLines(
                 AppSettings::instance().getString(recentKey(className)))) {
            const auto t = line.trim();
            if (t.isNotEmpty() && t != drop) keep.add(t);
        }
        AppSettings::instance().set(recentKey(className), keep.joinIntoString("\n"));
    }

    static files::BankSource sourceOfFile(const juce::File& f, const banks::Slot& slot,
                                          const std::string& className) {
        const juce::String kind(banks::kindFor(slot).id);
        if (const auto mine = assetKindDir(userContentRoot(), kind);
            mine != juce::File() && f.isAChildOf(mine))
            return files::BankSource::Yours;
        for (const auto& d : packAssetRoots(kind))
            if (f.isAChildOf(d)) return files::BankSource::Pack;
        for (const auto& root : banks::roots(slot, className))
            if (f.isAChildOf(root)) return files::BankSource::Bundled;
        return files::BankSource::Imported;
    }

    static std::vector<files::BankRef> entries(const std::string& className,
                                              const banks::Slot& slot) {
        std::vector<files::BankRef> out;
        for (const auto& row : rowsFor(className, slot))
            if (!row.missing) out.push_back({row.ref, row.name});
        return out;
    }

    static juce::String bucketLabel(Bucket bucket) {
        switch (bucket) {
            case Bucket::All:      return tr("bank-browser.source-all", "All");
            case Bucket::Recent:   return tr("bank-browser.source-recent", "Recent");
            case Bucket::Yours:    return tr("bank-browser.source-yours", "Yours");
            case Bucket::Imported: return tr("bank-browser.source-imported", "Imported");
            case Bucket::Bundled:  return tr("bank-browser.source-bundled", "Bundled");
            case Bucket::Pack:     return tr("bank-browser.source-pack", "Pack");
            case Bucket::Factory:  return tr("bank-browser.source-factory", "Factory");
            case Bucket::Missing:  return tr("bank-browser.source-missing", "Missing");
        }
        return {};
    }

    static std::string leafOf(const std::string& ref) {
        const auto slash = ref.find_last_of('/');
        const auto leaf = slash == std::string::npos ? ref : ref.substr(slash + 1);
        const auto dot = leaf.find_last_of('.');
        return dot == std::string::npos ? leaf : leaf.substr(0, dot);
    }

    static std::vector<Row> rowsFor(const std::string& className, const banks::Slot& slot,
                                    const std::string& current = {}) {
        std::vector<Row> out;
        auto fileRow = [&](const std::string& ref, bool recent) {
            const juce::File f(juce::String(banks::resolve(ref, className)));
            Row row;
            row.ref = ref;
            row.recent = recent;
            row.missing = !f.exists();
            row.name = row.missing ? leafOf(ref) : f.getFileNameWithoutExtension().toStdString();
            row.source = files::BankSource::Imported;
            if (!row.missing) {
                row.kind = banks::kindOf(f, slot);
                row.source = sourceOfFile(f, slot, className);
            }
            return row;
        };
        std::vector<std::string> keys;
        auto add = [&](Row row) {
            auto key = row.source == files::BankSource::Factory
                           ? row.ref
                           : banks::resolve(row.ref, className);
            for (std::size_t i = 0; i < keys.size(); ++i)
                if (keys[i] == key) {
                    out[i].recent = out[i].recent || row.recent;
                    return;
                }
            keys.push_back(std::move(key));
            out.push_back(std::move(row));
        };

        for (const auto& e : banks::factoryEntries(slot))
            add({e.ref, e.name, e.kind, files::BankSource::Factory, false, false});
        for (const auto& f : banks::scan(slot, className)) {
            if (!banks::browsable(f, slot)) continue;
            add({banks::referenceFor(f, slot, className),
                 f.getFileNameWithoutExtension().toStdString(), banks::kindOf(f, slot),
                 sourceOfFile(f, slot, className), false, false});
        }
        for (const auto& ref : recentRefs(className)) add(fileRow(ref, true));
        if (!current.empty()) add(fileRow(current, false));
        describeScales(out, className, slot);

        std::sort(out.begin(), out.end(), [](const Row& a, const Row& b) {
            const auto ak = files::lowerAscii(a.kind), bk = files::lowerAscii(b.kind);
            if (ak != bk) return ak < bk;
            const auto an = files::lowerAscii(a.name), bn = files::lowerAscii(b.name);
            return an != bn ? an < bn : a.ref < b.ref;
        });
        return out;
    }

    static void describeScales(std::vector<Row>& rows, const std::string& className,
                               const banks::Slot& slot) {
        if (std::string(banks::kindFor(slot).id) != kScalesKind) return;
        std::vector<juce::File> folders;
        for (const auto& row : rows)
            if (!row.missing && row.source != files::BankSource::Factory)
                folders.push_back(fileAt(banks::resolve(row.ref, className)).getParentDirectory());
        std::sort(folders.begin(), folders.end());
        folders.erase(std::unique(folders.begin(), folders.end()), folders.end());
        std::map<std::string, std::string> details;
        for (const auto& folder : folders)
            for (const auto& info : scanScaleDirs({folder}))
                details[pathOf(fileAt(info.path))] = files::scaleDetail(info.degrees, info.description);
        for (auto& row : rows)
            if (const auto hit = details.find(pathOf(fileAt(banks::resolve(row.ref, className))));
                hit != details.end())
                row.detail = hit->second;
    }

    static void show(juce::Rectangle<int> anchor, const std::string& className,
                     const banks::Slot& slot, const std::string& current,
                     std::function<void(const std::string&)> onPick,
                     std::function<void()> onOpenFile,
                     std::function<void(const std::string&)> onForget) {
        auto owned = std::make_unique<BankBrowser>(rowsFor(className, slot, current),
                                                   std::move(onPick), std::move(onOpenFile),
                                                   std::move(onForget), !current.empty());
        auto* raw = owned.get();
        auto& box = juce::CallOutBox::launchAsynchronously(std::move(owned), anchor, nullptr);
        raw->box_ = &box;
    }

    BankBrowser(std::vector<Row> rows, std::function<void(const std::string&)> onPick,
                std::function<void()> onOpenFile,
                std::function<void(const std::string&)> onForget = {}, bool clearable = false)
        : all_(std::move(rows)), onPick_(std::move(onPick)), onOpenFile_(std::move(onOpenFile)),
          onForget_(std::move(onForget)) {
        bool factory = false;
        for (const auto& row : all_) factory = factory || row.source == files::BankSource::Factory;
        clearBtn_.setButtonText(factory
                                    ? tr("bank-browser.back-to-the-factory-bank",
                                         "Back to the factory bank")
                                    : tr("bank-browser.no-bank", "No bank"));
        clearBtn_.onClick = [this] { pickRef({}); };
        addChildComponent(clearBtn_);
        clearBtn_.setVisible(clearable);
        openBtn_.setButtonText(juce::String::fromUTF8("Open a file\xe2\x80\xa6"));
        openBtn_.onClick = [this] {
            auto open = onOpenFile_;
            if (box_ != nullptr) box_->dismiss();
            if (open) open();
        };
        addAndMakeVisible(openBtn_);
        search_.setTextToShowWhenEmpty(tr("bank-browser.search-banks", "Search banks"), Palette::textDim);
        search_.onTextChange = [this] { refilter(); };
        addAndMakeVisible(search_);
        kinds_.onChange = [this] {
            kind_ = kinds_.getSelectedId() <= 1 ? std::string()
                                                : kinds_.getText().toStdString();
            refilter();
        };
        addAndMakeVisible(kinds_);
        sources_.setModel(&sourceModel_);
        sources_.setRowHeight(22);
        sources_.setColour(juce::ListBox::backgroundColourId, Palette::panel.darker(0.2f));
        addAndMakeVisible(sources_);
        list_.setModel(&model_);
        list_.setRowHeight(22);
        list_.setColour(juce::ListBox::backgroundColourId, Palette::panel);
        addAndMakeVisible(list_);
        rebuild();
        setSize(420, 380);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(6);
        search_.setBounds(r.removeFromTop(24));
        r.removeFromTop(4);
        kinds_.setBounds(r.removeFromTop(22));
        r.removeFromTop(4);
        auto bottom = r.removeFromBottom(24);
        if (clearBtn_.isVisible()) {
            clearBtn_.setBounds(bottom.removeFromRight(bottom.getWidth() / 2 - 2));
            bottom.removeFromRight(4);
        }
        openBtn_.setBounds(bottom);
        r.removeFromBottom(4);
        sources_.setBounds(r.removeFromLeft(kSourcesW));
        r.removeFromLeft(4);
        list_.setBounds(r);
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::panel); }

    std::vector<std::string> bucketsForTest() const {
        std::vector<std::string> out;
        for (const auto& b : buckets_)
            out.push_back(files::bankBucketName(b.bucket) + " " + std::to_string(b.count));
        return out;
    }
    std::vector<std::string> rowsForTest() const {
        std::vector<std::string> out;
        for (const auto& r : shown_) out.push_back(r.name);
        return out;
    }
    bool selectBucketForTest(const std::string& name) {
        for (int i = 0; i < (int) buckets_.size(); ++i)
            if (files::bankBucketName(buckets_[(size_t) i].bucket) == name) {
                pickBucket(i);
                return true;
            }
        return false;
    }
    bool selectKindForTest(const std::string& kind) {
        for (int i = 0; i < kinds_.getNumItems(); ++i)
            if (kinds_.getItemText(i).toStdString() == kind) {
                kinds_.setSelectedId(kinds_.getItemId(i), juce::sendNotificationSync);
                return true;
            }
        return false;
    }
    RowMarquee& marqueeForTest() { return marquee_; }
    void searchForTest(const std::string& text) {
        search_.setText(juce::String::fromUTF8(text.c_str()), true);
    }
    void clickRowForTest(int row, bool onCross) { pick(row, onCross ? kRowW - 6 : 20); }
    bool clearForTest() {
        if (!clearBtn_.isVisible()) return false;
        clearBtn_.onClick();
        return true;
    }

private:
    void rebuild() {
        buckets_ = files::bankBuckets(all_);
        if (bucket_ >= (int) buckets_.size()) bucket_ = 0;
        sources_.updateContent();
        sources_.selectRow(bucket_, true);
        kinds_.clear(juce::dontSendNotification);
        kinds_.addItem(tr("bank-browser.all-types", "All types"), 1);
        int id = 2, mine = 1;
        for (const auto& kind : files::bankKinds(all_)) {
            kinds_.addItem(juce::String::fromUTF8(kind.c_str()), id);
            if (kind == kind_) mine = id;
            ++id;
        }
        if (mine == 1) kind_.clear();
        kinds_.setSelectedId(mine, juce::dontSendNotification);
        kinds_.setVisible(id > 2);
        refilter();
    }

    void refilter() {
        const auto want = bucket_ < (int) buckets_.size() ? buckets_[(size_t) bucket_].bucket
                                                          : Bucket::All;
        shown_ = files::filterBankRows(all_, want, search_.getText().trim().toStdString(), kind_);
        list_.updateContent();
        list_.repaint();
    }

    void pickBucket(int row) {
        if (row < 0 || row >= (int) buckets_.size() || row == bucket_) return;
        bucket_ = row;
        refilter();
    }

    void pick(int row, int x) {
        if (row < 0 || row >= (int) shown_.size()) return;
        const auto entry = shown_[(size_t) row];
        const bool onCross = x >= crossFrom() && files::bankRowForgettable(entry);
        if (onCross) {
            auto forget = onForget_;
            all_.erase(std::remove_if(all_.begin(), all_.end(),
                                      [&entry](const Row& r) { return r.ref == entry.ref; }),
                       all_.end());
            rebuild();
            if (forget) forget(entry.ref);
            return;
        }
        if (entry.missing) return;
        pickRef(entry.ref);
    }

    void pickRef(const std::string& ref) {
        auto cb = onPick_;
        if (box_ != nullptr) box_->dismiss();
        if (cb) cb(ref);
    }

    int crossFrom() const {
        const int w = list_.getWidth() > 0 ? list_.getWidth() : kRowW;
        return w - 18;
    }

    struct SourceModel : juce::ListBoxModel {
        explicit SourceModel(BankBrowser& o) : owner(o) {}
        int getNumRows() override { return (int) owner.buckets_.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override {
            if (row < 0 || row >= (int) owner.buckets_.size()) return;
            const auto& b = owner.buckets_[(size_t) row];
            if (sel) g.fillAll(Palette::accent.withAlpha(alpha::scrim));
            g.setColour(b.bucket == Bucket::Missing ? Palette::warnAmber() : Palette::text);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(bucketLabel(b.bucket), 6, 0, w - 30, h, juce::Justification::centredLeft,
                       false);
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.0f));
            g.drawText(juce::String(b.count), w - 26, 0, 20, h, juce::Justification::centredRight,
                       false);
        }
        void selectedRowsChanged(int row) override { owner.pickBucket(row); }
        BankBrowser& owner;
    };

    struct Model : juce::ListBoxModel {
        explicit Model(BankBrowser& o) : owner(o) {}
        int getNumRows() override { return (int) owner.shown_.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override {
            if (row < 0 || row >= (int) owner.shown_.size()) return;
            const auto& e = owner.shown_[(size_t) row];
            if (sel) g.fillAll(Palette::accent.withAlpha(alpha::scrim));
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.0f));
            g.drawText(files::bankBadge(e.kind), 6, 0, 40, h, juce::Justification::centredLeft, false);
            const int cross = files::bankRowForgettable(e) ? 18 : 0;
            if (e.recent && !e.missing) {
                g.setColour(Palette::accent);
                g.fillEllipse((float) (w - cross) - 12.0f, (float) h * 0.5f - 2.0f, 4.0f, 4.0f);
            }
            if (cross > 0) {
                g.setColour(e.missing ? Palette::warnAmber() : Palette::textDim);
                g.setFont(juce::FontOptions(13.0f));
                g.drawText(juce::String::charToString(juce::juce_wchar(0x00d7)), w - 18, 0, 14, h,
                           juce::Justification::centred, false);
            }
            g.setColour(e.missing ? Palette::warnAmber() : Palette::text);
            g.setFont(juce::FontOptions(12.0f));
            const int room = w - 72 - cross;
            const int nameW = e.detail.empty() ? room : std::min(room, kNameColumnW);
            g.drawText(juce::String::fromUTF8(e.name.c_str()), 50, 0, nameW, h,
                       juce::Justification::centredLeft, true);
            if (e.detail.empty() || room <= nameW) return;
            const juce::Font font(juce::FontOptions(10.5f));
            const auto detail = juce::String::fromUTF8(e.detail.c_str());
            const int detailX = 50 + nameW + 6, detailW = room - nameW - 6;
            const double textW = juce::GlyphArrangement::getStringWidth(font, detail);
            const int slide = (int) std::lround(owner.marquee_.offsetFor(row, textW, detailW));
            g.setColour(Palette::textDim);
            g.setFont(font);
            if (row != owner.marquee_.row() || !files::marqueeNeeded(textW, detailW)) {
                g.drawText(detail, detailX, 0, detailW, h, juce::Justification::centredLeft, true);
                return;
            }
            g.saveState();
            g.reduceClipRegion(detailX, 0, detailW, h);
            g.drawText(detail, detailX - slide, 0, (int) std::ceil(textW) + 2, h,
                       juce::Justification::centredLeft, false);
            g.restoreState();
        }
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override { owner.pick(row, e.x); }
        juce::String getTooltipForRow(int row) override {
            if (row < 0 || row >= (int) owner.shown_.size()) return {};
            const auto& e = owner.shown_[(size_t) row];
            const auto ref = (e.detail.empty() ? juce::String()
                                               : juce::String::fromUTF8(e.detail.c_str()) + " - ")
                             + juce::String::fromUTF8(e.ref.c_str());
            if (e.missing)
                return tr("bank-browser.gone", "Not found - click the cross to forget it: ") + ref;
            if (files::bankRowForgettable(e))
                return tr("bank-browser.unlink",
                          "Brought in from outside your library - the cross unlinks it: ") + ref;
            return ref;
        }
        BankBrowser& owner;
    };

    static constexpr int kRecents = 8;
    static constexpr int kNameColumnW = 118;
    static constexpr int kSourcesW = 104;
    static constexpr int kRowW = 420 - 12 - kSourcesW - 4;

    std::vector<Row> all_, shown_;
    std::vector<files::BankBucketCount> buckets_;
    int bucket_ = 0;
    std::function<void(const std::string&)> onPick_;
    std::function<void()> onOpenFile_;
    std::function<void(const std::string&)> onForget_;
    std::string kind_;
    juce::TextEditor search_;
    juce::ComboBox kinds_;
    juce::ListBox sources_, list_;
    juce::TextButton openBtn_, clearBtn_;
    SourceModel sourceModel_{*this};
    Model model_{*this};
    RowMarquee marquee_{list_};
    juce::CallOutBox* box_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankBrowser)
};

}
