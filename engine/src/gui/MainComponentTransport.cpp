#include "gui/MainComponent.h"

#include <cmath>

#include "gui/AppSettings.h"
#include "gui/LookAndFeel.h"
#include "gui/NodeRandomize.h"
#include "gui/QwertyPiano.h"

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
    addAndMakeVisible(recordBtn_);        recordBtn_.onClick = [this] { host_.record().captureToggle(); };
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
        m.addItem(1, "Touch: write only while a control is held", true, !host_.latchMode());
        m.addItem(2, "Latch: keep writing the last value until stop", true, host_.latchMode());
        m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                        [this](int r) {
            if (r == 0) return;
            host_.setLatchMode(r == 2);
            AppSettings::instance().set("automation.latch", r == 2 ? 1 : 0);
        });
    };
    addAndMakeVisible(keepBtn_);
    keepBtn_.onClick = [this] {
        host_.keepLast(8);
        setStatus("kept the last 8 bars");
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
        setStatus(qp.enabled() ? "virtual keyboard enabled" : "virtual keyboard disabled");
    };

    tempo_.setTextValueSuffix(" BPM");
    addAndMakeVisible(tempo_);
    tempo_.setSliderStyle(juce::Slider::IncDecButtons);
    tempo_.setRange(kTempoMin, kTempoMax, 0.1);
    tempo_.setValue(host_.tempo(), juce::dontSendNotification);
    tempo_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 56, 22);
    tempo_.setTooltip(juce::String::fromUTF8(
        "Tempo in beats per minute - drag the number, or right-click to "
        "automate and map it"));
    tempo_.onValueChange = [this] { host_.setTempo(tempo_.getValue()); };
    tempo_.onPopup = [this](juce::Point<int> screen) { showTempoMenu(screen); };

    addAndMakeVisible(tsig_);
    tsig_.get = [this] { return host_.automation().timeSigNumerator(); };
    tsig_.set = [this](int n) {
        host_.automation().setTimeSignature(n, 4);
        refreshTimelinePanes();
    };

    addAndMakeVisible(tapBtn_);
    tapBtn_.setTooltip(juce::String::fromUTF8(
        "Tap the tempo - two or more taps set the BPM (pause 2 s to start over)"));
    tapBtn_.onClick = [this] {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (!tapTimesMs_.empty() && now - tapTimesMs_.back() > 2000.0) tapTimesMs_.clear();
        tapTimesMs_.push_back(now);
        if (tapTimesMs_.size() > 9) tapTimesMs_.erase(tapTimesMs_.begin());
        if (tapTimesMs_.size() < 2) { setStatus("tap tempo: keep tapping..."); return; }
        const double ms = (tapTimesMs_.back() - tapTimesMs_.front())
                          / (double) (tapTimesMs_.size() - 1);
        const double bpm = juce::jlimit(20.0, 999.0, 60000.0 / ms);
        host_.setTempo(bpm);
        tempo_.setValue(bpm, juce::dontSendNotification);
        setStatus("tap tempo: " + juce::String(bpm, 1) + " BPM ("
                  + juce::String((int) tapTimesMs_.size()) + " taps)");
    };
    addAndMakeVisible(beat1Btn_);
    beat1Btn_.setTooltip(juce::String::fromUTF8(
        "Mark now as beat 1 - re-phases the bars without changing the tempo"));
    beat1Btn_.onClick = [this] {
        const double perBar = 4.0;
        const double pos = host_.positionBeats();
        host_.setPositionBeats(std::max(0.0, std::round(pos / perBar) * perBar));
        setStatus("beat 1 marked");
    };
    addAndMakeVisible(metroBtn_);
    metroBtn_.setClickingTogglesState(true);
    metroBtn_.setTooltip(juce::String::fromUTF8(
        "Metronome - a tick every beat, brighter on beat 1"
        " (right-click: volume + count-in)"));
    metroBtn_.setColour(juce::TextButton::buttonOnColourId, Palette::accent.withAlpha(0.55f));
    metroBtn_.onClick = [this] { host_.setMetronome(metroBtn_.getToggleState()); };
    metroBtn_.addMouseListener(&metroMenu_, false);
    host_.setMetronomeGain((float) AppSettings::instance().getInt("metro.volume", 100) / 100.0f);
    host_.setCountIn(AppSettings::instance().getInt("metro.countin", 0) != 0);

    addAndMakeVisible(linkBtn_);
    linkBtn_.setClickingTogglesState(true);
    linkBtn_.setTooltip(juce::String::fromUTF8(
        "Ableton Link - lock tempo and bar phase with other"
        " Link-enabled apps and devices on this network. The label counts"
        " connected peers (right-click: start/stop sync)."));
    linkBtn_.setColour(juce::TextButton::buttonOnColourId, Palette::accent.withAlpha(0.55f));
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
    masterCaption_.setText("Out", juce::dontSendNotification);
    masterCaption_.setColour(juce::Label::textColourId, Palette::textDim);
    masterCaption_.setFont(juce::FontOptions(11.0f));
    masterCaption_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(masterLevel_);
    masterLevel_.setRange(0.0, 1.0, 0.0);
    masterLevel_.setValue(host_.outputGain(), juce::dontSendNotification);
    masterLevel_.setTooltip("Master output level");
    masterLevel_.paramLabel = "Master out";
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
    addAndMakeVisible(grooveBtn_);
    grooveBtn_.setTooltip(juce::String::fromUTF8(
        "Groove - how far the offbeats of the patch arrive late. Every"
        " sequencer set to Follow takes this, so one setting swings the whole"
        " patch; turn Follow off on an organism to keep it straight or give it"
        " its own. 100% is a full triplet feel. Saved with the patch."));
    grooveBtn_.onClick = [this] { showGrooveMenu(); };
    grooveBtn_.setColour(juce::TextButton::buttonColourId, Palette::panel);
    grooveBtn_.setColour(juce::TextButton::textColourOffId, Palette::textDim);
    grooveBtn_.getProperties().set("flatPill", true);
    refreshGrooveButton();
    addAndMakeVisible(meter_);
    addChildComponent(overflowBtn_);
    overflowBtn_.onClick = [this] { showOverflowMenu(); };
    addAndMakeVisible(dspLabel_);
    dspLabel_.setColour(juce::Label::textColourId, Palette::textDim);
    dspLabel_.setFont(juce::FontOptions(11.0f));
    dspLabel_.setJustificationType(juce::Justification::centredRight);
    dspLabel_.setTooltip(juce::String("Audio health: DSP load (callback time vs. its real-time "
                                      "deadline, peak-held) and, after a \"!\", blocks dropped "
                                      "to engine contention. ")
                         + juce::String::fromUTF8("Any dropout is an audible glitch - "
                                                  "report it if you hear one."));
}

void MainComponent::toggleAudio() {
    if (host_.audioRunning() || host_.audioStarting()) {
        host_.stopAudio();
        setStatus("audio disabled");
        AppSettings::instance().set("audio.enabled", 0);
        return;
    }
    setStatus(juce::String::fromUTF8("starting audio\xe2\x80\xa6"));
    host_.startAudioAsync([safe = juce::Component::SafePointer<MainComponent>(this)]
                          (bool ok, std::string err) {
        if (safe == nullptr) return;
        safe->setStatus(ok ? "audio enabled"
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
        setStatus("MIDI disabled");
    } else if (host_.midi().setEnabled(true)) {
        s.set("midi.enabled", 1);
        setStatus("MIDI enabled");
    } else {
        host_.midi().setEnabled(false);
        s.set("midi.enabled", 1);
        setStatus("no MIDI input devices found");
    }
}

void MainComponent::ensureAudio() {
    if (host_.audioRunning() || host_.audioStarting()) return;
    setStatus(juce::String::fromUTF8("starting audio\xe2\x80\xa6"));
    host_.startAudioAsync([safe = juce::Component::SafePointer<MainComponent>(this)]
                          (bool ok, std::string err) {
        if (safe == nullptr) return;
        if (ok) safe->setStatus("audio enabled");
        else if (err != "cancelled")
            safe->setStatus("no audio device (" + juce::String(err) + ")");
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

void MainComponent::showTempoMenu(juce::Point<int> screen) {
    showAutomateMenu(host_, host_.clockNodeName(), kTempoParam, screen,
                     [this] { refreshTimelinePanes(); }, true,
                     [this](bool on) {
                         host_.automation().setTempoAutomated(on);
                         setStatus(on ? juce::String::fromUTF8(
                                            "tempo lane added - draw it in the "
                                            "Automation pane")
                                      : juce::String("tempo automation removed"));
                     });
}

void MainComponent::globalRoll() {
    std::vector<std::string> targets;
    for (const auto& c : host_.model().organisms)
        if (nodeSupportsRandom(host_, c.name)) targets.push_back(c.name);
    if (targets.empty()) { setStatus("nothing here answers to the dice"); return; }
    host_.pushParamStep();
    auto& hist = host_.paramHistory();
    for (const auto& n : targets) {
        hist.commit(n, host_.captureNodeState(n));
        randomizeNode(host_, n,false);
        hist.commit(n, host_.captureNodeState(n));
    }
    for (const auto& n : targets) propsPane_->reloadValuesFor(n);
    setStatus("rolled " + juce::String((int) targets.size()) + " organisms");
}

void MainComponent::toggleLoop() {
    if (host_.automation().loopEnabled()) { host_.automation().setLoop(host_.automation().loopStartBeat(), host_.automation().loopEndBeat(), false); }
    else {
        double f, t;
        if (tracksPane_ && tracksPane_->timeSelection(f, t)) host_.automation().setLoop(f, t, true);
        else if (host_.automation().loopEndBeat() > host_.automation().loopStartBeat())
            host_.automation().setLoop(host_.automation().loopStartBeat(), host_.automation().loopEndBeat(), true);
        else host_.automation().setLoop(0.0, 4.0 * host_.automation().timeSigNumerator(), true);
    }
    if (tracksPane_) tracksPane_->repaint();
}

void MainComponent::showMetroMenu() {
    auto& s = AppSettings::instance();
    const int vol = s.getInt("metro.volume", 100);
    const bool ci = s.getInt("metro.countin", 0) != 0;
    juce::PopupMenu m;
    m.addSectionHeader("Metronome");
    for (int v : {25, 50, 75, 100})
        m.addItem(v, "Volume " + juce::String(v) + "%", true, vol == v);
    m.addSeparator();
    m.addItem(200, "Count-in: one bar before play", true, ci);
    const auto anchor = metroBtn_.getScreenBounds();
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(anchor),
                    [this](int r) {
        auto& st = AppSettings::instance();
        if (r >= 25 && r <= 100) {
            st.set("metro.volume", r);
            host_.setMetronomeGain((float) r / 100.0f);
        } else if (r == 200) {
            const bool on = st.getInt("metro.countin", 0) == 0;
            st.set("metro.countin", on ? 1 : 0);
            host_.setCountIn(on);
            setStatus(on ? "count-in on: play clicks one bar first" : "count-in off");
        }
    });
}

void MainComponent::showOverflowMenu() {
    juce::PopupMenu m;
    auto add = [&m](int id, juce::Button& b, const juce::String& label, bool ticked = false) {
        if (b.isVisible()) return;
        m.addItem(id, label, b.isEnabled(), ticked);
    };
    if (!dspLabel_.isVisible()) {
        const auto t = dspLabel_.getText();
        m.addSectionHeader(t.isEmpty() ? juce::String("Audio health: idle") : "Audio health: " + t);
    }
    add(1, goStartBtn_, "Go to Start");
    add(2, goEndBtn_, "Go to End");
    add(3, loopBtn_, "Automation Loop", host_.automation().loopEnabled());
    m.addSeparator();
    add(4, tapBtn_, "Tap Tempo");
    add(5, beat1Btn_, "Mark Beat 1");
    add(6, metroBtn_, "Click", metroBtn_.getToggleState());
    add(7, linkBtn_, "Link", linkBtn_.getToggleState());
    m.addSeparator();
    add(8, qwertyBtn_, "Computer Keyboard Notes", QwertyPiano::instance().enabled());
    m.showMenuAsync(juce::PopupMenu::Options()
                        .withTargetScreenArea(overflowBtn_.getScreenBounds()),
                    [this](int r) {
        switch (r) {
            case 1: goStartBtn_.triggerClick(); break;
            case 2: goEndBtn_.triggerClick(); break;
            case 3: loopBtn_.triggerClick(); break;
            case 4: tapBtn_.triggerClick(); break;
            case 5: beat1Btn_.triggerClick(); break;
            case 6: metroBtn_.triggerClick(); break;
            case 7: linkBtn_.triggerClick(); break;
            case 8: qwertyBtn_.triggerClick(); break;
            default: break;
        }
    });
}

void MainComponent::showLinkMenu() {
    juce::PopupMenu m;
    m.addSectionHeader("Ableton Link");
    m.addItem(1, "Sync start/stop with peers", true, host_.linkStartStopSync());
    const auto anchor = linkBtn_.getScreenBounds();
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(anchor),
                    [this](int r) {
        if (r != 1) return;
        const bool on = !host_.linkStartStopSync();
        host_.setLinkStartStopSync(on);
        setStatus(on ? juce::String::fromUTF8(
                           "start/stop shared - enable it in the other app too"
                           " (Live: Preferences \xe2\x80\xa3 Link \xe2\x80\xa3 Start Stop Sync)")
                     : "start/stop stays local");
    });
}

void MainComponent::refreshGrooveButton() {
    const double g = host_.groove();
    grooveBtn_.setButtonText(g <= 0.0
                                 ? "Groove"
                                 : "Groove " + juce::String((int) std::lround(g * 100.0)) + "%");
    grooveBtn_.setColour(juce::TextButton::textColourOffId,
                         g > 0.0 ? Palette::accent : Palette::textDim);
    grooveBtn_.repaint();
}

void MainComponent::showGrooveMenu() {
    juce::PopupMenu m;
    const double now = host_.groove();
    const auto unit = host_.grooveUnit();
    m.addSectionHeader("Groove");
    for (int pct : {0, 20, 40, 50, 60, 75, 100}) {
        const double v = pct / 100.0;
        m.addItem(100 + pct,
                  pct == 0 ? juce::String("Straight")
                           : juce::String(pct) + "%" + (pct == 100 ? " (triplet feel)" : ""),
                  true, std::abs(now - v) < 1e-6);
    }
    m.addSeparator();
    juce::PopupMenu grid;
    grid.addItem(1, "1/8", true, unit == "1/8");
    grid.addItem(2, "1/16", true, unit == "1/16");
    m.addSubMenu("Grid", grid);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&grooveBtn_),
                    [this](int r) {
                        if (r <= 0) return;
                        if (r == 1 || r == 2)
                            host_.setGroove(host_.groove(), r == 1 ? "1/8" : "1/16");
                        else
                            host_.setGroove((r - 100) / 100.0, host_.grooveUnit());
                        refreshGrooveButton();
                    });
}

}
