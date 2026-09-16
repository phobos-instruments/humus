// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "core/params/Metapad.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/host/PropertiesHost.h"
#include "gui/app/FreeWindow.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "gui/properties/MetapadMaskPanel.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/common/Localisation.h"

namespace hum {

class MetapadView : public juce::Component, private juce::Timer {
public:
    explicit MetapadView(PropertiesHost& host);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent&) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    struct Row : juce::Component {
        Row(MetapadView& v, int snapIndex, juce::String name, juce::Colour col)
            : view(v), index(snapIndex) {
            swatch = col;
            nameLbl.setText(name, juce::dontSendNotification);
            nameLbl.setColour(juce::Label::textColourId, Palette::text);
            nameLbl.setEditable(false, true);
            nameLbl.onTextChange = [this] { view.host_.metapad().renameSnapshot(index, nameLbl.getText().toStdString()); };
            addAndMakeVisible(nameLbl);
            addAndMakeVisible(recall);
            addAndMakeVisible(store);
            addAndMakeVisible(clear);
            view.styleBtn(clear);
            clear.setTooltip(tr("metapad.delete-this-snapshot", "Delete this snapshot"));
            recall.onClick = [this] {
                auto& h = view.host_;
                h.setParam(h.metapadNodeName(), kMetaRecallParam, (double) index + 1.0);
            };
            store.onClick  = [this] { view.host_.metapad().storeSnapshot(index); };
            clear.onClick  = [this] { view.confirmClear(index); };
            for (auto* c : std::initializer_list<juce::Component*>{&nameLbl, &recall, &store, &clear})
                c->addMouseListener(this, false);
        }
        void paint(juce::Graphics& g) override {
            if (view.selected_ == index) { g.setColour(Palette::accentDim); g.fillAll(); }
            g.setColour(swatch); g.fillRect(2, 4, 14, getHeight() - 8);
        }
        void mouseDown(const juce::MouseEvent& e) override {
            view.selected_ = index;
            view.repaint();
            getParentComponent()->repaint();
            if (e.originalComponent == this && e.x < 20)
                view.openColourPicker(index, localAreaToGlobal(getLocalBounds()));
        }
        void resized() override {
            auto r = getLocalBounds(); r.removeFromLeft(20);
            auto btns = r.removeFromRight(72);
            recall.setBounds(btns.removeFromLeft(24).reduced(1));
            store.setBounds(btns.removeFromLeft(24).reduced(1));
            clear.setBounds(btns.removeFromLeft(24).reduced(1));
            nameLbl.setBounds(r.reduced(2));
        }
        MetapadView& view; int index; juce::Colour swatch;
        juce::Label nameLbl;
        IconButton recall{IconGlyph::Open, tr("metapad.recall-this-snapshot", "Recall this snapshot")};
        IconButton store{IconGlyph::Save, tr("metapad.store-current-settings-into-this", "Store current settings into this snapshot")};
        juce::TextButton clear{juce::String::charToString(juce::juce_wchar(0x00d7))};
    };

    void targetMenu(const char* param, juce::Component& from);

    void updateMidiCaptions();

public:
    void syncFromHost();
private:
    void timerCallback() override { if (isShowing()) syncFromHost(); }

    void confirmClear(int index);

    void styleBtn(juce::Button& b);

    juce::Point<float> toScreen(float x, float y) const;
    juce::Point<double> toNorm(juce::Point<float> p) const;
    int pointAt(juce::Point<float> p) const;
    const DocumentSnapshot* snap(int index) const;
    juce::String nameFor(int index) const { auto* s = snap(index); return s ? juce::String(s->name) : juce::String(index); }
    juce::Colour colourFor(int index) const;
    MetaRGB rgbFor(int index) const;

    void ensureField();

    void openColourPicker(int snapIndex, juce::Rectangle<int> screenArea);

    void refreshColours();

    void rebuildList();
    void layoutList();

    PropertiesHost& host_;
    juce::TextButton modeBtn_, newBtn_, midiXBtn_, midiYBtn_;
    juce::Slider tempSlider_;
    juce::Label snapHdr_, maskHdr_;
    juce::Viewport listVp_;
    juce::Component list_;
    std::unique_ptr<MetapadMaskPanel> mask_;
    std::vector<std::unique_ptr<Row>> rows_;
    juce::Rectangle<float> surface_;
    juce::Image field_;
    juce::String fieldSig_;
    bool interpolate_ = false;
    bool surfaceDrag_ = false;
    int selected_ = -1, dragPoint_ = -1, hoverPoint_ = -1;
    bool moveUndone_ = true;
    juce::Point<float> cursor_{0.5f, 0.5f};

    friend struct Row;
};

class MetapadWindow : public FreeWindow {
public:
    explicit MetapadWindow(PropertiesHost& host)
        : FreeWindow("Metapad", new MetapadView(host)) {
        centreWithSize(740, 420);
    }
};

}
