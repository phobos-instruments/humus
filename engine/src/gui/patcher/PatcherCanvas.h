// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/graph/PodModel.h"
#include "gui/host/PatcherHost.h"
#include "gui/host/PodClip.h"
#include "gui/common/UiTicker.h"

namespace hum {

class PatcherCanvas : public juce::Component,
                      public juce::TooltipClient,
                      private juce::Timer {
public:
    explicit PatcherCanvas(PatcherHost& host) : host_(host) {
        setWantsKeyboardFocus(true);
        tickerId_ = UiTicker::instance().add([this] { timerCallback(); });
    }
    ~PatcherCanvas() override { UiTicker::instance().remove(tickerId_); stopTimer(); }

    std::function<void(const std::string&)> onSelect;
    std::function<void(const std::string&)> onActivate;
    std::function<void(const std::string&)> onOpenPluginUI;
    std::function<void(const std::string&)> onOpenVisuals;
    std::function<void(const std::string&)> onRecordNode;
    std::function<void(const std::string&, juce::Point<int>)> onAddRequest;
    std::function<void()> onUndoRedo;

    juce::Point<int> visibleCentreForNode() const {
        auto centre = getLocalBounds().getCentre();
        if (auto* vp = dynamic_cast<const juce::Viewport*>(getParentComponent()))
            centre = vp->getViewArea().getCentre();
        return {std::max(0, (int) (centre.x / zoom_) - kW / 2),
                std::max(0, (int) (centre.y / zoom_) - kH / 2)};
    }

    juce::Point<int> modelPos(const juce::MouseEvent& e) const {
        return modelPos(e.getPosition());
    }
    juce::Point<int> modelPos(juce::Point<int> viewPoint) const {
        return (viewPoint.toFloat() / zoom_).roundToInt();
    }
    juce::Point<int> viewPos(juce::Point<int> modelPoint) const {
        return (modelPoint.toFloat() * zoom_).roundToInt();
    }

    static constexpr float kZoomMin = 0.4f, kZoomMax = 2.0f;
    float zoom() const { return zoom_; }
    juce::Rectangle<int> nodeBoundsForTest(const std::string& n) { return nodeBounds(n); }
    void controlPortCountsForTest(const std::string& n, int& ins, int& outs) const { controlPortCounts(n, ins, outs); }
    void enterScopeForTest(const std::string& scope) { scope_ = scope; refresh(); }
    int dragHintsForTest(const std::string& node, bool control) {
        selection_ = {node};
        primary_ = node;
        updateDragHints();
        int n = 0;
        for (const auto& e : proxConnections_) n += e.control == control ? 1 : 0;
        return n;
    }
    void commitDragHintsForTest() { commitDragHints(); }
    juce::Rectangle<int> lastNodeRepaintForTest() const { return lastNodeRepaint_; }
    void timerCallbackForTest() { timerCallback(); }
    std::string hitNodeAtView(juce::Point<int> viewPoint) { return hitNode(modelPos(viewPoint)); }
    void setZoom(float z, juce::Point<int> anchor);
    void resetView();

    void refresh();
    void select(const std::string& n);
    const std::string& selected() const { return primary_; }
    bool hasSelection() const { return !selection_.empty(); }

    const std::string& scope() const { return scope_; }
    void enterPod(const std::string& pod);
    void videoPortCounts(const std::string& name, int& ins, int& outs) const;
    void exitToScope(const std::string& scope);

    void autoArrange();
    void renameSelection();
    void copySelection();
    void cutSelection();
    void pasteClipboard();
    void duplicateSelection();
    void deleteSelection();
    void selectAll();
    void undo();
    void redo();

    juce::String getTooltip() override;

    void paint(juce::Graphics&) override;
    void parentSizeChanged() override { updateContentSize(); }
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify(const juce::MouseEvent&, float) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;

    juce::Rectangle<int> nodeBounds(const std::string& name);
    void repaintNode(const std::string& name);
    juce::Rectangle<int> lastNodeRepaint_;

private:
    static constexpr int kW = 116, kH = 38;
    static constexpr int kPort = 7;
    static constexpr int kPortPad = 9;
    static constexpr int kPortGap = 5;
    static constexpr int kMidiGap = 10;
    static constexpr int kGrid = 8;

    struct Edge {
        std::string src; int srcOutlet; std::string dst; int dstInlet;
        bool midi = false;
        bool video = false;
        bool control = false;
        bool operator==(const Edge& o) const {
            return src == o.src && srcOutlet == o.srcOutlet && dst == o.dst
                && dstInlet == o.dstInlet && midi == o.midi && video == o.video
                && control == o.control;
        }
    };

    struct DisplayNode { std::string name; bool pod = false; };
    std::vector<DisplayNode> displayNodes() const;
    bool isPodBox(const std::string& name) const;
    void portCounts(const std::string& name, int& ins, int& outs) const;
    bool mapEndpoint(const std::string& node, int port, bool isDstSide, pods::Domain dom,
                     std::string& dispNode, int& dispPort) const;
    bool mapEndpoint(const std::string& node, int port, bool isDstSide, bool midi,
                     std::string& dispNode, int& dispPort) const {
        return mapEndpoint(node, port, isDstSide,
                           midi ? pods::Domain::Midi : pods::Domain::Audio, dispNode, dispPort);
    }
    void realPort(const std::string& dispNode, int port, bool isOutlet, pods::Domain dom,
                  std::string& realNode, int& realPort) const;
    void realPort(const std::string& dispNode, int port, bool isOutlet, bool midi,
                  std::string& realNode, int& realPortOut) const {
        realPort(dispNode, port, isOutlet, midi ? pods::Domain::Midi : pods::Domain::Audio,
                 realNode, realPortOut);
    }
    void showNewPodDialog(juce::Point<int> at);
    void podFromPatchDialog(juce::Point<int> at);
    void openQuickAdd(juce::Point<int> at, juce::Point<int> screenPos);
    void showPodMenu(const std::string& pod, juce::Point<int> screenPos);
    void renamePodDialog(const std::string& pod);

    void updateContentSize();
    juce::Point<int> viewOffset() const;
    void autoScrollWhileDragging(const juce::MouseEvent& e);

    int nodeWidth(const std::string& name);
    juce::Point<int> inletPos(const std::string& name, int inlet, int count);
    juce::Point<int> outletPos(const std::string& name, int outlet, int count);
    juce::Point<int> midiInletPos(const std::string& name, int port);
    juce::Point<int> midiOutletPos(const std::string& name, int port);
    void midiPortCounts(const std::string& name, int& ins, int& outs) const;
    juce::Point<int> videoInletPos(const std::string& name, int port);
    juce::Point<int> videoOutletPos(const std::string& name, int port);
    juce::Point<int> controlInletPos(const std::string& name, int port);
    juce::Point<int> controlOutletPos(const std::string& name, int port);
    void controlPortCounts(const std::string& name, int& ins, int& outs) const;
    bool hitPort(juce::Point<int> p, std::string& node, int& port, bool& isOutlet, bool& isMidi,
                 bool& isVideo, bool& isControl);
    bool hitPort(juce::Point<int> p, std::string& node, int& port, bool& isOutlet, bool& isMidi,
                 bool& isVideo) {
        bool isControl = false;
        return hitPort(p, node, port, isOutlet, isMidi, isVideo, isControl) && !isControl;
    }
    std::string hitNode(juce::Point<int> p);
    bool hitCord(juce::Point<int> p, Edge& out) const;
    void showAddMenu(juce::Point<int> at, juce::Point<int> screenPos);
    void showNodeMenu(const std::string& node, juce::Point<int> screenPos);
    void showCordMenu(const Edge& cord, juce::Point<int> at, juce::Point<int> screenPos);
    void renameNode(const std::string& node);
    void focusNewNode(const std::string& name);

    void instantiateClones(const std::vector<OrganismModel>& items,
                           const std::map<std::string, juce::Point<int>>& positions,
                           const std::vector<ConnectionModel>& cords,
                           const std::vector<ConnectionModel>& midiCords,
                           const std::vector<ConnectionModel>& videoCords,
                           juce::Point<int> offset);
    void notifySelection();

    void updateDragHints();
    void commitDragHints();

    PatcherHost& host_;
    int tickerId_ = 0;
    juce::uint64 paintedStamp_ = 0;
    std::unique_ptr<juce::FileChooser> chooser_;
    std::set<std::string> selection_;
    std::string primary_;

    enum class Drag { None, Move, Cord, Select, Pan } drag_ = Drag::None;
    bool beginPan(const juce::MouseEvent& e);
    juce::Point<int> panMouseAnchor_, panViewAnchor_;
    std::string dragNode_;
    std::string dragLead_;
    int alignGuideX_ = -1;
    int alignGuideY_ = -1;
    int cordOutlet_ = 0;
    bool cordMidi_ = false;
    bool cordControl_ = false;
    bool cordVideo_ = false;
    juce::Point<int> cordEnd_;
    juce::Point<int> dragStart_;
    std::map<std::string, juce::Point<int>> origPos_;
    juce::Rectangle<int> selectRect_;

    std::vector<OrganismModel> clipboard_;
    std::map<std::string, juce::Point<int>> clipboardPos_;
    std::vector<ConnectionModel> clipboardCords_;
    std::vector<ConnectionModel> clipboardMidiCords_;
    std::vector<ConnectionModel> clipboardVideoCords_;
    std::vector<PodClip> podClipboard_;
    int pasteCount_ = 0;

    std::string scope_;
    static constexpr int kCrumbH = 24;

    class CrumbBar : public juce::Component {
    public:
        explicit CrumbBar(PatcherCanvas& c) : canvas_(c) {}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void parentSizeChanged() override { layoutInViewport(); }
        void layoutInViewport();
    private:
        PatcherCanvas& canvas_;
        std::vector<std::pair<juce::Rectangle<int>, std::string>> crumbs_;
    };
    CrumbBar crumbBar_{*this};
    void updateCrumbBar();

    class ZoomBar : public juce::Component {
    public:
        explicit ZoomBar(PatcherCanvas& c) : canvas_(c) {}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void parentSizeChanged() override { layoutInViewport(); }
        void layoutInViewport();
        juce::Rectangle<int> cell(int i) const {
            return {i * kZoomCell, 0, kZoomCell, kZoomBarH};
        }
    private:
        PatcherCanvas& canvas_;
    };
    ZoomBar zoomBar_{*this};
    static constexpr int kZoomBarH = 24, kZoomCell = 30;
    void parentHierarchyChanged() override;

    std::vector<Edge> spliceBundle_;
    std::vector<Edge> proxConnections_;

    bool cordSelected_ = false;
    Edge selectedCord_;
    bool cordHovered_ = false;
    Edge hoverCord_;

    bool portHover_ = false;
    std::string hoverPortNode_; int hoverPort_ = 0; bool hoverPortOut_ = false;
    bool hoverPortMidi_ = false;
    bool hoverPortControl_ = false;
    bool hoverPortVideo_ = false;
    bool cordTarget_ = false;
    std::string cordTargetNode_; int cordTargetPort_ = 0;

    struct Flow {
        int level = 0;
        float midiCount = -1.0f;
        juce::uint32 flashUntil = 0;
        bool bypassed = false;
        bool bypassSeen = false;
    };
    std::map<std::string, Flow> flow_;
    void timerCallback() override;

    const juce::Image& nodeBody(int w, int h, bool sel, float scale);
    float zoom_ = 1.0f;
    std::map<juce::int64, juce::Image> bodyCache_;
    juce::uint32 bodySig_ = 0;
};

}
