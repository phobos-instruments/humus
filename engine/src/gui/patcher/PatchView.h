// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/graph/PodModel.h"
#include "gui/common/UiTicker.h"
#include "gui/patcher/PatchViewSource.h"
#include "gui/patcher/PortGroups.h"
#include "gui/patcher/PortSpot.h"

namespace hum {

class PatchView : public juce::Component {
public:
    explicit PatchView(PatchViewSource& source) : src_(source) {
        tickerId_ = UiTicker::instance().add([this] { timerCallback(); });
    }
    ~PatchView() override { UiTicker::instance().remove(tickerId_); }

    juce::Point<int> visibleCentreForNode() const {
        auto centre = getLocalBounds().getCentre();
        if (auto* vp = dynamic_cast<const juce::Viewport*>(getParentComponent()))
            centre = vp->getViewArea().getCentre();
        return {std::max(0, (int) (centre.x / zoom_) - kW / 2),
                std::max(0, (int) (centre.y / zoom_) - kH / 2)};
    }

    juce::Point<int> modelPos(const juce::MouseEvent& e) const { return modelPos(e.getPosition()); }
    juce::Point<int> modelPos(juce::Point<int> viewPoint) const { return (viewPoint.toFloat() / zoom_).roundToInt(); }
    juce::Point<int> viewPos(juce::Point<int> modelPoint) const { return (modelPoint.toFloat() * zoom_).roundToInt(); }

    static constexpr float kZoomMin = 0.4f, kZoomMax = 2.0f;
    float zoom() const { return zoom_; }
    void setZoom(float z, juce::Point<int> anchor);
    void resetView();
    void frameContent();

    juce::Rectangle<int> nodeBoundsForTest(const std::string& n) { return nodeBounds(n); }
    bool hitsCordForTest(juce::Point<int> p) const {
        Edge e;
        return hitCord(p, e);
    }
    juce::Point<int> videoInletPosForTest(const std::string& n, int port) { return videoInletPos(n, port); }
    juce::Point<int> videoOutletPosForTest(const std::string& n, int port) { return videoOutletPos(n, port); }
    juce::Point<int> midiOutletPosForTest(const std::string& n, int port) { return midiOutletPos(n, port); }
    juce::Point<int> portPosForTest(const std::string& n, int port, bool outlet, bool midi = false) {
        return portPos({n, port, outlet, midi});
    }
    void controlPortCountsForTest(const std::string& n, int& ins, int& outs) const { controlPortCounts(n, ins, outs); }
    void selectForTest(std::set<std::string> names) {
        selection_ = std::move(names);
        primary_ = selection_.empty() ? std::string() : *selection_.begin();
    }
    juce::Rectangle<int> lastNodeRepaintForTest() const { return lastNodeRepaint_; }
    void timerCallbackForTest() { timerCallback(); }
    std::string hitNodeAtView(juce::Point<int> viewPoint) { return hitNode(modelPos(viewPoint)); }

    const std::string& selected() const { return primary_; }
    bool hasSelection() const { return !selection_.empty(); }
    const std::string& scope() const { return scope_; }
    void videoPortCounts(const std::string& name, int& ins, int& outs) const;

    void paint(juce::Graphics&) override;
    void parentSizeChanged() override { updateContentSize(); }
    void parentHierarchyChanged() override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify(const juce::MouseEvent&, float) override;

    juce::Rectangle<int> nodeBounds(const std::string& name);
    std::string portName(const std::string& node, int port, bool outlet) const;
    std::vector<PortSpot> portSpots(const std::string& name);
    void repaintNode(const std::string& name);
    juce::Rectangle<int> lastNodeRepaint_;

protected:
    static constexpr int kW = 116, kH = 38;
    static constexpr int kPort = 7;
    static constexpr int kPortPad = 9;
    static constexpr int kPortGap = 5;
    static constexpr int kMidiGap = 10;
    static constexpr int kVideoGap = 26;
    static constexpr int kGroupGap = 6;
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

    struct PortRef {
        std::string node;
        int port = 0;
        bool outlet = false, midi = false, video = false, control = false;
    };
    struct CordLine { juce::Point<float> a, b; Edge cord; };

    virtual bool isCarried(const Edge&) const { return false; }
    virtual std::vector<CordLine> carriedLines() { return {}; }
    virtual bool cordInHand() const { return drag_ == Drag::Cord; }
    virtual bool fitsDrag(const PortRef& t) const;
    virtual bool isDragAnchor(const PortRef& t) const;
    bool refusesDuringDrag(const PortRef& t) const { return cordInHand() && !fitsDrag(t) && !isDragAnchor(t); }

    std::vector<CordLine> visibleCords();
    std::vector<Edge> cordsCrossing(juce::Rectangle<int> band);
    bool isCordSelected(const Edge& e) const;
    juce::Point<int> portPos(const PortRef& at);

    struct DisplayNode { std::string name; bool pod = false; };
    std::vector<DisplayNode> displayNodes() const;
    bool isPodBox(const std::string& name) const;
    void portCounts(const std::string& name, int& ins, int& outs) const;
    struct PortSides {
        int ins = -1, outs = -1;
        std::string cls;
        bool resolved = false;
        portgroups::Side in, out;
    };
    const portgroups::Side& portSide(const std::string& name, bool outlets) const;
    int audioBreaks(const std::string& name) const;
    mutable std::map<std::string, PortSides> portSides_;
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

    void updateContentSize();
    juce::Point<int> viewOffset() const;
    void autoScrollWhileDragging(const juce::MouseEvent& e);

    int nodeWidth(const std::string& name);
    juce::Point<int> inletPos(const std::string& name, int inlet, int count);
    juce::Point<int> outletPos(const std::string& name, int outlet, int count);
    juce::Point<int> midiInletPos(const std::string& name, int port);
    juce::Point<int> midiOutletPos(const std::string& name, int port);
    void midiPortCounts(const std::string& name, int& ins, int& outs) const;
    int afterAudioX(const std::string& name, bool outlets);
    int videoBlockWidth(const std::string& name, bool outlets);
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
    juce::Point<int> snapMove(juce::Point<int> delta, juce::Point<int> leadOrigin, bool free);
    bool hitCord(juce::Point<int> p, Edge& out) const;

    void timerCallback();

    PatchViewSource& src_;
    int tickerId_ = 0;
    juce::uint64 paintedStamp_ = 0;
    std::set<std::string> selection_;
    std::string primary_;
    std::string scope_;

    enum class Drag { None, Move, Cord, Select, CordBand, Pan, Grab } drag_ = Drag::None;
    std::string dragNode_;
    int alignGuideX_ = -1;
    int alignGuideY_ = -1;
    int cordOutlet_ = 0;
    bool cordMidi_ = false;
    bool cordControl_ = false;
    bool cordVideo_ = false;
    juce::Point<int> cordEnd_;
    juce::Rectangle<int> selectRect_;

    std::string dropInto_;
    std::vector<Edge> spliceBundle_;
    std::vector<Edge> proxConnections_;

    std::vector<Edge> selectedCords_;
    bool cordHovered_ = false;
    Edge hoverCord_;

    bool portHover_ = false;
    std::string hoverPortNode_; int hoverPort_ = 0; bool hoverPortOut_ = false;
    bool hoverPortMidi_ = false;
    bool hoverPortControl_ = false;
    bool hoverPortVideo_ = false;
    bool cordTarget_ = false;
    std::string cordTargetNode_; int cordTargetPort_ = 0;
    PortRef grabTarget_;
    bool refused_ = false;
    PortRef refusedAt_;

    struct Flow {
        int level = 0;
        float midiCount = -1.0f;
        juce::uint32 flashUntil = 0;
        bool bypassed = false;
        bool bypassSeen = false;
    };
    std::map<std::string, Flow> flow_;

    const juce::Image& nodeBody(int w, int h, bool sel, float scale);
    float zoom_ = 1.0f;
    std::map<juce::int64, juce::Image> bodyCache_;
    juce::uint32 bodySig_ = 0;

    class ZoomBar : public juce::Component {
    public:
        explicit ZoomBar(PatchView& v) : view_(v) {}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void parentSizeChanged() override { layoutInViewport(); }
        void layoutInViewport();
        juce::Rectangle<int> cell(int i) const {
            return {i * kZoomCell, 0, kZoomCell, kZoomBarH};
        }
    private:
        PatchView& view_;
    };
    ZoomBar zoomBar_{*this};
    static constexpr int kZoomBarH = 24, kZoomCell = 30;
};

}
