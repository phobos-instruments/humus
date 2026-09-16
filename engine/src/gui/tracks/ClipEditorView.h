// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <optional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostClips.h"
#include "gui/tracks/ClipDetail.h"
#include "gui/tracks/TimelineContext.h"
#include "gui/tracks/TimelineView.h"

namespace hum {

class ClipEditorView : public juce::Component, public juce::TooltipClient {
public:
    class Context : public TimelineContext {
    public:
        virtual int clipIndexOfId(const std::string& node, int id) const = 0;
        virtual void leaveClip() = 0;
        virtual void patchChanged() = 0;
        virtual void setClipboard(PatternChannel data, int spanTicks) = 0;
        virtual bool pasteClipboardInto(const std::string& node, int atTick) = 0;
    };

    ClipEditorView(Context& ctx, TimelineView& view) : ctx_(ctx), view_(view) { setOpaque(true); }

    void open(const std::string& node, int id);
    void close();
    const std::string& node() const { return node_; }
    int id() const { return id_; }
    int clipOrdinal() const;
    bool clipInfo(ClipEditor::ClipInfo& ci) const;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    juce::String getTooltip() override;
    bool keyPressed(const juce::KeyPress& k) override;

    bool clipSelectionTicks(int& from, int& to) const;
    void zoomToClip();
    void zoomToSelection();
    void splitClipSelection();
    bool deleteClipSelection(bool ripple);
    void trimClipToSelection();
    void copyClipSelection();
    bool pasteClipSelection(int atTick);
    bool tabToTransient(int dir);
    void splitAtTransients();
    void toggleClipReverse();
    void normalizeClip();
    void stretchClipBy(double factor);
    void setClipPitch(double semitones);
    int transientCount();
    void repaintCutGuide();

private:
    enum class Drag { None, Select, TrimL, TrimR, StretchL, StretchR, Slip, FadeL, FadeR,
                      CurveL, CurveR, Pending };
    enum class Hit { None, Body, EdgeL, EdgeR, FadeL, FadeR, CurveL, CurveR };

    class SelectionWatch {
    public:
        explicit SelectionWatch(ClipEditorView& v) : v_(v), was_(v.view_.sel) {}
        ~SelectionWatch();
    private:
        ClipEditorView& v_;
        TimeSelection was_;
    };

    TracksHost& host() const { return ctx_.timelineHost(); }
    juce::Point<int> toPane(juce::Point<int> p) const { return p + getPosition(); }
    float beatToX(double beat) const;
    double xToBeat(float x) const;
    float tickToX(int tick) const;
    double samplesPerBeat() const;
    juce::Rectangle<int> clipField() const;
    juce::Rectangle<int> clipBox() const;
    juce::Rectangle<int> ribbonBounds() const;
    Hit clipEditorHit(juce::Point<int> p) const;
    juce::MouseCursor clipEditorCursor(juce::Point<int> p) const;
    void splitClipAt(int tick);
    void straightenFade(bool in);
    void fadeClipToPlayhead(bool in);
    void setClipGainDb(double db);
    void loopClipSelection();
    void showClipDetailMenu(juce::Point<int> screenPos);
    bool mouseDownClip(const juce::MouseEvent& e, juce::Point<int> p);
    void mouseDragClip(const juce::MouseEvent& e);
    void mouseUpClip();
    bool keyPressedClip(const juce::KeyPress& k);
    void paintClip(juce::Graphics& g);
    void paintClipWave(juce::Graphics& g, juce::Rectangle<int> field, const ClipEditor::ClipInfo& ci,
                       juce::Colour accent);
    void paintClipRibbon(juce::Graphics& g, const ClipEditor::ClipInfo& ci);
    void paintCutGuide(juce::Graphics& g);
    std::vector<long long> clipTransients(const ClipEditor::ClipInfo& ci);
    long long sampleToTick(const ClipEditor::ClipInfo& ci, long long sessionSample) const;

    Context& ctx_;
    TimelineView& view_;
    std::string node_;
    int id_ = -1;
    Drag drag_ = Drag::None;
    std::optional<PatternSyncHold> sync_;
    double anchorBeat_ = 0.0;
    long long offset0_ = 0;
    int start0_ = 0, len0_ = 0;
    double curve0_ = 0.0;
    int curveY0_ = 0;
    clipdetail::SampleWindow window_;
    double sense_ = 0.5;
    juce::Point<int> hover_{-1, -1};
};

}
