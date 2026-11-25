// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/Dock.h"
#include "gui/app/DockLayout.h"
#include "gui/host/EngineHost.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PaneRefresher.h"
#include "gui/properties/PropertiesPane.h"
#include "plugin/PluginWindows.h"

namespace hum {

struct PaneLayout {
    bool patcher = true;
    bool properties = true;
    int sideWidth = 620;
};

class PatchPanes : public juce::Component {
public:
    enum class Pane { Patcher, Properties };

    PatchPanes(EngineHost& host, PluginWindows& windows, PaneLayout& layout);
    ~PatchPanes() override;

    std::function<void()> onTopologyChanged;
    std::function<void()> onLayoutChanged;

    PatcherCanvas& canvas() { return canvas_; }
    void refresh();
    void frame() { canvas_.frameContent(); }
    void tick() { refresher_.tick(props_, host_); }
    void openForTest(const std::string& name) { props_.openFor(name); }
    bool isShown(Pane pane) const { return pane == Pane::Patcher ? layout_.patcher : layout_.properties; }
    bool setShown(Pane pane, bool shown);
    juce::Rectangle<int> paneBoundsForTest(Pane pane) const;
    void dragSplitForTest(int dx) { dragSplit(dx); }
    void resized() override;

    static constexpr int kMinSide = 300, kMinPatch = 200;

private:
    void addAt(const std::string& className, juce::Point<int> at);
    void wireHost();
    void dragSplit(int dx);

    EngineHost& host_;
    PluginWindows& windows_;
    PaneLayout& layout_;
    PatcherCanvas canvas_;
    juce::Viewport patchView_;
    PropertiesPane props_;
    DockPane patchFrame_, propsFrame_;
    SplitterBar split_{SplitterBar::Vertical};
    PaneRefresher refresher_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchPanes)
};

}
