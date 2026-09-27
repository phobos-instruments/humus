// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserModel.h"
#include "gui/browser/AuditionStrip.h"
#include "gui/browser/BatchBar.h"
#include "gui/browser/BrowserInspector.h"
#include "core/browser/BrowserIndex.h"
#include "gui/browser/Breadcrumbs.h"
#include "gui/browser/BrowserSearchField.h"
#include "gui/browser/BrowserSidebar.h"
#include "gui/browser/BrowserTable.h"
#include "gui/browser/LoadingVeil.h"
#include "gui/browser/ProjectCards.h"
#include "gui/style/IconButton.h"

namespace hum::browser {

class FileBrowserView : public juce::Component, public juce::FileDragAndDropTarget {
public:
    std::function<void(const std::vector<std::string>&)> onPicked;
    std::function<void()> onCancelled;
    std::function<void()> onOther;
    std::function<void(const std::string&)> onOpen;
    std::function<std::string(const std::string& path)> loadTarget;
    std::function<void(const std::string& path)> onForgetRecent;
    std::function<bool()> engineRunning;
    std::function<void()> turnEngineOn;
    AuditionStrip* stripForTest() { return strip_.get(); }

    static constexpr int kSidebarW = 190, kSearchH = 34, kFooterH = 40, kStatusH = 26, kGap = 10;
    static constexpr int kIconW = 30, kSearchMaxW = 520;
    static constexpr int kDefaultW = 1000, kDefaultH = 620;
    static constexpr double kRefreshGapMs = 300.0;
    static constexpr double kFlashMs = 4000.0;

    FileBrowserView(BrowserIndex& index, PlaceRoots roots, Audition* audition = nullptr);
    ~FileBrowserView() override;

    void beginPick(const PickRequest& request);
    void endPick();
    void showPlace(const Place& place);
    void refreshNow();

    BrowserModel& model() { return model_; }
    void confirmPick();

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void confirmRemove(const std::vector<std::string>& paths);
    void setInspectorOpen(bool open);
    void refreshFromDisk();
    bool forgetIfGone(const std::string& path);
    enum ProjectsView { kCards = 0, kSmall = 1, kList = 2 };
    void setProjectsView(int view);
    int projectsView() const { return projectsView_; }
    void restyle();
    void lookAndFeelChanged() override { restyle(); }
    bool inspectorOpen() const { return inspectorOpen_; }
    bool showsLoading() const { return veil_.isVisible(); }
    BrowserInspector& inspector() { return inspector_; }
    BatchBar& batchBar() { return batch_; }

private:
    void poll();
    void wirePanels();
    void syncPanels();
    void tagSelection();
    void collectSelection();
    void selectionMoved();
    const Entry* focusedSound() const;
    bool stripShown() const;
    void open(int row);
    void rateSelection(int stars);
    void favouriteSelection();
    void showRowMenu(int row, juce::Point<int> at);
    void showPlaceMenu(const Place& place, juce::Point<int> at);
    void addWatchedFolder();
    void moveLibrary();
    void newCollection(const std::vector<std::string>& adding = {});
    void dragOut(const juce::StringArray& paths);
    juce::String statusText() const;
    bool pickable() const;
    juce::String stripHint(const Entry& e) const;

    BrowserIndex& index_;
    BrowserModel model_;
    BrowserSidebar sidebar_;
    BrowserSearchField search_;
    BrowserTable table_{model_};
    ProjectCards cards_{model_};
    juce::Viewport cardsView_;
    bool showsCards() const;
    bool placeLoading() const;
    LoadingVeil veil_;
    Audition* audition_ = nullptr;
    BrowserInspector inspector_;
    BatchBar batch_;
    juce::TextButton info_;
    IconButton viewCards_{IconGlyph::ViewCards, {}}, viewSmall_{IconGlyph::ViewSmall, {}};
    IconButton viewList_{IconGlyph::ViewList, {}}, refresh_{IconGlyph::Refresh, {}};
    juce::String flash_;
    double flashUntil_ = 0.0;
    int projectsView_ = 0;
    Breadcrumbs crumbs_;
    bool inspectorOpen_ = false;
    std::unique_ptr<AuditionStrip> strip_;
    bool autoplay_ = true;
    juce::Label title_;
    juce::TextButton open_, cancel_, other_;
    std::unique_ptr<juce::FileChooser> chooser_;
    int tickerId_ = 0;
    unsigned seen_ = 0;
    double lastRefresh_ = 0.0;
    bool dragging_ = false;
};

}
