// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamSchema.h"
#include "core/assistant/PresetGenie.h"
#include "gui/assistant/AiClient.h"
#include "gui/style/Colours.h"
#include "gui/style/IconButton.h"
#include "gui/host/PropertiesHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/host/NodeRandomize.h"
#include "gui/properties/PresetActions.h"
#include "gui/common/Localisation.h"

namespace hum {

class PresetRow : public juce::Component {
public:
    PresetRow(int number, const juce::String& nm, bool current, bool canPaste)
        : number_(number), current_(current), canPaste_(canPaste) {
        name_.setText(nm.isEmpty() ? tr("presets.untitled", "(Untitled)") : nm, juce::dontSendNotification);
        name_.setEditable(false, true, false);
        name_.setColour(juce::Label::textColourId, current ? Palette::accent : Palette::text);
        name_.setFont(juce::FontOptions(12.0f));
        name_.onTextChange = [this] { if (onRename) onRename(name_.getText()); };
        addAndMakeVisible(name_);
        addAndMakeVisible(recall_);
        addAndMakeVisible(store_);
        addAndMakeVisible(clear_);
        clear_.setTooltip(tr("presets.delete-this-preset", "Delete this preset"));
        recall_.onClick = [this] { if (onRecall) onRecall(); };
        store_.onClick  = [this] { if (onStore)  onStore(); };
        clear_.onClick  = [this] { if (onClear)  onClear(); };
    }

    std::function<void()> onRecall, onStore, onClear, onCut, onCopy, onPaste;
    std::function<void(const juce::String&)> onRename;

    void mouseDoubleClick(const juce::MouseEvent&) override { if (onRecall) onRecall(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) return;
        enum { Recall = 1, Store, Clear, Rename, Cut, Copy, Paste };
        juce::PopupMenu m;
        m.addItem(Recall, tr("presets.recall", "Recall"));
        m.addItem(Store, tr("presets.store", "Store"));
        m.addItem(Clear, tr("presets.clear", "Clear"));
        m.addItem(Rename, tr("presets.rename", "Rename..."));
        m.addSeparator();
        m.addItem(Cut, tr("presets.cut", "Cut"));
        m.addItem(Copy, tr("presets.copy", "Copy"));
        m.addItem(Paste, tr("presets.paste", "Paste"), canPaste_);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
                            {e.getScreenX(), e.getScreenY(), 1, 1}),
                        [this](int r) {
            switch (r) {
                case Recall: if (onRecall) onRecall(); break;
                case Store:  if (onStore)  onStore();  break;
                case Clear:  if (onClear)  onClear();  break;
                case Rename: name_.showEditor();       break;
                case Cut:    if (onCut)    onCut();    break;
                case Copy:   if (onCopy)   onCopy();   break;
                case Paste:  if (onPaste)  onPaste();  break;
                default: break;
            }
        });
    }

    void paint(juce::Graphics& g) override {
        if (current_) { g.setColour(Palette::accent.withAlpha(alpha::mist)); g.fillRect(getLocalBounds()); }
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String(number_), 2, 0, 16, getHeight(), juce::Justification::centred);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(1);
        r.removeFromLeft(18);
        clear_.setBounds(r.removeFromRight(22).reduced(1));
        store_.setBounds(r.removeFromRight(24).reduced(1));
        recall_.setBounds(r.removeFromRight(24).reduced(1));
        name_.setBounds(r);
    }

private:
    int number_;
    bool current_;
    bool canPaste_ = false;
    juce::Label name_;
    IconButton recall_{IconGlyph::Open, tr("presets.recall-this-preset", "Recall this preset")};
    IconButton store_{IconGlyph::Save, tr("presets.store-current-settings-into-this", "Store current settings into this slot")};
    juce::TextButton clear_{juce::String::charToString(juce::juce_wchar(0x00d7))};
};

class PresetsView : public juce::Component {
public:
    PresetsView(PropertiesHost& host, std::string organism, std::function<void()> onChanged)
        : host_(host), name_(std::move(organism)), onChanged_(std::move(onChanged)) {
        addBtn_.setTooltip(tr("presets.add-a-new-preset-from", "Add a new preset from the current settings"));
        prevBtn_.setTooltip(tr("presets.recall-previous-preset", "Recall previous preset"));
        nextBtn_.setTooltip(tr("presets.recall-next-preset", "Recall next preset"));
        for (auto* b : {&addBtn_, &prevBtn_, &nextBtn_, &recallBtn_, &storeBtn_, &clearBtn_})
            addAndMakeVisible(*b);
        addAndMakeVisible(curName_);
        curName_.setEditable(true);
        curName_.setJustificationType(juce::Justification::centredLeft);
        curName_.setColour(juce::Label::backgroundColourId, Palette::background);
        curName_.setColour(juce::Label::textColourId, Palette::accent);
        curName_.onTextChange = [this] {
            if (const auto r = current(); r.valid()) {
                host_.presets().rename(name_, r, curName_.getText().toStdString());
                changed();
            }
        };
        addBtn_.onClick    = [this] { host_.presets().store(name_, {}); changed(); };
        prevBtn_.onClick   = [this] { recallBracketed([this] { host_.presets().recallAdjacent(name_, -1); }); };
        nextBtn_.onClick   = [this] { recallBracketed([this] { host_.presets().recallAdjacent(name_, +1); }); };
        recallBtn_.onClick = [this] { if (const auto r = current(); r.valid()) recallBracketed([this, r] { host_.presets().recall(name_, r); }); };
        storeBtn_.onClick  = [this] { host_.presets().store(name_, current()); changed(); };
        clearBtn_.onClick  = [this] { if (const auto r = current(); r.valid()) confirmClear(r); };

        for (auto* b : {&genieBtn_, &randomBtn_, &evolveBtn_}) addAndMakeVisible(*b);
        genieBtn_.setTooltip(tr("presets.describe-the-sound-you-want",
           "Describe the sound you want; the model sets this device's parameters"
           " (one undo step)"));
        randomBtn_.setTooltip(tr("presets.randomize-every-parameter-within-its", "Randomize every parameter within its range (one undo step)"));
        evolveBtn_.setTooltip(juce::String(tr("presets.mutate-the-current-settings-slightly", "Mutate the current settings slightly - repeat to wander (one undo step)")));
        genieBtn_.onClick  = [this] { openGenie(); };
        randomBtn_.onClick = [this] { randomize(false); };
        evolveBtn_.onClick = [this] { randomize(true); };

        addAndMakeVisible(viewport_);
        viewport_.setViewedComponent(&slots_, false);
        viewport_.setScrollBarsShown(true, false);

        rebuild();
        setSize(268, 273);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(6);
        auto rowA = r.removeFromTop(24);
        addBtn_.setBounds(rowA.removeFromLeft(46));
        rowA.removeFromLeft(4);
        prevBtn_.setBounds(rowA.removeFromLeft(24));
        rowA.removeFromLeft(3);
        nextBtn_.setBounds(rowA.removeFromRight(24));
        rowA.removeFromRight(3);
        curName_.setBounds(rowA);
        r.removeFromTop(5);
        auto rowB = r.removeFromTop(24);
        const int bw = (rowB.getWidth() - 8) / 3;
        recallBtn_.setBounds(rowB.removeFromLeft(bw));
        rowB.removeFromLeft(4);
        storeBtn_.setBounds(rowB.removeFromLeft(bw));
        rowB.removeFromLeft(4);
        clearBtn_.setBounds(rowB);
        r.removeFromTop(5);
        auto rowC = r.removeFromTop(24);
        genieBtn_.setBounds(rowC.removeFromLeft(rowC.getWidth() - 2 * (bw + 4)));
        rowC.removeFromLeft(4);
        randomBtn_.setBounds(rowC.removeFromLeft(bw));
        rowC.removeFromLeft(4);
        evolveBtn_.setBounds(rowC);
        r.removeFromTop(6);
        viewport_.setBounds(r);
        layoutSlots();
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::panel); }

private:
    presets::Ref current() const { return host_.presets().current(name_); }
    std::vector<presets::Entry> stack() const {
        return presets::stack(host_.model().byName(name_));
    }

    void layoutSlots() {
        const int rowH = 24;
        const int w = juce::jmax(10, viewport_.getMaximumVisibleWidth());
        int y = 0;
        for (auto& row : rows_) { row->setBounds(0, y, w, rowH); y += rowH; }
        slots_.setSize(w, juce::jmax(viewport_.getMaximumVisibleHeight(), y));
    }

    void rebuild() {
        rows_.clear();
        slots_.removeAllChildren();
        {
            const auto all = stack();
            const auto cur = current();
            const auto* e = presets::find(all, cur);
            curName_.setText(e ? juce::String(e->name) : juce::String(), juce::dontSendNotification);

            const bool canPaste = host_.presets().canPaste(name_);
            for (size_t i = 0; i < all.size(); ++i) {
                const auto ref = all[i].ref;
                const bool mine = ref.source == presets::Source::Patch;
                const juce::String label = all[i].group.empty()
                    ? juce::String(all[i].name)
                    : juce::String(all[i].group) + " / " + juce::String(all[i].name);
                auto row = std::make_unique<PresetRow>((int) i + 1, label,
                                                       ref == cur, canPaste && mine);
                row->onRecall = [this, ref] { recallBracketed([this, ref] { host_.presets().recall(name_, ref); }); };
                row->onStore  = [this, ref] { host_.presets().store(name_, ref); changed(); };
                row->onClear  = [this, ref] { confirmClear(ref); };
                row->onRename = [this, ref](const juce::String& s) { host_.presets().rename(name_, ref, s.toStdString()); changed(); };
                row->onCut    = [this, ref] { host_.presets().cut(name_, ref); changed(); };
                row->onCopy   = [this, ref] { host_.presets().copy(name_, ref); changed(); };
                row->onPaste  = [this] { host_.presets().paste(name_); changed(); };
                slots_.addAndMakeVisible(*row);
                rows_.push_back(std::move(row));
            }
        }
        layoutSlots();
    }

    void confirmClear(const presets::Ref& ref) {
        const juce::String nm = juce::String(ref.name);
        const juce::String what = nm.isEmpty()
            ? juce::String("this preset")
            : juce::String::fromUTF8("\xe2\x80\x9c") + nm
                + juce::String::fromUTF8("\xe2\x80\x9d");
        if (ref.source == presets::Source::Shipped) return;
        const bool mine = ref.source == presets::Source::Patch;
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::QuestionIcon,
            "Delete preset",
            mine ? "Delete " + what + " from this patch?"
                 : "Delete " + what + " from your user presets?",
            "Delete", "Cancel", this,
            juce::ModalCallbackFunction::create(
                [&host = host_, name = name_, ref,
                 sp = juce::Component::SafePointer<PresetsView>(this)](int r) {
                    if (r != 1) return;
                    host.presets().clear(name, ref);
                    if (sp) sp->changed();
                }));
    }

    void changed() {
        if (onChanged_) onChanged_();
        juce::Component::SafePointer<PresetsView> sp(this);
        juce::MessageManager::callAsync([sp] { if (sp) sp->rebuild(); });
    }

    const std::vector<ParamDesc>& schema() const { return presets::schemaOf(host_, name_); }

    void recallBracketed(const std::function<void()>& op) {
        presets::recallBracketed(host_, name_, op);
        changed();
    }

    void applyValues(const std::vector<std::pair<std::string, double>>& vals) {
        if (vals.empty()) return;
        host_.pushUndo();
        for (const auto& [param, value] : vals) host_.setParam(name_, param, value);
        changed();
    }

    void randomize(bool evolve) {
        if (evolve) { presets::evolve(host_, name_); changed(); return; }
        if (nodeSupportsRandom(host_, name_)) {
            auto& hist = host_.paramHistory();
            hist.commit(name_, host_.captureNodeState(name_));
            host_.rollNode(name_);
            hist.commit(name_, host_.captureNodeState(name_));
            changed();
            return;
        }
        const bool curated = hasRandomParams(schema());
        auto& r = juce::Random::getSystemRandom();
        std::vector<std::pair<std::string, double>> vals;
        for (const auto& d : schema()) {
            if (d.isText || d.isRange) continue;
            if (curated && !d.randomize) continue;
            double v = d.min + r.nextDouble() * (d.max - d.min);
            v = juce::jlimit(d.min, d.max, v);
            if (d.isBool || d.isEnum || d.isInt) v = std::round(v);
            vals.emplace_back(d.name, v);
        }
        applyValues(vals);
    }

    void openGenie() {
        presets::showGenie(host_, name_,
            [sp = juce::Component::SafePointer<PresetsView>(this)] { if (sp) sp->changed(); });
    }

    PropertiesHost& host_;
    std::string name_;
    std::function<void()> onChanged_;

    juce::TextButton addBtn_{"+ Add"}, prevBtn_{"<"}, nextBtn_{">"},
                     recallBtn_{"Recall"}, storeBtn_{"Store"}, clearBtn_{"Clear"},
                     genieBtn_{"Generate..."}, randomBtn_{"Random"}, evolveBtn_{"Evolve"};
    juce::Label curName_;
    juce::Viewport viewport_;
    juce::Component slots_;
    std::vector<std::unique_ptr<PresetRow>> rows_;
};

}
