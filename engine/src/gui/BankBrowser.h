#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/BankLibrary.h"
#include "gui/AppSettings.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"

namespace hum {

class BankBrowser : public juce::Component {
public:
    struct Entry {
        std::string ref;
        std::string name;
        std::string kind;
        bool recent = false;
    };

    static juce::String recentKey(const std::string& className) {
        return juce::String("banks.recent.")
             + juce::String(juce::CharPointer_UTF8(className.c_str()));
    }
    static std::vector<Entry> recents(const std::string& className, const banks::Slot& slot) {
        std::vector<Entry> out;
        const auto raw = AppSettings::instance().getString(recentKey(className));
        for (const auto& line : juce::StringArray::fromLines(raw)) {
            const auto ref = line.trim();
            if (ref.isEmpty()) continue;
            const juce::File f(banks::resolve(ref.toStdString(), className));
            if (!f.existsAsFile()) continue;
            out.push_back({ref.toStdString(), f.getFileNameWithoutExtension().toStdString(),
                           banks::kindOf(f, slot), true});
        }
        return out;
    }
    static void remember(const std::string& ref, const std::string& className) {
        juce::StringArray keep;
        keep.add(juce::String(juce::CharPointer_UTF8(ref.c_str())));
        for (const auto& line : juce::StringArray::fromLines(
                 AppSettings::instance().getString(recentKey(className)))) {
            const auto t = line.trim();
            if (t.isNotEmpty() && !keep.contains(t)) keep.add(t);
        }
        while (keep.size() > kRecents) keep.remove(keep.size() - 1);
        AppSettings::instance().set(recentKey(className), keep.joinIntoString("\n"));
    }

    static std::vector<Entry> entries(const std::string& className,
                                      const banks::Slot& slot) {
        std::vector<Entry> out;
        for (const auto& e : banks::factoryEntries(slot))
            out.push_back({e.ref, e.name, e.kind});
        const auto first = out.size();
        for (const auto& f : banks::scan(slot, className)) {
            if (!banks::browsable(f, slot)) continue;
            out.push_back({banks::referenceFor(f, slot, className),
                           f.getFileNameWithoutExtension().toStdString(),
                           banks::kindOf(f, slot)});
        }
        std::sort(out.begin() + (long) first, out.end(),
                  [](const Entry& a, const Entry& b) {
                      return a.kind != b.kind ? a.kind < b.kind : a.name < b.name;
                  });
        return out;
    }

    static void show(juce::Rectangle<int> anchor, const std::string& className,
                     const banks::Slot& slot,
                     std::function<void(const std::string&)> onPick) {
        auto owned = std::make_unique<BankBrowser>(className, slot, std::move(onPick));
        auto* raw = owned.get();
        auto& box = juce::CallOutBox::launchAsynchronously(std::move(owned), anchor, nullptr);
        raw->box_ = &box;
    }

    BankBrowser(std::string className, banks::Slot slot,
                std::function<void(const std::string&)> onPick)
        : all_(entries(className, slot)), className_(std::move(className)),
          slot_(std::move(slot)), onPick_(std::move(onPick)) {
        {
            std::vector<Entry> head;
            for (const auto& r : recents(className_, slot_)) {
                bool listed = false;
                for (const auto& e : all_) listed |= e.ref == r.ref;
                if (!listed) head.push_back(r);
            }
            all_.insert(all_.begin(), head.begin(), head.end());
        }
        openBtn_.setButtonText(juce::String::fromUTF8("Open a file\xe2\x80\xa6"));
        openBtn_.onClick = [this] { chooseFile(); };
        addAndMakeVisible(openBtn_);
        search_.setTextToShowWhenEmpty("Search banks", Palette::textDim);
        search_.onTextChange = [this] { refilter(); };
        addAndMakeVisible(search_);
        list_.setModel(&model_);
        list_.setRowHeight(22);
        list_.setColour(juce::ListBox::backgroundColourId, Palette::panel);
        addAndMakeVisible(list_);
        refilter();
        setSize(320, 360);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(6);
        search_.setBounds(r.removeFromTop(24));
        r.removeFromTop(4);
        openBtn_.setBounds(r.removeFromBottom(24));
        r.removeFromBottom(4);
        list_.setBounds(r);
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::panel); }

private:
    void refilter() {
        const auto q = search_.getText().trim().toLowerCase();
        shown_.clear();
        for (const auto& e : all_)
            if (q.isEmpty()
                || juce::String(juce::CharPointer_UTF8(e.name.c_str())).toLowerCase().contains(q))
                shown_.push_back(e);
        list_.updateContent();
        list_.repaint();
    }

    void chooseFile() {
        const auto& kind = banks::kindFor(slot_);
        chooser_ = std::make_unique<juce::FileChooser>(
            "Open a bank", juce::File(),
            juce::String(juce::CharPointer_UTF8(
                (slot_.filter.empty() ? kind.wildcard : slot_.filter).c_str())));
        chooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this, safe = juce::Component::SafePointer<BankBrowser>(this)]
                              (const juce::FileChooser& fc) {
                                  if (safe == nullptr) return;
                                  const auto f = fc.getResult();
                                  if (f == juce::File{}) return;
                                  const auto ref = banks::referenceFor(f, slot_, className_);
                                  remember(ref, className_);
                                  auto cb = onPick_;
                                  if (box_ != nullptr) box_->dismiss();
                                  if (cb) cb(ref);
                              });
    }

    void pick(int row) {
        if (row < 0 || row >= (int) shown_.size()) return;
        auto cb = onPick_;
        const auto ref = shown_[(size_t) row].ref;
        remember(ref, className_);
        if (box_ != nullptr) box_->dismiss();
        if (cb) cb(ref);
    }

    struct Model : juce::ListBoxModel {
        explicit Model(BankBrowser& o) : owner(o) {}
        int getNumRows() override { return (int) owner.shown_.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override {
            if (row < 0 || row >= (int) owner.shown_.size()) return;
            const auto& e = owner.shown_[(size_t) row];
            if (sel) g.fillAll(Palette::accent.withAlpha(0.25f));
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.0f));
            g.drawText(e.kind, 6, 0, 40, h, juce::Justification::centredLeft, false);
            if (e.recent) {
                g.setColour(Palette::accent);
                g.fillEllipse((float) w - 12.0f, (float) h * 0.5f - 2.0f, 4.0f, 4.0f);
            }
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(juce::String::fromUTF8(e.name.c_str()), 50, 0, w - 56, h,
                       juce::Justification::centredLeft, true);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent&) override { owner.pick(row); }
        BankBrowser& owner;
    };

    static constexpr int kRecents = 8;

    std::vector<Entry> all_, shown_;
    std::string className_;
    banks::Slot slot_;
    std::function<void(const std::string&)> onPick_;
    juce::TextEditor search_;
    juce::ListBox list_;
    juce::TextButton openBtn_;
    Model model_{*this};
    juce::CallOutBox* box_ = nullptr;
    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankBrowser)
};

class BankArrow : public juce::Button {
public:
    explicit BankArrow(bool forward) : juce::Button({}), fwd_(forward) {
        setRepeatSpeed(400, 140);
    }
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

class BankFileSlot : public juce::Component {
public:
    BankFileSlot(EngineHost& host, std::string organism, std::string param,
                 banks::Slot slot = {})
        : host_(host), name_(std::move(organism)), param_(std::move(param)),
          slot_(std::move(slot)) {
        nameLabel_.setColour(juce::Label::backgroundColourId, Palette::panel.darker(0.3f));
        nameLabel_.setColour(juce::Label::textColourId, Palette::text);
        nameLabel_.setFont(juce::FontOptions(11.0f));
        nameLabel_.setJustificationType(juce::Justification::centredLeft);
        nameLabel_.setMinimumHorizontalScale(0.7f);
        addAndMakeVisible(nameLabel_);

        browseBtn_.setButtonText(juce::String::fromUTF8("Browse\xe2\x80\xa6"));
        browseBtn_.onClick = [this] { browse(); };
        addAndMakeVisible(browseBtn_);

        prevBtn_.setTooltip("Previous bank");
        nextBtn_.setTooltip("Next bank");
        prevBtn_.onClick = [this] { step(-1); };
        nextBtn_.onClick = [this] { step(+1); };
        addAndMakeVisible(prevBtn_);
        addAndMakeVisible(nextBtn_);

        clearBtn_.setButtonText(juce::String::charToString(juce::juce_wchar(0x00d7)));
        clearBtn_.setTooltip("Back to the factory bank");
        clearBtn_.onClick = [this] {
            auto* host = &host_;
            const auto node = name_, param = param_;
            juce::Component::SafePointer<BankFileSlot> safe(this);
            host->setParamText(node, param, "");
            if (safe != nullptr) safe->refresh();
        };
        addChildComponent(clearBtn_);
        refresh();
    }

    void refresh() {
        const auto ref = host_.liveParamText(name_, param_);
        nameLabel_.setText(juce::String::fromUTF8(displayFor(ref, slot_).c_str()),
                           juce::dontSendNotification);
        nameLabel_.setTooltip(juce::String::fromUTF8(ref.c_str()));
        const bool loaded = !ref.empty();
        if (loaded != clearBtn_.isVisible()) {
            clearBtn_.setVisible(loaded);
            resized();
        }
    }

    void resized() override {
        auto r = getLocalBounds();
        if (clearBtn_.isVisible()) {
            clearBtn_.setBounds(r.removeFromRight(r.getHeight()));
            r.removeFromRight(2);
        }
        browseBtn_.setBounds(r.removeFromRight(66));
        r.removeFromRight(3);
        nextBtn_.setBounds(r.removeFromRight(18));
        prevBtn_.setBounds(r.removeFromRight(18));
        r.removeFromRight(3);
        nameLabel_.setBounds(r);
    }

private:
    static std::string displayFor(const std::string& ref, const banks::Slot& slot) {
        const auto factory = banks::factoryEntries(slot);
        for (const auto& e : factory) if (ref == e.ref) return e.name;
        if (ref.empty()) return factory.empty() ? "(no bank)" : factory.front().name;
        auto leaf = ref;
        for (const char* p : {kAssetScheme, banks::kLegacyPrefix})
            if (leaf.rfind(p, 0) == 0) leaf = leaf.substr(std::string(p).size());
        return juce::File(juce::String(juce::CharPointer_UTF8(leaf.c_str())))
            .getFileNameWithoutExtension().toStdString();
    }

    void step(int dir) {
        const auto* cm = host_.model().byName(name_);
        const auto all = BankBrowser::entries(
            cm != nullptr ? cm->displayClass : std::string{}, slot_);
        if (all.empty()) return;
        const auto cur = host_.liveParamText(name_, param_);
        int at = 0;
        for (int i = 0; i < (int) all.size(); ++i)
            if (all[(size_t) i].ref == cur || (cur.empty() && i == 0)) { at = i; break; }
        const int next = (at + dir + (int) all.size()) % (int) all.size();
        auto* host = &host_;
        const auto node = name_, param = param_;
        juce::Component::SafePointer<BankFileSlot> safe(this);
        host->setParamText(node, param, all[(size_t) next].ref);
        if (safe != nullptr) safe->refresh();
    }

    void browse() {
        const auto* cm = host_.model().byName(name_);
        auto* host = &host_;
        const auto node = name_, param = param_;
        BankBrowser::show(
            browseBtn_.getScreenBounds(), cm != nullptr ? cm->displayClass : std::string{},
            slot_,
            [host, node, param, safe = juce::Component::SafePointer<BankFileSlot>(this)]
            (const std::string& ref) {
                host->setParamText(node, param, ref);
                if (safe != nullptr) safe->refresh();
            });
    }

    EngineHost& host_;
    std::string name_, param_;
    banks::Slot slot_;
    juce::Label nameLabel_;
    juce::TextButton browseBtn_, clearBtn_;
    BankArrow prevBtn_{false}, nextBtn_{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankFileSlot)
};

}
