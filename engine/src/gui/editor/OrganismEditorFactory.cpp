// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/OrganismEditorFactory.h"

#include <memory>
#include <string>

#include "core/packs/PackRegistry.h"
#include "core/graph/PodModel.h"
#include "hum/Registry.h"
#include "gui/host/EditorHost.h"
#include "gui/editor/LayoutEditor.h"
#include "gui/editor/BankSlotSpec.h"
#include "gui/editor/LayoutLoader.h"
#include "gui/editor/ParameterPanel.h"
#include "gui/common/Localisation.h"

namespace hum {

namespace {

class PodPortEditor : public OrganismEditor {
public:
    PodPortEditor(EditorHost& host, std::string name, bool inlet, int kind)
        : host_(host), name_(std::move(name)), inlet_(inlet) {
        mode_.addItem(tr("organism-editor-factory.mono", "Mono"), 1);
        mode_.addItem(tr("organism-editor-factory.stereo", "Stereo"), 2);
        mode_.addItem(tr("organism-editor-factory.midi", "MIDI"), 3);
        mode_.addItem(tr("organism-editor-factory.video", "Video"), 4);
        mode_.addItem(tr("organism-editor-factory.control", "Control"), 5);
        mode_.setSelectedId(kind, juce::dontSendNotification);
        mode_.setColour(juce::ComboBox::textColourId, Palette::text);
        mode_.onChange = [this] {
            const int want = mode_.getSelectedId();
            const std::string next =
                inlet_ ? (want == 5 ? pods::kControlInletClass
                          : want == 4 ? pods::kVideoInletClass
                          : want == 3 ? pods::kMidiInletClass
                          : want == 2 ? pods::kInletStereoClass : pods::kInletClass)
                       : (want == 5 ? pods::kControlOutletClass
                          : want == 4 ? pods::kVideoOutletClass
                          : want == 3 ? pods::kMidiOutletClass
                          : want == 2 ? pods::kOutletStereoClass : pods::kOutletClass);
            const auto* cm = host_.model().byName(name_);
            if (!cm || cm->displayClass == next) return;
            host_.replaceOrganism(name_, next);
            host_.noteTopologyChanged();
        };
        addAndMakeVisible(mode_);
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 236; }
    int preferredContentHeight(int) const override { return 36; }

    void resized() override {
        auto r = getLocalBounds().reduced(8, 6);
        mode_.setBounds(r.removeFromTop(22).removeFromLeft(130));
    }

private:
    EditorHost& host_;
    std::string name_;
    bool inlet_;
    juce::ComboBox mode_;
};

}

std::unique_ptr<OrganismEditor> makeOrganismEditor(EditorHost& host,
                                                                const std::string& name) {
    std::string cls;
    if (auto* cm = host.model().byName(name)) cls = cm->displayClass;

    if (pods::isInletClass(cls) || pods::isOutletClass(cls))
        return std::make_unique<PodPortEditor>(
            host, name, pods::isInletClass(cls),
            pods::isControlPortClass(cls) ? 5
            : pods::isVideoPortClass(cls) ? 4
            : pods::isMidiPortClass(cls) ? 3 : pods::portChannels(cls) == 2 ? 2 : 1);

    if (const auto* m = PackRegistry::instance().classManifest(cls)) {
        const auto& e = m->editor;
        if (e.rfind("gen:", 0) == 0)
            if (auto spec = makeGeneratedLayout(e.substr(4), cls); !spec.controls.empty())
                return std::make_unique<LayoutEditor>(host, name, std::move(spec));
        if (e.rfind("layout:", 0) == 0)
            if (const auto* folder = PackRegistry::instance().folderOf(cls)) {
                auto spec = loadLayoutSpecFromFile(folder->dir + "/" + e.substr(7));
                if (!spec.controls.empty())
                    return std::make_unique<LayoutEditor>(host, name, std::move(spec));
            }
    } else {
        if (auto spec = makeGeneratedLayout("", cls); !spec.controls.empty())
            return std::make_unique<LayoutEditor>(host, name, std::move(spec));
    }

    auto p = std::make_unique<ParameterPanel>(host);
    p->show(name);
    return p;
}

}
