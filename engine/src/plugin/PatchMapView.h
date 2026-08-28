#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "io/PatchDocument.h"
#include "plugin/FxLook.h"

namespace hum {

class PatchMapView : public juce::Component {
public:
    PatchMapView() = default;

    std::function<void(const std::string&)> onSelect;

    void setModel(const PatchDocumentModel* m) {
        model_ = m;
        rebuild();
        repaint();
    }
    void setSelected(const std::string& name) {
        selected_ = name;
        repaint();
    }
    void resized() override { rebuild(); }

    void paint(juce::Graphics& g) override {
        g.fillAll(fxlook::panel());
        g.setColour(fxlook::line());
        g.drawRect(getLocalBounds());
        if (model_ == nullptr || model_->organisms.empty()) {
            g.setColour(fxlook::dim());
            g.setFont(juce::FontOptions(13.0f));
            g.drawFittedText("Load a .hum patch.\n\n"
                             "The DAW's audio enters the patch at SoundIn;\n"
                             "whatever reaches SoundOut is the plugin's output.",
                             getLocalBounds().reduced(20), juce::Justification::centred, 6);
            return;
        }
        drawCords(g, model_->midiConnections, true);
        drawCords(g, model_->connections, false);
        g.setFont(juce::FontOptions(11.0f));
        for (const auto& b : boxes_) {
            const bool sel = b.name == selected_;
            g.setColour(fxlook::box());
            g.fillRoundedRectangle(b.r, 4.0f);
            g.setColour(sel ? fxlook::accent() : b.isIn || b.isOut
                                ? fxlook::accent().withAlpha(0.45f) : fxlook::line());
            g.drawRoundedRectangle(b.r, 4.0f, sel ? 1.6f : 1.0f);
            g.setColour(fxlook::text());
            g.drawFittedText(b.name, b.r.reduced(4, 2).toNearestInt(),
                             juce::Justification::centred, 1);
            if (b.isIn || b.isOut) {
                g.setColour(fxlook::accent());
                g.setFont(juce::FontOptions(8.0f).withStyle("Bold"));
                g.drawText(b.isIn ? "DAW IN" : "DAW OUT",
                           b.r.toNearestInt().removeFromTop(9).reduced(3, 0),
                           b.isIn ? juce::Justification::topLeft
                                  : juce::Justification::topRight, false);
                g.setFont(juce::FontOptions(11.0f));
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        for (auto it = boxes_.rbegin(); it != boxes_.rend(); ++it)
            if (it->r.contains(e.position)) {
                if (onSelect) onSelect(it->name);
                return;
            }
    }

private:
    struct Box {
        std::string name;
        juce::Rectangle<float> r;
        bool isIn = false, isOut = false;
    };

    static bool classIsIn(const std::string& c) { return c == "SoundIn" || c == "AuxIn"; }
    static bool classIsOut(const std::string& c) { return c == "SoundOut" || c == "AuxOut"; }

    void rebuild() {
        boxes_.clear();
        if (model_ == nullptr || model_->organisms.empty() || getWidth() < 40) return;
        constexpr float kW = 104.0f, kH = 26.0f;
        juce::Rectangle<float> all;
        int fallback = 0;
        for (const auto& cm : model_->organisms) {
            Box b;
            b.name = cm.name;
            b.isIn = classIsIn(cm.displayClass);
            b.isOut = classIsOut(cm.displayClass);
            float x = (float) (120 * (fallback % 4)), y = (float) (44 * (fallback / 4));
            for (const auto& v : model_->views)
                if (v.organismName == cm.name) {
                    x = (float) v.patcherX;
                    y = (float) v.patcherY;
                    break;
                }
            ++fallback;
            b.r = {x, y, kW, kH};
            all = boxes_.empty() ? b.r : all.getUnion(b.r);
            boxes_.push_back(std::move(b));
        }
        const auto area = getLocalBounds().toFloat().reduced(14.0f);
        const float sc = juce::jlimit(0.25f, 1.15f,
                                      std::min(area.getWidth() / std::max(1.0f, all.getWidth()),
                                               area.getHeight() / std::max(1.0f, all.getHeight())));
        const float ox = area.getX() + (area.getWidth() - all.getWidth() * sc) / 2.0f;
        const float oy = area.getY() + (area.getHeight() - all.getHeight() * sc) / 2.0f;
        for (auto& b : boxes_)
            b.r = {ox + (b.r.getX() - all.getX()) * sc, oy + (b.r.getY() - all.getY()) * sc,
                   b.r.getWidth() * sc, b.r.getHeight() * sc};
    }

    const Box* boxFor(const std::string& name) const {
        for (const auto& b : boxes_)
            if (b.name == name) return &b;
        return nullptr;
    }

    void drawCords(juce::Graphics& g, const std::vector<ConnectionModel>& conns, bool dotted) {
        g.setColour(dotted ? fxlook::dim().withAlpha(0.7f) : fxlook::accent().withAlpha(0.4f));
        for (const auto& c : conns) {
            const Box* s = boxFor(c.src);
            const Box* d = boxFor(c.dst);
            if (s == nullptr || d == nullptr) continue;
            const auto p1 = juce::Point<float>(
                juce::jmin(s->r.getRight() - 5.0f, s->r.getX() + 8.0f + 9.0f * (float) c.srcOutlet),
                s->r.getBottom());
            const auto p2 = juce::Point<float>(
                juce::jmin(d->r.getRight() - 5.0f, d->r.getX() + 8.0f + 9.0f * (float) c.dstInlet),
                d->r.getY());
            juce::Path path;
            path.startNewSubPath(p1);
            const float bend = juce::jmax(14.0f, std::abs(p2.y - p1.y) * 0.4f);
            path.cubicTo(p1.translated(0, bend), p2.translated(0, -bend), p2);
            if (dotted) {
                const float dashes[] = {3.0f, 3.0f};
                juce::PathStrokeType(1.0f).createDashedStroke(path, path, dashes, 2);
                g.strokePath(path, juce::PathStrokeType(1.0f));
            } else {
                g.strokePath(path, juce::PathStrokeType(1.2f));
            }
        }
    }

    const PatchDocumentModel* model_ = nullptr;
    std::vector<Box> boxes_;
    std::string selected_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchMapView)
};

}
