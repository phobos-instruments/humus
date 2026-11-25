// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/common/PerfLog.h"
#include "gui/patcher/PatchView.h"

#include <cmath>

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/plugins/PluginNode.h"
#include "core/graph/PodModel.h"
#include "gui/style/DropBadge.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

static constexpr int kBodyPad = 2;

const juce::Image& PatchView::nodeBody(int w, int h, bool sel, float scale) {
    const juce::uint32 sig = Palette::panel.getARGB()
                             ^ (Palette::accent.getARGB() * 2654435761u)
                             ^ (Palette::border.getARGB() * 40503u);
    if (sig != bodySig_) {
        bodyCache_.clear();
        bodySig_ = sig;
    }
    const auto q = (juce::int64) std::lround(scale * 4.0f);
    const juce::int64 key =
        ((((q << 16) | (juce::int64) h) << 16 | (juce::int64) w) << 1) | (sel ? 1 : 0);
    if (auto it = bodyCache_.find(key); it != bodyCache_.end()) return it->second;

    juce::Image img(juce::Image::ARGB,
                    (int) std::ceil((float) (w + kBodyPad * 2) * scale),
                    (int) std::ceil((float) (h + kBodyPad * 2) * scale), true);
    {
        juce::Graphics ig(img);
        ig.addTransform(juce::AffineTransform::scale(scale));
        const juce::Rectangle<float> bf((float) kBodyPad, (float) kBodyPad, (float) w, (float) h);
        ig.setColour(juce::Colours::black.withAlpha(alpha::scrim));
        ig.fillRoundedRectangle(bf.translated(0.0f, 1.5f), 7.0f);
        if (sel) {
            ig.setColour(Palette::accent);
            ig.fillRoundedRectangle(bf, 7.0f);
        } else {
            juce::ColourGradient grad(Palette::panel.brighter(0.10f), 0.0f, bf.getY(),
                                      Palette::panel.darker(0.15f), 0.0f, bf.getBottom(), false);
            ig.setGradientFill(grad);
            ig.fillRoundedRectangle(bf, 7.0f);
        }
        ig.setColour(sel ? Palette::accent : Palette::border);
        ig.drawRoundedRectangle(bf, 7.0f, 1.0f);
    }
    return bodyCache_.emplace(key, std::move(img)).first->second;
}

void PatchView::paint(juce::Graphics& g) {
    perf::Scope scope("canvas.paint");
    g.fillAll(Palette::background);
    const auto clipR = (g.getClipBounds().toFloat() / zoom_).getSmallestIntegerContainer();
    g.addTransform(juce::AffineTransform::scale(zoom_));

    auto drawCord = [&](juce::Point<float> a, juce::Point<float> b, bool midi,
                        bool video, bool sel, bool hov, juce::Colour audioBase) {
        const juce::Rectangle<float> box(juce::jmin(a.x, b.x) - 4.0f,
                                         juce::jmin(a.y, b.y) - 34.0f,
                                         std::abs(a.x - b.x) + 8.0f,
                                         std::abs(a.y - b.y) + 68.0f);
        if (!box.intersects(clipR.toFloat())) return;
        juce::Path path;
        path.startNewSubPath(a);
        path.cubicTo({a.x, a.y + 30}, {b.x, b.y - 30}, b);
        const auto base = midi ? Palette::midiCord()
                         : video ? Palette::videoCord() : audioBase;
        g.setColour(sel ? Palette::accent : (hov ? Palette::accent.withAlpha(alpha::mid) : base));
        juce::PathStrokeType stroke(sel ? 3.0f : (hov ? 2.5f : 2.0f),
                                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        if (midi) {
            const float dashes[] = {4.0f, 4.0f};
            juce::Path dashed;
            stroke.createDashedStroke(dashed, path, dashes, 2);
            g.fillPath(dashed);
        } else {
            g.strokePath(path, stroke);
        }
    };
    for (const auto& line : visibleCords()) {
        if (isCarried(line.cord)) continue;
        drawCord(line.a, line.b, line.cord.midi, line.cord.video, isCordSelected(line.cord),
                 cordHovered_ && line.cord == hoverCord_,
                 line.cord.control ? Palette::controlCord() : Palette::cord);
    }
    if (drag_ == Drag::Cord) {
        int tmp, dragOuts;
        portCounts(dragNode_, tmp, dragOuts);
        auto a = cordControl_ ? controlOutletPos(dragNode_, cordOutlet_).toFloat()
               : cordMidi_ ? midiOutletPos(dragNode_, cordOutlet_).toFloat()
               : cordVideo_ ? videoOutletPos(dragNode_, cordOutlet_).toFloat()
                            : outletPos(dragNode_, cordOutlet_, dragOuts).toFloat();
        g.setColour(Palette::accent);
        g.drawLine({a, cordEnd_.toFloat()}, 1.5f);
    }
    for (const auto& line : carriedLines())
        drawCord(line.a, line.b, line.cord.midi, line.cord.video, false, true,
                 line.cord.control ? Palette::controlCord() : Palette::cord);

    if (!primary_.empty() && (!spliceBundle_.empty() || !proxConnections_.empty())) {
        auto nb = nodeBounds(primary_);
        int ins, outs;
        portCounts(primary_, ins, outs);
        g.setColour(Palette::accent);
        if (!spliceBundle_.empty()) {
            g.drawRoundedRectangle(nb.toFloat().expanded(3.0f), 5.0f, 2.0f);
            int i = 0;
            for (auto& e : spliceBundle_) {
                int so = src_.outletsOf(e.src), si = src_.inletsOf(e.dst);
                auto a = outletPos(e.src, e.srcOutlet, so).toFloat();
                auto b = inletPos(e.dst, e.dstInlet, si).toFloat();
                auto ni = inletPos(primary_, std::min(i, ins - 1), ins).toFloat();
                auto no = outletPos(primary_, std::min(i, outs - 1), outs).toFloat();
                g.drawLine({a, ni}, 2.0f);
                g.drawLine({no, b}, 2.0f);
                ++i;
            }
        } else {
            for (auto& e : proxConnections_) {
                if (e.control) {
                    g.setColour(Palette::controlCord());
                    g.drawLine({controlOutletPos(e.src, e.srcOutlet).toFloat(),
                                controlInletPos(e.dst, e.dstInlet).toFloat()}, 2.0f);
                    g.setColour(Palette::accent);
                } else if (e.video) {
                    g.setColour(Palette::videoCord());
                    g.drawLine({videoOutletPos(e.src, e.srcOutlet).toFloat(),
                                videoInletPos(e.dst, e.dstInlet).toFloat()}, 2.0f);
                    g.setColour(Palette::accent);
                } else if (e.midi) {
                    g.setColour(Palette::midiCord());
                    g.drawLine({midiOutletPos(e.src, e.srcOutlet).toFloat(),
                                midiInletPos(e.dst, e.dstInlet).toFloat()}, 2.0f);
                    g.setColour(Palette::accent);
                } else {
                    int so = src_.outletsOf(e.src), si = src_.inletsOf(e.dst);
                    g.drawLine({outletPos(e.src, e.srcOutlet, so).toFloat(),
                                inletPos(e.dst, e.dstInlet, si).toFloat()}, 2.0f);
                }
            }
        }
    }

    if (alignGuideX_ >= 0) {
        g.setColour(Palette::accent.withAlpha(alpha::muted));
        g.fillRect(alignGuideX_, 0, 1, getHeight());
    }
    if (alignGuideY_ >= 0) {
        g.setColour(Palette::accent.withAlpha(alpha::muted));
        g.fillRect(0, alignGuideY_, getWidth(), 1);
    }

    const auto nowMs = juce::Time::getMillisecondCounter();
    for (auto& d : displayNodes()) {
        const auto& name = d.name;
        auto b = nodeBounds(name);
        if (!b.expanded(10).intersects(clipR)) continue;
        const auto bf = b.toFloat();
        bool sel = selection_.count(name) > 0;
        int flowLevel = 0;
        bool midiFlash = false;
        if (auto fit = flow_.find(name); fit != flow_.end()) {
            flowLevel = fit->second.level;
            midiFlash = fit->second.flashUntil > nowMs;
        }
        const auto& body = nodeBody(b.getWidth(), b.getHeight(), sel,
                                    g.getInternalContext().getPhysicalPixelScaleFactor());
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.setOpacity(1.0f);
        g.drawImage(body, bf.expanded((float) kBodyPad));
        if (flowLevel > 0 && !sel) {
            g.setColour(Palette::accent.withAlpha(flowLevel == 2 ? 0.65f : 0.30f));
            g.drawRoundedRectangle(bf.reduced(0.5f), 7.0f, 1.6f);
        }
        if (d.pod) {
            if (flowLevel == 2 && !sel) {
                g.setColour(Palette::accent.withAlpha(alpha::veil));
                g.drawRoundedRectangle(bf.expanded(2.0f), 9.0f, 4.5f);
            }
            const float a = flowLevel == 2 ? 0.95f : flowLevel == 1 ? 0.65f : 0.25f;
            g.setColour(sel ? Palette::background.withAlpha(alpha::dim)
                            : Palette::accent.withAlpha(a));
            g.drawRoundedRectangle(bf.reduced(3.0f), 5.0f,
                                   flowLevel > 0 && !sel ? 1.6f : 1.0f);
        }

        if (!d.pod && !sel) {
            if (const auto* cm = src_.model().byName(name)) {
                const auto f = familyOf(cm->displayClass);
                if (f != Family::Utility) {
                    const auto fam = Palette::familyAccent(f);
                    const float y = bf.getY() + 1.0f;
                    const float x0 = bf.getX() + 6.0f, x1 = bf.getRight() - 6.0f;
                    juce::ColourGradient spine(fam.withAlpha(alpha::none), x0, y,
                                               fam.withAlpha(alpha::none), x1, y, false);
                    spine.addColour(0.18, fam.withAlpha(alpha::nearOpaque));
                    spine.addColour(0.82, fam.withAlpha(alpha::nearOpaque));
                    g.setGradientFill(spine);
                    g.fillRect(x0, y, x1 - x0, 1.6f);
                }
            }
        }

        const bool off = src_.bypassed(name);
        if (off) {
            g.setColour(Palette::background.withAlpha(alpha::mid));
            g.fillRoundedRectangle(bf.reduced(0.5f), 7.0f);
        }

        g.setColour(off ? Palette::textDim : (sel ? Palette::background : Palette::accent));
        g.setFont(13.5f);
        g.drawText(d.pod ? juce::String::fromUTF8((pods::leafOf(name) + "  \xe2\x96\xb8").c_str())
                         : juce::String::fromUTF8(pods::leafOf(name).c_str()),
                   b.reduced(6, 2), juce::Justification::centred, true);
        if (off) {
            g.setColour(Palette::recordRed().withAlpha(alpha::heavy));
            g.drawLine(bf.getX() + 7.0f, bf.getBottom() - 7.0f,
                       bf.getRight() - 7.0f, bf.getY() + 7.0f, 2.0f);
        }

        if (!d.pod) {
            if (src_.silent(name)) {
                g.setColour(Palette::warnAmber());
                g.fillEllipse((float) b.getRight() - 12.0f, (float) b.getY() + 4.0f, 7.0f, 7.0f);
            }
        }

        int ins, outs;
        portCounts(name, ins, outs);
        auto portSeed = [&](juce::Point<int> p, juce::Colour c) {
            const float r = kPort * 0.5f;
            g.setColour(c);
            g.fillEllipse((float) p.x - r, (float) p.y - r, r * 2.0f, r * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(alpha::scrim));
            g.fillEllipse((float) p.x - r * 0.45f, (float) p.y - r * 0.65f, r * 0.55f, r * 0.55f);
        };
        const bool fading = cordInHand();
        auto seed = [&](juce::Point<int> p, juce::Colour c, PortRef at) {
            portSeed(p, fading && refusesDuringDrag(at) ? c.withMultipliedAlpha(alpha::scrim) : c);
        };
        const auto portCol = sel ? Palette::background : Palette::textDim;
        const int strayIn = d.pod ? pods::strayCount(src_.model(), name, true) : 0;
        const int strayOut = d.pod ? pods::strayCount(src_.model(), name, false) : 0;
        for (int i = 0; i < ins; ++i)
            seed(inletPos(name, i, ins), i >= ins - strayIn ? Palette::warnAmber() : portCol,
                 {fading ? name : std::string(), i, false});
        for (int o = 0; o < outs; ++o)
            seed(outletPos(name, o, outs), o >= outs - strayOut ? Palette::warnAmber() : portCol,
                 {fading ? name : std::string(), o, true});
        int cIns, cOuts;
        controlPortCounts(name, cIns, cOuts);
        const auto controlCol = sel ? Palette::background : Palette::controlCord();
        for (int i = 0; i < cIns; ++i)
            seed(controlInletPos(name, i), controlCol, {fading ? name : std::string(), i, false, false, false, true});
        for (int o = 0; o < cOuts; ++o)
            seed(controlOutletPos(name, o), controlCol, {fading ? name : std::string(), o, true, false, false, true});

        int mIns, mOuts;
        midiPortCounts(name, mIns, mOuts);
        const auto midiCol = sel ? Palette::background
                           : midiFlash ? Palette::midiCord().brighter(0.9f)
                                       : Palette::midiCord();
        auto midiSeed = [&](juce::Point<int> p, int port, bool outlet) {
            if (midiFlash) {
                g.setColour(Palette::midiCord().withAlpha(alpha::muted));
                const float hr = kPort * 1.1f;
                g.fillEllipse((float) p.x - hr, (float) p.y - hr, hr * 2.0f, hr * 2.0f);
            }
            seed(p, midiCol, {fading ? name : std::string(), port, outlet, true});
        };
        for (int i = 0; i < mIns; ++i) midiSeed(midiInletPos(name, i), i, false);
        for (int o = 0; o < mOuts; ++o) midiSeed(midiOutletPos(name, o), o, true);

        int vIns, vOuts;
        videoPortCounts(name, vIns, vOuts);
        const auto videoCol = sel ? Palette::background : Palette::videoCord();
        for (int i = 0; i < vIns; ++i)
            seed(videoInletPos(name, i), videoCol, {fading ? name : std::string(), i, false, false, true});
        for (int o = 0; o < vOuts; ++o)
            seed(videoOutletPos(name, o), videoCol, {fading ? name : std::string(), o, true, false, true});
    }

    auto ring = [&](juce::Point<int> pt, juce::Colour col, float thick) {
        const float r = kPort / 2 + 2.0f;
        g.setColour(col);
        g.drawEllipse(pt.x - r, pt.y - r, r * 2.0f, r * 2.0f, thick);
    };
    if (portHover_ && drag_ != Drag::Cord) {
        juce::Point<int> pp;
        int hIns, hOuts;
        portCounts(hoverPortNode_, hIns, hOuts);
        if (hoverPortControl_)
            pp = hoverPortOut_ ? controlOutletPos(hoverPortNode_, hoverPort_)
                               : controlInletPos(hoverPortNode_, hoverPort_);
        else if (hoverPortMidi_)
            pp = hoverPortOut_ ? midiOutletPos(hoverPortNode_, hoverPort_)
                               : midiInletPos(hoverPortNode_, hoverPort_);
        else if (hoverPortVideo_)
            pp = hoverPortOut_ ? videoOutletPos(hoverPortNode_, hoverPort_)
                               : videoInletPos(hoverPortNode_, hoverPort_);
        else
            pp = hoverPortOut_ ? outletPos(hoverPortNode_, hoverPort_, hOuts)
                               : inletPos(hoverPortNode_, hoverPort_, hIns);
        ring(pp, Palette::accent, 2.0f);
    }
    if (drag_ == Drag::Cord && cordTarget_) {
        int tIns, tOuts;
        portCounts(cordTargetNode_, tIns, tOuts);
        auto pp = cordControl_ ? controlInletPos(cordTargetNode_, cordTargetPort_)
                : cordMidi_ ? midiInletPos(cordTargetNode_, cordTargetPort_)
                : cordVideo_ ? videoInletPos(cordTargetNode_, cordTargetPort_)
                             : inletPos(cordTargetNode_, cordTargetPort_, tIns);
        ring(pp, Palette::accent, 2.5f);
    }
    if (drag_ == Drag::Grab && cordTarget_) ring(portPos(grabTarget_), Palette::accent, 2.5f);
    if (refused_) ring(portPos(refusedAt_), ink::state::danger, 2.5f);

    if (!dropInto_.empty()) {
        const auto box = nodeBounds(dropInto_).toFloat().expanded(3.0f);
        g.setColour(Palette::accent);
        g.drawRoundedRectangle(box, 9.0f, 2.0f);
        drop::paintCornerBadge(g, box);
    }

    if (drag_ == Drag::Select || drag_ == Drag::CordBand) {
        g.setColour(Palette::accent.withAlpha(alpha::mist));
        g.fillRect(selectRect_);
        g.setColour(Palette::accent.withAlpha(alpha::strong));
        g.drawRect(selectRect_, 1);
    }

}

}
