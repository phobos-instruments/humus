#pragma once
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "core/Metapad.h"
#include "gui/EngineHost.h"
#include "gui/FreeWindow.h"
#include "gui/IconButton.h"
#include "gui/LookAndFeel.h"
#include "gui/MetapadMaskPanel.h"
#include "gui/OrganismEditor.h"
#include "gui/Localisation.h"

namespace hum {

class MetapadView : public juce::Component, private juce::Timer {
public:
    explicit MetapadView(EngineHost& host) : host_(host) {
        addAndMakeVisible(modeBtn_);
        modeBtn_.setClickingTogglesState(true);
        modeBtn_.setButtonText(tr("metapad.interpolate", "Interpolate"));
        modeBtn_.onClick = [this] {
            host_.setParam(host_.metapadNodeName(), kMetaInterpolateParam,
                           modeBtn_.getToggleState() ? 1.0 : 0.0);
            syncFromHost();
        };
        addAndMakeVisible(newBtn_);
        newBtn_.setButtonText(tr("metapad.new-snapshot", "New Snapshot"));
        newBtn_.onClick = [this] {
            selected_ = host_.metapad().addSnapshot("");
            rebuildList();
            repaint();
        };
        for (auto* b : {&midiXBtn_, &midiYBtn_}) { addAndMakeVisible(*b); styleBtn(*b); }
        midiXBtn_.onClick = [this] { targetMenu(kMetaXParam, midiXBtn_); };
        midiYBtn_.onClick = [this] { targetMenu(kMetaYParam, midiYBtn_); };
        addAndMakeVisible(tempSlider_);
        tempSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        tempSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        tempSlider_.setRange(0.0, 2.0, 0.0);
        tempSlider_.setValue(host_.model().metapad.temperature, juce::dontSendNotification);
        tempSlider_.setTooltip(tr("metapad.morph-temperature",
                                  "Morph temperature - left snaps to the nearest snapshot, "
                                  "right blends them evenly. Right-click to map a control"));
        tempSlider_.addMouseListener(this, false);
        tempSlider_.onValueChange = [this] {
            host_.metapad().setTemperature(tempSlider_.getValue());
            repaint();
        };
        for (auto* b : {&modeBtn_, &newBtn_}) b->addMouseListener(this, false);
        updateMidiCaptions();
        startTimerHz(15);
        addAndMakeVisible(listVp_);
        listVp_.setViewedComponent(&list_, false);
        for (auto* l : {&snapHdr_, &maskHdr_}) {
            l->setColour(juce::Label::textColourId, Palette::textDim);
            l->setFont(juce::FontOptions(11.0f).withStyle("Bold"));
            addAndMakeVisible(*l);
        }
        snapHdr_.setText(tr("metapad.snapshots", "Snapshots"), juce::dontSendNotification);
        maskHdr_.setText(tr("metapad.morph-parameters", "Morph parameters"), juce::dontSendNotification);
        mask_ = std::make_unique<MetapadMaskPanel>(host_);
        addAndMakeVisible(*mask_);
        styleBtn(modeBtn_); styleBtn(newBtn_);
        rebuildList();
        setSize(720, 400);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(6);
        auto top = r.removeFromTop(26);
        modeBtn_.setBounds(top.removeFromLeft(110));
        top.removeFromLeft(6);
        newBtn_.setBounds(top.removeFromLeft(120));
        top.removeFromLeft(12);
        midiXBtn_.setBounds(top.removeFromLeft(96));
        top.removeFromLeft(6);
        midiYBtn_.setBounds(top.removeFromLeft(96));
        top.removeFromLeft(12);
        tempSlider_.setBounds(top.removeFromLeft(juce::jmin(150, top.getWidth())));
        r.removeFromTop(6);
        auto right = r.removeFromRight(232);
        r.removeFromRight(8);
        snapHdr_.setBounds(right.removeFromTop(16));
        listVp_.setBounds(right.removeFromTop(juce::jmax(52, right.getHeight() * 4 / 10)));
        right.removeFromTop(6);
        maskHdr_.setBounds(right.removeFromTop(16));
        if (mask_) mask_->setBounds(right);
        surface_ = r.toFloat();
        layoutList();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::background);
        g.setColour(Palette::panel);
        g.fillRect(surface_);
        if (!host_.model().metapad.points.empty()) {
            ensureField();
            if (field_.isValid()) g.drawImage(field_, surface_);
        }
        g.setColour(Palette::panelLight.withAlpha(0.4f));
        for (int i = 1; i < 4; ++i) {
            float fx = surface_.getX() + surface_.getWidth() * i / 4.0f;
            float fy = surface_.getY() + surface_.getHeight() * i / 4.0f;
            g.drawVerticalLine((int) fx, surface_.getY(), surface_.getBottom());
            g.drawHorizontalLine((int) fy, surface_.getX(), surface_.getRight());
        }
        const auto& ms = host_.model().metapad;
        for (int i = 0; i < (int) ms.points.size(); ++i) {
            const auto& p = ms.points[(size_t) i];
            auto pos = toScreen((float) p.x, (float) p.y);
            auto col = colourFor(p.snapshotIndex);
            const bool live = i == dragPoint_ || (dragPoint_ < 0 && i == hoverPoint_);
            if (live) {
                g.setColour(col.withAlpha(0.35f));
                g.fillEllipse(pos.x - 12, pos.y - 12, 24, 24);
            }
            g.setColour(col);
            g.fillEllipse(pos.x - 7, pos.y - 7, 14, 14);
            g.setColour(live ? Palette::text : Palette::background);
            g.drawEllipse(pos.x - 7, pos.y - 7, 14, 14, live ? 2.0f : 1.5f);
            g.setColour(Palette::text);
            g.setFont(11.0f);
            g.drawText(nameFor(p.snapshotIndex), (int) pos.x + 9, (int) pos.y - 8, 120, 16,
                       juce::Justification::centredLeft);
        }
        if (interpolate_) {
            auto c = toScreen(cursor_.x, cursor_.y);
            g.setColour(Palette::accent);
            g.drawLine(c.x, surface_.getY(), c.x, surface_.getBottom(), 1.0f);
            g.drawLine(surface_.getX(), c.y, surface_.getRight(), c.y, 1.0f);
            g.fillEllipse(c.x - 4, c.y - 4, 8, 8);
        }
        g.setColour(Palette::textDim);
        g.setFont(11.0f);
        g.drawText(interpolate_
                       ? "Drag to morph  -  Alt-drag a point to move it  -  Ctrl-click = new snapshot here"
                       : "Drag a point to move it  -  click empty space to place the selected snapshot",
                   (int) surface_.getX() + 4, (int) surface_.getBottom() - 18,
                   (int) surface_.getWidth() - 8, 16, juce::Justification::centredLeft);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.eventComponent != this) {
            if (!e.mods.isPopupMenu()) return;
            if (e.eventComponent == &newBtn_)  targetMenu(kMetaSnapshotAction, newBtn_);
            if (e.eventComponent == &modeBtn_) targetMenu(kMetaInterpolateParam, modeBtn_);
            if (e.eventComponent == &tempSlider_) targetMenu(kMetaTemperatureParam, tempSlider_);
            return;
        }
        if (!surface_.contains(e.position.toFloat())) return;
        surfaceDrag_ = !e.mods.isPopupMenu();
        if (e.mods.isPopupMenu()) {
            const int pi = pointAt(e.position.toFloat());
            if (pi < 0) return;
            juce::PopupMenu m;
            m.addItem(1, tr("metapad.remove-point", "Remove point"));
            m.showMenuAsync(juce::PopupMenu::Options(), [this, pi](int r) {
                if (r != 1) return;
                host_.metapad().removePoint(pi);
                repaint();
            });
            return;
        }
        const auto n = toNorm(e.position.toFloat());
        if (interpolate_) {
            if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
                host_.pushUndo();
                int idx = host_.metapad().addSnapshot("");
                host_.metapad().placeSnapshot(idx, n.x, n.y);
                rebuildList();
            } else if (e.mods.isAltDown()) {
                dragPoint_ = pointAt(e.position.toFloat());
                if (dragPoint_ >= 0) { moveUndone_ = false; repaint(); return; }
            }
            host_.metapad().morph(n.x, n.y);
            syncFromHost();
        } else {
            dragPoint_ = pointAt(e.position.toFloat());
            if (dragPoint_ >= 0) {
                moveUndone_ = false;
            } else if (selected_ >= 0) {
                host_.pushUndo();
                host_.metapad().placeSnapshot(selected_, n.x, n.y);
                dragPoint_ = (int) host_.model().metapad.points.size() - 1;
                moveUndone_ = true;
            }
        }
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!surfaceDrag_) return;
        const auto n = toNorm(e.position.toFloat());
        if (dragPoint_ >= 0) {
            if (!moveUndone_) { host_.pushUndo(); moveUndone_ = true; }
            host_.metapad().movePoint(dragPoint_, n.x, n.y);
        }
        else if (interpolate_) { host_.metapad().morph(n.x, n.y); syncFromHost(); }
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override {
        surfaceDrag_ = false;
        dragPoint_ = -1;
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int h = surface_.contains(e.position.toFloat()) ? pointAt(e.position.toFloat()) : -1;
        if (h == hoverPoint_) return;
        hoverPoint_ = h;
        setMouseCursor(h >= 0 ? juce::MouseCursor::DraggingHandCursor
                              : juce::MouseCursor::NormalCursor);
        repaint();
    }
    void mouseExit(const juce::MouseEvent&) override {
        if (hoverPoint_ < 0) return;
        hoverPoint_ = -1;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }

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

    void targetMenu(const char* param, juce::Component& from) {
        showAutomateMenu(host_, host_.metapadNodeName(), param,
                         from.getScreenBounds().getBottomLeft(),
                         [this] { updateMidiCaptions(); },
!isMetapadAction(param));
    }

    void updateMidiCaptions() {
        const auto node = host_.metapadNodeNameIfAny();
        auto caption = [&](const char* param, const juce::String& idle) {
            if (!node.empty())
                for (const auto& e : host_.midi().map().entries())
                    if (e.organism == node && e.param == param)
                        return juce::String(param) + ": " + juce::String(midiSourceLabel(e.source()));
            return idle;
        };
        midiXBtn_.setButtonText(caption(kMetaXParam, tr("metapad.midi-x", "MIDI X")));
        midiYBtn_.setButtonText(caption(kMetaYParam, tr("metapad.midi-y", "MIDI Y")));
    }

public:
    void syncFromHost() {
        const bool mode = host_.model().metapad.interpolateMode != 0;
        const double temp = host_.model().metapad.temperature;
        if (std::abs(tempSlider_.getValue() - temp) > 1e-6) {
            tempSlider_.setValue(temp, juce::dontSendNotification);
            repaint();
        }
        const juce::Point<float> cur{(float) host_.metapadX(), (float) host_.metapadY()};
        if (mode == interpolate_ && cur == cursor_) return;
        interpolate_ = mode;
        cursor_ = cur;
        modeBtn_.setToggleState(mode, juce::dontSendNotification);
        repaint();
    }
private:
    void timerCallback() override { if (isShowing()) syncFromHost(); }

    void confirmClear(int index) {
        const juce::String nm = nameFor(index);
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::QuestionIcon, "Delete snapshot",
            "Delete " + (nm.isEmpty() ? "this snapshot"
                                      : juce::String::fromUTF8("\xe2\x80\x9c") + nm
                                            + juce::String::fromUTF8("\xe2\x80\x9d"))
                + "? Its points on the surface go with it.",
            "Delete", "Cancel", this,
            juce::ModalCallbackFunction::create(
                [&host = host_, index,
                 sp = juce::Component::SafePointer<MetapadView>(this)](int r) {
                    if (r != 1) return;
                    host.metapad().clearSnapshot(index);
                    if (sp == nullptr) return;
                    if (sp->selected_ == index) sp->selected_ = -1;
                    sp->rebuildList();
                    sp->repaint();
                }));
    }

    void styleBtn(juce::Button& b) {
        b.setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        b.setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        b.setColour(juce::TextButton::textColourOffId, Palette::text);
        b.setColour(juce::TextButton::textColourOnId, Palette::background);
    }

    juce::Point<float> toScreen(float x, float y) const {
        return {surface_.getX() + x * surface_.getWidth(), surface_.getY() + y * surface_.getHeight()};
    }
    juce::Point<double> toNorm(juce::Point<float> p) const {
        return {juce::jlimit(0.0, 1.0, (double) ((p.x - surface_.getX()) / surface_.getWidth())),
                juce::jlimit(0.0, 1.0, (double) ((p.y - surface_.getY()) / surface_.getHeight()))};
    }
    int pointAt(juce::Point<float> p) const {
        const auto& pts = host_.model().metapad.points;
        for (int i = (int) pts.size() - 1; i >= 0; --i)
            if (toScreen((float) pts[(size_t) i].x, (float) pts[(size_t) i].y).getDistanceFrom(p) <= 12.0f) return i;
        return -1;
    }
    const DocumentSnapshot* snap(int index) const {
        for (auto& s : host_.model().metapad.snapshots) if (s.index == index) return &s;
        return nullptr;
    }
    juce::String nameFor(int index) const { auto* s = snap(index); return s ? juce::String(s->name) : juce::String(index); }
    juce::Colour colourFor(int index) const {
        auto* s = snap(index);
        return (s && s->colour.size() >= 7) ? juce::Colour::fromString("ff" + juce::String(s->colour.substr(1)))
                                            : Palette::accent;
    }
    MetaRGB rgbFor(int index) const {
        MetaRGB m;
        if (auto* s = snap(index); s && parseHexColour(s->colour, m)) return m;
        const auto a = Palette::accent;
        return {a.getFloatRed(), a.getFloatGreen(), a.getFloatBlue()};
    }

    void ensureField() {
        const auto& ms = host_.model().metapad;
        juce::String sig;
        sig << juce::String(ms.temperature, 3) << '|';
        for (const auto& p : ms.points)
            sig << p.snapshotIndex << ':' << juce::String(p.x, 4) << ',' << juce::String(p.y, 4) << ';';
        for (const auto& s : ms.snapshots) sig << s.index << '=' << juce::String(s.colour) << ';';
        if (sig == fieldSig_ && field_.isValid()) return;
        if (dragPoint_ >= 0 && field_.isValid()) return;
        fieldSig_ = sig;
        constexpr int W = 72, H = 48;
        field_ = juce::Image(juce::Image::RGB, W, H, false);
        std::vector<MetaRGB> cols(ms.points.size());
        for (size_t i = 0; i < ms.points.size(); ++i)
            cols[i] = rgbFor(ms.points[i].snapshotIndex);
        for (int py = 0; py < H; ++py)
            for (int px = 0; px < W; ++px) {
                const auto c = metapadFieldColour(ms.points, cols, (px + 0.5) / W,
                                                      (py + 0.5) / H, ms.temperature);
                field_.setPixelAt(px, py,
                                  juce::Colour::fromFloatRGBA(c.r, c.g, c.b, 1.0f)
                                      .withMultipliedBrightness(0.85f));
            }
    }

    void openColourPicker(int snapIndex, juce::Rectangle<int> screenArea) {
        struct Picker : juce::ColourSelector, juce::ChangeListener {
            Picker(MetapadView& v, int idx)
                : juce::ColourSelector(juce::ColourSelector::showColourspace), view(&v), index(idx) {
                addChangeListener(this);
            }
            ~Picker() override { removeChangeListener(this); }
            void changeListenerCallback(juce::ChangeBroadcaster*) override {
                if (view == nullptr) return;
                view->host_.metapad().setSnapshotColour(
                    index, ("#" + getCurrentColour().toDisplayString(false)).toStdString());
                view->refreshColours();
            }
            juce::Component::SafePointer<MetapadView> view;
            int index;
        };
        auto picker = std::make_unique<Picker>(*this, snapIndex);
        picker->setCurrentColour(colourFor(snapIndex), juce::dontSendNotification);
        picker->setSize(220, 170);
        juce::CallOutBox::launchAsynchronously(std::move(picker), screenArea, nullptr);
    }

    void refreshColours() {
        for (auto& r : rows_) r->swatch = colourFor(r->index);
        list_.repaint();
        repaint();
    }

    void rebuildList() {
        rows_.clear();
        for (auto& s : host_.model().metapad.snapshots)
            rows_.push_back(std::make_unique<Row>(*this, s.index, juce::String(s.name), colourFor(s.index)));
        for (auto& r : rows_) list_.addAndMakeVisible(*r);
        if (mask_) mask_->rebuild();
        layoutList();
    }
    void layoutList() {
        const int rowH = 26;
        list_.setSize(juce::jmax(200, listVp_.getWidth() - 8), juce::jmax(1, (int) rows_.size() * rowH));
        for (size_t i = 0; i < rows_.size(); ++i) rows_[i]->setBounds(0, (int) i * rowH, list_.getWidth(), rowH);
    }

    EngineHost& host_;
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
    explicit MetapadWindow(EngineHost& host)
        : FreeWindow("Metapad", new MetapadView(host)) {
        centreWithSize(740, 420);
    }
};

}
