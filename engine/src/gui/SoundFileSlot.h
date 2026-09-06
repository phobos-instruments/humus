#pragma once
#include <memory>
#include <string>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/AppPaths.h"
#include "core/ClipDrag.h"
#include "core/Catalogue.h"
#include "core/UserLibrary.h"
#include "gui/EngineHost.h"
#include "gui/IconButton.h"
#include "gui/LookAndFeel.h"
#include "gui/UiTicker.h"
#include "gui/Localisation.h"

namespace hum {

class SoundFileSlot : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      public juce::DragAndDropTarget {
public:
    SoundFileSlot(EngineHost& host, std::string organism, std::string param,
                  std::string wildcard = {}, std::string title = {},
                  std::string kind = {})
        : host_(host), name_(std::move(organism)), param_(std::move(param)),
          wildcard_(std::move(wildcard)), title_(std::move(title)),
          kind_(std::move(kind)),
          chooseBtn_(IconButton::Glyph::Open,
                     title_.empty() ? juce::String(tr("sound-file-slot.select-a-sound-file", "Select a sound file..."))
                                    : juce::String(juce::CharPointer_UTF8(title_.c_str()))) {
        addAndMakeVisible(nameField_);

        chooseBtn_.onClick = [this] { choose(); };
        addAndMakeVisible(chooseBtn_);

        clearBtn_.setButtonText(juce::String::charToString(juce::juce_wchar(0x00d7)));
        clearBtn_.setTooltip(tr("sound-file-slot.clear-this-slot", "Clear this slot"));
        clearBtn_.onClick = [this] {
            host_.setParamText(name_, param_, "");
            refresh();
        };
        addChildComponent(clearBtn_);

        tickerId_ = UiTicker::instance().add([this] { nameField_.tick(); });
        nameField_.addMouseListener(this, false);
        refresh();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) return;
        const juce::String cur = stripScheme(host_.liveParamText(name_, param_));
        if (cur.isEmpty() || !juce::File(cur).existsAsFile()) return;
        const juce::File f(cur);
        if (f.isAChildOf(library::root())) return;
        juce::PopupMenu m;
        m.addItem(1, tr("sound-file-slot.add-to-library", "Add to Library"));
        m.showMenuAsync(juce::PopupMenu::Options(), [this, f](int r) {
            if (r != 1) return;
            const auto dir = kindDir();
            dir.createDirectory();
            auto target = dir.getChildFile(f.getFileName());
            if (target.existsAsFile() && target.getSize() != f.getSize())
                target = target.getNonexistentSibling();
            if (target.existsAsFile() || f.copyFileTo(target)) {
                host_.setParamText(name_, param_, target.getFullPathName().toStdString());
                refresh();
            }
        });
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging_ || e.mods.isPopupMenu() || e.getDistanceFromDragStart() < 10) return;
        const juce::File f(stripScheme(host_.liveParamText(name_, param_)));
        if (!f.existsAsFile()) return;
        dragging_ = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            {f.getFullPathName()}, false, this, [this] { dragging_ = false; });
    }

    ~SoundFileSlot() override { UiTicker::instance().remove(tickerId_); }

    juce::File kindDir() const {
        return userContentRoot().getChildFile(kind_.empty() ? "Samples" : kind_);
    }

    juce::File startDir() const {
        const auto mine = kindDir();
        if (mine.isDirectory()
            && !mine.findChildFiles(juce::File::findFilesAndDirectories, false,
                                    "*").isEmpty())
            return mine;
        const auto path = assetSearchPath(kind_.empty() ? "Samples" : kind_);
        for (const auto& d : path)
            if (d != mine && d.isDirectory()) return d;
        return mine;
    }

    void refresh() {
        juce::String s = stripScheme(host_.liveParamText(name_, param_));
        shown_ = s.isEmpty() ? juce::String("(no file)") : juce::File(s).getFileName();
        nameField_.setText(shown_);
        nameField_.setTooltip(s);
        clearBtn_.setVisible(s.isNotEmpty());
    }

    juce::String shownName() const { return shown_; }

    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        if (files.size() != 1) return false;
        const juce::File f(files[0]);
        const auto wc = acceptWildcard();
        for (auto& pat : juce::StringArray::fromTokens(wc, ";", {}))
            if (f.getFileName().matchesWildcard(pat.trim(), true)) return true;
        return false;
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        if (files.isEmpty()) return;
        host_.setParamText(name_, param_, juce::File(files[0]).getFullPathName().toStdString());
        refresh();
    }

    bool isInterestedInDragSource(const SourceDetails& d) override {
        return draggedClipFile(d).existsAsFile();
    }
    void itemDropped(const SourceDetails& d) override {
        const auto f = draggedClipFile(d);
        if (!f.existsAsFile()) return;
        host_.setParamText(name_, param_, f.getFullPathName().toStdString());
        refresh();
    }
    juce::File draggedClipFile(const SourceDetails& d) {
        std::string track;
        int clipId = 0;
        if (!clipdrag::parseVideoClip(d.description.toString().toStdString(), track, clipId))
            return {};
        const auto r = host_.clips().rangeOf(track, clipId);
        if (r.file.empty()) return {};
        const juce::File f(juce::String(juce::CharPointer_UTF8(r.file.c_str())));
        return isInterestedInFileDrag({f.getFullPathName()}) ? f : juce::File();
    }

    void resized() override {
        auto r = getLocalBounds();
        clearBtn_.setBounds(r.removeFromRight(r.getHeight()));
        r.removeFromRight(2);
        chooseBtn_.setBounds(r.removeFromRight(r.getHeight()));
        r.removeFromRight(4);
        nameField_.setBounds(r);
    }

private:

    class NameField : public juce::Component, public juce::SettableTooltipClient {
    public:
        void setText(const juce::String& t) {
            if (t == text_) return;
            text_ = t;
            offset_ = 0.0f;
            repaint();
        }

        void tick() {
            if (!overflow_ || isMouseOver() || !isShowing()) return;
            offset_ += 0.6f;
            const float wrap = textW_ + kGap;
            if (offset_ >= wrap) offset_ -= wrap;
            repaint();
        }

        void paint(juce::Graphics& g) override {
            g.fillAll(Palette::panel.darker(0.3f));
            const juce::Font f(juce::FontOptions(11.0f));
            textW_ = juce::GlyphArrangement::getStringWidth(f, text_);
            const auto area = getLocalBounds().reduced(kPad, 0);
            overflow_ = textW_ > (float) area.getWidth();
            g.setColour(Palette::text);
            g.setFont(f);
            if (!overflow_) {
                offset_ = 0.0f;
                g.drawText(text_, area, juce::Justification::centredLeft, false);
                return;
            }
            g.reduceClipRegion(area);
            const float y = (float) getHeight();
            const float x0 = (float) area.getX() - offset_;
            g.drawSingleLineText(text_, (int) x0, (int) (y * 0.5f + f.getAscent() * 0.5f) - 1);
            g.drawSingleLineText(text_, (int) (x0 + textW_ + kGap),
                                 (int) (y * 0.5f + f.getAscent() * 0.5f) - 1);
        }

        void mouseEnter(const juce::MouseEvent&) override { repaint(); }

    private:
        static constexpr int kPad = 4;
        static constexpr float kGap = 28.0f;
        juce::String text_;
        float textW_ = 0.0f, offset_ = 0.0f;
        bool overflow_ = false;
    };

    static juce::String stripScheme(const std::string& uri) {
        juce::String s(juce::CharPointer_UTF8(uri.c_str()));
        return s.startsWith("file://") ? s.substring(7) : s;
    }

    void choose() {
        juce::String cur = stripScheme(host_.liveParamText(name_, param_));
        juce::File start = cur.isNotEmpty()
                               ? juce::File(cur).getParentDirectory()
                               : juce::File::getSpecialLocation(juce::File::userMusicDirectory);
        if (cur.isEmpty())
            if (const auto d = startDir(); d.isDirectory()) start = d;
        juce::String wc(juce::CharPointer_UTF8(wildcard_.c_str()));
        if (wc.isEmpty() && !kind_.empty())
            if (const auto* k = catalogue::find(kind_))
                wc = juce::String(juce::CharPointer_UTF8(k->wildcard.c_str()));
        if (wc.isEmpty()) {

            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            wc = fm.getWildcardForAllFormats();
        }
        chooser_ = std::make_unique<juce::FileChooser>(
            title_.empty() ? juce::String(tr("sound-file-slot.select-a-sound-file-2", "Select a sound file"))
                           : juce::String(juce::CharPointer_UTF8(title_.c_str())),
            start, wc);
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        if (wc.contains("synScene"))
            flags |= juce::FileBrowserComponent::canSelectDirectories;
        chooser_->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            host_.setParamText(name_, param_, f.getFullPathName().toStdString());
            refresh();
        });
    }

    juce::String acceptWildcard() const {
        if (!wildcard_.empty()) return juce::String(juce::CharPointer_UTF8(wildcard_.c_str()));
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        return fm.getWildcardForAllFormats();
    }

    std::string displayClass() const {
        const auto* cm = host_.model().byName(name_);
        return cm != nullptr ? cm->displayClass : std::string();
    }

    EngineHost& host_;
    std::string name_, param_;
    std::string wildcard_, title_, kind_;
    NameField nameField_;
    juce::String shown_;
    bool dragging_ = false;
    IconButton chooseBtn_;
    juce::TextButton clearBtn_;
    std::unique_ptr<juce::FileChooser> chooser_;
    int tickerId_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFileSlot)
};

}
