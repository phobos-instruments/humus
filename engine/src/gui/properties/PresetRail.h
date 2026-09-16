// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/ClassString.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/host/PropertiesHost.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "gui/properties/PresetActions.h"
#include "gui/properties/PresetLibrary.h"
#include "gui/app/StartDirs.h"
#include "gui/common/Localisation.h"
#include "gui/properties/PresetField.h"

namespace hum {

class PresetRail : public juce::Component {
public:
    PresetRail(PropertiesHost& host, std::string node);

    void refresh();

    std::function<void()> onChanged;
    std::function<void()> onOpenBrowser;

    struct Geometry {
        juce::Rectangle<int> recall, store, prev, field, next, evolve, more;
        bool evolveShown = false;
    };
    Geometry geometry() const;
    bool drewDirty() const { return field_.drifted(); }

    void paint(juce::Graphics& g) override;

    void resized() override;

private:
    static constexpr int kBtn = 22;
    static constexpr int kArrow = 16;

    presets::Ref current() const { return host_.presets().current(node_); }
    std::vector<presets::Entry> stack() const;

    void changed();

    void step(int dir);

    void store(bool asNew);

    void confirmClear(const presets::Ref& ref);

    void browseMenu();

    void pinCurrent();

    void adoptFromLibrary(const std::vector<PresetDef>& lib, size_t i);

    void adoptDef(const PresetDef& def);

    void exportPreset();

    void importPreset();

    void promptSaveToLibrary();

    void actMenu(juce::Point<int> at);

    enum : int {
        kEvolve = 10000, kGenerate, kRecall, kStore, kStoreNew, kSaveLib,
        kRename, kClear, kCut, kCopy, kPaste, kExport, kImport, kBrowser, kPin
    };

    PropertiesHost& host_;
    std::string node_;
    IconButton recall_{IconGlyph::Open, {}};
    IconButton store_{IconGlyph::Save, {}};
    IconButton evolve_{IconGlyph::Evolve, {}};
    IconButton more_{IconGlyph::Overflow, {}};
    StepArrow prev_{false}, next_{true};
    PresetField field_;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::Rectangle<int> group_;
    bool groupHover_ = false;
};

}
