#pragma once
#include <functional>
#include <map>
#include <memory>
#include <set>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/UiWatchdog.h"
#include "gui/UiTicker.h"
#include "io/AutosaveStore.h"
#include "gui/TracksPane.h"
#include "gui/MetapadView.h"
#include "gui/FreeWindow.h"
#include "gui/Dock.h"
#include "gui/ParamSlider.h"
#include "gui/EngineHost.h"
#include "gui/IconButton.h"
#include "gui/TransportStrip.h"
#include "gui/Mappable.h"
#include "gui/PatcherCanvas.h"
#include "gui/PluginEditorWindow.h"
#include "gui/VideoPreviewRig.h"
#include "gui/VisualWindow.h"
#include "gui/GuideView.h"
#include "gui/AppUpdater.h"
#include "gui/UpdateNotice.h"
#include "gui/PropertiesPane.h"
#include "gui/SetupWizard.h"
#include "gui/StartWindow.h"
#include "gui/TempoSlider.h"
#include "gui/TransportWidgets.h"

namespace hum {

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
    void promptOpen() { openPatch(); }
    void showAbout();
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

    void newPatch();
    void newPatchImpl();
    void openPatch();
    void openPatchImpl();
    void savePatch(std::function<void()> onSaved = nullptr);
    void savePatchAs(std::function<void()> onSaved = nullptr);
    void exportSound();
    void toggleMixRecording();
    void revertPatch();
    void openSettings(int category = -1);
    void openAudioSettings();
    void openNotes();
    void openLibrary();
    void openHelpBrowser();
    void openDocSwitcher();
    void openParameterControl(const std::string& organism = {},
                              const std::string& param = {});
    void wireGlobalKeys(FreeWindow&);
    void openPluginUI(const std::string& name);
    void openPluginUIImpl(const std::string& name);
    void openVisualUI(const std::string& name);
    void closeVisualUI(const std::string& name);
    void openClipEditor(const std::string& node, int clip);
    void refreshTimelinePanes();
    void closePluginUI(const std::string& name);
    void updateDspReadout();
    void setStatus(const juce::String& s) { statusLabel_.setText(s, juce::dontSendNotification); }

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

    IconButton undoBtn_{IconButton::Glyph::Undo, "Undo (Ctrl+Z) - takes back the last roll"};
    IconButton redoBtn_{IconButton::Glyph::Redo, "Redo (Ctrl+Shift+Z)"};
    IconButton playFromStartBtn_{IconButton::Glyph::PlayFromStart, "Play From Start"};
    IconButton playBtn_{IconButton::Glyph::Play, "Play (Space)"};
    IconButton stopBtn_{IconButton::Glyph::Stop, "Stop"};
    Mappable<IconButton> recordBtn_{IconButton::Glyph::Record,
                                    "Record the performance (every knob move, morph & MIDI in one "
                                    "pass). Right-click: Touch or Latch automation"};
    IconButton keepBtn_{IconButton::Glyph::Keep,
                        "Keep the last 8 bars (retroactive: what you just played "
                        "becomes lanes + audio - no arming needed, the soil remembers)"};
    IconButton goStartBtn_{IconButton::Glyph::GoToStart, "Go to Start (reset clock to 1-1.00)"};
    IconButton goEndBtn_{IconButton::Glyph::GoToEnd,
                         "Go to End (song-end marker, or the end of the content)"};
    IconButton loopBtn_{IconButton::Glyph::Loop, "Enable Automation Loop"};
    IconButton enableAudioBtn_{IconButton::Glyph::EnableAudio, "Enable Audio (real-time engine on/off)"};
    IconButton enableMidiBtn_{IconButton::Glyph::EnableMidi,
                              "Enable MIDI (open the MIDI devices for control, notes and sync)"};
    IconButton qwertyBtn_{IconButton::Glyph::QwertyPiano,
                          "Virtual MIDI keyboard - play notes with the computer keyboard"
                          " while a piano strip is open (Z/X shift octave)"};
    Mappable<IconButton> globalDiceBtn_{IconButton::Glyph::Dice,
                                        "Roll the dice on every organism at once"
                                        " (right-click to map a pad or an LFO)"};
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
    juce::TextButton limiterBtn_{"Lim"};
    juce::TextButton grooveBtn_{"Groove"};
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
    void showGrooveMenu();
    void refreshGrooveButton();
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
    IconButton viewNotes_{IconButton::Glyph::Notes, "Notes (free text, saved with the patch)"};
    IconButton viewDocSwitcher_{IconButton::Glyph::DocumentSwitcher,
                                "Document Switcher (a set of patches for live switching, F9)"};
    IconButton viewHelp_{IconButton::Glyph::Help, "Help (browse every organism's reference)"};
    IconButton viewLibrary_{IconButton::Glyph::Library,
                            "Library (your samples and impulses - drag onto file slots)"};
    IconButton settingsBtn_{IconButton::Glyph::Gear, "Settings"};
    juce::Label statusLabel_;

    std::unique_ptr<TracksPane> tracksPane_;
    std::unique_ptr<MetapadWindow> metaWindow_;
    std::unique_ptr<juce::DocumentWindow> assistantWin_;
    std::unique_ptr<StartWindow> aboutWin_;
    std::unique_ptr<SetupWizard> wizard_;
    std::unique_ptr<GuideView> guide_;
    std::unique_ptr<UpdateNotice> updateNotice_;
    std::unique_ptr<AppUpdater> updater_;
    juce::File downloaded_;
    std::unique_ptr<NagCard> nagCard_;
    bool crashedLastRun_ = false;
    void showUpdateNotice(const std::string& version, const std::string& url,
                          const std::string& notes, const std::string& sha256);
    void placeUpdateNotice();
    void dismissUpdateNotice();
    void maybeShowNag();
    void dismissNag();
    void flushTelemetry();
    juce::int64 lastTelemetryFlushMs_ = 0;
    juce::int64 lastTelemetrySaveMs_ = 0;
    juce::int64 sessionStartMs_ = 0;

    DockPane paneCenter_{"Patcher"}, paneRight_{"Properties"}, paneBottom_{"Automation"};
    SplitterBar splitV_{SplitterBar::Vertical}, splitH_{SplitterBar::Horizontal};
    int rightW_ = 636, bottomH_ = 150;

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
    UiWatchdog watchdog_;
    int tickerId_ = 0;
    juce::String lastTitle_;
    unsigned lastLiveGen_ = 0;
    unsigned lastLaneStamp_ = 0;
    unsigned recBlink_ = 0;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::StringArray recentMenu_;
    juce::Component::SafePointer<juce::DialogWindow> settingsWindow_;
    std::unique_ptr<FreeWindow> paramControlWindow_;
    std::unique_ptr<FreeWindow> notesWindow_;
    std::unique_ptr<FreeWindow> libraryWindow_;
    std::unique_ptr<FreeWindow> helpWindow_;
    std::unique_ptr<FreeWindow> docSwitcherWindow_;
    std::map<std::string, std::unique_ptr<PluginEditorWindow>> pluginWindows_;
    std::map<std::string, std::unique_ptr<VisualWindow>> visualWindows_;
    std::map<std::string, std::unique_ptr<VideoPreviewRig>> previewRigs_;
    void serviceVideoPreviewRigs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}
