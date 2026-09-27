// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/juce/JuceRichView.h"

#include "gui/bricks/BankFileSlot.h"
#include "gui/bricks/FileBox.h"
#include "gui/bricks/FileTransport.h"
#include "gui/bricks/RhythmicUnitPicker.h"
#include "gui/bricks/SoundFileSlot.h"
#include "gui/editor/BrickBindings.h"
#include "gui/editor/KnobModel.h"
#include "gui/editor/RangeKnob.h"
#include "gui/editor/RangeSlider.h"
#include "core/packs/Categories.h"
#include "gui/style/OriginColours.h"
#include "hum/ParseInt.h"

namespace hum {

std::unique_ptr<RichView> makeJucePickerBrick(juce::Component& parent, EditorHost& host, const LayoutSpec& spec,
                                              const LayoutSpec::Control& s, RichOwner& owner) {
    using CT = LayoutSpec::ControlType;
    const std::string pn = s.param;
    const std::string cn = owner.organism();

    switch (s.type) {
        case CT::RhythmicUnit:
            return reloadingView(parent, std::make_unique<RhythmicUnitPicker>(host, cn, s.param, s.param2),
                                 [](RhythmicUnitPicker& w) { w.refresh(); });
        case CT::SoundFile: {
            auto sf = std::make_unique<SoundFileSlot>(host, cn, pn, s.extraOr("filter"), s.extraOr("title"),
                                                      s.extraOr("kind"), s.extraOr("pick") == "save");
            using VideoOffer = files::SoundSlotModel::VideoOffer;
            const auto vids = s.extraOr("videos");
            sf->offersVideos(vids == "only" ? VideoOffer::Only
                             : vids == "1"  ? VideoOffer::Also
                                              : VideoOffer::No);
            if (const int origin = parseBoundedInt(s.extraOr("origin"), 2); origin >= 0)
                sf->setSwatch(originColour(origin));
            auto& slot = *sf;
            auto view = reloadingView(parent, std::move(sf), [](SoundFileSlot& w) { w.refresh(); });
            view->onText = [&slot] { return slot.shownName().toStdString(); };
            return view;
        }
        case CT::FileBox: {
            auto box = std::make_unique<FileBox>(host, cn, pn, s.extraOr("filter"), s.extraOr("title"),
                                                 s.extraOr("kind"), Bindings(spec, s));
            if (const int origin = parseBoundedInt(s.extraOr("origin"), 2); origin >= 0)
                box->setSwatch(originColour(origin));
            auto& b = *box;
            auto view = reloadingView(parent, std::move(box), [](FileBox& w) { w.refresh(); });
            view->onText = [&b] { return b.shownName().toStdString(); };
            return view;
        }
        case CT::ScaleFile:
        case CT::BankFile:
            return reloadingView(parent,
                                 std::make_unique<BankFileSlot>(
                                     host, cn, pn,
                                     banks::Slot{s.extraOr("kind", s.type == CT::ScaleFile ? BankBrowser::kScalesKind
                                                                                          : "Samples"),
                                                 s.extraOr("filter"), s.extraOr("factory")},
                                     Bindings(spec, s).assignment(bind::kSelect)),
                                 [](BankFileSlot& w) { w.refresh(); });
        case CT::FileTransport:
            return brickView(parent, std::make_unique<FileTransport>(host, cn, Bindings(spec, s),
                                                                     s.extraOr("seek", "1") != "0"));
        case CT::RangeKnob: {
            const auto setup = knob::setupFor(s, cn, owner.className());
            std::string declared;
            for (const auto& d : schemaFor(owner.className()))
                if (d.name == pn) declared = d.unit;
            auto knob = std::make_unique<RangeKnob>(host, setup.organism, setup.param, setup.lo, setup.hi,
                                                    s.decimalPlaces, setup.logarithmic,
                                                    (int) familyOf(owner.className()),
                                                    unitResolve(pn, declared, setup.lo, setup.hi));
            knob->onAutomationChanged = changedHook(owner);
            auto& k = *knob;
            auto view = brickView(parent, std::move(knob));
            view->onReload = [&k] { k.refresh(); };
            view->onLive = view->onReload;
            view->onMarks = [&k](bool controlled, bool) { k.setExternallyControlled(controlled); };
            return view;
        }
        case CT::RangeVSlider: {
            const auto setup = knob::setupFor(s, cn, owner.className());
            auto range = std::make_unique<RangeSlider>(host, setup.organism, setup.param, setup.lo, setup.hi,
                                                       s.decimalPlaces, setup.logarithmic);
            range->onAutomationChanged = changedHook(owner);
            auto& r = *range;
            auto view = brickView(parent, std::move(range));
            view->onReload = [&r] { r.refresh(); };
            view->onLive = view->onReload;
            view->onMarks = [&r](bool controlled, bool) { r.setExternallyControlled(controlled); };
            return view;
        }
        default:
            return nullptr;
    }
}

}
