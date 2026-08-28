#include <iostream>
#include <csignal>
#include <cstdio>
#include <cstring>
#if defined(_WIN32)
#include <io.h>
#else
#include <execinfo.h>
#include <unistd.h>
#endif

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum { void registerBuiltinOrganisms(); }

static char gAutosaveSlotPath[1024];
static char gAutosaveMetaPath[1024];
static void removeRaw(const char* p) {
#if defined(_WIN32)
    ::_unlink(p);
#else
    ::unlink(p);
#endif
}

static void deliberateQuitHandler(int sig) {
    removeRaw(gAutosaveSlotPath);
    removeRaw(gAutosaveMetaPath);
    signal(sig, SIG_DFL);
    raise(sig);
}

static void writeRaw(const char* s) {
#if defined(_WIN32)
    ::_write(2, s, (unsigned) strlen(s));
#else
    const auto ignored = ::write(2, s, strlen(s));
    (void) ignored;
#endif
}

static void crashHandler(int sig) {
    char head[64] = "\n=== CRASH: signal ";
    char d[4] = { (char) ('0' + (sig / 10) % 10), (char) ('0' + sig % 10), ' ', 0 };
    writeRaw(head);
    writeRaw(sig >= 10 ? d : d + 1);
    writeRaw("===\n");
#if !defined(_WIN32)
    void* frames[64];
    backtrace_symbols_fd(frames, backtrace(frames, 64), 2);
#endif
    writeRaw("=== END CRASH ===\n");
    signal(sig, SIG_DFL);
    raise(sig);
}

#include "core/AppPaths.h"
#include "core/WindowGeometry.h"
#include "core/BridgeClient.h"
#include "core/BridgeWorker.h"
#include "gui/BridgeWorkerEditor.h"
#include "core/PluginHost.h"
#include "core/PluginListStore.h"
#include "core/PluginScanner.h"
#include "core/TuningProbe.h"
#include "core/UiWatchdog.h"
#include "io/AutosaveStore.h"
#include "gui/AiClient.h"
#include "gui/AppSettings.h"
#include "gui/PluginQuarantine.h"
#include "gui/LookAndFeel.h"
#include "gui/MainComponent.h"
#include "gui/DefaultPatchHandler.h"
#include "io/PatchFormat.h"
#include "gui/SplashWindow.h"
#include "gui/StartWindow.h"

namespace hum {

#if JUCE_MAC
void disableAutomaticWindowTabbing();
#endif

#if JUCE_MAC
void becomeWindowlessProcess();
#endif

static void hideFromDock() {
#if JUCE_MAC
    juce::Process::setDockIconVisible(false);
#endif
}

static void hideEverything() {
#if JUCE_MAC
    becomeWindowlessProcess();
#endif
}

class Application : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Humus"; }
    const juce::String getApplicationVersion() override {
        return JUCE_APPLICATION_VERSION_STRING;
    }

    void initialise(const juce::String&) override {
        signal(SIGSEGV, crashHandler);
        signal(SIGABRT, crashHandler);
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

        const auto args = getCommandLineParameterArray();

        for (const char* child : {"--ai-ping", "--tuning-probe", "--scan-enumerate-out",
                                  "--scan-plugin-out", "--scan-plugin"})
            if (args.contains(child)) { hideEverything(); break; }

        if (const int pi = args.indexOf("--ai-ping"); pi >= 0) {
            int status = 0;
            const auto base = pi + 1 < args.size() && args[pi + 1].startsWith("http")
                                  ? args[pi + 1] : AiClient::endpoint();
            auto stream = juce::URL(base + "/api/version").createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(5000)
                    .withStatusCode(&status));
            std::cout << (stream != nullptr
                              ? "OK " + stream->readEntireStreamAsString()
                              : "FAIL (no connection, status " + juce::String(status) + ")")
                      << " - " << base << std::endl;
            setApplicationReturnValue(stream != nullptr ? 0 : 1);
            quit();
            return;
        }

        if (const int tp = args.indexOf("--tuning-probe"); tp >= 0 && tp + 2 < args.size()) {
            hum::registerBuiltinOrganisms();
            hum::PluginHost::instance().restoreKnownListFromXml(hum::pluginListStore::load());
            const auto verdict = hum::probeTuning(args[tp + 2].toStdString());
            juce::File(args[tp + 1]).replaceWithText(hum::tuningProbeVerdictName(verdict));
            setApplicationReturnValue(0);
            quit();
            return;
        }

        if (const int en = args.indexOf("--scan-enumerate-out"); en >= 0 && en + 2 < args.size()) {
            juce::File(args[en + 1]).replaceWithText(PluginScanner::enumerateLines(args[en + 2]));
            setApplicationReturnValue(0);
            quit();
            return;
        }

        if (const int so = args.indexOf("--scan-plugin-out"); so >= 0 && so + 2 < args.size()) {
            const juce::File out(args[so + 1]);
            const juce::String fmt = args[so + 2];
            juce::StringArray files;
            for (int i = so + 3; i < args.size(); ++i) files.add(args[i]);
            out.replaceWithText(PluginScanner::probeFilesXml(fmt, files));
            setApplicationReturnValue(0);
            quit();
            return;
        }
        const int si = args.indexOf("--scan-plugin");
        if (si >= 0) {
            const juce::String fmt = si + 1 < args.size() ? args[si + 1] : juce::String();
            juce::StringArray files;
            for (int i = si + 2; i < args.size(); ++i) files.add(args[i]);
            const juce::String xml = PluginScanner::probeFilesXml(fmt, files);
            std::cout << xml << std::endl;
            setApplicationReturnValue(0);
            quit();
            return;
        }

        {
            const AutosaveStore slot;
            snprintf(gAutosaveSlotPath, sizeof(gAutosaveSlotPath), "%s",
                     slot.autosaveFile().getFullPathName().toRawUTF8());
            snprintf(gAutosaveMetaPath, sizeof(gAutosaveMetaPath), "%s",
                     slot.autosaveFile().withFileExtension("meta.xml")
                         .getFullPathName().toRawUTF8());
            signal(SIGINT, deliberateQuitHandler);
            signal(SIGTERM, deliberateQuitHandler);
#if !defined(_WIN32)
            signal(SIGHUP, deliberateQuitHandler);
#endif
        }

#if JUCE_MAC
        disableAutomaticWindowTabbing();
#endif
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
        splash_->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
        splash_->setCentrePosition(juce::Desktop::getInstance().getDisplays()
                                       .getPrimaryDisplay()->userArea.getCentre());
        splash_->setVisible(true);
        splash_->toFront(false);
    }

    void openStartWindow() {
        if (auto* s = splash_.release()) {
            s->setVisible(false);
            juce::MessageManager::callAsync([s] { delete s; });
        }
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
        a.onRecover = [this](const AutosaveStore::Recovery& r, bool restore) {
            if (!restore) { AutosaveStore::discard(r); return; }
            showMainWindow();
            if (auto* mc = mainComponent()) mc->restoreAutosave(r);
        };
        start_ = std::make_unique<StartWindow>(std::move(a), std::move(orphans));
        start_->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
        start_->setCentrePosition(juce::Desktop::getInstance().getDisplays()
                                       .getPrimaryDisplay()->userArea.getCentre());
        start_->setVisible(true);
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
        aw->addButton("Quarantine", 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Ignore", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        aw->enterModalState(true, juce::ModalCallbackFunction::create(
            [cls = rep->pluginClassRaw.toStdString()](int r) {
                if (r == 1) quarantine::add(cls);
            }), true);
    }

    MainComponent* mainComponent() {
        return window_ ? dynamic_cast<MainComponent*>(window_->getContentComponent()) : nullptr;
    }

    void showMainWindow(bool offerHandler = true) {
        if (auto* w = start_.release()) {
            w->setVisible(false);
            juce::MessageManager::callAsync([w] { delete w; });
        }
        window_ = std::make_unique<MainWindow>();
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
            w->setVisible(false);
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
#if !defined(_WIN32)
        void* frames[64];
        const int n = backtrace(frames, 64);
        char** syms = backtrace_symbols(frames, n);
        for (int i = 0; i < n; ++i)
            fprintf(stderr, "  %s\n", syms ? syms[i] : "?");
#endif
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
    }

    static juce::File firstExistingFile(const juce::String& commandLine) {
        for (const auto& a : juce::StringArray::fromTokens(commandLine, true)) {
            if (a.startsWithChar('-')) continue;
            if (const juce::File f(a.unquoted()); f.existsAsFile()) return f;
        }
        return {};
    }

    void anotherInstanceStarted(const juce::String& commandLine) override {
        const auto f = firstExistingFile(commandLine);
        if (f == juce::File()) return;
        pendingOpen_ = f;
        if (bootRouted_) openPending();
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
    class MainWindow : public juce::DocumentWindow {
    public:
        static constexpr int kMinW = 900, kMinH = 600;

        MainWindow()
            : juce::DocumentWindow(juce::String("Untitled  -  Humus"),
                                   juce::Colours::black,
                                   juce::DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(kMinW, kMinH, 100000, 100000);
            const auto state = AppSettings::instance().getString("window.state");
            if (!savedWindowSizeIsUsable(state.toStdString(), kMinW, kMinH)
                || !restoreWindowStateFromString(state)) {
                const auto area = juce::Desktop::getInstance().getDisplays()
                                      .getPrimaryDisplay()->userArea;
                const auto s = firstRunWindowSize(area.getWidth(), area.getHeight(),
                                                  kMinW, kMinH);
                centreWithSize(s.w, s.h);
            }
            setVisible(true);
#if JUCE_LINUX
            if (auto* peer = getPeer()) {
                peer->setConstrainer(getConstrainer());
                if (getWidth() < kMinW || getHeight() < kMinH)
                    setSize(juce::jmax(kMinW, getWidth()), juce::jmax(kMinH, getHeight()));
            }
#endif
        }
        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    HumLookAndFeel laf_;
    bool forceFirstBoot_ = false;
    bool bootRouted_ = false;
    juce::File pendingOpen_;
    std::unique_ptr<SplashWindow> splash_;
    std::unique_ptr<StartWindow> start_;
    std::unique_ptr<juce::FileChooser> startChooser_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<BridgeWorker> bridgeWorker_;
    std::unique_ptr<BridgeWorkerEditor> bridgeWorkerEditor_;
};

}

START_JUCE_APPLICATION(hum::Application)
