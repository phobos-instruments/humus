// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/dsp/FadeLaw.h"
#include "gui/tracks/MediaInfo.h"
#include "gui/tracks/SongView.h"
#include "gui/tracks/QuantiseMenu.h"

#include "core/packs/Roles.h"

#include <cmath>
#include <juce_audio_formats/juce_audio_formats.h>

#include "gui/tracks/ClipColors.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"


namespace hum {

namespace {
enum { kMenuOpen = 1, kMenuSplit, kMenuCutAll, kMenuLoop, kMenuRename, kMenuDuplicate, kMenuDelete,
       kMenuSetBpm, kMenuQuantise, kMenuUp, kMenuDown, kMenuOctUp, kMenuOctDown,
       kMenuLouder, kMenuSofter, kMenuMerge, kMenuToMidi, kMenuInfo,
       kMenuColor0 = 100, kMenuWarp0 = 200, kMenuQuant0 = quantise::kMenuBase, kMenuStretch0 = 400,
       kMenuFadeIn0 = 400, kMenuFadeOut0 = 410 };

struct FadeShape { const char* key; const char* name; double curve; };
constexpr FadeShape kFadeShapes[] = {
    {"tracks-pane-menu.fade-linear", "Linear", hum::kFadeLinear},
    {"tracks-pane-menu.fade-equal-power", "Equal Power", hum::kFadeEqualPower},
    {"tracks-pane-menu.fade-logarithmic", "Logarithmic", hum::kFadeLog},
    {"tracks-pane-menu.fade-exponential", "Exponential", hum::kFadeExp}};
constexpr int kFadeShapeCount = (int) (sizeof(kFadeShapes) / sizeof(kFadeShapes[0]));

inline int fadeShapeIndex(double curve) {
    int best = 0;
    for (int i = 1; i < kFadeShapeCount; ++i)
        if (std::abs(curve - kFadeShapes[i].curve)
            < std::abs(curve - kFadeShapes[best].curve)) best = i;
    return best;
}


}

void SongView::showClipMenu(int row, int clip, juce::Point<int> screenPos, int atTick) {
    const auto node = rows_[(size_t) row];
    const auto clips = host().clips().list(node);
    if (clip >= (int) clips.size()) return;
    const auto ci = clips[(size_t) clip];

    juce::PopupMenu menu;
    menu.addItem(kMenuOpen,
                 ci.isCompound ? tr("tracks-pane-menu.open-reel", "Open Reel")
                               : tr("tracks-pane-menu.edit-in-organism", "Edit in Organism"),
                 ci.isCompound || !ci.hasMedia());
    menu.addSeparator();
    menu.addItem(kMenuSplit, tr("tracks-pane-menu.split-here", "Split Here"),
                 atTick > ci.startTick && atTick < ci.startTick + ci.lengthTicks);
    menu.addItem(kMenuCutAll, tr("tracks-pane-menu.cut-at-playhead", "Cut at Playhead"), clipsUnderPlayhead() > 0);
    menu.addItem(kMenuLoop, tr("tracks-pane-menu.loop", "Loop"), true, ci.looped);
    menu.addItem(kMenuDuplicate, tr("tracks-pane-menu.duplicate-after", "Duplicate After"));
    menu.addItem(kMenuMerge, tr("tracks-pane-menu.merge", "Merge"), sel_.size() > 1);
    menu.addItem(kMenuRename, tr("tracks-pane-menu.rename", "Rename..."));
    menu.addItem(kMenuInfo, tr("tracks-pane-menu.media-info", "Media Info..."),
                 ci.hasMedia() && !ci.isCompound);
    juce::PopupMenu colors;
    for (int i = 0; i <= kNumClipColors; ++i) {
        juce::PopupMenu::Item it(clipColourName(i));
        it.itemID = kMenuColor0 + i;
        it.colour = i == 0 ? Palette::text : clipColour(i);
        it.isTicked = ci.color == i;
        colors.addItem(std::move(it));
    }
    menu.addSubMenu(tr("tracks-pane-menu.color", "Color"), colors);
    if (!ci.hasMedia()) {
        const int oct = noteedit::octaveSteps(host().model());
        juce::PopupMenu notes;
        notes.addItem(kMenuUp, tr("tracks-pane-menu.up", "Up"));
        notes.addItem(kMenuDown, tr("tracks-pane-menu.down", "Down"));
        if (oct > 0) {
            notes.addItem(kMenuOctUp, tr("tracks-pane-menu.up-an-octave", "Up an Octave"));
            notes.addItem(kMenuOctDown, tr("tracks-pane-menu.down-an-octave", "Down an Octave"));
        }
        notes.addSeparator();
        notes.addItem(kMenuLouder, tr("tracks-pane-menu.louder", "Louder"));
        notes.addItem(kMenuSofter, tr("tracks-pane-menu.softer", "Softer"));
        menu.addSubMenu(tr("tracks-pane-menu.notes", "Notes"), notes);
        menu.addSubMenu(tr("tracks-pane-menu.quantise-to", "Quantise to"), quantise::menu(gridBeats()));
    }
    if (ci.hasMedia()) {
        juce::PopupMenu warp;
        const char* names[] = {"Off", "Beats", "Tone"};
        for (int i = 0; i < 3; ++i)
            warp.addItem(kMenuWarp0 + i, names[i], true, ci.warpMode == i);
        warp.addSeparator();
        warp.addItem(kMenuSetBpm, ci.sourceBpm > 0.0
                                      ? "Source Tempo: " + juce::String(ci.sourceBpm, 1) + "..."
                                      : juce::String("Source Tempo..."));
        menu.addSubMenu(tr("tracks-pane-menu.warp", "Warp"), warp);

        juce::PopupMenu fades;
        const int inAt = fadeShapeIndex(ci.fadeInCurve);
        const int outAt = fadeShapeIndex(ci.fadeOutCurve);
        juce::PopupMenu fadeIn, fadeOut;
        for (int i = 0; i < kFadeShapeCount; ++i) {
            fadeIn.addItem(kMenuFadeIn0 + i, tr(kFadeShapes[i].key, kFadeShapes[i].name),
                           ci.fadeInTicks > 0, i == inAt);
            fadeOut.addItem(kMenuFadeOut0 + i, tr(kFadeShapes[i].key, kFadeShapes[i].name), ci.fadeOutTicks > 0, i == outAt);
        }
        fades.addSubMenu(tr("tracks-pane-menu.in", "In"), fadeIn, ci.fadeInTicks > 0);
        fades.addSubMenu(tr("tracks-pane-menu.out", "Out"), fadeOut, ci.fadeOutTicks > 0);
        menu.addSubMenu(tr("tracks-pane-menu.fade-shape", "Fade Shape"), fades, ci.fadeInTicks > 0 || ci.fadeOutTicks > 0);
    }
    if (ci.isAudio) {
        menu.addItem(kMenuToMidi, tr("tracks-pane-menu.convert-to-midi-track", "Convert to MIDI Track"));
        juce::PopupMenu stretch;
        static const int kStretchFactors[] = {4, 8, 16, 50};
        for (int i = 0; i < 4; ++i)
            stretch.addItem(kMenuStretch0 + i,
                            juce::String(kStretchFactors[i]) + "x to New Track");
        menu.addSubMenu(tr("tracks-pane-menu.paulstretch", "Paulstretch"), stretch);
    }
    menu.addSeparator();
    menu.addItem(kMenuDelete, tr("tracks-pane-menu.delete", "Delete"));

    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                       [this, node, clip, ci, atTick](int res) {
        if (res == 0) return;
        if (res >= kMenuFadeIn0 && res < kMenuFadeIn0 + kFadeShapeCount) {
            host().pushUndo();
            host().clips().setFadeCurves(node, clip, kFadeShapes[res - kMenuFadeIn0].curve,
                                        ci.fadeOutCurve);
            repaintAll();
            return;
        }
        if (res >= kMenuFadeOut0 && res < kMenuFadeOut0 + kFadeShapeCount) {
            host().pushUndo();
            host().clips().setFadeCurves(node, clip, ci.fadeInCurve,
                                        kFadeShapes[res - kMenuFadeOut0].curve);
            repaintAll();
            return;
        }
        if (res >= kMenuWarp0 && res <= kMenuWarp0 + 2) {
            host().pushParamStep();
            host().clips().setWarp(node, clip, res - kMenuWarp0);
            repaintAll();
            return;
        }
        if (res == kMenuToMidi) {
            convertClipToMidi(node, ci);
            return;
        }
        if (res == kMenuInfo) {
            mediainfo::show(host(), node, ci);
            return;
        }
        if (res >= kMenuStretch0 && res < kMenuStretch0 + 4) {
            static const double kFactors[] = {4.0, 8.0, 16.0, 50.0};
            stretchClip(node, clip, ci, kFactors[res - kMenuStretch0]);
            return;
        }
        if (res == kMenuSetBpm) {
            auto* aw = new juce::AlertWindow("Source Tempo",
                                             "What does this material run at? (0 = unknown)",
                                             juce::MessageBoxIconType::NoIcon);
            aw->addTextEditor("bpm", juce::String(ci.sourceBpm > 0.0 ? ci.sourceBpm : 0.0, 2));
            aw->addButton(tr("tracks-pane-menu.set", "Set"), 1, juce::KeyPress(juce::KeyPress::returnKey));
            aw->addButton(tr("tracks-pane-menu.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
            aw->enterModalState(true, juce::ModalCallbackFunction::create(
                [this, aw, node, clip](int r) {
                    if (r != 1) return;
                    host().pushParamStep();
                    host().clips().setSourceBpm(node, clip,
                                               aw->getTextEditorContents("bpm").getDoubleValue());
                    repaintAll();
                }), true);
            return;
        }
        if (res >= kMenuColor0 && res <= kMenuColor0 + kNumClipColors) {
            host().pushUndo();
            host().clips().setColor(node, clip, res - kMenuColor0);
            repaintAll();
            return;
        }
        if (const int q = quantise::ticksFor(res, gridBeats()); q > 0) {
            host().pushUndo();
            host().clips().quantise(node, clip, q);
            rebuild();
            return;
        }
        if (res == kMenuQuantise || (res >= kMenuUp && res <= kMenuSofter)) {
            host().pushUndo();
            const int oct = noteedit::octaveSteps(host().model());
            if (res == kMenuQuantise)
                host().clips().quantise(node, clip,
                                       std::max(1, (int) std::llround(gridBeats()
                                                                      * Pattern::kTicksPerBeat)));
            else if (res == kMenuLouder)      host().clips().nudgeVelocity(node, clip, 10);
            else if (res == kMenuSofter)      host().clips().nudgeVelocity(node, clip, -10);
            else host().clips().transpose(node, clip,
                                         res == kMenuUp ? 1 : res == kMenuDown ? -1
                                         : res == kMenuOctUp ? oct : -oct);
            repaintAll();
            return;
        }
        switch (res) {
            case kMenuOpen:
                if (const auto reel = host().clips().compoundReel(node, clip); !reel.empty()) {
                    ctx_.enterTrack(reel);
                    rebuild();
                } else {
                    ctx_.openClip(node, clip);
                }
                break;
            case kMenuSplit:
                host().pushUndo();
                host().clips().split(node, clip, atTick);
                break;
            case kMenuCutAll:
                cutAtPlayhead();
                break;
            case kMenuMerge: {
                const auto why = mergeRefusal();
                if (mergeSelection() == 0)
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::InfoIcon,
                        tr("tracks-pane-menu.merge-title", "Merge"), why);
                break;
            }
            case kMenuLoop:
                host().pushUndo();
                host().clips().setLooped(node, clip, !ci.looped);
                break;
            case kMenuDuplicate:
                host().pushUndo();
                host().clips().duplicate(node, clip, ci.startTick + ci.lengthTicks);
                break;
            case kMenuDelete:
                host().pushUndo();
                host().clips().remove(node, clip);
                break;
            case kMenuRename: {
                auto* aw = new juce::AlertWindow(tr("tracks-pane-menu.rename-clip", "Rename Clip"), tr("tracks-pane-menu.new-clip-name", "New clip name:"),
                                                 juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor("name", juce::String(ci.name));
                aw->addButton(tr("tracks-pane-menu.ok", "OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
                aw->addButton(tr("tracks-pane-menu.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
                aw->enterModalState(true, juce::ModalCallbackFunction::create(
                    [this, aw, node, clip](int r2) {
                        if (r2 == 1) {
                            host().pushUndo();
                            host().clips().rename(node, clip,
                                             aw->getTextEditorContents("name").toStdString());
                            repaintAll();
                        }
                        delete aw;
                    }), false);
                return;
            }
            default: break;
        }
        repaintAll();
    });
}

namespace {

}

std::string SongView::addMidiTrack() {
    const auto node = host().addOrganism(classWithRole(role::kMidiTrack), host().spotBelowPatch());
    if (!node.empty()) host().patterns().ensureNote(node);
    return node;
}

std::string SongView::addTrack(bool audio, const std::string& target) {
    view_.sel.active = false;
    repaintAll();
    if (audio) return host().addOrganism(classWithRole(role::kAudioTrack), host().spotBelowPatch());
    if (!target.empty()) {
        host().patterns().ensureNote(target);
        return target;
    }
    const auto node = host().addOrganism(classWithRole(role::kDefaultInstrument), host().spotBelowPatch());
    if (node.empty()) return node;
    host().connectToMaster(node);
    host().patterns().ensureNote(node);
    return node;
}

void SongView::showAddTrackMenu(juce::Point<int> screenPos) {
    juce::PopupMenu m;
    m.addItem(1, tr("tracks-pane-menu.audio-track", "Audio Track"));
    m.addItem(3, tr("tracks-pane-menu.midi-track", "MIDI Track"));
    m.addItem(4, tr("tracks-pane-menu.video-track", "Video Track"));

    m.showMenuAsync(juce::PopupMenu::Options()
                        .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                    [this](int res) {
        if (res == 0) return;
        host().pushUndo();
        if (res == 1) addTrack(true, {});
        else if (res == 4) host().addOrganism(classWithRole(role::kVideoTrack), host().spotBelowPatch());
        else if (res == 3) addMidiTrack();
        rebuild();
        repaintAll();
        ctx_.patchChanged();
    });
}

}
