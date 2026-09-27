// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/juce/JuceRichView.h"

#include <cstdlib>

#include "hum/caps/Files.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/BrickBindings.h"
#include "gui/bricks/GainReductionView.h"
#include "gui/bricks/TextFieldBrick.h"
#include "gui/bricks/IntervalRowsBrick.h"
#include "gui/bricks/StepStripBrick.h"
#include "gui/bricks/NumberBoxBrick.h"
#include "gui/bricks/LedView.h"
#include "gui/bricks/NumberFieldBrick.h"
#include "gui/bricks/MidiLogView.h"
#include "gui/bricks/OscLogView.h"
#include "gui/bricks/LevelMeterView.h"
#include "gui/bricks/InletLamp.h"
#include "gui/bricks/PictureLamp.h"
#include "gui/bricks/ThresholdMeterBrick.h"
#include "gui/bricks/VuMeterBrick.h"
#include "gui/bricks/FieldScopeBrick.h"
#include "gui/bricks/LfoScopeBrick.h"
#include "gui/bricks/RingFaceBrick.h"
#include "gui/bricks/SpectrumScopeBrick.h"
#include "gui/bricks/TraceScopeBrick.h"
#include "gui/bricks/NoteFieldBrick.h"
#include "gui/bricks/TapeLabelBrick.h"
#include "gui/bricks/KeyedTapeBrick.h"
#include "gui/editor/LayoutModel.h"
#include "gui/bricks/PitchFieldBrick.h"
#include "gui/bricks/PitchReadoutView.h"
#include "gui/bricks/RecorderTransportBrick.h"
#include "gui/bricks/TimecodeReadoutView.h"
#include "gui/bricks/ReadoutView.h"
#include "gui/bricks/TextReadoutView.h"
#include "gui/bricks/SigilView.h"
#include "gui/bricks/SoundMapView.h"
#include "gui/bricks/WaveDrawBrick.h"

namespace hum {

std::unique_ptr<RichView> makeJuceReadoutBrick(juce::Component& parent, EditorHost& host, const LayoutSpec& spec,
                                  const LayoutSpec::Control& s, RichOwner& owner) {
    using CT = LayoutSpec::ControlType;
    const std::string pn = s.param;
    const std::string cn = owner.organism();
    const Bindings bound(spec, s);

    switch (s.type) {
        case CT::MidiLog: {
            auto mv = std::make_unique<MidiLogView>(host, cn, bound);
            return editorView(parent, std::move(mv));
        }
        case CT::OscLog: {
            auto ov = std::make_unique<OscLogView>(host, cn, bound);
            return editorView(parent, std::move(ov));
        }
        case CT::LevelBars: {
            auto mv = std::make_unique<LevelMeterView>(host, cn);
            return editorView(parent, std::move(mv));
        }
        case CT::InletBars: {
            auto mv = std::make_unique<LevelMeterView>(host, cn, true);
            return editorView(parent, std::move(mv));
        }
        case CT::PictureLamp: {
            auto pl = std::make_unique<hum::PictureLamp>(host, cn);
            return editorView(parent, std::move(pl));
        }
        case CT::InletLamp: {
            auto il = std::make_unique<hum::InletLamp>(host, cn);
            return editorView(parent, std::move(il));
        }
        case CT::ThresholdMeter: {
            auto tm = std::make_unique<ThresholdMeterBrick>(host, cn, pn);
            tm->onPopup = [&host, cn, pn, changed = changedHook(owner)](juce::Point<int> pos) {
                showAutomateMenu(host, cn, pn, pos, changed);
            };
            return editorView(parent, std::move(tm));
        }
        case CT::VuMeter: {
            auto vu = std::make_unique<VuMeterView>(host, cn);
            return editorView(parent, std::move(vu));
        }
        case CT::FieldScope: {
            auto fs = std::make_unique<FieldScopeView>(host, cn);
            return editorView(parent, std::move(fs));
        }
        case CT::LfoScope: {
            auto ls = std::make_unique<LfoScopeView>(host, cn, bound);
            return editorView(parent, std::move(ls));
        }
        case CT::SpectrumScope: {
            auto ss = std::make_unique<SpectrumScopeView>(host, cn);
            return editorView(parent, std::move(ss));
        }
        case CT::RingFace: {
            auto rf = std::make_unique<RingFaceView>(host, cn);
            return editorView(parent, std::move(rf));
        }
        case CT::TraceScope: {
            auto ts = std::make_unique<TraceScopeView>(host, cn);
            return editorView(parent, std::move(ts));
        }
        case CT::PitchReadout: {
            auto pv = std::make_unique<PitchReadoutView>(host, cn);
            return editorView(parent, std::move(pv));
        }
        case CT::RecordTransport: {
            auto rt = std::make_unique<RecorderTransportBrick>(host, cn,
                                                               Bindings(spec, s)(bind::kRecord));
            return editorView(parent, std::move(rt));
        }
        case CT::TimecodeReadout: {
            auto tv = std::make_unique<TimecodeReadoutView>(host, cn);
            return editorView(parent, std::move(tv));
        }
        case CT::Readout: {
            auto rv = std::make_unique<ReadoutView>(host, cn, s.decimalPlaces);
            return editorView(parent, std::move(rv));
        }
        case CT::TextReadout: {
            auto tv = std::make_unique<TextReadoutView>(host, cn);
            return editorView(parent, std::move(tv));
        }
        case CT::Sigil: {
            auto sv = std::make_unique<SigilView>(host, cn);
            return editorView(parent, std::move(sv));
        }
        case CT::NoteField: {
            auto nf = std::make_unique<NoteFieldBrick>(host, cn, pn, layout::extraTrue(s.extraOr("pitch-class")));
            nf->onPopup = [&host, cn, pn, changed = changedHook(owner)](juce::Point<int> pos) {
                showAutomateMenu(host, cn, pn, pos, changed);
            };
            return editorView(parent, std::move(nf));
        }
        case CT::KeyedTape: {
            auto kt = std::make_unique<KeyedTapeBrick>(host, cn);
            return editorView(parent, std::move(kt));
        }
        case CT::TapeLabel: {
            auto tl = std::make_unique<TapeLabelBrick>(host, cn, pn, std::atoi(s.extraOr("inlet", "0").c_str()),
                                                       std::atoi(s.extraOr("width", "2").c_str()));
            return editorView(parent, std::move(tl));
        }
        case CT::PitchField: {
            auto pf = std::make_unique<PitchFieldBrick>(host, cn, pn);
            pf->onPopup = [&host, cn, pn, changed = changedHook(owner)](juce::Point<int> pos) {
                showAutomateMenu(host, cn, pn, pos, changed);
            };
            return editorView(parent, std::move(pf));
        }
        case CT::StepStrip: {
            auto ds = std::make_unique<StepStripBrick>(host, cn, bound);
            return editorView(parent, std::move(ds));
        }
        case CT::IntervalRows: {
            auto db = std::make_unique<IntervalRowsBrick>(host, cn, owner.className(), bound);
            return editorView(parent, std::move(db));
        }
        case CT::NumberField: {
            auto nf = std::make_unique<NumberFieldBrick>(host, cn, pn);
            nf->onAutomationChanged = changedHook(owner);
            return editorView(parent, std::move(nf));
        }
        case CT::NumberBox: {
            auto nb = std::make_unique<NumberBoxBrick>(host, cn, pn, s.decimalPlaces,
                                                       std::atof(s.extraOr("step", "0").c_str()),
                                                       s.extraOr("shows"));
            nb->onAutomationChanged = changedHook(owner);
            return editorView(parent, std::move(nb));
        }
        case CT::Led: {
            auto lv = std::make_unique<LedView>(host, cn, s.extraOr("shows"));
            return editorView(parent, std::move(lv));
        }
        case CT::GainReduction: {
            auto gr = std::make_unique<GainReductionView>(host, cn);
            return editorView(parent, std::move(gr));
        }
        case CT::TextField: {
            auto tf = std::make_unique<TextFieldBrick>(host, cn, pn,
                                                       juce::String::fromUTF8(s.label.c_str()),
                                                       std::atoi(s.extraOr("lines", "1").c_str()));
            return editorView(parent, std::move(tf));
        }
        case CT::WaveDraw: {
            auto wd = std::make_unique<WaveDrawBrick>(host, cn, pn, bound);
            return editorView(parent, std::move(wd));
        }
        case CT::SoundMap: {
            auto mv = std::make_unique<SoundMapView>(host, cn, pn, s.param2, bound);
            mv->onAutomationChanged = changedHook(owner);
            return editorView(parent, std::move(mv));
        }
        default:
            return nullptr;
    }
}

}
