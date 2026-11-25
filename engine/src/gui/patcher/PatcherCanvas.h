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
#include "gui/patcher/PatchView.h"
#include "gui/patcher/PatcherHostSource.h"

namespace hum {

struct PatcherSourceHolder {
    explicit PatcherSourceHolder(PatcherHost& host) : patchSource(host) {}
    PatcherHostSource patchSource;
};

class PatcherCanvas : private PatcherSourceHolder,
                      public PatchView,
                      public juce::TooltipClient,
                      public juce::FileDragAndDropTarget,
                      public juce::DragAndDropTarget {
public:
    explicit PatcherCanvas(PatcherHost& host)
        : PatcherSourceHolder(host), PatchView(patchSource), host_(host) {
        setWantsKeyboardFocus(true);
    }

    std::function<void(const std::string&)> onSelect;
    std::function<void(const std::string&)> onActivate;
    std::function<void(const std::string&)> onOpenPluginUI;
    std::function<void(const std::string&)> onRandomisePod;
    std::function<void(const std::string&, juce::Point<int>)> onMapPodRandom;
    std::function<void(const std::string&)> onOpenVisuals;
    std::function<void(const std::string&)> onRecordNode;
    std::function<void(const std::string&, juce::Point<int>)> onAddRequest;
    std::function<void()> onUndoRedo;

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
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override { fileDragMove(files, x, y); }
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    const std::string& dropIntoForTest() const { return dropInto_; }
    void dropNewOrganisms(const std::string& className, const std::vector<std::string>& paths, juce::Point<int> at);

    void refresh();
    void select(const std::string& n);
    void enterPod(const std::string& pod);
    void exitToScope(const std::string& scope);

    void autoArrange();
    bool arrangesSelection() const;
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

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void modifierKeysChanged(const juce::ModifierKeys&) override;
    bool keyPressed(const juce::KeyPress&) override;

    void parentHierarchyChanged() override;

private:

    struct ShownCord {
        Edge real;
        std::string srcNode;
        int srcPort = 0;
        std::string dstNode;
        int dstPort = 0;
        int channel = 0;
    };
    struct Carried { ShownCord cord; int shift = 0; };

    std::vector<ShownCord> shownCords(bool midi, bool video, bool control) const;
    std::vector<ShownCord> cordsAt(const PortRef& at) const;
    bool grabbable(const PortRef& at, const juce::ModifierKeys& mods) const;
    bool beginGrab(const PortRef& at, juce::Point<int> p, const juce::ModifierKeys& mods);
    void dragGrab(juce::Point<int> p);
    void dropGrab(juce::Point<int> p, const juce::ModifierKeys& mods);
    void cancelGrab();
    bool landingAt(juce::Point<int> p, PortRef& out);
    bool fitsDrag(const PortRef& t) const override;
    bool isDragAnchor(const PortRef& t) const override;
    bool cordInHand() const override { return drag_ == Drag::Cord || (drag_ == Drag::Grab && grabLifted_); }
    void noteRefusal(juce::Point<int> p);
    bool pairFits(const PortRef& at) const;
    std::vector<std::pair<int, int>> cordsForDrop(const std::string& node, int port,
                                                  const juce::ModifierKeys& mods) const;
    void dropAudioCords(const std::string& node, int port, const juce::ModifierKeys& mods);
    void moveCarried(const PortRef& to, const juce::ModifierKeys& mods);
    void reconnect(const ShownCord& c, const PortRef& to, int port);
    void removeCord(const Edge& e);
    bool isCarried(const Edge& e) const override;
    std::vector<CordLine> carriedLines() override;
    void toggleCord(const Edge& e);
    void forgetCords(const std::vector<Edge>& cords);
    std::vector<Edge> liveSelectedCords();
    void removeCords(const std::vector<Edge>& cords);
    void beginBand(juce::Point<int> p, bool cords, bool additive);
    void dragBand(juce::Point<int> p);

    std::vector<DisplayNode> arrangeTargets() const;
    void showNewPodDialog(juce::Point<int> at);
    void podFromPatchDialog(juce::Point<int> at);
    void openQuickAdd(juce::Point<int> at, juce::Point<int> screenPos);
    void showPodMenu(const std::string& pod, juce::Point<int> screenPos);
    void renamePodDialog(const std::string& pod);

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
    std::unique_ptr<juce::FileChooser> chooser_;

    bool beginPan(const juce::MouseEvent& e);
    juce::Point<int> panMouseAnchor_, panViewAnchor_;
    std::string dragLead_;
    juce::Point<int> dragStart_;
    std::map<std::string, juce::Point<int>> origPos_;
    std::set<std::string> bandNodes_;
    std::vector<Edge> bandCords_;

    std::vector<OrganismModel> clipboard_;
    std::map<std::string, juce::Point<int>> clipboardPos_;
    std::vector<ConnectionModel> clipboardCords_;
    std::vector<ConnectionModel> clipboardMidiCords_;
    std::vector<ConnectionModel> clipboardVideoCords_;
    std::vector<PodClip> podClipboard_;
    int pasteCount_ = 0;

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

    std::vector<Carried> carried_;
    PortRef grabFrom_;
    juce::Point<int> grabStart_;
    bool grabLifted_ = false;

};

}
