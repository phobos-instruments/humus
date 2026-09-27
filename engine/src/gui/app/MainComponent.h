// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/DeviceWatch.h"
#include "core/app/UiWatchdog.h"
#include "io/AutosaveStore.h"
#include "gui/app/Dock.h"
#include "gui/editor/ParamSlider.h"
#include "gui/host/EngineHost.h"
#include "gui/style/IconButton.h"
#include "gui/style/LitPad.h"
#include "gui/editor/Mappable.h"
#include "gui/app/CardDesk.h"
#include "gui/host/HostHistory.h"
#include "gui/app/CardStack.h"
#include "gui/app/NoticeCard.h"
#include "gui/app/TempoSlider.h"
#include "gui/app/GroovePots.h"
#include "gui/app/TransportWidgets.h"
#include "gui/common/Localisation.h"

namespace hum {

class AboutWindow;
class AppUpdater;
class BounceJob;
struct BounceWants;
class FloatingPaneWindow;
class FreeWindow;
class GuideView;
class MetapadWindow;
class PatcherCanvas;
class PluginEditorWindow;
class PropertiesPane;
class SetupWizard;
class StartWindow;
class TracksPane;
class TransportStrip;
class UpdateNotice;
class VideoRenderService;
class VideoTrackerFeed;
class VisualWindow;

namespace examples { struct Node; }
namespace browser { class BrowserIndex; class Audition; class FileBrowserView; struct PickRequest; struct PlaceRoots; }

class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      private juce::MenuBarModel,
                      private juce::Timer {
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    void paint(juce::Graphics&) override;
    bool keyPressed(const juce::KeyPress&) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;
    void ensureKeyboardFocus();
    bool keyStateChanged(bool isKeyDown) override;

    void demoScene();

    bool openFileAt(const juce::File& f);
    void openSetupWizard(bool firstBoot);
    void openGuide(bool firstBoot);
    void openHelpBrowser();
    void promptOpen() { openPatch(); }
    void showAbout();
    void showStartWindow();
    void openBugReport();
    juce::PopupMenu menuForTest(int index) { return getMenuForIndex(index, {}); }
    EngineHost& hostForTest() { return host_; }
    void routeFilePicksToBrowser();
    browser::FileBrowserView* libraryViewForTest() { return libraryView_; }
    bool libraryOpenForTest() const { return libraryWindow_ != nullptr; }
    void pickForTest(const browser::PickRequest& r, std::function<void(const std::vector<std::string>&)> done) {
        pickWithBrowser(r, std::move(done));
    }
    void browseLibraryForTest() { browseLibrary(); }
    int cardCountForTest() const { return cards_.count(); }
    void noteBounceForTest(double seconds) { noteBounce(seconds); }
    bool bounceWindowOpenForTest() const { return bounceWindow_ != nullptr; }
    void closeBounceWindowForTest() { closeBounceWindow(); }
    PropertiesPane& propsForTest() { return *propsPane_; }
    void setRightWidthForTest(int w) { rightW_ = w; resized(); }
    juce::StringArray menuNamesForTest() { return getMenuBarNames(); }
    void startupCheckin();
    void checkForUpdatesManually();
    void restoreAutosave(const AutosaveStore::Recovery& r);

    void confirmDiscardThenRun(std::function<void()> action);
    void closeProject();
    std::function<void()> onCloseProject;

private:
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int index, const juce::String& name) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    void addAt(const std::string& className, juce::Point<int> at);

    void buildTransportRow();
    void buildWorkspaceRail();

    void toggleAudio();
    void toggleMidi();
    void toggleMetapad();
    void openAssistant();
    void ensureAudio();
    void togglePlay();
    void stopTransport();
    void toggleLoop();
    void globalRoll();
    void showTempoMenu(juce::Point<int> screen);
    void timerCallback() override;
    void refreshTransportMarks();
    std::vector<std::pair<Mappable<IconButton>*, std::string>> mappedTransport_;
    void showLoadProgress();
    static juce::String loadLeftText(const EngineHost::Loading& load);

    void newPatch();
    void newPatchImpl();
    void openPatch();
    void openPatchImpl();
    void savePatch(std::function<void()> onSaved = nullptr);
    void savePatchAs(std::function<void()> onSaved = nullptr);
    void savePatchInto(const juce::File& chosen, std::function<void()> onSaved);
    void bounce();
    void startBounce(const BounceWants& wants);
    void closeBounceWindow();
    void toggleMixRecording();
    void revertPatch();
    void locateMissingMedia();
    void offerToLocate(int missing);
    void openSettings(int category = -1);
    void openAudioSettings();
    void openNotes();
    void openLibrary();
    browser::BrowserIndex& browserIndex();
    browser::Audition& audition();
    void noteOpened(const juce::File& f);
    void openFromBrowser(const std::string& path);
    std::string browserLoadTarget(const std::string& path);
    browser::PlaceRoots browserRoots() const;
    void pickWithBrowser(const browser::PickRequest& request, std::function<void(const std::vector<std::string>&)> done,
                         std::function<void()> other = {});
    void openPatchNative();
    void endBrowserPick(bool closeWindow);
    void browseLibrary();
    void openDocSwitcher();
    void openParameterControl(const std::string& organism = {},
                              const std::string& param = {});
    void wireGlobalKeys(FreeWindow&);
    void openPluginUI(const std::string& name);
    void openPluginUIImpl(const std::string& name);
    void openVisualUI(const std::string& name, int width = 0, int height = 0);
    void reopenVisualOutputs();
    int displayHoldingMain() const;
    void closeVisualUI(const std::string& name);
    void openClipEditor(const std::string& node, int clip);
    void refreshTimelinePanes();
    void closePluginUI(const std::string& name);
    void updateDspReadout();
    void presentCard(std::unique_ptr<juce::Component> card);
    void notify(const juce::String& s) { notices_.say(s); }
    void notifyError(const juce::String& s, const juce::String& details = {}) {
        notices_.warn(s, {}, details);
    }
    void notifyOn(const char* key, const juce::String& s) {
        notices_.say(s, NoticeCard::Kind::Passing, key);
    }
    void notifyErrorOn(const char* key, const juce::String& s) {
        notices_.warn(s, key);
    }

    void openHistory();
    void refreshAfterLoad();
    static std::int64_t nowMs() { return juce::Time::currentTimeMillis(); }

    void autosaveTick();
    void performAutosave();
    void clearAutosave();

    juce::File lastDir(bool forSave) const;
    void rememberDir(const juce::File& chosen, bool forSave);
    void refreshAppearance();
    juce::String strayWarning() const;
    void updateWindowTitle();

    EngineHost host_;

    juce::TooltipWindow tooltips_{this, 600};
    unsigned lastDeviceRestarts_ = 0;

    juce::MenuBarComponent menuBar_{this};
    std::unique_ptr<PatcherCanvas> canvas_;
    juce::Viewport patcherView_;
    std::unique_ptr<PropertiesPane> propsPane_;

    IconButton undoBtn_{IconButton::Glyph::Undo, "Undo (Ctrl+Z)"};
    IconButton redoBtn_{IconButton::Glyph::Redo, "Redo (Ctrl+Shift+Z)"};
    Mappable<IconButton> playFromStartBtn_{IconButton::Glyph::PlayFromStart, tr("main.play-from-start", "Play From Start")};
    Mappable<IconButton> playBtn_{IconButton::Glyph::Play, tr("main.play-space", "Play (Space)")};
    Mappable<IconButton> stopBtn_{IconButton::Glyph::Stop, "Stop"};
    Mappable<IconButton> recordBtn_{IconButton::Glyph::Record,
                                    "Record the performance"};
    Mappable<IconButton> panicBtn_{IconButton::Glyph::Panic,
        tr("main.panic", "Panic - stop every hanging note")};
    IconButton keepBtn_{IconButton::Glyph::Keep,
                        "Keep the last 8 bars"};
    Mappable<IconButton> goStartBtn_{IconButton::Glyph::GoToStart, tr("main.go-to-start", "Go to start")};
    Mappable<IconButton> goEndBtn_{IconButton::Glyph::GoToEnd,
                         "Go to end"};
    Mappable<IconButton> loopBtn_{IconButton::Glyph::Loop, tr("main.enable-automation-loop", "Enable Automation Loop")};
    IconButton enableAudioBtn_{IconButton::Glyph::EnableAudio, tr("main.audio-on-off", "Audio on or off")};
    IconButton enableMidiBtn_{IconButton::Glyph::EnableMidi,
                              "MIDI on or off"};
    IconButton qwertyBtn_{IconButton::Glyph::QwertyPiano,
                          "Play notes from the computer keyboard"};
    Mappable<IconButton> globalDiceBtn_{IconButton::Glyph::Dice,
                                        "Randomise everything"};
    TempoSlider tempo_;
    TimeSigChip tsig_;
    juce::TextButton tapBtn_{"Tap"};
    juce::TextButton beat1Btn_{"1"};
    juce::TextButton metroBtn_{"Click"};
    juce::TextButton linkBtn_{"Link"};
    struct MetroMenu : juce::MouseListener {
        explicit MetroMenu(MainComponent& o) : mc(o) {}
        void mouseDown(const juce::MouseEvent& e) override {
            if (e.mods.isPopupMenu()) mc.showMetroMenu();
        }
        MainComponent& mc;
    };
    MetroMenu metroMenu_{*this};
    void rollScope(const std::string& pod);
    void showMetroMenu();
    struct LinkMenu : juce::MouseListener {
        explicit LinkMenu(MainComponent& o) : mc(o) {}
        void mouseDown(const juce::MouseEvent& e) override {
            if (e.mods.isPopupMenu()) mc.showLinkMenu();
        }
        MainComponent& mc;
    };
    LinkMenu linkMenu_{*this};
    void showLinkMenu();
    std::vector<double> tapTimesMs_;
    ClockReadout clock_;
    ParamSlider masterLevel_{juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox};
    juce::Label masterCaption_;
    LitPad limiterBtn_{LitPad::Look{std::nullopt, "LIMIT"}};
    GroovePots groove_{host_};
    bool limGlowLit_ = false;
    TransportMeter meter_;
    juce::Label dspLabel_;
    juce::Rectangle<int> timeWell_;
    juce::Image hypha_[2];
    std::vector<int> sepX_;
    void rebuildChromeTextures();
    IconButton overflowBtn_{IconButton::Glyph::Overflow,
                            "Toolbar controls that do not fit at this window width"};
    void showOverflowMenu();
    void showGrooveMenu(const std::string& param, juce::Point<int> screen);
    float dspLoadHold_ = 0.0f;
    unsigned lastDropouts_ = 0;
    int dropFlashTicks_ = 0;
    IconButton viewPatcher_{IconButton::Glyph::Patcher, "Patcher"};
    IconButton viewProperties_{IconButton::Glyph::Properties, "Properties"};
    IconButton viewAutomation_{IconButton::Glyph::Automation, "Timeline"};
    IconButton viewMetapad_{IconButton::Glyph::Metapad,
                                "Metapad (morph the whole patch between snapshots)"};
    IconButton viewParamControl_{IconButton::Glyph::ParameterControl,
                                 "Parameter Control (MIDI / OSC / modulation and follow routes, F3)"};
    IconButton viewNotes_{IconButton::Glyph::Notes, tr("main.notes-free-text-saved-with", "Notes (free text, saved with the patch)")};
    IconButton viewDocSwitcher_{IconButton::Glyph::DocumentSwitcher,
                                "Document Switcher (a set of patches for live switching, F9)"};
    IconButton viewHelp_{IconButton::Glyph::Help, tr("main.help-browse-every-organism-s", "Help (browse every organism's reference)")};
    IconButton viewLibrary_{IconButton::Glyph::Library,
                            "Library (your samples and impulses - drag onto file slots)"};
    IconButton settingsBtn_{IconButton::Glyph::Gear, "Settings"};
    NoticeDesk notices_;
    bool loadNoticeShown_ = false;

    std::unique_ptr<TracksPane> tracksPane_;
    std::unique_ptr<MetapadWindow> metaWindow_;
    std::unique_ptr<juce::DocumentWindow> assistantWin_;
    std::unique_ptr<AboutWindow> aboutWin_;
    std::unique_ptr<StartWindow> startWin_;
    std::unique_ptr<SetupWizard> wizard_;
    std::unique_ptr<GuideView> guide_;
    std::unique_ptr<UpdateNotice> updateNotice_;
    std::unique_ptr<AppUpdater> updater_;
    juce::File downloaded_;
    juce::int64 openSinceMs_ = 0;
    std::unique_ptr<DeviceWatch> deviceWatch_;
    void showDeviceNotice(const DeviceChange& change);
    CardStack cards_;
    bool crashedLastRun_ = false;
    void showUpdateNotice(const std::string& version, const std::string& url,
                          const std::string& notes, const std::string& sha256);
    void placeUpdateNotice();
    void dismissUpdateNotice();
    void noteOpenTime();
    void noteBounce(double seconds);
    void flushTelemetry();
    juce::int64 lastTelemetryFlushMs_ = 0;
    juce::int64 lastTelemetrySaveMs_ = 0;
    juce::int64 sessionStartMs_ = 0;

    DockPane paneCenter_{"Patcher"}, paneRight_{"Properties"}, paneBottom_{"Automation"};
    SplitterBar splitV_{SplitterBar::Vertical}, splitH_{SplitterBar::Horizontal};
    int rightW_ = 636, bottomH_ = 150;
    bool dockRelayoutQueued_ = false;
    void queueDockRelayout();

    struct Dockable {
        DockPane* pane = nullptr;
        juce::Component* content = nullptr;
        IconButton* toggle = nullptr;
        juce::String key;
        bool floated = false;
        std::unique_ptr<FloatingPaneWindow> win;
        std::unique_ptr<TransportStrip> strip;
        bool wantsTransport = false;
        juce::Rectangle<int> floatBounds;
    };
    Dockable center_, right_, bottom_;

    void buildDock();
    void updateDock();
    void detach(Dockable&);
    void redock(Dockable&);
    void persistDock();
    void restoreDock();

    juce::String currentFile_;
    bool startupOutCentered_ = false;
    AutosaveStore autosave_;
    int autosaveTicks_ = 0;
    juce::uint64 lastAutosaveStamp_ = 0;
    HostHistory history_{host_};
    std::unique_ptr<browser::BrowserIndex> browserIndex_;
    std::unique_ptr<browser::Audition> audition_;
    std::unique_ptr<FreeWindow> historyWindow_;
    UiWatchdog watchdog_;
    int tickerId_ = 0;
    juce::String lastTitle_;
    unsigned lastLiveGen_ = 0;
    unsigned lastLaneStamp_ = 0;
    unsigned recBlink_ = 0;
    std::unique_ptr<juce::FileChooser> chooser_;
    std::unique_ptr<BounceJob> bounce_;
    std::unique_ptr<juce::Component> bounceWindow_;
    juce::StringArray recentMenu_;
    static constexpr int kExamplesIdBase = 1000;
    std::vector<juce::File> exampleMenu_;
    void addExampleItems(juce::PopupMenu& into, const examples::Node& node);
    juce::Component::SafePointer<juce::DialogWindow> settingsWindow_;
    std::unique_ptr<FreeWindow> paramControlWindow_;
    std::unique_ptr<FreeWindow> notesWindow_;
    std::unique_ptr<FreeWindow> libraryWindow_;
    browser::FileBrowserView* libraryView_ = nullptr;
    std::unique_ptr<FreeWindow> helpWindow_;
    std::unique_ptr<FreeWindow> docSwitcherWindow_;
    std::map<std::string, std::unique_ptr<PluginEditorWindow>> pluginWindows_;
    std::map<std::string, std::unique_ptr<VisualWindow>> visualWindows_;
    std::unique_ptr<VideoRenderService> renderService_;
    std::map<std::string, std::unique_ptr<VideoTrackerFeed>> trackerFeeds_;
    void serviceTrackerFeeds();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}
