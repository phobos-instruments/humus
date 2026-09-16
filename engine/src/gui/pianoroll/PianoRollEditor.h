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
    struct Playhead { double tick = -1.0; bool preview = false; };
    Playhead playheadForTest() const { return playhead(); }
    juce::String barsTextForTest() const { return bars_.getText(); }
    struct Params { std::string bars, swing, swingFollow, swingUnit; };

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
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;

    juce::Point<int> pointForTest(int tick, int pitch) const {
        return {(int) tickToX(tick), (int) pitchToY(pitch) + kRowH / 2};
    }
    int topPitchForTest() const { return topPitch_; }
    int selectionCountForTest() const { return (int) selection_.size(); }
    int paintedNoteCountForTest() const {
        return (int) (gestureEditsNotes() ? gestureNotes_.size() : notes().size());
    }

    void selectAllNotes();
    void printGrooveToSelection();

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
    double ppt() const;
    float tickToX(double tick) const { return (float) (gridLeft() + tick * ppt()); }
    int xToTick(float x) const { return (int) std::lround((x - gridLeft()) / ppt()); }
    int pitchAt(int y) const { return topPitch_ - (y - gridTop()) / kRowH; }
    float pitchToY(int pitch) const { return (float) (gridTop() + (topPitch_ - pitch) * kRowH); }
    int snapTicks() const;
    int snapTick(int tick) const;

    std::vector<NoteEvent> notes() const { return host_.clips().notes(name_, clip_); }
    void commit(const std::vector<NoteEvent>& notes);
    double playheadClipTick() const;
    Playhead playhead() const;
    juce::Rectangle<float> noteBounds(const NoteEvent& e) const {
        const float x = tickToX(e.tick);
        return {x, pitchToY(e.pitch),
                juce::jmax(3.0f, tickToX(e.tick + e.lengthTicks) - x - 1.0f),
                (float) (kRowH - 1)};
    }
    int noteAt(int tick, int pitch, noteedit::Grab& grab, int x) const;

    void fitPitch();
    void buildToolbar();
    void timerCallback() override;

    void loopTap();
    void loopClear();
    LooperState loopState() const {
        return looperState(host_.midi().isRecordTarget(name_), loopTakeOpen_,
                           !notes().empty());
    }
    void updateLoopButton();

    void nudgeSelection(int dTicks, int dSemis, int dVel);
    void deleteSelection();
    void copySelection(bool cut);
    void pasteClipboard();
    void duplicateSelection();
    void splitNoteAt(int noteIndex, int atTick);
    void applyMarquee(juce::Rectangle<int> area, bool additive);

    void paintKeys(juce::Graphics& g);
    void repaintKeys() { repaint(0, gridTop(), kKeyW, gridBottom() - gridTop()); }
    void paintRuler(juce::Graphics& g);
    void paintGrid(juce::Graphics& g);
    void paintNotes(juce::Graphics& g);
    void paintVelocity(juce::Graphics& g);
    void paintCCLane(juce::Graphics& g, int top, int h);
    void applyVelocityLane(juce::Point<int> p);
    void applyCCLane(juce::Point<int> p);
    void showLaneMenu();

    BrickHost& host_;
    std::string name_;
    Params params_;

    struct LoopButton : juce::TextButton {
        using juce::TextButton::TextButton;
        std::function<void()> onRightClick;
        void mouseDown(const juce::MouseEvent& e) override {
            if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
            juce::TextButton::mouseDown(e);
        }
    };

    void showBars(int bars);

    juce::ComboBox bars_, snap_;
    LoopButton loop_{"Loop"};
    bool loopTakeOpen_ = false;
    juce::TextButton record_{"Rec"}, quantize_{"Q"};
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
            g.setColour(on ? Palette::accent : Palette::textDim);
            timelinechrome::paintToolIcon(g, b.reduced(5.0f, 4.0f), tool);
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

    Tool tool_ = Tool::Pointer;
    std::set<int> selection_;
    bool nudgeOpen_ = false;
    juce::Rectangle<int> marquee_;
    static std::vector<NoteEvent> sharedClipboard_;

    int topPitch_ = 83;
    noteedit::WheelAccum pitchWheel_;
    bool pitchScrolled_ = false;
    int clip_ = 0;
    double lastPlayheadTick_ = -1.0;
    int loopTick_ = 0;

    enum class Gesture { None, Create, Move, MoveGroup, Resize, ResizeL, Velocity,
                         Marquee, Erase, VelLane, CCLane, Keys } gesture_ = Gesture::None;
    bool gestureEditsNotes() const {
        return gesture_ != Gesture::None && gesture_ != Gesture::Marquee
            && gesture_ != Gesture::CCLane;
    }
    std::vector<NoteEvent> gestureNotes_;
    std::vector<NoteEvent> gestureBase_;
    std::vector<CCEvent> gestureCCs_;
    int laneCC_ = -1;
    int gestureIndex_ = -1;
    int gestureStartTick_ = 0, gestureStartPitch_ = 0, gestureStartVel_ = 100;
    int gestureTickOffset_ = 0;
    juce::Point<int> dragStart_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};

}
