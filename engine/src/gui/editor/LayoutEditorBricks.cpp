// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHostPattern.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/bricks/KnobGridBrick.h"
#include "gui/bricks/BasslineImportBrick.h"
#include "gui/editor/BrickBindings.h"
#include "gui/bricks/StepNudgeBrick.h"
#include "gui/bricks/TakeLaneBrick.h"
#include "gui/editor/LayoutEditor.h"
#include "gui/bricks/TapButton.h"
#include "gui/bricks/DeckView.h"
#include "gui/bricks/DeckControls.h"
#include "gui/bricks/FaderBank.h"
#include "gui/bricks/LooperBricks.h"
#include "gui/bricks/MidiKeyboardPanel.h"
#include "gui/bricks/PatternEditor.h"
#include "gui/bricks/PatternStepGrid.h"
#include "gui/pianoroll/PianoRollEditor.h"
#include "gui/bricks/PitchFader.h"
#include "gui/bricks/WaveformDisplay.h"
#include "gui/bricks/GainShapeBrick.h"
#include "gui/bricks/HandGestureBrick.h"
#include "gui/bricks/SequenceGridBrick.h"

#include <cstdlib>

#include "gui/bricks/CamPreview.h"
#include "gui/bricks/FormulaBrick.h"
#include "gui/bricks/GainReductionView.h"
#include "gui/bricks/TextFieldBrick.h"
#include "gui/bricks/StrandsBrick.h"
#include "gui/bricks/PictureFieldBrick.h"
#include "gui/bricks/SliceMapBrick.h"
#include "gui/bricks/VideoPreview.h"
#include "gui/bricks/ClipGridBrick.h"
#include "gui/bricks/ScreenButton.h"
#include "gui/bricks/VideoTransportBrick.h"
#include "gui/bricks/IntervalRowsBrick.h"
#include "gui/bricks/StepStripBrick.h"
#include "gui/bricks/NumberBoxBrick.h"
#include "gui/bricks/LedView.h"
#include "gui/bricks/NumberFieldBrick.h"
#include "gui/bricks/MidiLogView.h"
#include "gui/bricks/OscLogView.h"
#include "gui/bricks/LevelMeterView.h"
#include "gui/bricks/ThresholdMeterBrick.h"
#include "gui/bricks/VuMeterBrick.h"
#include "gui/bricks/FieldScopeBrick.h"
#include "gui/bricks/LfoScopeBrick.h"
#include "gui/bricks/SpectrumScopeBrick.h"
#include "gui/bricks/NoteFieldBrick.h"
#include "gui/bricks/PitchReadoutView.h"
#include "gui/bricks/ReadoutView.h"
#include "gui/bricks/TextReadoutView.h"
#include "gui/editor/Mappable.h"
#include "gui/bricks/SigilView.h"
#include "gui/bricks/SoundMapView.h"
#include "gui/bricks/WaveDrawBrick.h"

namespace hum {

bool LayoutEditor::buildBrick(const LayoutSpec::Control& s, Control& c) {
    using CT = LayoutSpec::ControlType;
    const std::string pn = s.param;
    const std::string cn = name_;
    const Bindings bound(spec_, s);

    switch (s.type) {
        case CT::Deck: {
            auto dv = std::make_unique<DeckView>(host_, cn, bound);
            addAndMakeVisible(*dv);
            c.adopt(std::move(dv));
            return true;
        }
        case CT::DeckPitch: {
            auto pf = std::make_unique<PitchFader>(host_, cn, bound);
            addAndMakeVisible(*pf);
            c.adopt(std::move(pf));
            return true;
        }
        case CT::DeckControls: {
            auto dc = std::make_unique<DeckControls>(host_, cn, bound);
            addAndMakeVisible(*dc);
            c.adopt(std::move(dc));
            return true;
        }
        case CT::MidiKeyboard: {
            auto kb = std::make_unique<MidiKeyboardStrip>(host_, cn);
            addAndMakeVisible(*kb);
            c.adopt(std::move(kb));
            return true;
        }
        case CT::FaderBank: {
            std::vector<FaderBank::Spec> specs;
            for (const auto& d : schemaFor(className()))
                if (d.name.rfind(pn, 0) == 0)
                    specs.push_back({d.name, juce::String::fromUTF8(s.label.c_str())
                                          + juce::String(d.name.substr(pn.size())),
                                     d.min, d.max});
            auto fb = std::make_unique<FaderBank>(host_, cn, std::move(specs));
            fb->onChange = [this] { repaint(); };
            fb->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
            addAndMakeVisible(*fb);
            c.adopt(std::move(fb));
            return true;
        }
        case CT::Waveform: {
            std::vector<std::string> params;
            for (const auto& d : schemaFor(className()))
                if (d.name.rfind(pn, 0) == 0) params.push_back(d.name);
            auto wf = std::make_unique<WaveformDisplay>(host_, cn, std::move(params));
            addAndMakeVisible(*wf);
            c.adopt(std::move(wf));
            return true;
        }
        case CT::Momentary: {
            auto mb = std::make_unique<Mappable<MomentaryButton>>(
                host_, cn, pn, juce::String::fromUTF8(s.label.c_str()),
                s.extraOr("hold", "") == "true");
            mb->onRightClick = [this, cn, pn](juce::Point<int> pos) {
                showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
            };
            addAndMakeVisible(*mb);
            auto* w = c.adopt(std::move(mb));
            c.refreshBrickLive = [this, w, cn, pn] {
                const bool on = host_.isLiveTracked(cn, pn) && host_.liveParamValue(cn, pn) >= 0.5;
                if (w->getToggleState() != on) w->setToggleState(on, juce::dontSendNotification);
            };
            return true;
        }
        case CT::TapTempo: {
            auto tb = std::make_unique<Mappable<TapButton>>(
                host_, cn, pn, s.extraOr("off", ""),
                juce::String::fromUTF8(s.label.c_str()));
            tb->onRightClick = [this, cn, pn](juce::Point<int> pos) {
                showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
            };
            addAndMakeVisible(*tb);
            c.adopt(std::move(tb));
            return true;
        }
        case CT::LooperTracks: {
            int count = 0;
            for (const auto& d : schemaFor(className()))
                if (d.name.rfind(pn, 0) == 0) ++count;
            auto ts = std::make_unique<LooperTrackStrip>(host_, cn, pn, count);
            addAndMakeVisible(*ts);
            auto* w = c.adopt(std::move(ts));
            c.reloadBrick = [w] { w->reload(); };
            return true;
        }
        case CT::HandGestures: {
            auto hg = std::make_unique<HandGestureBrick>(host_, cn, bound);
            addAndMakeVisible(*hg);
            c.adopt(std::move(hg));
            return true;
        }
        case CT::SequenceGrid: {
            auto sg = std::make_unique<SequenceGridBrick>(host_, cn, bound);
            addAndMakeVisible(*sg);
            c.adopt(std::move(sg));
            return true;
        }
        case CT::GainShapeCurve: {
            auto gs = std::make_unique<GainShapeBrick>(host_, cn, pn);
            addAndMakeVisible(*gs);
            c.adopt(std::move(gs));
            return true;
        }
        case CT::StepGrid: {
            const bool arp = s.extraOr("mode", "bassline") == "arp";
            const int steps = std::atoi(s.extraOr("steps", arp ? "32" : "16").c_str());
            host_.patterns().ensureMatrix(cn, arp ? "trigger-tie-matrix" : "bassline-pattern-matrix",
                                          steps, s.extraOr("seed", ""));
            auto g = std::make_unique<PatternStepGrid>(
                host_, cn, arp ? PatternStepGrid::Mode::Arp : PatternStepGrid::Mode::Bassline,
                bound(bind::kNudge), bound(bind::kTranspose));
            addAndMakeVisible(*g);
            auto* w = c.adopt(std::move(g));
            c.reloadBrick = [w] { w->reload(); };
            return true;
        }
        case CT::PatternGrid: {
            PatternEditorSpec ps;
            ps.valid = true;
            ps.laneCount = std::atoi(s.extraOr("lanes", "8").c_str());
            ps.masterVolumeParam = bound(bind::kMasterVolume);
            ps.muteParam = bound(bind::kMute);
            ps.gateParam = bound(bind::kGate);
            ps.volumeParamPrefix = bound(bind::kVolumePrefix);
            ps.enableParamPrefix = bound(bind::kEnablePrefix);
            ps.fileParamPrefix = bound(bind::kFilePrefix);
            auto pe = std::make_unique<PatternEditor>(host_, cn, std::move(ps));
            pe->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
            addAndMakeVisible(*pe);
            c.rich = std::move(pe);
            return true;
        }
        case CT::PianoRoll: {
            auto pr = std::make_unique<PianoRollEditor>(
                host_, cn, PianoRollEditor::Params{bound(bind::kBars), bound(bind::kSwing),
                                                   bound(bind::kSwingFollow), bound(bind::kSwingUnit)});
            pr->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
            addAndMakeVisible(*pr);
            c.rich = std::move(pr);
            return true;
        }
        case CT::Camera: {
            auto cv = std::make_unique<CamPreview>(host_, cn);
            addAndMakeVisible(*cv);
            c.rich = std::move(cv);
            return true;
        }
        case CT::VideoPreview: {
            auto vp = std::make_unique<hum::VideoPreview>(host_, cn, bound);
            addAndMakeVisible(*vp);
            c.rich = std::move(vp);
            return true;
        }
        case CT::VideoTransport: {
            auto vt = std::make_unique<VideoTransportBrick>(host_, cn, bound);
            addAndMakeVisible(*vt);
            c.rich = std::move(vt);
            return true;
        }
        case CT::ClipGrid: {
            auto cr = std::make_unique<ClipGridBrick>(host_, cn, bound);
            addAndMakeVisible(*cr);
            c.rich = std::move(cr);
            return true;
        }
        case CT::ScreenButton: {
            auto sb = std::make_unique<ScreenButton>(host_, cn);
            addAndMakeVisible(*sb);
            c.rich = std::move(sb);
            return true;
        }
        case CT::KnobGrid: {
            std::vector<std::string> fields;
            const auto spec = s.extraOr("fields");
            for (size_t i = 0, start = 0; i <= spec.size(); ++i)
                if (i == spec.size() || spec[i] == ',') {
                    auto f = spec.substr(start, i - start);
                    while (!f.empty() && f.front() == ' ') f.erase(f.begin());
                    while (!f.empty() && f.back() == ' ') f.pop_back();
                    if (!f.empty()) fields.push_back(f);
                    start = i + 1;
                }
            const int rows = std::max(1, std::atoi(s.extraOr("rows").c_str()));
            auto kg = std::make_unique<KnobGridBrick>(host_, cn, s.extraOr("prefix"),
                                                      rows, std::move(fields));
            addAndMakeVisible(*kg);
            c.rich = std::move(kg);
            return true;
        }
        case CT::Formula: {
            auto fb = std::make_unique<FormulaBrick>(host_, cn, pn, bound);
            addAndMakeVisible(*fb);
            c.rich = std::move(fb);
            return true;
        }
        case CT::PictureField: {
            auto pf = std::make_unique<PictureFieldBrick>(host_, cn, pn, bound);
            addAndMakeVisible(*pf);
            c.rich = std::move(pf);
            return true;
        }
        case CT::SliceMap: {
            auto sm = std::make_unique<SliceMapBrick>(host_, cn, bound);
            addAndMakeVisible(*sm);
            c.rich = std::move(sm);
            return true;
        }
        case CT::Strands: {
            auto hv = std::make_unique<StrandsView>(host_, cn, bound);
            addAndMakeVisible(*hv);
            c.rich = std::move(hv);
            return true;
        }
        case CT::MidiLog: {
            auto mv = std::make_unique<MidiLogView>(host_, cn, bound);
            addAndMakeVisible(*mv);
            c.rich = std::move(mv);
            return true;
        }
        case CT::OscLog: {
            auto ov = std::make_unique<OscLogView>(host_, cn, bound);
            addAndMakeVisible(*ov);
            c.rich = std::move(ov);
            return true;
        }
        case CT::StepNudge: {
            auto nudge = std::make_unique<StepNudgeBrick>(host_, cn, pn);
            nudge->onNudged = [this] { reloadValues(); };
            addAndMakeVisible(*nudge);
            c.rich = std::move(nudge);
            return true;
        }
        case CT::BasslineImport: {
            auto load = std::make_unique<BasslineImportBrick>(host_, cn, juce::String::fromUTF8(s.label.c_str()));
            load->onLoaded = [this] { reloadValues(); };
            addAndMakeVisible(*load);
            c.rich = std::move(load);
            return true;
        }
        case CT::TakeLane: {
            auto lane = std::make_unique<TakeLaneBrick>(
                host_, cn, std::max(1, std::atoi(s.extraOr("track", "1").c_str())), bound);
            addAndMakeVisible(*lane);
            c.rich = std::move(lane);
            return true;
        }
        case CT::LevelBars: {
            auto mv = std::make_unique<LevelMeterView>(host_, cn);
            addAndMakeVisible(*mv);
            c.rich = std::move(mv);
            return true;
        }
        case CT::ThresholdMeter: {
            auto tm = std::make_unique<ThresholdMeterBrick>(host_, cn, pn);
            tm->onPopup = [this, cn, pn](juce::Point<int> pos) {
                showAutomateMenu(host_, cn, pn, pos,
                                 [this] { if (onAutomationChanged) onAutomationChanged(); });
            };
            addAndMakeVisible(*tm);
            c.rich = std::move(tm);
            return true;
        }
        case CT::VuMeter: {
            auto vu = std::make_unique<VuMeterView>(host_, cn);
            addAndMakeVisible(*vu);
            c.rich = std::move(vu);
            return true;
        }
        case CT::FieldScope: {
            auto fs = std::make_unique<FieldScopeView>(host_, cn);
            addAndMakeVisible(*fs);
            c.rich = std::move(fs);
            return true;
        }
        case CT::LfoScope: {
            auto ls = std::make_unique<LfoScopeView>(host_, cn, bound);
            addAndMakeVisible(*ls);
            c.rich = std::move(ls);
            return true;
        }
        case CT::SpectrumScope: {
            auto ss = std::make_unique<SpectrumScopeView>(host_, cn);
            addAndMakeVisible(*ss);
            c.rich = std::move(ss);
            return true;
        }
        case CT::PitchReadout: {
            auto pv = std::make_unique<PitchReadoutView>(host_, cn);
            addAndMakeVisible(*pv);
            c.rich = std::move(pv);
            return true;
        }
        case CT::Readout: {
            auto rv = std::make_unique<ReadoutView>(host_, cn, s.decimalPlaces);
            addAndMakeVisible(*rv);
            c.rich = std::move(rv);
            return true;
        }
        case CT::TextReadout: {
            auto tv = std::make_unique<TextReadoutView>(host_, cn);
            addAndMakeVisible(*tv);
            c.rich = std::move(tv);
            return true;
        }
        case CT::Sigil: {
            auto sv = std::make_unique<SigilView>(host_, cn);
            addAndMakeVisible(*sv);
            c.rich = std::move(sv);
            return true;
        }
        case CT::NoteField: {
            auto nf = std::make_unique<NoteFieldBrick>(host_, cn, pn);
            nf->onPopup = [this, cn, pn](juce::Point<int> pos) {
                showAutomateMenu(host_, cn, pn, pos,
                                 [this] { if (onAutomationChanged) onAutomationChanged(); });
            };
            addAndMakeVisible(*nf);
            c.rich = std::move(nf);
            return true;
        }
        case CT::StepStrip: {
            auto ds = std::make_unique<StepStripBrick>(host_, cn, bound);
            addAndMakeVisible(*ds);
            c.rich = std::move(ds);
            return true;
        }
        case CT::IntervalRows: {
            auto db = std::make_unique<IntervalRowsBrick>(host_, cn, className(), bound);
            addAndMakeVisible(*db);
            c.rich = std::move(db);
            return true;
        }
        case CT::NumberField: {
            auto nf = std::make_unique<NumberFieldBrick>(host_, cn, pn);
            nf->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
            addAndMakeVisible(*nf);
            c.rich = std::move(nf);
            return true;
        }
        case CT::NumberBox: {
            auto nb = std::make_unique<NumberBoxBrick>(host_, cn, pn, s.decimalPlaces,
                                                       std::atof(s.extraOr("step", "0").c_str()),
                                                       s.extraOr("shows"));
            nb->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
            addAndMakeVisible(*nb);
            c.rich = std::move(nb);
            return true;
        }
        case CT::Led: {
            auto lv = std::make_unique<LedView>(host_, cn, s.extraOr("shows"));
            addAndMakeVisible(*lv);
            c.rich = std::move(lv);
            return true;
        }
        case CT::GainReduction: {
            auto gr = std::make_unique<GainReductionView>(host_, cn);
            addAndMakeVisible(*gr);
            c.rich = std::move(gr);
            return true;
        }
        case CT::TextField: {
            auto tf = std::make_unique<TextFieldBrick>(host_, cn, pn,
                                                       juce::String::fromUTF8(s.label.c_str()),
                                                       std::atoi(s.extraOr("lines", "1").c_str()));
            addAndMakeVisible(*tf);
            c.rich = std::move(tf);
            return true;
        }
        case CT::WaveDraw: {
            auto wd = std::make_unique<WaveDrawBrick>(host_, cn, pn, bound);
            addAndMakeVisible(*wd);
            c.rich = std::move(wd);
            return true;
        }
        case CT::SoundMap: {
            auto mv = std::make_unique<SoundMapView>(host_, cn, pn, s.param2, bound);
            mv->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
            addAndMakeVisible(*mv);
            c.rich = std::move(mv);
            return true;
        }
        default:
            return false;
    }
}

}
