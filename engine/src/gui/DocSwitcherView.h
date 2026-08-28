#pragma once
#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AppSettings.h"
#include "gui/LookAndFeel.h"
#include "io/DocumentSet.h"
#include "io/PatchFormat.h"

namespace hum {

class DocSwitcherView : public juce::Component,
                        public juce::FileDragAndDropTarget,
                        public juce::DragAndDropContainer,
                        public juce::DragAndDropTarget,
                        private juce::ListBoxModel,
                        private juce::Timer {
public:
    struct Actions {
        std::function<void(const juce::File&)> openDocument;
        std::function<juce::String()> currentPath;
    };

    explicit DocSwitcherView(Actions actions) : actions_(std::move(actions)) {
        auto initButton = [this](juce::TextButton& b, const juce::String& text, auto fn) {
            b.setButtonText(text);
            b.onClick = fn;
            addAndMakeVisible(b);
        };
        initButton(newBtn_, "New", [this] { newSet(); });
        initButton(openBtn_, juce::String::fromUTF8("Open\xe2\x80\xa6"), [this] { openSet(); });
        initButton(saveBtn_, "Save", [this] { saveSet(false); });
        initButton(saveAsBtn_, juce::String::fromUTF8("Save As\xe2\x80\xa6"),
                   [this] { saveSet(true); });
        initButton(insertBtn_, "+ Current", [this] { insertCurrent(); });
        initButton(removeBtn_, "Remove", [this] { removeSelected(); });

        list_.setModel(this);
        list_.setRowHeight(24);
        list_.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list_.setMultipleSelectionEnabled(false);
        addAndMakeVisible(list_);

        status_.setFont(juce::FontOptions(11.0f));
        status_.setColour(juce::Label::textColourId, Palette::textDim);
        addAndMakeVisible(status_);

        const auto last = AppSettings::instance().getString("docswitch.lastSet", "");
        if (last.isNotEmpty()) loadSetFile(juce::File(last), false);
        refreshStatus();
        startTimerHz(2);
        setSize(400, 460);
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        auto top = r.removeFromTop(24);
        const int bw = (top.getWidth() - 5 * 4) / 6;
        for (auto* b : {&newBtn_, &openBtn_, &saveBtn_, &saveAsBtn_, &insertBtn_, &removeBtn_}) {
            b->setBounds(top.removeFromLeft(bw));
            top.removeFromLeft(4);
        }
        r.removeFromTop(6);
        status_.setBounds(r.removeFromBottom(18));
        r.removeFromBottom(4);
        list_.setBounds(r);
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        for (const auto& f : files)
            if (f.endsWithIgnoreCase(".hum") || f.endsWithIgnoreCase(".amh")) return true;
        return false;
    }
    void filesDropped(const juce::StringArray& files, int, int y) override {
        int at = rowForY(y);
        for (const auto& f : files) {
            if ((int) slots_.size() >= kDocumentSetMaxSlots) break;
            if (!(f.endsWithIgnoreCase(".hum") || f.endsWithIgnoreCase(".amh"))) continue;
            slots_.insert(slots_.begin() + at++, f);
        }
        markDirty();
    }

    bool isInterestedInDragSource(const SourceDetails& s) override {
        return s.description.isInt();
    }
    void itemDropped(const SourceDetails& s) override {
        const int from = (int) s.description;
        const int listY = (int) (s.localPosition.y - list_.getY());
        int to = rowForY(list_.getY() + listY);
        if (from < 0 || from >= (int) slots_.size()) return;
        const auto moved = slots_[(size_t) from];
        slots_.erase(slots_.begin() + from);
        if (to > from) --to;
        to = juce::jlimit(0, (int) slots_.size(), to);
        slots_.insert(slots_.begin() + to, moved);
        list_.selectRow(to);
        markDirty();
    }

private:
    int getNumRows() override { return juce::jmax(1, (int) slots_.size()); }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
        if (slots_.empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(juce::String::fromUTF8(
                           "drop patches here, or \xe2\x80\x9c+ Current\xe2\x80\x9d"),
                       8, 0, w - 16, h, juce::Justification::centredLeft);
            return;
        }
        if (row < 0 || row >= (int) slots_.size()) return;
        const juce::File f(slots_[(size_t) row]);
        const bool missing = !f.existsAsFile();
        const bool current = actions_.currentPath
                          && actions_.currentPath() == f.getFullPathName();
        if (current) {
            g.setColour(Palette::accent.withAlpha(0.18f));
            g.fillRoundedRectangle(2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
        }
        if (selected) {
            g.setColour(Palette::accent.withAlpha(0.6f));
            g.drawRoundedRectangle(2.5f, 1.5f, (float) w - 5.0f, (float) h - 3.0f, 4.0f, 1.2f);
        }
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(Palette::textDim);
        g.drawText(juce::String(row + 1), 6, 0, 24, h, juce::Justification::centredRight);
        g.setFont(juce::FontOptions(13.0f));
        g.setColour(missing ? Palette::textDim : Palette::text);
        const auto name = (missing ? juce::String("? ") : juce::String())
                        + f.getFileName();
        g.drawText(name, 36, 0, w / 2 - 36, h, juce::Justification::centredLeft);
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(Palette::textDim);
        g.drawText(f.getParentDirectory().getFullPathName(), w / 2 + 4, 0, w / 2 - 12, h,
                   juce::Justification::centredLeft);
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override { openRow(row); }
    void returnKeyPressed(int row) override { openRow(row); }
    void deleteKeyPressed(int) override { removeSelected(); }
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rows) override {
        return rows.size() == 1 ? juce::var(rows[0]) : juce::var();
    }

    void openRow(int row) {
        if (row < 0 || row >= (int) slots_.size() || !actions_.openDocument) return;
        const juce::File f(slots_[(size_t) row]);
        if (!f.existsAsFile()) {
            status_.setText("missing: " + f.getFullPathName(), juce::dontSendNotification);
            return;
        }
        actions_.openDocument(f);
    }

    void insertCurrent() {
        const auto p = actions_.currentPath ? actions_.currentPath() : juce::String();
        if (p.isEmpty()) {
            status_.setText(juce::String::fromUTF8(
                                "save the patch first - the set stores file paths"),
                            juce::dontSendNotification);
            return;
        }
        if ((int) slots_.size() >= kDocumentSetMaxSlots) return;
        const int sel = list_.getSelectedRow();
        const int at = sel >= 0 ? sel + 1 : (int) slots_.size();
        slots_.insert(slots_.begin() + at, p);
        list_.selectRow(at);
        markDirty();
    }

    void removeSelected() {
        const int sel = list_.getSelectedRow();
        if (sel < 0 || sel >= (int) slots_.size()) return;
        slots_.erase(slots_.begin() + sel);
        markDirty();
    }

    void newSet() {
        slots_.clear();
        setFile_ = juce::File();
        dirty_ = false;
        AppSettings::instance().set("docswitch.lastSet", "");
        refresh();
    }

    void openSet() {
        chooser_ = std::make_unique<juce::FileChooser>("Open document set", juce::File(),
                                                       kDocumentSetFilter);
        chooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this](const juce::FileChooser& fc) {
                                  const auto f = fc.getResult();
                                  if (f != juce::File()) loadSetFile(f, true);
                              });
    }

    void saveSet(bool as) {
        if (!as && setFile_ != juce::File()) { writeSetFile(setFile_); return; }
        chooser_ = std::make_unique<juce::FileChooser>("Save document set", juce::File(),
                                                       kDocumentSetFilter);
        chooser_->launchAsync(juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this](const juce::FileChooser& fc) {
                                  auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  if (!f.hasFileExtension("hums"))
                                      f = f.withFileExtension(kDocumentSetExtension);
                                  writeSetFile(f);
                              });
    }

    void loadSetFile(const juce::File& f, bool announce) {
        const auto xml = f.loadFileAsString();
        auto parsed = parseDocumentSet(xml, f);
        if (parsed.empty() && !xml.contains("humus-document-set")) {
            if (announce)
                status_.setText("not a document set: " + f.getFileName(),
                                juce::dontSendNotification);
            return;
        }
        slots_ = std::move(parsed);
        setFile_ = f;
        dirty_ = false;
        AppSettings::instance().set("docswitch.lastSet", f.getFullPathName());
        refresh();
    }

    void writeSetFile(const juce::File& f) {
        if (!f.replaceWithText(serializeDocumentSet(slots_, f))) {
            status_.setText("could not write " + f.getFileName(), juce::dontSendNotification);
            return;
        }
        setFile_ = f;
        dirty_ = false;
        AppSettings::instance().set("docswitch.lastSet", f.getFullPathName());
        refresh();
    }

    int rowForY(int y) {
        const int row = list_.getRowContainingPosition(list_.getWidth() / 2, y - list_.getY());
        return row < 0 ? (int) slots_.size() : row;
    }
    void markDirty() {
        dirty_ = true;
        refresh();
    }
    void refresh() {
        list_.updateContent();
        list_.repaint();
        refreshStatus();
    }
    void refreshStatus() {
        const auto name = setFile_ == juce::File() ? juce::String("untitled set")
                                                   : setFile_.getFileName();
        status_.setText(name + (dirty_ ? "*" : "") + juce::String::fromUTF8("  \xc2\xb7  ")
                            + juce::String((int) slots_.size()) + " / "
                            + juce::String(kDocumentSetMaxSlots),
                        juce::dontSendNotification);
    }
    void timerCallback() override {
        juce::String sig = actions_.currentPath ? actions_.currentPath() : juce::String();
        for (const auto& s : slots_) sig << (juce::File(s).existsAsFile() ? "1" : "0");
        if (sig != lastSig_) {
            lastSig_ = sig;
            list_.repaint();
        }
    }

    Actions actions_;
    std::vector<juce::String> slots_;
    juce::File setFile_;
    bool dirty_ = false;
    juce::TextButton newBtn_, openBtn_, saveBtn_, saveAsBtn_, insertBtn_, removeBtn_;
    juce::ListBox list_;
    juce::Label status_;
    juce::String lastSig_;
    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocSwitcherView)
};

}
