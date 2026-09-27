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
        if (rolling) return;
        host_.stopPlayingFiles();
        host_.goToStart();
    };
    addAndMakeVisible(recordBtn_);        recordBtn_.onClick = [this] {
        host_.record().captureToggle();
        if (host_.record().preRolling())
            notifyOn("record", tr("main-transport.pre-roll-status", "pre-roll: ") + juce::String(host_.beforeRecordBars())
                      + tr("main-transport.pre-roll-status-tail", " bar(s) of the song, recording from bar ")
                      + juce::String(host_.automation().meterMap().barAt(host_.punchInBeat())));
        else if (host_.countInRunning())
            notifyOn("record", tr("main-transport.count-in-status", "count-in: ") + juce::String(host_.beforeRecordBars())
                      + tr("main-transport.count-in-status-tail", " bar(s) of click, then recording"));
    };
    const auto mapAction = [this](auto& btn, const char* action) {
        btn.onRightClick = [this, action](juce::Point<int> at) {
            showAutomateMenu(host_, host_.clockNodeName(), action, at,
                             [this] { refreshTimelinePanes(); }, false);
        };
        mappedTransport_.push_back({&btn, action});
    };
    addAndMakeVisible(panicBtn_);         panicBtn_.onClick = [this] { host_.panic(); };
    mapAction(panicBtn_, kPanicAction);
    mapAction(playFromStartBtn_, kPlayFromStartAction);
    mapAction(playBtn_, kPlayAction);
    mapAction(stopBtn_, kStopAction);
    mapAction(goStartBtn_, kGoToStartAction);
    mapAction(goEndBtn_, kGoToEndAction);
    mapAction(loopBtn_, kLoopToggleAction);
    mappedTransport_.push_back({&recordBtn_, kCaptureAction});
    mappedTransport_.push_back({&globalDiceBtn_, kRandomAction});
    recordBtn_.onRightClick = [this](juce::Point<int> at) {
        const auto mode = host_.beforeRecord();
        const int bars = host_.beforeRecordBars();
        auto barsText = [](int n) { return juce::String(n) + (n == 1 ? tr("main-transport.bar", " bar") : tr("main-transport.bars", " bars")); };
        const int first = kAutomateMenuFirstOwnId;
        juce::PopupMenu countIn, preRoll;
        for (int n : {1, 2, 4}) {
            countIn.addItem(first + 10 + n, barsText(n), true,
                            mode == EngineHost::BeforeRecord::CountIn && bars == n);
            preRoll.addItem(first + 20 + n, barsText(n), true,
                            mode == EngineHost::BeforeRecord::PreRoll && bars == n);
        }
        juce::PopupMenu before;
        before.addItem(first + 10, tr("main-transport.start-right-away", "Start right away"), true,
                       mode == EngineHost::BeforeRecord::None);
        before.addSubMenu(tr("main-transport.count-in-click", "Count-in with the click"), countIn, true,
                          juce::Image(), mode == EngineHost::BeforeRecord::CountIn);
        before.addSubMenu(tr("main-transport.pre-roll-song", "Pre-roll the song"), preRoll, true,
                          juce::Image(), mode == EngineHost::BeforeRecord::PreRoll);
        juce::PopupMenu write;
        write.addItem(first + 1, tr("main-transport.touch", "Touch: only while a control is held"), true,
                      !host_.latchMode());
        write.addItem(first + 2, tr("main-transport.latch", "Latch: keep the last value until stop"), true,
                      host_.latchMode());
        juce::PopupMenu head;
        head.addSubMenu(tr("main-transport.before-recording", "Before recording"), before);
        head.addSubMenu(tr("main-transport.automation-write", "Automation write"), write);
        showAutomateMenu(host_, host_.clockNodeName(), kCaptureAction, at,
                         [this] { refreshTimelinePanes(); }, false, {}, std::move(head),
                         [this, first](int r) {
            const int pick = r - first;
            if (pick == 1 || pick == 2) {
                host_.setLatchMode(pick == 2);
                AppSettings::instance().set("automation.latch", pick == 2 ? 1 : 0);
                return;
            }
            const auto chosen = pick == 10 ? EngineHost::BeforeRecord::None
                              : pick < 20 ? EngineHost::BeforeRecord::CountIn
                                          : EngineHost::BeforeRecord::PreRoll;
            const int chosenBars = pick == 10 ? host_.beforeRecordBars() : pick % 10;
            host_.setBeforeRecord(chosen, chosenBars);
            AppSettings::instance().set("record.before", (int) chosen);
            AppSettings::instance().set("record.bars", chosenBars);
        });
    };
    addAndMakeVisible(keepBtn_);
    keepBtn_.onClick = [this] {
        host_.keepLast(8);
        notify(tr("main-transport.kept-the-last-8-bars", "kept the last 8 bars"));
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
        notify(qp.enabled() ? tr("main-transport.virtual-keyboard-enabled", "virtual keyboard enabled") : tr("main-transport.virtual-keyboard-disabled", "virtual keyboard disabled"));
    };

    tempo_.setTextValueSuffix(" BPM");
    addAndMakeVisible(tempo_);
    tempo_.setSliderStyle(juce::Slider::IncDecButtons);
    tempo_.setRange(kTempoMin, kTempoMax, 0.1);
    tempo_.setValue(host_.tempo(), juce::dontSendNotification);
    tempo_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, TempoSlider::kNumberW, 22);
    tempo_.setTooltip(juce::String::fromUTF8("Tempo - drag to change"));
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
    };

    addAndMakeVisible(tapBtn_);
    tapBtn_.setTooltip(juce::String::fromUTF8("Tap tempo"));
    tapBtn_.onClick = [this] {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (!tapTimesMs_.empty() && now - tapTimesMs_.back() > 2000.0) tapTimesMs_.clear();
        tapTimesMs_.push_back(now);
        if (tapTimesMs_.size() > 9) tapTimesMs_.erase(tapTimesMs_.begin());
        if (tapTimesMs_.size() < 2) { notifyOn("tap", tr("main-transport.tap-tempo-keep-tapping", "tap tempo: keep tapping...")); return; }
        const double ms = (tapTimesMs_.back() - tapTimesMs_.front())
                          / (double) (tapTimesMs_.size() - 1);
        const double bpm = juce::jlimit(kTempoMin, kTempoMax, 60000.0 / ms);
        host_.performTempo(bpm);
        tempo_.setValue(bpm, juce::dontSendNotification);
        notifyOn("tap", tr("main-transport.tap-tempo", "tap tempo: ") + juce::String(bpm, 1) + tr("main-transport.bpm", " BPM (")
                  + juce::String((int) tapTimesMs_.size()) + " taps)");
    };
    addAndMakeVisible(beat1Btn_);
    beat1Btn_.setTooltip(juce::String::fromUTF8("Mark now as beat 1"));
    beat1Btn_.onClick = [this] {
        const double pos = host_.positionBeats();
        host_.setPositionBeats(std::max(0.0, host_.automation().meterMap().nearestBarStart(pos)));
        notify(tr("main-transport.beat-1-marked", "beat 1 marked"));
    };
    addAndMakeVisible(metroBtn_);
    metroBtn_.setClickingTogglesState(true);
    metroBtn_.setTooltip(juce::String::fromUTF8("Metronome"));
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
    linkBtn_.setTooltip(juce::String::fromUTF8("Ableton Link - the number is connected peers"));
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
    limiterBtn_.setTooltip(juce::String::fromUTF8("Master limiter - amber while it is working"));
    limiterBtn_.onClick = [this] { host_.setLimiter(limiterBtn_.getToggleState()); };
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
    dspLabel_.setTooltip(tr("main-transport.audio-load", "Audio load, and dropouts after the !"));
}

void MainComponent::toggleAudio() {
    if (host_.audioRunning() || host_.audioStarting()) {
        host_.stopAudio();
        notifyOn("audio", tr("main-transport.audio-disabled", "audio disabled"));
        AppSettings::instance().set("audio.enabled", 0);
        return;
    }
    notifyOn("audio", juce::String::fromUTF8("starting audio\xe2\x80\xa6"));
    host_.startAudioAsync([safe = juce::Component::SafePointer<MainComponent>(this)]
                          (bool ok, std::string err) {
        if (safe == nullptr) return;
        if (ok) safe->notifyOn("audio", tr("main-transport.audio-enabled", "audio enabled"));
        else if (err == "cancelled") safe->notifyOn("audio", tr("main-transport.audio-disabled", "audio disabled"));
        else safe->notifyErrorOn("audio", tr("main-transport.no-audio-device", "no audio device (") + juce::String(err) + ")");
        AppSettings::instance().set("audio.enabled", safe->host_.audioRunning() ? 1 : 0);
    });
}

void MainComponent::toggleMidi() {
    auto& s = AppSettings::instance();
    if (host_.midi().enabled()) {
        host_.midi().setEnabled(false);
        s.set("midi.enabled", 0);
        notify(tr("main-transport.midi-disabled", "MIDI disabled"));
    } else if (host_.midi().setEnabled(true)) {
        s.set("midi.enabled", 1);
        notify(tr("main-transport.midi-enabled", "MIDI enabled"));
    } else {
        host_.midi().setEnabled(false);
        s.set("midi.enabled", 1);
        notifyError(tr("main-transport.no-midi-input-devices-found", "no MIDI input devices found"));
    }
}

void MainComponent::ensureAudio() {
    if (host_.audioRunning() || host_.audioStarting()) return;
    notifyOn("audio", juce::String::fromUTF8("starting audio\xe2\x80\xa6"));
    host_.startAudioAsync([safe = juce::Component::SafePointer<MainComponent>(this)]
                          (bool ok, std::string err) {
        if (safe == nullptr) return;
        if (ok) safe->notifyOn("audio", tr("main-transport.audio-enabled", "audio enabled"));
        else if (err != "cancelled")
            safe->notifyErrorOn("audio", tr("main-transport.no-audio-device", "no audio device (") + juce::String(err) + ")");
    });
}

void MainComponent::stopTransport() {
    host_.stop();
    if (host_.record().armed()) host_.record().captureToggle();
}

void MainComponent::togglePlay() {
    if (host_.isPlaying()) stopTransport();
    else { ensureAudio(); host_.play(); }
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

void MainComponent::refreshTransportMarks() {
    const auto clock = host_.clockNodeName();
    for (const auto& [button, action] : mappedTransport_) {
        button->setMarks(paramIsControlled(host_, clock, action), false);
        button->setLit(host_.firedRecently(clock, action));
    }
}

}
