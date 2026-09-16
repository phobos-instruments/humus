// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/app/MainComponent.h"
#include "gui/properties/PropertiesPane.h"
#include <cmath>
#include "gui/app/AppSettings.h"
#include "gui/style/LookAndFeel.h"
#include "gui/host/NodeRoll.h"
#include "gui/app/QwertyPiano.h"
#include "gui/common/Localisation.h"

namespace hum {

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
    if (targets.empty()) { setStatus(tr("main-transport.nothing-here-answers-to-the", "nothing here answers to the dice")); return; }
    host_.pushParamStep();
    auto& hist = host_.paramHistory();
    for (const auto& n : targets) {
        hist.commit(n, host_.captureNodeState(n));
        randomizeNode(host_, n,false);
        hist.commit(n, host_.captureNodeState(n));
    }
    for (const auto& n : targets) propsPane_->reloadValuesFor(n);
    setStatus(tr("main-transport.rolled", "rolled ") + juce::String((int) targets.size()) + tr("main-transport.organisms", " organisms"));
}

void MainComponent::showMetroMenu() {
    auto& s = AppSettings::instance();
    const int vol = s.getInt("metro.volume", 100);
    const bool ci = host_.beforeRecord() == EngineHost::BeforeRecord::CountIn;
    juce::PopupMenu m;
    m.addSectionHeader(tr("main-transport.metronome", "Metronome"));
    for (int v : {25, 50, 75, 100})
        m.addItem(v, tr("main-transport.volume", "Volume ") + juce::String(v) + "%", true, vol == v);
    m.addSeparator();
    m.addItem(200, tr("main-transport.count-in-before-recording", "Count-in before recording (right-click the record button for bars and pre-roll)"), true, ci);
    const auto anchor = metroBtn_.getScreenBounds();
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(anchor),
                    [this](int r) {
        auto& st = AppSettings::instance();
        if (r >= 25 && r <= 100) {
            st.set("metro.volume", r);
            host_.setMetronomeGain((float) r / 100.0f);
        } else if (r == 200) {
            const bool on = host_.beforeRecord() != EngineHost::BeforeRecord::CountIn;
            host_.setBeforeRecord(on ? EngineHost::BeforeRecord::CountIn : EngineHost::BeforeRecord::None,
                                  host_.beforeRecordBars());
            st.set("record.before", (int) host_.beforeRecord());
            setStatus(on ? tr("main-transport.count-in-on", "count-in on: recording clicks ")
                               + juce::String(host_.beforeRecordBars())
                               + tr("main-transport.count-in-bars-first", " bar(s) first")
                         : tr("main-transport.count-in-off", "count-in off"));
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
        m.addSectionHeader(t.isEmpty() ? juce::String(tr("main-transport.audio-health-idle", "Audio health: idle")) : tr("main-transport.audio-health-prefix", "Audio health: ") + t);
    }
    add(1, goStartBtn_, tr("main-transport.go-to-start", "Go to Start"));
    add(2, goEndBtn_, tr("main-transport.go-to-end", "Go to End"));
    add(3, loopBtn_, tr("main-transport.automation-loop", "Automation Loop"), host_.automation().loopEnabled());
    m.addSeparator();
    add(4, tapBtn_, tr("main-transport.tap-tempo-2", "Tap Tempo"));
    add(5, beat1Btn_, tr("main-transport.mark-beat-1", "Mark Beat 1"));
    add(6, metroBtn_, "Click", metroBtn_.getToggleState());
    add(7, linkBtn_, "Link", linkBtn_.getToggleState());
    m.addSeparator();
    add(8, qwertyBtn_, tr("main-transport.computer-keyboard-notes", "Computer Keyboard Notes"), QwertyPiano::instance().enabled());
    m.addSeparator();
    add(9, globalDiceBtn_, tr("main-transport.roll-everything", "Roll Everything"));
    add(10, keepBtn_, tr("main-transport.keep-the-last-8-bars", "Keep the Last 8 Bars"));
    add(11, limiterBtn_, tr("main-transport.master-limiter", "Master Limiter"), host_.limiterEnabled());
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
            case 9: globalDiceBtn_.triggerClick(); break;
            case 10: keepBtn_.triggerClick(); break;
            case 11: limiterBtn_.triggerClick(); break;
            default: break;
        }
    });
}

void MainComponent::showLinkMenu() {
    juce::PopupMenu m;
    m.addSectionHeader(tr("main-transport.ableton-link", "Ableton Link"));
    m.addItem(1, tr("main-transport.sync-start-stop-with-peers", "Sync start/stop with peers"), true, host_.linkStartStopSync());
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

void MainComponent::showGrooveMenu(const std::string& param, juce::Point<int> screen) {
    showAutomateMenu(host_, host_.clockNodeName(), param, screen,
                     [this] { refreshTimelinePanes(); });
}

}
