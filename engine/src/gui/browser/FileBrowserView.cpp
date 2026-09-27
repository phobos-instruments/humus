// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/FileBrowserView.h"

#include <cmath>
#include <utility>

#include "core/browser/PlaceMemory.h"
#include "gui/app/AppSettings.h"
#include "gui/common/UiTicker.h"

namespace hum::browser {

namespace {

constexpr const char* kColumnsKey = "browser.columns";

}

FileBrowserView::FileBrowserView(BrowserIndex& index, PlaceRoots roots, Audition* audition)
    : index_(index), audition_(audition) {
    if (audition_ != nullptr) {
        strip_ = std::make_unique<AuditionStrip>(*audition_);
        strip_->engineRunning = [this] { return !engineRunning || engineRunning(); };
        strip_->turnEngineOn = [this] { if (turnEngineOn) turnEngineOn(); };
        addChildComponent(*strip_);
    }
    model_.setRoots(std::move(roots));
    addAndMakeVisible(sidebar_);
    addAndMakeVisible(search_);
    addAndMakeVisible(table_);
    addChildComponent(title_);
    addChildComponent(open_);
    addChildComponent(cancel_);
    addChildComponent(other_);
    other_.setComponentID("browse-other");
    other_.setButtonText(paint::systemDialogLabel());
    other_.setTooltip(tr("browser.other-tip", "Choose with the system's own file dialog instead"));
    other_.onClick = [this] { if (onOther) onOther(); };
    title_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    open_.setButtonText(tr("browser.open", "Open"));
    cancel_.setButtonText(tr("browser.cancel", "Cancel"));
    open_.onClick = [this] { confirmPick(); };
    cancel_.onClick = [this] { if (onCancelled) onCancelled(); };

    sidebar_.onChoose = [this](const Place& p) { showPlace(p); };
    sidebar_.onAddFolder = [this] { addWatchedFolder(); };
    sidebar_.onNewCollection = [this] { newCollection(); };
    sidebar_.onMenu = [this](const Place& p, juce::Point<int> at) { showPlaceMenu(p, at); };
    search_.onChange = [this](const std::string& text) {
        model_.setSearch(text);
        refreshNow();
    };
    search_.onDown = [this] { table_.focusList(); };
    table_.onOpen = [this](int row) { open(row); };
    table_.onSelection = [this] { selectionMoved(); };
    table_.onRate = [this](const std::string& path, int stars) {
        index_.edit([&](FileIndex& i) { i.setRating(path, stars, index_.now()); });
        refreshNow();
    };
    table_.onFavourite = [this](const std::string& path, bool on) {
        index_.edit([&](FileIndex& i) { i.setFavourite(path, on, index_.now()); });
        refreshNow();
    };
    table_.onSort = [this](Column c, bool up) {
        model_.sortBy(c, up);
        table_.refresh();
    };
    table_.onMenu = [this](int row, juce::Point<int> at) { showRowMenu(row, at); };
    table_.onDragOut = [this](const juce::StringArray& paths) { dragOut(paths); };
    table_.restoreColumns(AppSettings::instance().getString(kColumnsKey, "").toStdString());

    wirePanels();
    tickerId_ = UiTicker::instance().add([this] { poll(); });
    setWantsKeyboardFocus(true);
    setSize(kDefaultW, kDefaultH);
    refreshNow();
}

FileBrowserView::~FileBrowserView() {
    UiTicker::instance().remove(tickerId_);
    if (audition_ != nullptr) audition_->stop();
    AppSettings::instance().set(kColumnsKey, juce::String(table_.columnState()));
    index_.saveNow();
}

void FileBrowserView::beginPick(const PickRequest& request) {
    auto req = request;
    if (!req.remember.empty() && !req.resume)
        req.resume = decodePlace(AppSettings::instance().getString(juce::String(req.remember)).toStdString());
    index_.read([&](const FileIndex& i) {
        model_.beginPick(req, i);
        return 0;
    });
    title_.setText(juce::String::fromUTF8(request.title.c_str()), juce::dontSendNotification);
    std::vector<std::string> locked;
    for (const auto k : request.kinds) locked.push_back(std::string("kind:") + kindWord(k));
    search_.setLocked(locked);
    for (auto* c : std::initializer_list<juce::Component*>{&title_, &open_, &cancel_}) c->setVisible(true);
    other_.setVisible(request.offerOther);
    resized();
    refreshNow();
    table_.scrollToSelection();
}

void FileBrowserView::endPick() {
    model_.endPick();
    search_.setLocked({});
    for (auto* c : std::initializer_list<juce::Component*>{&title_, &open_, &cancel_, &other_}) c->setVisible(false);
    resized();
    refreshNow();
}

void FileBrowserView::showPlace(const Place& place) {
    model_.setPlace(place);
    if (const auto& key = model_.pick().remember; model_.pick().active() && !key.empty())
        AppSettings::instance().set(juce::String(key), juce::String(encodePlace(place)));
    refreshNow();
    resized();
}

void FileBrowserView::refreshNow() {
    seen_ = index_.version();
    lastRefresh_ = juce::Time::getMillisecondCounterHiRes();
    const auto items = index_.read([&](const FileIndex& i) {
        model_.refresh(i);
        return model_.sidebar(i);
    });
    sidebar_.setItems(items, model_.place(), index_.scanningPlace());
    table_.refresh();
    const bool inFolder = model_.place().type == Place::Type::Folder;
    crumbs_.setVisible(inFolder);
    if (inFolder) crumbs_.setTrail(crumbsFor(model_.place().path, model_.roots()), statusText());
    const bool loading = model_.rows().empty() && placeLoading();
    veil_.setVisible(loading);
    table_.setVisible(!showsCards() && !loading);
    cardsView_.setVisible(showsCards() && !loading);
    cards_.setSize(cards_.getWidth(), std::max(cardsView_.getHeight(), cards_.heightFor(cards_.getWidth())));
    cards_.repaint();
    selectionMoved();
}

const Entry* FileBrowserView::focusedSound() const {
    const int row = model_.rowOf(model_.focused());
    if (row < 0 || model_.selection().size() != 1) return nullptr;
    const auto& e = model_.rows()[(size_t) row];
    return e.kind == Kind::Sound || e.kind == Kind::Impulse ? &e : nullptr;
}

juce::String FileBrowserView::stripHint(const Entry& e) const {
    if (model_.pick().active()) return tr("browser.hint-pick", "Double-click or press Return to choose it");
    if (loadTarget)
        if (const auto target = loadTarget(e.path); !target.empty())
            return tr("browser.hint-load-into", "Double-click loads into") + " " + juce::String::fromUTF8(target.c_str());
    return tr("browser.hint-browse", "Drag onto an organism or the timeline");
}

bool FileBrowserView::stripShown() const { return strip_ != nullptr && focusedSound() != nullptr; }

void FileBrowserView::selectionMoved() {
    open_.setEnabled(pickable());
    const auto* sound = focusedSound();
    if (strip_ != nullptr) {
        const bool was = strip_->isVisible();
        strip_->setVisible(sound != nullptr);
        if (sound != nullptr)
            strip_->show(*sound, stripHint(*sound));
        if (was != strip_->isVisible()) resized();
    }
    syncPanels();
    if (sound != nullptr && audition_ != nullptr && autoplay_ && audition_->current() != sound->path)
        audition_->play(sound->path, sound->facts.bpm);
    repaint();
}

void FileBrowserView::poll() {
    if (flash_.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() >= flashUntil_) {
        flash_.clear();
        repaint();
    }
    if (strip_ != nullptr && strip_->isVisible()) strip_->tick();
    if (veil_.isVisible()) {
        if (!placeLoading()) refreshNow();
        else veil_.repaint();
    }
    if (index_.version() == seen_) return;
    if (juce::Time::getMillisecondCounterHiRes() - lastRefresh_ < kRefreshGapMs) return;
    refreshNow();
}

void FileBrowserView::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    const auto status = getLocalBounds().withTrimmedLeft(kSidebarW).removeFromBottom(model_.pick().active() ? kFooterH : kStatusH);
    g.setColour(Palette::panel);
    g.fillRect(status);
    g.setColour(Palette::border.withAlpha(alpha::muted));
    g.drawHorizontalLine(status.getY(), (float) status.getX(), (float) status.getRight());
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(11.5f));
    if (!crumbs_.isVisible())
        g.drawText(statusText(), status.reduced(kGap, 0).withTrimmedRight(model_.pick().active() ? 330 : 0),
                   juce::Justification::centredLeft, true);
}

void FileBrowserView::resized() {
    auto r = getLocalBounds();
    sidebar_.setBounds(r.removeFromLeft(kSidebarW));
    const bool picking = model_.pick().active();
    auto footer = r.removeFromBottom(picking ? kFooterH : kStatusH);
    crumbs_.setBounds(footer.reduced(kGap, 0).withTrimmedRight(picking ? 330 : 0));
    if (picking) {
        auto buttons = footer.reduced(kGap, 7);
        open_.setBounds(buttons.removeFromRight(84));
        buttons.removeFromRight(8);
        cancel_.setBounds(buttons.removeFromRight(84));
        buttons.removeFromRight(8);
        if (other_.isVisible()) other_.setBounds(buttons.removeFromRight(120));
    }
    auto top = r.removeFromTop(kSearchH + 2 * kGap).reduced(kGap);
    info_.setBounds(top.removeFromRight(kSearchH));
    top.removeFromRight(6);
    refresh_.setBounds(top.removeFromRight(kIconW).reduced(0, 2));
    top.removeFromRight(6);
    const bool projects = model_.place().type == Place::Type::Projects;
    for (auto* b : {&viewList_, &viewSmall_, &viewCards_}) {
        b->setVisible(projects);
        if (projects) b->setBounds(top.removeFromRight(kIconW).reduced(0, 2));
    }
    if (projects) top.removeFromRight(8);
    if (inspectorOpen_) inspector_.setBounds(r.removeFromRight(BrowserInspector::kWidth));
    if (stripShown()) strip_->setBounds(r.removeFromBottom(AuditionStrip::kHeight));
    if (batch_.isVisible()) batch_.setBounds(r.removeFromBottom(BatchBar::kHeight));
    if (picking) {
        const int titleW = (int) std::ceil(juce::GlyphArrangement::getStringWidth(title_.getFont(), title_.getText())) + 2 * kGap;
        title_.setBounds(top.removeFromLeft(std::min(titleW, top.getWidth() / 2)));
    }
    search_.setBounds(top.removeFromLeft(std::min(kSearchMaxW, top.getWidth())));
    table_.setBounds(r);
    cardsView_.setBounds(r);
    veil_.setBounds(r);
    cards_.setSize(std::max(1, r.getWidth() - cardsView_.getScrollBarThickness()), std::max(r.getHeight(), cards_.heightFor(r.getWidth())));
}

juce::String FileBrowserView::statusText() const {
    if (flash_.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() < flashUntil_) return flash_;
    const int n = (int) model_.rows().size();
    const int picked = (int) model_.selection().size();
    juce::String s = juce::String(n) + " " + (n == 1 ? tr("browser.item", "item") : tr("browser.items", "items"));
    if (picked > 1) s += "   " + juce::String(picked) + " " + tr("browser.selected", "selected");
    const auto scanning = index_.scanningPlace();
    if (!scanning.empty()) s += "   " + tr("browser.indexing", "Indexing") + " " + juce::String::fromUTF8(fileName(scanning).c_str());
    return s;
}

}
