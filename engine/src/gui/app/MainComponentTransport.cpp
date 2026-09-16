// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/app/MainComponent.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/tracks/TracksPane.h"

#include <cmath>

#include "gui/app/AppSettings.h"
#include "gui/style/LookAndFeel.h"
#include "gui/host/NodeRoll.h"
#include "gui/app/QwertyPiano.h"
#include "gui/common/Localisation.h"

namespace hum {

void MainComponent::buildTransportRow() {
    playBtn_.setLead(true);
    addAndMakeVisible(undoBtn_); undoBtn_.onClick = [this] { canvas_->undo(); };
    addAndMakeVisible(redoBtn_); redoBtn_.onClick = [this] { canvas_->redo(); };
    addAndMakeVisible(playFromStartBtn_); playFromStartBtn_.onClick = [this] { ensureAudio(); host_.playFromStart(); };
    addAndMakeVisible(playBtn_);          playBtn_.onClick          = [this] { ensureAudio(); host_.play(); };
    addAndMakeVisible(stopBtn_);          stopBtn_.onClick          = [this] {
        const bool rolling = host_.isPlaying() || host_.countInRunning();
        stopTransport();
        if (!rolling) host_.goToStart();
    };
    addAndMakeVisible(recordBtn_);        recordBtn_.onClick = [this] {
        host_.record().captureToggle();
        if (host_.record().preRolling())
            setStatus(tr("main-transport.pre-roll-status", "pre-roll: ") + juce::String(host_.beforeRecordBars())
                      + tr("main-transport.pre-roll-status-tail", " bar(s) of the song, recording from bar ")
                      + juce::String(host_.automation().meterMap().barAt(host_.punchInBeat())));
        else if (host_.countInRunning())
            setStatus(tr("main-transport.count-in-status", "count-in: ") + juce::String(host_.beforeRecordBars())
                      + tr("main-transport.count-in-status-tail", " bar(s) of click, then recording"));
    };
    const auto mapAction = [this](auto& btn, const char* action) {
        btn.onRightClick = [this, action](juce::Point<int> at) {
            showAutomateMenu(host_, host_.clockNodeName(), action, at,
                             [this] { refreshTimelinePanes(); }, false);
        };
    };
    mapAction(playFromStartBtn_, kPlayFromStartAction);
    mapAction(playBtn_, kPlayAction);
    mapAction(stopBtn_, kStopAction);
    mapAction(goStartBtn_, kGoToStartAction);
    mapAction(goEndBtn_, kGoToEndAction);
    mapAction(loopBtn_, kLoopToggleAction);
    recordBtn_.onRightClick = [this](juce::Point<int> at) {
        juce::PopupMenu m;
        const auto mode = host_.beforeRecord();
        const int bars = host_.beforeRecordBars();
        auto barsText = [](int n) { return juce::String(n) + (n == 1 ? tr("main-transport.bar", " bar") : tr("main-transport.bars", " bars")); };
        juce::PopupMenu countIn, preRoll;
        for (int n : {1, 2, 4}) {
            countIn.addItem(10 + n, barsText(n), true, mode == EngineHost::BeforeRecord::CountIn && bars == n);
            preRoll.addItem(20 + n, barsText(n), true, mode == EngineHost::BeforeRecord::PreRoll && bars == n);
        }
        juce::PopupMenu before;
        before.addItem(10, tr("main-transport.start-right-away", "Start right away"), true,
                       mode == EngineHost::BeforeRecord::None);
        before.addSubMenu(tr("main-transport.count-in-click", "Count-in with the click"), countIn, true,
                          juce::Image(), mode == EngineHost::BeforeRecord::CountIn);
        before.addSubMenu(tr("main-transport.pre-roll-song", "Pre-roll the song"), preRoll, true,
                          juce::Image(), mode == EngineHost::BeforeRecord::PreRoll);
        juce::PopupMenu write;
        write.addItem(1, tr("main-transport.touch", "Touch: only while a control is held"), true, !host_.latchMode());
        write.addItem(2, tr("main-transport.latch", "Latch: keep the last value until stop"), true, host_.latchMode());
        m.addSubMenu(tr("main-transport.before-recording", "Before recording"), before);
        m.addSubMenu(tr("main-transport.automation-write", "Automation write"), write);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                        [this](int r) {
            if (r == 0) return;
            if (r == 1 || r == 2) {
                host_.setLatchMode(r == 2);
                AppSettings::instance().set("automation.latch", r == 2 ? 1 : 0);
                return;
            }
            const auto mode = r == 10 ? EngineHost::BeforeRecord::None
                            : r < 20 ? EngineHost::BeforeRecord::CountIn : EngineHost::BeforeRecord::PreRoll;
            const int bars = r == 10 ? host_.beforeRecordBars() : r % 10;
            host_.setBeforeRecord(mode, bars);
            AppSettings::instance().set("record.before", (int) mode);
            AppSettings::instance().set("record.bars", bars);
        });
    };
    addAndMakeVisible(keepBtn_);
    keepBtn_.onClick = [this] {
        host_.keepLast(8);
        setStatus(tr("main-transport.kept-the-last-8-bars", "kept the last 8 bars"));
    };
    addAndMakeVisible(globalDiceBtn_);
    globalDiceBtn_.onClick = [this] { globalRoll(); };
    globalDiceBtn_.onRightClick = [this](juce::Point<int> at) {
        showAutomateMenu(host_, host_.clockNodeName(), kRandomAction, at, nullptr, false);
    };
    addAndMakeVisible(goStartBtn_);       goStartBtn_.onClick       = [this] { host_.goToStart(); };
    addAndMakeVisible(goEndBtn_);         goEndBtn_.onClick         = [this] { host_.goToEnd(); };
    addAndMakeVisible(loopBtn_);          loopBtn_.onClick = [this] { toggleLoop(); };
    addAndMakeVisible(enableAudioBtn_);   enableAudioBtn_.onClick   = [this] { toggleAudio(); };
    addAndMakeVisible(enableMidiBtn_);    enableMidiBtn_.onClick = [this] { toggleMidi(); };
    addAndMakeVisible(qwertyBtn_);
    qwertyBtn_.onClick = [this] {
        auto& qp = QwertyPiano::instance();
        qp.setEnabled(!qp.enabled());
        setStatus(qp.enabled() ? tr("main-transport.virtual-keyboard-enabled", "virtual keyboard enabled") : tr("main-transport.virtual-keyboard-disabled", "virtual keyboard disabled"));
    };

    tempo_.setTextValueSuffix(" BPM");
    addAndMakeVisible(tempo_);
    tempo_.setSliderStyle(juce::Slider::IncDecButtons);
    tempo_.setRange(kTempoMin, kTempoMax, 0.1);
    tempo_.setValue(host_.tempo(), juce::dontSendNotification);
    tempo_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, TempoSlider::kNumberW, 22);
    tempo_.setTooltip(juce::String::fromUTF8(
        "Tempo in beats per minute - drag the number, or right-click to "
        "automate and map it"));
    tempo_.onValueChange = [this] { host_.performTempo(tempo_.getValue()); };
    tempo_.onPopup = [this](juce::Point<int> screen) { showTempoMenu(screen); };

    addAndMakeVisible(tsig_);
    tsig_.get = [this] { return host_.automation().meterAt(host_.positionBeats()); };
    tsig_.set = [this](Meter m) {
        host_.automation().setTimeSignature(m.beats, m.unit);
        refreshTimelinePanes();
    };
    tsig_.automated = [this] { return host_.automation().meterAutomated(); };
    tsig_.setAutomated = [this](bool on) {
        host_.automation().setMeterAutomated(on);
        refreshTimelinePanes();
        setStatus(on ? juce::String::fromUTF8("meter lanes added - draw the beats and the unit "
                                              "in the Automation pane")
                     : juce::String("meter automation removed"));
    };

    addAndMakeVisible(tapBtn_);
    tapBtn_.setTooltip(juce::String::fromUTF8(
        "Tap the tempo - two or more taps set the BPM (pause 2 s to start over)"));
    tapBtn_.onClick = [this] {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (!tapTimesMs_.empty() && now - tapTimesMs_.back() > 2000.0) tapTimesMs_.clear();
        tapTimesMs_.push_back(now);
        if (tapTimesMs_.size() > 9) tapTimesMs_.erase(tapTimesMs_.begin());
        if (tapTimesMs_.size() < 2) { setStatus(tr("main-transport.tap-tempo-keep-tapping", "tap tempo: keep tapping...")); return; }
        const double ms = (tapTimesMs_.back() - tapTimesMs_.front())
                          / (double) (tapTimesMs_.size() - 1);
        const double bpm = juce::jlimit(20.0, 999.0, 60000.0 / ms);
        host_.performTempo(bpm);
        tempo_.setValue(bpm, juce::dontSendNotification);
        setStatus(tr("main-transport.tap-tempo", "tap tempo: ") + juce::String(bpm, 1) + tr("main-transport.bpm", " BPM (")
                  + juce::String((int) tapTimesMs_.size()) + " taps)");
    };
    addAndMakeVisible(beat1Btn_);
    beat1Btn_.setTooltip(juce::String::fromUTF8(
        "Mark now as beat 1 - re-phases the bars without changing the tempo"));
    beat1Btn_.onClick = [this] {
        const double pos = host_.positionBeats();
        host_.setPositionBeats(std::max(0.0, host_.automation().meterMap().nearestBarStart(pos)));
        setStatus(tr("main-transport.beat-1-marked", "beat 1 marked"));
    };
    addAndMakeVisible(metroBtn_);
    metroBtn_.setClickingTogglesState(true);
    metroBtn_.setTooltip(juce::String::fromUTF8(
        "Metronome - a tick every beat, brighter on beat 1"
        " (right-click: volume + count-in)"));
    metroBtn_.setColour(juce::TextButton::buttonOnColourId, Palette::accent.withAlpha(alpha::mid));
    metroBtn_.onClick = [this] { host_.setMetronome(metroBtn_.getToggleState()); };
    metroBtn_.addMouseListener(&metroMenu_, false);
    host_.setMetronomeGain((float) AppSettings::instance().getInt("metro.volume", 100) / 100.0f);
    {
        auto& s = AppSettings::instance();
        const int legacyCountIn = s.getInt("metro.countin", 0);
        const int before = s.getInt("record.before", legacyCountIn != 0 ? 1 : 0);
        host_.setBeforeRecord((EngineHost::BeforeRecord) juce::jlimit(0, 2, before), s.getInt("record.bars", 1));
    }

    addAndMakeVisible(linkBtn_);
    linkBtn_.setClickingTogglesState(true);
    linkBtn_.setTooltip(juce::String::fromUTF8(
        "Ableton Link - lock tempo and bar phase with other"
        " Link-enabled apps and devices on this network. The label counts"
        " connected peers (right-click: start/stop sync)."));
    linkBtn_.setColour(juce::TextButton::buttonOnColourId, Palette::accent.withAlpha(alpha::mid));
    linkBtn_.onClick = [this] {
        host_.setLinkEnabled(linkBtn_.getToggleState());
        linkBtn_.setToggleState(host_.linkEnabled(), juce::dontSendNotification);
    };
    linkBtn_.addMouseListener(&linkMenu_, false);
    for (auto* b : {&tapBtn_, &beat1Btn_, &metroBtn_, &linkBtn_}) {
        b->setColour(juce::TextButton::buttonColourId, Palette::panel);
        b->setColour(juce::TextButton::buttonOnColourId, Palette::panelLight);
        b->setColour(juce::TextButton::textColourOffId, Palette::textDim);
        b->setColour(juce::TextButton::textColourOnId, Palette::accent);
        b->getProperties().set("flatPill", true);
    }

    addAndMakeVisible(clock_);
    addAndMakeVisible(masterCaption_);
    masterCaption_.setText(tr("main-transport.out", "Out"), juce::dontSendNotification);
    masterCaption_.setColour(juce::Label::textColourId, Palette::textDim);
    masterCaption_.setFont(juce::FontOptions(11.0f));
    masterCaption_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(masterLevel_);
    masterLevel_.setRange(0.0, 1.0, 0.0);
    masterLevel_.setValue(host_.outputGain(), juce::dontSendNotification);
    masterLevel_.setTooltip(tr("main-transport.master-output-level", "Master output level"));
    masterLevel_.paramLabel = tr("main-transport.master-out", "Master out");
    masterLevel_.setDoubleClickReturnValue(true, 1.0);
    masterLevel_.onValueChange = [this] { host_.setOutputGain((float) masterLevel_.getValue()); };
    addAndMakeVisible(limiterBtn_);
    limiterBtn_.setClickingTogglesState(true);
    limiterBtn_.setToggleState(host_.limiterEnabled(), juce::dontSendNotification);
    limiterBtn_.setTooltip(juce::String::fromUTF8(
        "Master limiter - keeps a runaway patch from slamming your"
        " speakers and ears. Transparent until the output nears full scale,"
        " then it leans in instantly (no added latency); glows amber while"
        " it is actually working. Saved with the patch."));
    limiterBtn_.onClick = [this] { host_.setLimiter(limiterBtn_.getToggleState()); };
    limiterBtn_.setColour(juce::TextButton::buttonColourId, Palette::panel);
    limiterBtn_.setColour(juce::TextButton::buttonOnColourId, Palette::panelLight);
    limiterBtn_.setColour(juce::TextButton::textColourOffId, Palette::textDim);
    limiterBtn_.setColour(juce::TextButton::textColourOnId, Palette::accent);
    limiterBtn_.getProperties().set("flatPill", true);
    addAndMakeVisible(groove_);
    groove_.onMenu = [this](const std::string& param, juce::Point<int> at) {
        showGrooveMenu(param, at);
    };
    addAndMakeVisible(meter_);
    addChildComponent(overflowBtn_);
    overflowBtn_.onClick = [this] { showOverflowMenu(); };
    addAndMakeVisible(dspLabel_);
    dspLabel_.setColour(juce::Label::textColourId, Palette::textDim);
    dspLabel_.setFont(juce::FontOptions(11.0f));
    dspLabel_.setJustificationType(juce::Justification::centredRight);
    dspLabel_.setTooltip(tr("main-transport.audio-health",
                            "Audio health: DSP load (callback time vs. its real-time "
                            "deadline, peak-held) and, after a \"!\", blocks dropped "
                            "to engine contention. Any dropout is an audible glitch - "
                            "report it if you hear one."));
}

void MainComponent::toggleAudio() {
    if (host_.audioRunning() || host_.audioStarting()) {
        host_.stopAudio();
        setStatus(tr("main-transport.audio-disabled", "audio disabled"));
        AppSettings::instance().set("audio.enabled", 0);
        return;
    }
    setStatus(juce::String::fromUTF8("starting audio\xe2\x80\xa6"));
    host_.startAudioAsync([safe = juce::Component::SafePointer<MainComponent>(this)]
                          (bool ok, std::string err) {
        if (safe == nullptr) return;
        safe->setStatus(ok ? tr("main-transport.audio-enabled", "audio enabled")
                           : err == "cancelled" ? "audio disabled"
                                                : "no audio device (" + juce::String(err) + ")");
        AppSettings::instance().set("audio.enabled", safe->host_.audioRunning() ? 1 : 0);
    });
}

void MainComponent::toggleMidi() {
    auto& s = AppSettings::instance();
    if (host_.midi().enabled()) {
        host_.midi().setEnabled(false);
        s.set("midi.enabled", 0);
        setStatus(tr("main-transport.midi-disabled", "MIDI disabled"));
    } else if (host_.midi().setEnabled(true)) {
        s.set("midi.enabled", 1);
        setStatus(tr("main-transport.midi-enabled", "MIDI enabled"));
    } else {
        host_.midi().setEnabled(false);
        s.set("midi.enabled", 1);
        setStatus(tr("main-transport.no-midi-input-devices-found", "no MIDI input devices found"));
    }
}

void MainComponent::ensureAudio() {
    if (host_.audioRunning() || host_.audioStarting()) return;
    setStatus(juce::String::fromUTF8("starting audio\xe2\x80\xa6"));
    host_.startAudioAsync([safe = juce::Component::SafePointer<MainComponent>(this)]
                          (bool ok, std::string err) {
        if (safe == nullptr) return;
        if (ok) safe->setStatus(tr("main-transport.audio-enabled", "audio enabled"));
        else if (err != "cancelled")
            safe->setStatus(tr("main-transport.no-audio-device", "no audio device (") + juce::String(err) + ")");
    });
}

void MainComponent::stopTransport() {
    host_.stop();
    if (host_.record().armed()) host_.record().captureToggle();
}

void MainComponent::togglePlay() {
    if (host_.isPlaying()) { stopTransport(); setStatus("stopped"); }
    else                   { ensureAudio(); host_.play(); setStatus("playing"); }
}

void MainComponent::toggleLoop() {
    if (host_.automation().loopEnabled()) { host_.automation().setLoop(host_.automation().loopStartBeat(), host_.automation().loopEndBeat(), false); }
    else {
        double f, t;
        if (tracksPane_ && tracksPane_->timeSelection(f, t)) host_.automation().setLoop(f, t, true);
        else if (host_.automation().loopEndBeat() > host_.automation().loopStartBeat())
            host_.automation().setLoop(host_.automation().loopStartBeat(), host_.automation().loopEndBeat(), true);
        else host_.automation().setLoop(0.0, host_.automation().meterMap().barStart(4), true);
    }
    if (tracksPane_) tracksPane_->repaint();
}

}
