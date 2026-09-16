// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <cstdio>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "core/app/WindowGeometry.h"
#include "core/plugins/BridgeClient.h"
#include "core/plugins/BridgeWorker.h"
#include "core/plugins/HelperLaunch.h"
#include "gui/plugins/BridgeWorkerEditor.h"
#include "core/plugins/PluginHost.h"
#include "core/app/UiWatchdog.h"
#include "io/AutosaveStore.h"
#include "gui/assistant/AiClient.h"
#include "gui/app/AppSettings.h"
#include "gui/plugins/PluginQuarantine.h"
#include "gui/style/LookAndFeel.h"
#include "gui/app/AppCommandLine.h"
#include "gui/app/AppSignals.h"
#include "gui/app/MainComponent.h"
#include "gui/app/MainWindow.h"
#include "gui/app/DefaultPatchHandler.h"
#include "io/PatchFormat.h"
#include "gui/app/SplashWindow.h"
#include "gui/common/Localisation.h"
#include "gui/app/NativeMac.h"
#include "gui/app/StartWindow.h"
#include "gui/settings/ThemeStore.h"

namespace hum {

static void hideFromDock() {
#if JUCE_MAC
    juce::Process::setDockIconVisible(false);
#endif
}

static void hideEverything() {
    becomeWindowlessProcess();
}

class Application : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Humus"; }
    const juce::String getApplicationVersion() override {
        return JUCE_APPLICATION_VERSION_STRING;
    }

    void initialise(const juce::String&) override {
        appsignals::installCrashHandlers();
        {
            auto worker = std::make_unique<BridgeWorker>();
            if (worker->initialiseFromCommandLine(getCommandLineParameters(),
                                                  BridgeClient::kUid)) {
                hideFromDock();
                juce::LookAndFeel::setDefaultLookAndFeel(&laf_);
                bridgeWorker_ = std::move(worker);
                bridgeWorker_->onQuit = [] { juce::JUCEApplication::getInstance()->quit(); };
                bridgeWorkerEditor_ = std::make_unique<BridgeWorkerEditor>(*bridgeWorker_);
                return;
            }
        }

        logger_ = std::make_unique<juce::FileLogger>(
            appDataDir().getChildFile("logs").getChildFile("humus.log"),
            "Humus " + getApplicationVersion(), 512 * 1024);
        juce::Logger::setCurrentLogger(logger_.get());

        const auto args = getCommandLineParameterArray();

        i18n::applySaved();
        themestore::upgradeStoredThemes();

        for (const char* child : launch::helperFlags())
            if (args.contains(child)) { hideEverything(); break; }

        if (const auto cli = appcli::run(args); cli.handled) {
            setApplicationReturnValue(cli.returnValue);
            quit();
            return;
        }

        appsignals::installQuitHandlers(AutosaveStore().autosaveFile());

        disableAutomaticWindowTabbing();
        juce::LookAndFeel::setDefaultLookAndFeel(&laf_);
        migrateLegacyUserFolders(
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            appDataDir());

        if (const int oi = args.indexOf("--open"); oi >= 0 && oi + 1 < args.size()) {
            bootRouted_ = true;
            showMainWindow();
            if (auto* mc = mainComponent())
                mc->openFileAt(juce::File::getCurrentWorkingDirectory()
                                   .getChildFile(args[oi + 1]));
            return;
        }
        if (const auto f = firstExistingFile(getCommandLineParameters());
            f != juce::File()) {
            bootRouted_ = true;
            showMainWindow();
            if (auto* mc = mainComponent()) mc->openFileAt(f);
            return;
        }
        if (args.contains("--first-boot")) {
            AppSettings::instance().set("setup.completed", 0);
            AppSettings::instance().set("guide.seen", 0);
            forceFirstBoot_ = true;
        }
        splash_ = std::make_unique<SplashWindow>([this] {
            bootRouted_ = true;
            if (openPending()) return;
            if (isFirstBoot()) showMainWindowWithGuide();
            else openStartWindow();
        });
        raiseOnDesktop(*splash_);
        splash_->setCentrePosition(juce::Desktop::getInstance().getDisplays()
                                       .getPrimaryDisplay()->userArea.getCentre());
        splash_->setVisible(true);
        splash_->toFront(false);
    }

    static constexpr int kBootFadeMs = 140;

    static void raiseOnDesktop(juce::Component& c) {
        c.addToDesktop(juce::ComponentPeer::windowHasDropShadow
                       | juce::ComponentPeer::windowAppearsOnTaskbar);
#if JUCE_LINUX || JUCE_WINDOWS
        if (auto* peer = c.getPeer())
            peer->setIcon(juce::ImageCache::getFromMemory(
                BinaryData::icon1024_png, BinaryData::icon1024_pngSize));
#endif
    }

    void handOver(juce::Component* going) { bootFade_.fadeOut(going, kBootFadeMs); }

    void handIn(juce::Component* coming) { bootFade_.fadeIn(coming, kBootFadeMs); }

    template <class T>
    void retire(std::unique_ptr<T>& held) {
        if (auto* c = held.release()) {
            handOver(c);
            juce::MessageManager::callAsync([c] { delete c; });
        }
    }

    void openStartWindow() {
        retire(splash_);
        auto orphans = AutosaveStore().findOrphans();
        StartWindow::Actions a;
        a.onNewSession = [this] { showMainWindow(); };
        a.onOpenFile = [this](juce::File f) {
            showMainWindow();
            if (auto* mc = mainComponent()) mc->openFileAt(f);
        };
        a.onOpenOther = [this] {
            auto& s = AppSettings::instance();
            juce::File dir(s.getString("lastDir.open", ""));
            if (!dir.isDirectory()) dir = juce::File(s.getString("lastDir.save", ""));
            if (!dir.isDirectory()) dir = juce::File(s.getString("lastDir", ""));
            if (!dir.isDirectory())
                dir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
            startChooser_ = std::make_unique<juce::FileChooser>("Open patch", dir, kPatchOpenFilter);
            startChooser_->launchAsync(
                juce::FileBrowserComponent::openMode
                    | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& fc) {
                    const auto f = fc.getResult();
                    if (f == juce::File()) return;
                    showMainWindow();
                    if (auto* mc = mainComponent()) mc->openFileAt(f);
                });
        };
        a.onTour = [this] { showMainWindowWithGuide(); };
        a.onWizard = [this] {
            showMainWindow();
            if (auto* mc = mainComponent()) mc->openSetupWizard(false);
        };
        a.onHelp = [this] {
            showMainWindow();
            if (auto* mc = mainComponent()) mc->openHelpBrowser();
        };
        a.onRecover = [this](const AutosaveStore::Recovery& r, bool restore) {
            if (!restore) { AutosaveStore::discard(r); return; }
            showMainWindow();
            if (auto* mc = mainComponent()) mc->restoreAutosave(r);
        };
        start_ = std::make_unique<StartWindow>(std::move(a), std::move(orphans));
        raiseOnDesktop(*start_);
        start_->setCentrePosition(juce::Desktop::getInstance().getDisplays()
                                       .getPrimaryDisplay()->userArea.getCentre());
        handIn(start_.get());
        start_->toFront(false);
        offerQuarantineIfHungLastTime();
    }

    void offerQuarantineIfHungLastTime() {
        auto rep = UiWatchdog::consumeBreadcrumb(UiWatchdog::defaultBreadcrumb());
        if (!rep || rep->pluginClassRaw.isEmpty()) return;
        const juce::String display =
            rep->pluginClassRaw.upToFirstOccurrenceOf("?", false, false);
        auto* aw = new juce::AlertWindow(
            "Humus hung last time",
            display + "'s interface was active when Humus stopped responding.\n"
            "Quarantine its UI? (You can still open it explicitly.)",
            juce::MessageBoxIconType::WarningIcon);
        aw->addButton(tr("main.quarantine", "Quarantine"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton(tr("main.ignore", "Ignore"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        aw->enterModalState(true, juce::ModalCallbackFunction::create(
            [cls = rep->pluginClassRaw.toStdString()](int r) {
                if (r == 1) quarantine::add(cls);
            }), true);
    }

    MainComponent* mainComponent() {
        return window_ ? dynamic_cast<MainComponent*>(window_->getContentComponent()) : nullptr;
    }

    void showMainWindow(bool offerHandler = true) {
        retire(splash_);
        retire(start_);
        window_ = std::make_unique<MainWindow>();
        handIn(window_.get());
        if (auto* mc = mainComponent())
            mc->onCloseProject = [this] { closeToStartWindow(); };
        if (offerHandler) offerDefaultPatchHandlerOnce();
    }

    bool isFirstBoot() const {
        return forceFirstBoot_
            || wantsOnboarding(AppSettings::instance().getInt("guide.seen", 0) != 0,
                               AppSettings::instance().getInt("setup.completed", 0) != 0);
    }

    void showMainWindowWithGuide() {
        showMainWindow(false);
        if (auto* mc = mainComponent()) mc->openGuide(true);
    }

    void closeToStartWindow() {
        if (auto* w = window_.release()) {
            AppSettings::instance().set("window.state", w->getWindowStateAsString());
            handOver(w);
            juce::MessageManager::callAsync([w] { delete w; });
        }
        AutosaveStore().clear();
        openStartWindow();
    }
    void unhandledException(const std::exception* e, const juce::String& src,
                            int line) override {
        fprintf(stderr, "=== UNHANDLED EXCEPTION (session kept alive) ===\n  %s\n  caught at %s:%d\n",
                e != nullptr ? e->what() : "unknown exception type",
                src.toRawUTF8(), line);
        appsignals::printBacktrace();
        fprintf(stderr, "=== END EXCEPTION ===\n");
        fflush(stderr);
    }

    void shutdown() override {
        if (bridgeWorker_) { bridgeWorkerEditor_ = nullptr; bridgeWorker_ = nullptr; return; }
        if (window_) AppSettings::instance().set("window.state", window_->getWindowStateAsString());
        window_ = nullptr;
        start_ = nullptr;
        splash_ = nullptr;
        AutosaveStore().clear();
        UiWatchdog::defaultBreadcrumb().deleteFile();
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        juce::Logger::setCurrentLogger(nullptr);
        logger_ = nullptr;
    }

    static juce::File firstExistingFile(const juce::String& commandLine) {
        for (const auto& a : juce::StringArray::fromTokens(commandLine, true)) {
            if (a.startsWithChar('-')) continue;
            if (const juce::File f(a.unquoted()); f.existsAsFile()) return f;
        }
        return {};
    }

    bool moreThanOneInstanceAllowed() override {
        return launch::isHelper(getCommandLineParameterArray());
    }

    void anotherInstanceStarted(const juce::String& commandLine) override {
        if (const auto f = firstExistingFile(commandLine); f != juce::File()) {
            pendingOpen_ = f;
            if (bootRouted_) openPending();
        }
        comeForward();
    }

    void comeForward() {
        juce::Component* c = window_ != nullptr ? static_cast<juce::Component*>(window_.get())
                           : start_ != nullptr  ? static_cast<juce::Component*>(start_.get())
                                                : static_cast<juce::Component*>(splash_.get());
        if (c == nullptr) return;
        if (auto* w = dynamic_cast<juce::ResizableWindow*>(c); w != nullptr && w->isMinimised())
            w->setMinimised(false);
        c->setVisible(true);
        c->toFront(true);
    }

    bool openPending() {
        if (pendingOpen_ == juce::File()) return false;
        const auto f = pendingOpen_;
        pendingOpen_ = juce::File();
        if (mainComponent() == nullptr) showMainWindow();
        auto* mc = mainComponent();
        if (mc == nullptr) return false;
        mc->openFileAt(f);
        if (window_ != nullptr) window_->toFront(true);
        return true;
    }

    void systemRequestedQuit() override {
        if (window_) {
            if (auto* mc = dynamic_cast<MainComponent*>(window_->getContentComponent())) {
                mc->confirmDiscardThenRun([] { juce::JUCEApplication::getInstance()->quit(); });
                return;
            }
        }
        quit();
    }

private:
    HumLookAndFeel laf_;
    bool forceFirstBoot_ = false;
    bool bootRouted_ = false;
    juce::File pendingOpen_;
    std::unique_ptr<juce::FileLogger> logger_;
    juce::ComponentAnimator bootFade_;
    std::unique_ptr<SplashWindow> splash_;
    std::unique_ptr<StartWindow> start_;
    std::unique_ptr<juce::FileChooser> startChooser_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<BridgeWorker> bridgeWorker_;
    std::unique_ptr<BridgeWorkerEditor> bridgeWorkerEditor_;
};

}

START_JUCE_APPLICATION(hum::Application)
