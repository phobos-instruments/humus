// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "core/library/BankLibrary.h"
#include "gui/bricks/BankBrowser.h"
#include "gui/editor/files/BankSlotModels.h"
#include "gui/editor/juce/JuceFilePicker.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"
#include "io/PatchDocument.h"

namespace hum {

class BankArrow : public juce::Button {
public:
    explicit BankArrow(bool forward) : juce::Button({}), fwd_(forward) {}
    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const auto r = getLocalBounds().toFloat();
        const float w = 6.0f, h = 8.0f;
        const float x = r.getCentreX() - w * 0.5f, y = r.getCentreY() - h * 0.5f;
        juce::Path p;
        if (fwd_) p.addTriangle(x, y, x, y + h, x + w, y + h * 0.5f);
        else      p.addTriangle(x + w, y, x + w, y + h, x, y + h * 0.5f);
        g.setColour((down || over) ? Palette::text : Palette::textDim);
        g.fillPath(p);
    }

private:
    bool fwd_;
};

class JuceBankShelf : public files::BankShelf {
public:
    explicit JuceBankShelf(banks::Slot slot) : slot_(std::move(slot)) {}

    std::vector<files::BankRef> factory() const override {
        std::vector<files::BankRef> out;
        for (const auto& e : banks::factoryEntries(slot_)) out.push_back({e.ref, e.name});
        return out;
    }
    std::vector<files::BankRef> entries(const std::string& className) const override {
        return BankBrowser::entries(className, slot_);
    }
    std::string referenceFor(const std::string& path, const std::string& className) const override {
        return banks::referenceFor(juce::File(juce::String::fromUTF8(path.c_str())), slot_, className);
    }
    void remember(const std::string& ref, const std::string& className) override {
        BankBrowser::remember(ref, className);
    }
    std::string patterns() const override {
        return slot_.filter.empty() ? banks::kindFor(slot_).wildcard : slot_.filter;
    }
    std::vector<std::string> schemes() const override { return {kAssetScheme, banks::kLegacyPrefix}; }
    std::string kind() const override { return banks::kindFor(slot_).id; }
    Words emptyName() const override {
        return kind() == BankBrowser::kScalesKind ? files::kNoScale : files::kNoBank;
    }
    std::string keyFor(const std::string& ref, const std::string& className) const override {
        if (ref.empty()) return ref;
        for (const auto& e : banks::factoryEntries(slot_))
            if (e.ref == ref) return ref;
        return banks::resolve(ref, className);
    }
    bool available(const std::string& ref, const std::string& className) const override {
        if (ref.empty()) return true;
        for (const auto& e : banks::factoryEntries(slot_))
            if (e.ref == ref) return true;
        const auto path = banks::resolve(ref, className);
        return fileAt(path).exists();
    }

private:
    banks::Slot slot_;
};

class BankSlotName : public juce::Component, public juce::SettableTooltipClient {
public:
    std::function<void()> onClick;

    void setText(const juce::String& text) {
        if (text == text_) return;
        text_ = text;
        repaint();
    }
    void setMissing(bool missing) {
        if (missing == missing_) return;
        missing_ = missing;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel.darker(over_ ? 0.1f : 0.3f));
        g.setColour(missing_ ? Palette::warnAmber() : Palette::text);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(text_, getLocalBounds().reduced(4, 0), juce::Justification::centredLeft, true);
    }
    void mouseEnter(const juce::MouseEvent&) override {
        over_ = true;
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        repaint();
    }
    void mouseExit(const juce::MouseEvent&) override {
        over_ = false;
        repaint();
    }
    void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    juce::String text_;
    bool missing_ = false, over_ = false;
};

class BankFileSlot : public juce::Component {
public:
    BankFileSlot(BrickHost& host, std::string organism, std::string param,
                 banks::Slot slot = {}, std::pair<std::string, double> select = {})
        : host_(host), name_(organism), slot_(std::move(slot)), shelf_(slot_),
          bank_(host, shelf_, organism, std::move(param), std::move(select)) {
        nameLabel_.onClick = [this] { browse(); };
        addAndMakeVisible(nameLabel_);

        prevBtn_.setTooltip(tr("bank-browser.previous-bank", "Previous bank"));
        nextBtn_.setTooltip(tr("bank-browser.next-bank", "Next bank"));
        prevBtn_.onClick = [this] { step(-1); };
        nextBtn_.onClick = [this] { step(+1); };
        addAndMakeVisible(prevBtn_);
        addAndMakeVisible(nextBtn_);

        refresh();
    }

    void refresh() {
        const auto shown = bank_.shown();
        nameLabel_.setText(juce::String::fromUTF8(shown.text.c_str()));
        nameLabel_.setMissing(shown.missing);
        const auto tip = juce::String::fromUTF8(shown.tooltip.c_str());
        nameLabel_.setTooltip(tr("bank-browser.click-to-choose-a-bank", "Click to choose a bank")
                              + (tip.isEmpty() ? juce::String() : juce::String(" - ") + tip));
    }

    void resized() override {
        auto r = getLocalBounds();
        nextBtn_.setBounds(r.removeFromRight(18));
        prevBtn_.setBounds(r.removeFromRight(18));
        r.removeFromRight(3);
        nameLabel_.setBounds(r);
    }

    void chooseFileForTest() { chooseFile(); }
    void takeForTest(const std::string& ref) { bank_.take(ref); refresh(); }
    bool chooserReturnedForTest() const { return chooserReturned_; }
    int browsesForTest() const { return browses_; }

private:
    void step(int dir) {
        juce::Component::SafePointer<BankFileSlot> safe(this);
        if (bank_.step(dir) && safe != nullptr) safe->refresh();
    }

    void browse() {
        ++browses_;
        juce::Component::SafePointer<BankFileSlot> safe(this);
        const auto cls = bank_.className();
        BankBrowser::show(
            nameLabel_.getScreenBounds(), cls, slot_, bank_.ref(),
            [safe](const std::string& ref) {
                if (safe == nullptr) return;
                safe->bank_.take(ref);
                safe->refresh();
            },
            [safe] { if (safe != nullptr) safe->chooseFile(); },
            [safe, cls](const std::string& ref) {
                BankBrowser::forget(ref, cls);
                if (safe == nullptr || safe->bank_.ref() != ref) return;
                safe->bank_.clear();
                safe->refresh();
            });
    }

    void chooseFile() {
        juce::Logger::writeToLog("bank slot " + juce::String(name_) + "/" + juce::String(bank_.param())
                                 + ": opening the chooser");
        picker_.pick(bank_.request(), [this](const std::vector<std::string>& paths) {
            chooserReturned_ = true;
            juce::Logger::writeToLog("bank slot " + juce::String(name_) + "/" + juce::String(bank_.param())
                                     + ": chooser returned '"
                                     + juce::String::fromUTF8(paths.empty() ? "" : paths.front().c_str()) + "'");
            if (bank_.chosen(paths)) refresh();
        });
    }

    BrickHost& host_;
    std::string name_;
    banks::Slot slot_;
    JuceBankShelf shelf_;
    files::BankSlotModel bank_;
    BankSlotName nameLabel_;
    BankArrow prevBtn_{false}, nextBtn_{true};
    JuceFilePicker picker_;
    bool chooserReturned_ = false;
    int browses_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankFileSlot)
};

}
