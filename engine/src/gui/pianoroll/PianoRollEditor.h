// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/pianoroll/RollModel.h"
#include "gui/tracks/TimelineTools.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostClips.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/tracks/LooperFlow.h"
#include "gui/bricks/MidiKeyboardPanel.h"

namespace hum {

class PianoRollEditor : public OrganismEditor, private juce::Timer {
public:
    using Playhead = roll::Playhead;
    Playhead playheadForTest() const { return playhead(); }
    juce::String barsTextForTest() const { return bars_.getText(); }
    struct Params { std::string bars, swing, swingFollow, swingUnit, record, loop, quantize; };

    PianoRollEditor(BrickHost& host, std::string organism, Params params);
    ~PianoRollEditor() override;

    void reloadValues() override;
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 640; }
    int preferredContentHeight(int width) const override;

    void setClip(int clip);
    void openClip(int clip) override { setClip(clip); }
    int clip() const { return clip_; }

    using Tool = noteedit::Tool;
    void setTool(Tool t);

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;

    juce::Point<int> pointForTest(int tick, int pitch) const {
        return {(int) geometry().tickToX(tick), (int) geometry().pitchToY(pitch) + kRowH / 2};
    }
    int paintedNoteCountForTest() const {
        return (int) (model_.editsNotes() ? model_.gestureNotes().size() : model_.notes().size());
    }

    void selectAllNotes();
    void printGrooveToSelection();
    void quantiseSelection(int gridTicks);

    int selectedNotesColour() const;
    void colourSelectedNotes(int colour, bool asUndoStep = true);
    int selectedCCsColour() const;
    void colourSelectedCCs(int colour, bool asUndoStep = true);
    void deleteSelectedCCs();

    roll::RollModel& modelForTest() { return model_; }
    juce::Point<int> ccPointForTest(const CCEvent& c) const {
        return {(int) geometry().tickToX(c.tick), (int) geometry().ccY(c.value, model_.laneCC)};
    }

private:
    friend class PianoRollRecordBridge;
    static constexpr int kToolbarH = 30, kRulerH = 18, kKeyW = 72, kRowH = 13;
    static constexpr int kVisibleRows = 20;
    static constexpr int kKbH = 0;
    static constexpr int kVelH = 42;

    int gridTop() const { return kToolbarH + kRulerH; }
    int gridBottom() const { return getHeight() - kKbH - kVelH; }
    int gridLeft() const { return kKeyW; }
    int durationTicks() const;
    static constexpr int kSnapFreeId = 5;
    int snapTicks() const;
    bool snapFree() const { return snap_.getSelectedId() == kSnapFreeId; }
    roll::Geometry geometry() const;
    class Clip : public roll::ClipNotes {
    public:
        explicit Clip(PianoRollEditor& editor) : editor_(editor) {}
        std::vector<NoteEvent> notes() const override;
        void setNotes(const std::vector<NoteEvent>& notes, int durationTicks) override;
        std::vector<CCEvent> ccs() const override;
        void setCCs(const std::vector<CCEvent>& ccs) override;
        void pushUndo() override;

    private:
        PianoRollEditor& editor_;
    };

    std::vector<roll::ClipSpan> clipSpans() const;
    double playheadClipTick() const;
    Playhead playhead() const;

    void fitPitch();
    void buildToolbar();
    void timerCallback() override;

    void buildRecordButtons();
    void loopTap();
    void showLoopMenu();
    LooperState loopState() const {
        return looperState(host_.midi().isRecordTarget(name_), host_.midi().loopTakeOpen(name_),
                           !model_.notes().empty());
    }
    void updateLoopButton();
    void syncRecordButtons();

    void nudgeSelection(int dTicks, int dSemis, int dVel);
    void deleteSelection();
    void copySelection(bool cut);
    void pasteClipboard();
    void duplicateSelection();
    void splitNoteAt(int noteIndex, int atTick);
    juce::Rectangle<int> marqueeRect() const { return toJuce(model_.marquee()); }

    void paintKeys(juce::Graphics& g);
    void repaintKeys() { repaint(0, gridTop(), kKeyW, gridBottom() - gridTop()); }
    void paintRuler(juce::Graphics& g);
    void paintGrid(juce::Graphics& g);
    void paintNotes(juce::Graphics& g);
    void paintVelocity(juce::Graphics& g);
    void paintCutGuide(juce::Graphics& g);
    void trackHover(juce::Point<int> p);
    bool cutGuideAt(juce::Point<int> p) const;
    float cutGuideX(int hoverX) const;
    void paintCCLane(juce::Graphics& g, int top, int h);
    void showLaneMenu();
    void showNoteMenu(int hit);
    void openColourPicker(bool ccs);

    BrickHost& host_;
    std::string name_;
    Params params_;

    struct MenuButton : juce::TextButton {
        using juce::TextButton::TextButton;
        std::function<void()> onRightClick;
        void mouseDown(const juce::MouseEvent& e) override {
            if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
            juce::TextButton::mouseDown(e);
        }
    };

    void showBars(int bars);

    juce::ComboBox bars_, snap_;
    MenuButton loop_{"Loop"}, record_{"Rec"}, quantize_{"Q"};
    LooperState shownLoop_ = LooperState::Empty;
    struct ToolButton : juce::Button {
        explicit ToolButton(noteedit::Tool t) : juce::Button({}), tool(t) {}
        void paintButton(juce::Graphics& g, bool hover, bool) override {
            const auto b = getLocalBounds().toFloat().reduced(0.5f);
            const bool on = getToggleState();
            g.setColour(on ? Palette::accent.withAlpha(alpha::scrim)
                           : hover ? Palette::panel.brighter(0.15f) : Palette::panel);
            g.fillRoundedRectangle(b, 3.0f);
            g.setColour(on ? Palette::accent : Palette::border);
            g.drawRoundedRectangle(b, 3.0f, 1.0f);
            timelinechrome::paintToolIcon(g, b.reduced(3.5f, 3.0f), tool,
                                          on ? Palette::accent : Palette::textDim);
        }
        noteedit::Tool tool;
    };
    ToolButton toolP_{Tool::Pointer}, toolD_{Tool::Draw},
               toolS_{Tool::Scissors}, toolE_{Tool::Eraser};
    juce::Label barsLabel_{{}, "bars"}, snapLabel_{{}, "snap"};
    MidiKeyboardStrip keyboard_;
    int keyNote_ = -1;
    void soundKey(int pitch);
    void releaseKey();

    int clip_ = 0;
    Clip clipNotes_{*this};
    roll::RollModel model_{clipNotes_};
    noteedit::WheelAccum pitchWheel_;
    juce::Point<int> hover_{-1, -1};
    double lastPlayheadTick_ = -1.0;
    int loopTick_ = 0;

    using Gesture = roll::RollModel::Gesture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};

}
