// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"

#include <string>
#include <vector>

#include "core/app/AppPaths.h"
#include "core/browser/ProjectFacts.h"
#include "core/project/ProjectFolder.h"
#include "gui/app/FreeWindow.h"
#include "gui/app/RecentFiles.h"
#include "gui/browser/Audition.h"
#include "gui/browser/FileBrowserView.h"
#include "gui/browser/Loaders.h"
#include "gui/browser/SoundProbe.h"
#include "gui/common/Localisation.h"
#include "gui/editor/juce/JuceFilePicker.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"

namespace hum {

browser::BrowserIndex& MainComponent::browserIndex() {
    if (!browserIndex_) {
        browser::IndexConfig config;
        const auto indexFile = userLibraryRoot().getChildFile(".index.json");
        if (const auto former = userLibraryRoot().getChildFile("Library Index.json"); former.existsAsFile() && !indexFile.exists())
            former.moveFileTo(indexFile);
        config.indexFile = pathOf(indexFile);
        config.places = [] {
            return std::vector<std::string>{pathOf(userContentRoot()), pathOf(userPatchesDir()),
                                            pathOf(userRecordingsDir()), pathOf(userPresetsDir())};
        };
        config.busy = [this] { return host_.isPlaying(); };
        config.soundFacts = [](const std::string& path) { return browser::probeSound(path); };
        browserIndex_ = std::make_unique<browser::BrowserIndex>(std::move(config));
        std::vector<std::string> recent;
        for (const auto& p : recents::get()) recent.push_back(p.toStdString());
        auto& index = *browserIndex_;
        index.edit([&](browser::FileIndex& i) { i.importRecents(recent, index.now()); });
    }
    return *browserIndex_;
}

browser::Audition& MainComponent::audition() {
    if (!audition_) {
        audition_ = std::make_unique<browser::Audition>(host_.preview(), [this] { return host_.tempo(); });
    }
    return *audition_;
}

void MainComponent::noteOpened(const juce::File& f) {
    recents::push(f);
    const auto root = project::rootOf(pathOf(f));
    const auto used = root.empty() ? pathOf(f) : root;
    auto& index = browserIndex();
    index.edit([&](browser::FileIndex& i) {
        i.noteUsed(used, index.now());
        if (!root.empty()) i.observe(root, 0, 0, index.now(), browser::Kind::Project);
    });
}

browser::PlaceRoots MainComponent::browserRoots() const {
    browser::PlaceRoots roots{pathOf(userContentRoot()), pathOf(userPatchesDir()), pathOf(userRecordingsDir()),
                              pathOf(userPresetsDir()), {}, pathOf(juce::File::getSpecialLocation(juce::File::userHomeDirectory))};
    auto add = [&](const juce::String& label, const juce::File& dir) {
        if (dir.isDirectory()) roots.computer.push_back({label.toStdString(), pathOf(dir)});
    };
    add(tr("browser.home", "Home"), juce::File::getSpecialLocation(juce::File::userHomeDirectory));
    add(tr("browser.desktop", "Desktop"), juce::File::getSpecialLocation(juce::File::userDesktopDirectory));
    add(tr("browser.documents", "Documents"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));
    add(tr("browser.downloads", "Downloads"), juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads"));
    juce::Array<juce::File> drives;
    juce::File::findFileSystemRoots(drives);
    for (const auto& d : drives) {
        if (d.isSymbolicLink() || d.getFullPathName() == "/") continue;
        const auto label = d.getVolumeLabel().isNotEmpty() ? d.getVolumeLabel() : d.getFileName();
        add(label.isNotEmpty() ? label : d.getFullPathName(), d);
    }
    return roots;
}

std::string MainComponent::browserLoadTarget(const std::string& path) {
    const auto node = canvas_ != nullptr ? canvas_->selected() : std::string();
    if (node.empty()) return {};
    const auto* cm = host_.model().byName(node);
    if (cm == nullptr) return {};
    for (const auto* cls : {&cm->displayClass, &cm->classRaw})
        if (browser::LoaderTable::shared().slotIn(*cls, path) != nullptr) return node;
    return {};
}

void MainComponent::openFromBrowser(const std::string& path) {
    if (const auto node = browserLoadTarget(path); !node.empty()) {
        browser::loadInto(host_, node, path);
        propsPane_->syncFromModel();
        return;
    }
    auto target = fileAt(path);
    if (target.isDirectory() && project::isProject(path)) {
        const auto patch = browser::patchOf(path);
        if (patch.empty() || patch == path) return;
        target = fileAt(patch);
    }
    if (browser::kindOfFile(pathOf(target)) != browser::Kind::Patch) {
        target.revealToUser();
        return;
    }
    confirmDiscardThenRun([this, target] { openFileAt(target); });
}

void MainComponent::pickWithBrowser(const browser::PickRequest& request,
                                    std::function<void(const std::vector<std::string>&)> done,
                                    std::function<void()> other) {
    const bool wasOpen = libraryWindow_ != nullptr;
    openLibrary();
    auto* view = libraryView_;
    if (view == nullptr) return;
    auto settled = std::make_shared<bool>(false);
    auto afterPick = [safe = juce::Component::SafePointer<MainComponent>(this), wasOpen, settled](std::function<void()> then) {
        if (*settled) return;
        *settled = true;
        juce::MessageManager::callAsync([safe, wasOpen, then = std::move(then)] {
            if (safe == nullptr) return;
            safe->endBrowserPick(!wasOpen);
            if (then) then();
        });
    };
    view->onPicked = [afterPick, done](const std::vector<std::string>& paths) {
        afterPick([done, paths] { if (done) done(paths); });
    };
    view->onCancelled = [afterPick] { afterPick({}); };
    view->onOther = [afterPick, other] { afterPick(other); };
    auto req = request;
    req.offerOther = static_cast<bool>(other);
    libraryWindow_->setName(juce::String::fromUTF8(req.title.c_str()));
    view->beginPick(req);
}

void MainComponent::endBrowserPick(bool closeWindow) {
    if (libraryView_ != nullptr) {
        libraryView_->endPick();
        libraryView_->onPicked = {};
        libraryView_->onCancelled = {};
        libraryView_->onOther = {};
    }
    if (libraryWindow_ == nullptr) return;
    libraryWindow_->setName(tr("main-windows.library", "Library"));
    if (closeWindow) {
        libraryView_ = nullptr;
        libraryWindow_.reset();
    }
}

void MainComponent::browseLibrary() {
    openLibrary();
    if (libraryView_ != nullptr && libraryView_->model().pick().active()) endBrowserPick(false);
}

void MainComponent::routeFilePicksToBrowser() {
    JuceFilePicker::browserRoute() = [this](const files::FilePick& request, files::Picked done, std::function<void()> other) {
        auto kinds = request.kind == "Impulses" ? std::vector<browser::Kind>{browser::Kind::Impulse, browser::Kind::Sound}
                                                : browser::kindsInPatterns(request.patterns);
        if (kinds.empty()) return false;
        browser::PickRequest req;
        req.title = request.title;
        req.kinds = std::move(kinds);
        req.current = request.current;
        req.patterns = request.patterns;
        pickWithBrowser(req, [done](const std::vector<std::string>& paths) { if (done) done(paths); }, std::move(other));
        return true;
    };
}

void MainComponent::openLibrary() {
    if (!libraryWindow_) {
        const auto roots = browserRoots();
        auto* view = new browser::FileBrowserView(browserIndex(), roots, &audition());
        view->onOpen = [this](const std::string& path) { openFromBrowser(path); };
        view->loadTarget = [this](const std::string& path) { return browserLoadTarget(path); };
        view->engineRunning = [this] { return host_.audioRunning() || host_.audioStarting(); };
        view->turnEngineOn = [this] { if (!host_.audioRunning() && !host_.audioStarting()) toggleAudio(); };
        view->onForgetRecent = [](const std::string& path) {
            recents::remove(fileAt(path));
            if (project::isProject(path)) recents::remove(fileAt(browser::patchOf(path)));
        };
        libraryView_ = view;
        libraryWindow_ = std::make_unique<FreeWindow>(tr("main-windows.library", "Library"), view);
        libraryWindow_->onClose = [this] {
            libraryView_ = nullptr;
            libraryWindow_.reset();
        };
        wireGlobalKeys(*libraryWindow_);
        browserIndex().startScanning();
    }
    libraryWindow_->setVisible(true);
    libraryWindow_->toFront(true);
}

}
