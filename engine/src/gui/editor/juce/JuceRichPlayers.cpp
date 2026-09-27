// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/juce/JuceRichView.h"

#include <cstdlib>

#include "hum/caps/Files.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/bricks/KnobGridBrick.h"
#include "gui/bricks/BasslineImportBrick.h"
#include "gui/editor/BrickBindings.h"
#include "gui/editor/BrickModel.h"
#include "gui/bricks/StepNudgeBrick.h"
#include "gui/bricks/TakeLaneBrick.h"
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
#include "gui/bricks/CamPreview.h"
#include "gui/bricks/FormulaBrick.h"
#include "gui/bricks/StrandsBrick.h"
#include "gui/bricks/PictureFieldBrick.h"
#include "gui/bricks/SliceMapBrick.h"
#include "gui/bricks/VideoPreview.h"
#include "gui/bricks/ClipGridBrick.h"
#include "gui/bricks/ScreenButton.h"
#include "gui/bricks/VideoTransportBrick.h"
#include "gui/editor/Mappable.h"

namespace hum {

std::unique_ptr<RichView> makeJucePlayerBrick(juce::Component& parent, EditorHost& host, const LayoutSpec& spec,
                                  const LayoutSpec::Control& s, RichOwner& owner) {
    using CT = LayoutSpec::ControlType;
    const std::string pn = s.param;
    const std::string cn = owner.organism();
    const Bindings bound(spec, s);

    switch (s.type) {
        case CT::Deck: {
            auto dv = std::make_unique<DeckView>(host, cn, bound);
            return brickView(parent, std::move(dv));
        }
        case CT::DeckPitch: {
            auto pf = std::make_unique<PitchFader>(host, cn, bound);
            return brickView(parent, std::move(pf));
        }
        case CT::DeckControls: {
            auto dc = std::make_unique<DeckControls>(host, cn, bound);
            return brickView(parent, std::move(dc));
        }
        case CT::MidiKeyboard: {
            auto kb = std::make_unique<MidiKeyboardStrip>(host, cn);
            return brickView(parent, std::move(kb));
        }
        case CT::FaderBank: {
            std::vector<FaderBank::Spec> specs;
            for (const auto& f : brick::faderFamily(schemaFor(owner.className()), pn))
                specs.push_back({f.param, juce::String::fromUTF8(s.label.c_str())
                                      + juce::String(f.labelSuffix),
                                 f.min, f.max});
            auto fb = std::make_unique<FaderBank>(host, cn, std::move(specs));
            fb->onChange = [&owner] { owner.repaintFace(); };
            fb->onAutomationChanged = changedHook(owner);
            return brickView(parent, std::move(fb));
        }
        case CT::Waveform: {
            auto params = brick::familyOf(schemaFor(owner.className()), pn);
            auto wf = std::make_unique<WaveformDisplay>(host, cn, std::move(params));
            return brickView(parent, std::move(wf));
        }
        case CT::TapTempo: {
            auto tb = std::make_unique<Mappable<TapButton>>(
                host, cn, pn, s.extraOr("off", ""),
                juce::String::fromUTF8(s.label.c_str()));
            tb->onRightClick = [&host, cn, pn, changed = changedHook(owner)](juce::Point<int> pos) {
                showAutomateMenu(host, cn, pn, pos, changed);
            };
            return brickView(parent, std::move(tb));
        }
        case CT::LooperTracks: {
            const int count = brick::familySize(schemaFor(owner.className()), pn);
            auto ts = std::make_unique<LooperTrackStrip>(host, cn, pn, count);
            return reloadingView(parent, std::move(ts), [](LooperTrackStrip& w) { w.reload(); });
        }
        case CT::HandGestures: {
            auto hg = std::make_unique<HandGestureBrick>(host, cn, bound);
            return brickView(parent, std::move(hg));
        }
        case CT::SequenceGrid: {
            auto sg = std::make_unique<SequenceGridBrick>(
                host, cn, bound, std::atoi(s.extraOr("rows", "8").c_str()));
            return brickView(parent, std::move(sg));
        }
        case CT::GainShapeCurve: {
            auto gs = std::make_unique<GainShapeBrick>(host, cn, pn);
            return brickView(parent, std::move(gs));
        }
        case CT::StepGrid: {
            const auto grid = brick::stepGridSetupFor(s);
            host.patterns().ensureMatrix(cn, grid.matrixId, grid.steps, grid.seed);
            auto g = std::make_unique<PatternStepGrid>(
                host, cn, grid.arp ? PatternStepGrid::Mode::Arp : PatternStepGrid::Mode::Bassline,
                bound(bind::kNudge), bound(bind::kTranspose));
            return reloadingView(parent, std::move(g), [](PatternStepGrid& w) { w.reload(); });
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
            auto pe = std::make_unique<PatternEditor>(host, cn, std::move(ps));
            pe->onAutomationChanged = changedHook(owner);
            return editorView(parent, std::move(pe));
        }
        case CT::PianoRoll: {
            auto pr = std::make_unique<PianoRollEditor>(
                host, cn, PianoRollEditor::Params{bound(bind::kBars), bound(bind::kSwing),
                                                   bound(bind::kSwingFollow), bound(bind::kSwingUnit),
                                                   bound(bind::kRecord), bound(bind::kLoop),
                                                   bound(bind::kQuantize)});
            pr->onAutomationChanged = changedHook(owner);
            return editorView(parent, std::move(pr));
        }
        case CT::Camera: {
            auto cv = std::make_unique<CamPreview>(host, cn);
            return editorView(parent, std::move(cv));
        }
        case CT::VideoPreview: {
            auto vp = std::make_unique<hum::VideoPreview>(host, cn, bound);
            return editorView(parent, std::move(vp));
        }
        case CT::VideoTransport: {
            auto vt = std::make_unique<VideoTransportBrick>(host, cn, bound);
            return editorView(parent, std::move(vt));
        }
        case CT::ClipGrid: {
            auto cr = std::make_unique<ClipGridBrick>(host, cn, bound);
            return editorView(parent, std::move(cr));
        }
        case CT::ScreenButton: {
            auto sb = std::make_unique<ScreenButton>(host, cn);
            return editorView(parent, std::move(sb));
        }
        case CT::KnobGrid: {
            auto fields = brick::csvFields(s.extraOr("fields"));
            const int rows = brick::atLeastOne(s.extraOr("rows"));
            auto kg = std::make_unique<KnobGridBrick>(host, cn, s.extraOr("prefix"),
                                                      rows, std::move(fields));
            return editorView(parent, std::move(kg));
        }
        case CT::Formula: {
            auto fb = std::make_unique<FormulaBrick>(host, cn, pn, bound);
            return editorView(parent, std::move(fb));
        }
        case CT::PictureField: {
            auto pf = std::make_unique<PictureFieldBrick>(host, cn, pn, bound);
            return editorView(parent, std::move(pf));
        }
        case CT::SliceMap: {
            auto sm = std::make_unique<SliceMapBrick>(host, cn, bound);
            return editorView(parent, std::move(sm));
        }
        case CT::Strands: {
            auto hv = std::make_unique<StrandsView>(host, cn, bound);
            return editorView(parent, std::move(hv));
        }
        case CT::StepNudge: {
            auto nudge = std::make_unique<StepNudgeBrick>(host, cn, pn);
            nudge->onNudged = [&owner] { owner.reloadFace(); };
            return editorView(parent, std::move(nudge));
        }
        case CT::BasslineImport: {
            auto load = std::make_unique<BasslineImportBrick>(host, cn, juce::String::fromUTF8(s.label.c_str()));
            load->onLoaded = [&owner] { owner.reloadFace(); };
            return editorView(parent, std::move(load));
        }
        case CT::TakeLane: {
            auto lane = std::make_unique<TakeLaneBrick>(
                host, cn, brick::atLeastOne(s.extraOr("track", "1")), bound);
            return editorView(parent, std::move(lane));
        }
        default:
            return nullptr;
    }
}

}
