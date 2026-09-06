#pragma once
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Catalogue.h"
#include "core/UserLibrary.h"
#include "gui/AppSettings.h"
#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

namespace hum {

class LibraryPane : public juce::Component, private juce::FileBrowserListener {
public:
    LibraryPane() : scanThread_("library scan") {
        scanThread_.startThread(juce::Thread::Priority::background);
        list_ = std::make_unique<juce::DirectoryContentsList>(nullptr, scanThread_);
        tree_ = std::make_unique<juce::FileTreeComponent>(*list_);
        tree_->setColour(juce::FileTreeComponent::backgroundColourId,
                         Palette::panel.darker(0.15f));
        tree_->addListener(this);
        tree_->addMouseListener(this, true);
        addAndMakeVisible(*tree_);

        pathLbl_.setColour(juce::Label::textColourId, Palette::textDim);
        pathLbl_.setFont(juce::FontOptions(11.0f));
        pathLbl_.setMinimumHorizontalScale(0.7f);
        addAndMakeVisible(pathLbl_);

        for (auto* b : {&changeBtn_, &revealBtn_}) {
            b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
            b->setColour(juce::TextButton::textColourOffId, Palette::text);
            addAndMakeVisible(*b);
        }
        changeBtn_.setButtonText(tr("library-pane.move", "Move..."));
        changeBtn_.setTooltip(tr("library-pane.choose-a-different-library-folder", "Choose a different library folder (a sample drive)"));
        changeBtn_.onClick = [this] { chooseRoot(); };
        revealBtn_.setButtonText(tr("library-pane.show", "Show"));
        revealBtn_.setTooltip(tr("library-pane.open-the-library-folder-itself", "Open the library folder itself"));
        revealBtn_.onClick = [] { library::root().revealToUser(); };

        adoptRoot();
        setSize(340, 460);
    }

    ~LibraryPane() override {
        tree_ = nullptr;
        list_ = nullptr;
        scanThread_.stopThread(2000);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        auto top = r.removeFromTop(24);
        revealBtn_.setBounds(top.removeFromRight(52));
        top.removeFromRight(4);
        changeBtn_.setBounds(top.removeFromRight(62));
        top.removeFromRight(6);
        pathLbl_.setBounds(top);
        r.removeFromTop(6);
        tree_->setBounds(r);
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging_ || e.getDistanceFromDragStart() < 10) return;
        const auto f = tree_->getSelectedFile();
        if (!f.existsAsFile()) return;
        dragging_ = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            {f.getFullPathName()}, false, this, [this] { dragging_ = false; });
    }

private:
    void adoptRoot() {
        const auto root = library::root();
        catalogue::plantUserFolders();
        pathLbl_.setText(root.getFullPathName(), juce::dontSendNotification);
        pathLbl_.setTooltip(root.getFullPathName());
        list_->setDirectory(root, true, true);
        tree_->refresh();
    }

    void chooseRoot() {
        chooser_ = std::make_unique<juce::FileChooser>(tr("library-pane.choose-the-library-folder", "Choose the library folder"),
                                                       library::root());
        chooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectDirectories,
                              [this](const juce::FileChooser& fc) {
                                  const auto dir = fc.getResult();
                                  if (dir == juce::File() || !dir.isDirectory()) return;
                                  const auto path = dir.getFullPathName();
                                  AppSettings::instance().set("library.path", path);
                                  library::configuredRoot() = path.toStdString();
                                  adoptRoot();
                              });
    }

    void selectionChanged() override {}
    void fileClicked(const juce::File&, const juce::MouseEvent&) override {}
    void fileDoubleClicked(const juce::File& f) override { f.revealToUser(); }
    void browserRootChanged(const juce::File&) override {}

    juce::TimeSliceThread scanThread_;
    std::unique_ptr<juce::DirectoryContentsList> list_;
    std::unique_ptr<juce::FileTreeComponent> tree_;
    juce::Label pathLbl_;
    juce::TextButton changeBtn_, revealBtn_;
    std::unique_ptr<juce::FileChooser> chooser_;
    bool dragging_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryPane)
};

}
