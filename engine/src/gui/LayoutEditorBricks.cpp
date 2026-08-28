#include "gui/KnobGridBrick.h"
#include "gui/LayoutEditor.h"

#include <cstdlib>

#include "gui/CamPreview.h"
#include "gui/FormulaBrick.h"
#include "gui/GainReductionView.h"
#include "gui/TextFieldBrick.h"
#include "gui/HelixBrick.h"
#include "gui/PictureFieldBrick.h"
#include "gui/SliceMapBrick.h"
#include "gui/VideoPreview.h"
#include "gui/DnaBasesBrick.h"
#include "gui/DnaStrandBrick.h"
#include "gui/NumberFieldBrick.h"
#include "gui/MidiLogView.h"
#include "gui/OscLogView.h"
#include "gui/LevelMeterView.h"
#include "gui/ThresholdMeterBrick.h"
#include "gui/VuMeterBrick.h"
#include "gui/FieldScopeBrick.h"
#include "gui/LfoScopeBrick.h"
#include "gui/SpectrumScopeBrick.h"
#include "gui/NoteFieldBrick.h"
#include "gui/PitchReadoutView.h"
#include "gui/Mappable.h"
#include "gui/SigilView.h"
#include "gui/SoundMapView.h"
#include "gui/WaveDrawBrick.h"

namespace hum {

bool LayoutEditor::buildBrick(const LayoutSpec::Control& s, Control& c) {
    using CT = LayoutSpec::ControlType;
    const std::string pn = s.param;
    const std::string cn = name_;

    switch (s.type) {
        case CT::Deck: {
            auto dv = std::make_unique<DeckView>(host_, cn);
            addAndMakeVisible(*dv);
            c.deck = std::move(dv);
            return true;
        }
        case CT::DeckPitch: {
            auto pf = std::make_unique<PitchFader>(host_, cn);
            addAndMakeVisible(*pf);
            c.deckPitch = std::move(pf);
            return true;
        }
        case CT::DeckControls: {
            auto dc = std::make_unique<DeckControls>(host_, cn);
            addAndMakeVisible(*dc);
            c.deckControls = std::move(dc);
            return true;
        }
        case CT::MidiKeyboard: {
            auto kb = std::make_unique<MidiKeyboardStrip>(host_, cn);
            addAndMakeVisible(*kb);
            c.keyboard = std::move(kb);
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
            c.faderBank = std::move(fb);
            return true;
        }
        case CT::Waveform: {
            std::vector<std::string> params;
            for (const auto& d : schemaFor(className()))
                if (d.name.rfind(pn, 0) == 0) params.push_back(d.name);
            auto wf = std::make_unique<WaveformDisplay>(host_, cn, std::move(params));
            addAndMakeVisible(*wf);
            c.waveform = std::move(wf);
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
            c.momentary = std::move(mb);
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
            c.tap = std::move(tb);
            return true;
        }
        case CT::LooperTracks: {
            int count = 0;
            for (const auto& d : schemaFor(className()))
                if (d.name.rfind(pn, 0) == 0) ++count;
            auto ts = std::make_unique<LooperTrackStrip>(host_, cn, pn, count);
            addAndMakeVisible(*ts);
            c.looperTracks = std::move(ts);
            return true;
        }
        case CT::HandGestures: {
            auto hg = std::make_unique<HandGestureBrick>(host_, cn);
            addAndMakeVisible(*hg);
            c.handGestures = std::move(hg);
            return true;
        }
        case CT::SequenceGrid: {
            auto sg = std::make_unique<SequenceGridBrick>(host_, cn);
            addAndMakeVisible(*sg);
            c.seqGrid = std::move(sg);
            return true;
        }
        case CT::GainShapeCurve: {
            auto gs = std::make_unique<GainShapeBrick>(host_, cn, pn);
            addAndMakeVisible(*gs);
            c.gainShape = std::move(gs);
            return true;
        }
        case CT::StepGrid: {
            const bool arp = s.extraOr("mode", "bassline") == "arp";
            const int steps = std::atoi(s.extraOr("steps", arp ? "32" : "16").c_str());
            host_.patterns().ensureMatrix(cn, arp ? "trigger-tie-matrix" : "bassline-pattern-matrix",
                                          steps, s.extraOr("seed", ""));
            auto g = std::make_unique<PatternStepGrid>(
                host_, cn, arp ? PatternStepGrid::Mode::Arp : PatternStepGrid::Mode::Bassline);
            addAndMakeVisible(*g);
            c.stepGrid = std::move(g);
            return true;
        }
        case CT::PatternGrid: {
            PatternEditorSpec ps;
            ps.valid = true;
            ps.laneCount = std::atoi(s.extraOr("lanes", "8").c_str());
            ps.masterVolumeParam = s.extraOr("master-volume", "Volume");
            ps.muteParam = s.extraOr("mute", "Mute");
            ps.gateParam = s.extraOr("gate");
            ps.volumeParamPrefix = s.extraOr("volume-prefix", "Volume_");
            ps.enableParamPrefix = s.extraOr("enable-prefix", "Enable_");
            ps.fileParamPrefix = s.extraOr("file-prefix", "File_");
            auto pe = std::make_unique<PatternEditor>(host_, cn, std::move(ps));
            pe->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
            addAndMakeVisible(*pe);
            c.rich = std::move(pe);
            return true;
        }
        case CT::PianoRoll: {
            auto pr = std::make_unique<PianoRollEditor>(host_, cn);
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
            auto vp = std::make_unique<hum::VideoPreview>(host_, cn);
            addAndMakeVisible(*vp);
            c.rich = std::move(vp);
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
            auto fb = std::make_unique<FormulaBrick>(host_, cn,
                                                     pn.empty() ? "Expression" : pn);
            addAndMakeVisible(*fb);
            c.rich = std::move(fb);
            return true;
        }
        case CT::PictureField: {
            auto pf = std::make_unique<PictureFieldBrick>(host_, cn,
                                                          pn.empty() ? "File" : pn);
            addAndMakeVisible(*pf);
            c.rich = std::move(pf);
            return true;
        }
        case CT::SliceMap: {
            auto sm = std::make_unique<SliceMapBrick>(host_, cn);
            addAndMakeVisible(*sm);
            c.rich = std::move(sm);
            return true;
        }
        case CT::HelixStrands: {
            auto hv = std::make_unique<HelixStrandView>(host_, cn);
            addAndMakeVisible(*hv);
            c.rich = std::move(hv);
            return true;
        }
        case CT::MidiLog: {
            auto mv = std::make_unique<MidiLogView>(host_, cn);
            addAndMakeVisible(*mv);
            c.rich = std::move(mv);
            return true;
        }
        case CT::OscLog: {
            auto ov = std::make_unique<OscLogView>(host_, cn);
            addAndMakeVisible(*ov);
            c.rich = std::move(ov);
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
            auto ls = std::make_unique<LfoScopeView>(host_, cn);
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
        case CT::DnaStrand: {
            auto ds = std::make_unique<DnaStrandBrick>(host_, cn);
            addAndMakeVisible(*ds);
            c.rich = std::move(ds);
            return true;
        }
        case CT::DnaBases: {
            auto db = std::make_unique<DnaBasesBrick>(host_, cn);
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
        case CT::GainReduction: {
            auto gr = std::make_unique<GainReductionView>(host_, cn);
            addAndMakeVisible(*gr);
            c.rich = std::move(gr);
            return true;
        }
        case CT::TextField: {
            auto tf = std::make_unique<TextFieldBrick>(host_, cn, pn,
                                                       juce::String::fromUTF8(s.label.c_str()));
            addAndMakeVisible(*tf);
            c.rich = std::move(tf);
            return true;
        }
        case CT::WaveDraw: {
            auto wd = std::make_unique<WaveDrawBrick>(host_, cn, pn.empty() ? "Table" : pn);
            addAndMakeVisible(*wd);
            c.rich = std::move(wd);
            return true;
        }
        case CT::SoundMap: {
            auto mv = std::make_unique<SoundMapView>(host_, cn,
                                                     pn.empty() ? "X" : pn,
                                                     s.param2.empty() ? "Y" : s.param2);
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
