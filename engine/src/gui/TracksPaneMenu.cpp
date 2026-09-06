#include "hum/dsp/FadeLaw.h"
#include "hum/dsp/MelodyTrace.h"
#include "gui/TracksPane.h"

#include "core/BeatDetector.h"

#include <cmath>
#include <juce_audio_formats/juce_audio_formats.h>

#include "gui/ClipColors.h"
#include "gui/LookAndFeel.h"

namespace hum {

namespace {
constexpr const char* kDefaultInstrument = "Rhizome";
enum { kMenuOpen = 1, kMenuSplit, kMenuLoop, kMenuRename, kMenuDuplicate, kMenuDelete,
       kMenuSetBpm, kMenuQuantise, kMenuUp, kMenuDown, kMenuOctUp, kMenuOctDown,
       kMenuLouder, kMenuSofter, kMenuMerge, kMenuToMidi,
       kMenuColor0 = 100, kMenuWarp0 = 200, kMenuQuant0 = 300, kMenuStretch0 = 400,
       kMenuFadeIn0 = 400, kMenuFadeOut0 = 410 };

struct FadeShape { const char* name; double curve; };
constexpr FadeShape kFadeShapes[] = {{"Linear", hum::kFadeLinear},
                                     {"Equal Power", hum::kFadeEqualPower},
                                     {"Logarithmic", hum::kFadeLog},
                                     {"Exponential", hum::kFadeExp}};
constexpr int kFadeShapeCount = (int) (sizeof(kFadeShapes) / sizeof(kFadeShapes[0]));

inline int fadeShapeIndex(double curve) {
    int best = 0;
    for (int i = 1; i < kFadeShapeCount; ++i)
        if (std::abs(curve - kFadeShapes[i].curve)
            < std::abs(curve - kFadeShapes[best].curve)) best = i;
    return best;
}

struct QuantChoice { const char* name; int ticks; };
constexpr QuantChoice kQuantChoices[] = {
    {"1/4",   Pattern::kTicksPerBeat},
    {"1/8",   Pattern::kTicksPerBeat / 2},
    {"1/8T",  Pattern::kTicksPerBeat / 3},
    {"1/16",  Pattern::kTicksPerBeat / 4},
    {"1/16T", Pattern::kTicksPerBeat / 6},
    {"1/32",  Pattern::kTicksPerBeat / 8},
};
constexpr int kQuantCount = (int) (sizeof(kQuantChoices) / sizeof(kQuantChoices[0]));

juce::PopupMenu quantiseMenu(double gridBeats) {
    juce::PopupMenu m;
    const int snap = std::max(1, (int) std::llround(gridBeats * Pattern::kTicksPerBeat));
    m.addItem(kMenuQuant0 + kQuantCount,
              "Snap grid (" + juce::String(trackslayout::gridLabel(gridBeats)) + ")");
    m.addSeparator();
    for (int i = 0; i < kQuantCount; ++i)
        m.addItem(kMenuQuant0 + i, kQuantChoices[i].name, true,
                  kQuantChoices[i].ticks == snap);
    return m;
}

int quantTicksFor(int id, double gridBeats) {
    if (id < kMenuQuant0 || id > kMenuQuant0 + kQuantCount) return -1;
    if (id == kMenuQuant0 + kQuantCount)
        return std::max(1, (int) std::llround(gridBeats * Pattern::kTicksPerBeat));
    return kQuantChoices[id - kMenuQuant0].ticks;
}

}

void TracksPane::showNoteMenu(juce::Point<int> screenPos) {
    enum { kNoteDelete = 1, kNoteLouder, kNoteSofter, kNoteSelectAll };
    juce::PopupMenu m;
    const int n = (int) selNotes_.size();
    const auto count = n == 1 ? juce::String("note") : juce::String(n) + " notes";
    m.addSectionHeader(n > 0 ? count : juce::String("No notes selected"));
    m.addItem(kNoteSelectAll, "Select All", true);
    if (n > 0) {
        m.addSeparator();
        m.addSubMenu("Quantise to", quantiseMenu(gridBeats()));
        m.addSeparator();
        m.addItem(kNoteLouder, "Louder");
        m.addItem(kNoteSofter, "Softer");
        m.addSeparator();
        m.addItem(kNoteDelete, "Delete");
    }
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos, screenPos}),
                    [this](int res) {
        if (res == 0) return;
        if (res == kNoteSelectAll) { selectAllNotes(); return; }
        if (const int q = quantTicksFor(res, gridBeats()); q > 0) {
            quantiseSelectedNotes(q);
            return;
        }
        if (res == kNoteDelete) { deleteSelectedNotes(); rebuild(); return; }
        if (res == kNoteLouder) nudgeNotes(0, 0, 10);
        if (res == kNoteSofter) nudgeNotes(0, 0, -10);
    });
}

void TracksPane::showClipMenu(int row, int clip, juce::Point<int> screenPos, int atTick) {
    const auto node = rows_[(size_t) row];
    const auto clips = host_.clips().list(node);
    if (clip >= (int) clips.size()) return;
    const auto ci = clips[(size_t) clip];

    juce::PopupMenu menu;
    menu.addItem(kMenuOpen, "Edit in Organism", !ci.isAudio);
    menu.addSeparator();
    menu.addItem(kMenuSplit, "Split Here",
                 atTick > ci.startTick && atTick < ci.startTick + ci.lengthTicks);
    menu.addItem(kMenuLoop, "Loop", true, ci.looped);
    menu.addItem(kMenuDuplicate, "Duplicate After");
    menu.addItem(kMenuMerge, "Merge", sel_.size() > 1);
    menu.addItem(kMenuRename, "Rename...");
    juce::PopupMenu colors;
    for (int i = 0; i <= kNumClipColors; ++i) {
        juce::PopupMenu::Item it(clipColourName(i));
        it.itemID = kMenuColor0 + i;
        it.colour = i == 0 ? Palette::text : clipColour(i);
        it.isTicked = ci.color == i;
        colors.addItem(std::move(it));
    }
    menu.addSubMenu("Color", colors);
    if (!ci.isAudio) {
        const int oct = noteedit::octaveSteps(host_.model());
        juce::PopupMenu notes;
        notes.addItem(kMenuUp, "Up");
        notes.addItem(kMenuDown, "Down");
        if (oct > 0) {
            notes.addItem(kMenuOctUp, "Up an Octave");
            notes.addItem(kMenuOctDown, "Down an Octave");
        }
        notes.addSeparator();
        notes.addSubMenu("Quantise to", quantiseMenu(gridBeats()));
        notes.addSeparator();
        notes.addItem(kMenuLouder, "Louder");
        notes.addItem(kMenuSofter, "Softer");
        menu.addSubMenu("Notes", notes);
    }
    if (ci.isAudio) {
        juce::PopupMenu warp;
        const char* names[] = {"Off", "Beats", "Tone"};
        for (int i = 0; i < 3; ++i)
            warp.addItem(kMenuWarp0 + i, names[i], true, ci.warpMode == i);
        warp.addSeparator();
        warp.addItem(kMenuSetBpm, ci.sourceBpm > 0.0
                                      ? "Source Tempo: " + juce::String(ci.sourceBpm, 1) + "..."
                                      : juce::String("Source Tempo..."));
        menu.addSubMenu("Warp", warp);

        juce::PopupMenu fades;
        const int inAt = fadeShapeIndex(ci.fadeInCurve);
        const int outAt = fadeShapeIndex(ci.fadeOutCurve);
        juce::PopupMenu fadeIn, fadeOut;
        for (int i = 0; i < kFadeShapeCount; ++i) {
            fadeIn.addItem(kMenuFadeIn0 + i, kFadeShapes[i].name, ci.fadeInTicks > 0, i == inAt);
            fadeOut.addItem(kMenuFadeOut0 + i, kFadeShapes[i].name, ci.fadeOutTicks > 0, i == outAt);
        }
        fades.addSubMenu("In", fadeIn, ci.fadeInTicks > 0);
        fades.addSubMenu("Out", fadeOut, ci.fadeOutTicks > 0);
        menu.addSubMenu("Fade Shape", fades, ci.fadeInTicks > 0 || ci.fadeOutTicks > 0);
        menu.addItem(kMenuToMidi, "Convert to MIDI Track");
        juce::PopupMenu stretch;
        static const int kStretchFactors[] = {4, 8, 16, 50};
        for (int i = 0; i < 4; ++i)
            stretch.addItem(kMenuStretch0 + i,
                            juce::String(kStretchFactors[i]) + "x to New Track");
        menu.addSubMenu("Paulstretch", stretch);
    }
    menu.addSeparator();
    menu.addItem(kMenuDelete, "Delete");

    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                       [this, node, clip, ci, atTick](int res) {
        if (res == 0) return;
        if (res >= kMenuFadeIn0 && res < kMenuFadeIn0 + kFadeShapeCount) {
            host_.pushUndo();
            host_.clips().setFadeCurves(node, clip, kFadeShapes[res - kMenuFadeIn0].curve,
                                        ci.fadeOutCurve);
            repaint();
            return;
        }
        if (res >= kMenuFadeOut0 && res < kMenuFadeOut0 + kFadeShapeCount) {
            host_.pushUndo();
            host_.clips().setFadeCurves(node, clip, ci.fadeInCurve,
                                        kFadeShapes[res - kMenuFadeOut0].curve);
            repaint();
            return;
        }
        if (res >= kMenuWarp0 && res <= kMenuWarp0 + 2) {
            host_.pushParamStep();
            host_.clips().setWarp(node, clip, res - kMenuWarp0);
            repaint();
            return;
        }
        if (res == kMenuToMidi) {
            convertClipToMidi(node, ci);
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
            aw->addButton("Set", 1, juce::KeyPress(juce::KeyPress::returnKey));
            aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            aw->enterModalState(true, juce::ModalCallbackFunction::create(
                [this, aw, node, clip](int r) {
                    if (r != 1) return;
                    host_.pushParamStep();
                    host_.clips().setSourceBpm(node, clip,
                                               aw->getTextEditorContents("bpm").getDoubleValue());
                    repaint();
                }), true);
            return;
        }
        if (res >= kMenuColor0 && res <= kMenuColor0 + kNumClipColors) {
            host_.pushUndo();
            host_.clips().setColor(node, clip, res - kMenuColor0);
            repaint();
            return;
        }
        if (const int q = quantTicksFor(res, gridBeats()); q > 0) {
            host_.pushUndo();
            host_.clips().quantise(node, clip, q);
            rebuild();
            return;
        }
        if (res == kMenuQuantise || (res >= kMenuUp && res <= kMenuSofter)) {
            host_.pushUndo();
            const int oct = noteedit::octaveSteps(host_.model());
            if (res == kMenuQuantise)
                host_.clips().quantise(node, clip,
                                       std::max(1, (int) std::llround(gridBeats()
                                                                      * Pattern::kTicksPerBeat)));
            else if (res == kMenuLouder)      host_.clips().nudgeVelocity(node, clip, 10);
            else if (res == kMenuSofter)      host_.clips().nudgeVelocity(node, clip, -10);
            else host_.clips().transpose(node, clip,
                                         res == kMenuUp ? 1 : res == kMenuDown ? -1
                                         : res == kMenuOctUp ? oct : -oct);
            repaint();
            return;
        }
        switch (res) {
            case kMenuOpen:
                if (onOpenClip) onOpenClip(node, clip);
                break;
            case kMenuSplit:
                host_.pushUndo();
                host_.clips().split(node, clip, atTick);
                break;
            case kMenuMerge:
                if (mergeSelection() == 0)
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::InfoIcon, "Merge",
                        "Nothing here could be merged: select two or more clips on one "
                        "row, or two or more automation boxes.");
                break;
            case kMenuLoop:
                host_.pushUndo();
                host_.clips().setLooped(node, clip, !ci.looped);
                break;
            case kMenuDuplicate:
                host_.pushUndo();
                host_.clips().duplicate(node, clip, ci.startTick + ci.lengthTicks);
                break;
            case kMenuDelete:
                host_.pushUndo();
                host_.clips().remove(node, clip);
                break;
            case kMenuRename: {
                auto* aw = new juce::AlertWindow("Rename Clip", "New clip name:",
                                                 juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor("name", juce::String(ci.name));
                aw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                aw->enterModalState(true, juce::ModalCallbackFunction::create(
                    [this, aw, node, clip](int r2) {
                        if (r2 == 1) {
                            host_.pushUndo();
                            host_.clips().rename(node, clip,
                                             aw->getTextEditorContents("name").toStdString());
                            repaint();
                        }
                        delete aw;
                    }), false);
                return;
            }
            default: break;
        }
        repaint();
    });
}

bool TracksPane::consolidateRange(int row, double& from, double& to) const {
    if (timeSelection(from, to) && to > from) return true;
    from = to = 0.0;
    bool any = false;
    for (const auto& ci : host_.clips().list(rows_[(size_t) row])) {
        const double s = ci.startTick / (double) Pattern::kTicksPerBeat;
        const double e = (ci.startTick + std::max(1, ci.lengthTicks))
                             / (double) Pattern::kTicksPerBeat;
        from = any ? std::min(from, s) : s;
        to = any ? std::max(to, e) : e;
        any = true;
    }
    if (!any) { from = 0.0; to = host_.songEndBeat(); }
    return to > from;
}

void TracksPane::consolidateRow(int row) {
    if (row < 0 || row >= (int) rows_.size()) return;
    const auto node = rows_[(size_t) row];
    double from = 0.0, to = 0.0;
    if (!consolidateRange(row, from, to)) return;

    juce::MouseCursor::showWaitCursor();
    std::string error;
    const auto track = host_.consolidate(node, from, to, error);
    juce::MouseCursor::hideWaitCursor();

    if (track.empty()) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Consolidate", juce::String(error));
        return;
    }
    rebuild();
    repaint();
    if (onPatchChanged) onPatchChanged();
}

int TracksPane::placeAudioFile(const std::string& node, int atTick, const juce::File& f) {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
    if (!reader || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0) return -1;
    const double seconds = (double) reader->lengthInSamples / reader->sampleRate;
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    const int ticks = std::max(1, (int) std::llround(
        seconds * (bpm / 60.0) * Pattern::kTicksPerBeat));
    const int clip = host_.clips().addAudio(node, atTick, ticks, f.getFullPathName().toStdString());

    if (clip >= 0 && seconds >= 2.0) {
        juce::AudioBuffer<float> buf((int) reader->numChannels,
                                     (int) juce::jmin(reader->lengthInSamples,
                                                      (juce::int64) (reader->sampleRate * 30.0)));
        reader->read(&buf, 0, buf.getNumSamples(), 0, true, true);
        const auto est = detectBeat(buf, reader->sampleRate);
        if (est.confidence > 0.0) {
            host_.clips().setSourceBpm(node, clip, est.bpm);
            host_.clips().setWarp(node, clip, (int) PatternChannel::Warp::Tone);
        }
    }
    return clip;
}

void TracksPane::convertClipToMidi(const std::string& node,
                                   const ClipEditor::ClipInfo& ci) {
    std::string uri = ci.audioFile;
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(f));
    if (!rd || rd->lengthInSamples <= 0) return;

    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    const double ticksPerSecond = bpm / 60.0 * Pattern::kTicksPerBeat;
    const double hostSr = host_.sampleRate() > 0.0 ? host_.sampleRate() : 44100.0;
    const double seconds = std::min(600.0, (double) ci.lengthTicks / ticksPerSecond);
    const auto from = (juce::int64) ((double) ci.audioOffset * rd->sampleRate / hostSr);
    const int want = (int) std::min<juce::int64>(
        (juce::int64) std::llround(seconds * rd->sampleRate),
        rd->lengthInSamples - from);
    if (want <= 0) return;

    juce::AudioBuffer<float> buf((int) rd->numChannels, want);
    if (!rd->read(&buf, 0, want, from, true, true)) return;
    std::vector<float> mono((size_t) want, 0.0f);
    for (int c = 0; c < buf.getNumChannels(); ++c) {
        const float* src = buf.getReadPointer(c);
        for (int i = 0; i < want; ++i) mono[(size_t) i] += src[i];
    }
    const float norm = 1.0f / (float) std::max(1, buf.getNumChannels());
    for (auto& v : mono) v *= norm;

    const auto notes = melodytrace::trace(mono.data(), want, rd->sampleRate,
                                          ticksPerSecond);
    if (notes.empty()) return;

    host_.pushUndo();
    const auto midiNode = addTrack(false, {});
    if (midiNode.empty()) return;
    const int clip = host_.clips().add(midiNode, ci.startTick, ci.lengthTicks);
    if (clip < 0) return;
    host_.clips().setNotes(midiNode, clip, notes, 0);
    if (!ci.name.empty()) host_.clips().rename(midiNode, clip, ci.name + " melody");
    rebuild();
    repaint();
}

void TracksPane::stretchClip(const std::string& node, int clip,
                             const ClipEditor::ClipInfo& ci, double factor) {
    const auto path = host_.clips().stretchAudioFile(node, clip, factor);
    if (path.empty()) return;
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(
        juce::File(juce::String(juce::CharPointer_UTF8(path.c_str())))));
    if (!rd || rd->sampleRate <= 0.0) return;
    const double bpm = host_.tempo() > 0.0 ? host_.tempo() : 120.0;
    const int ticks = std::max(1, (int) std::llround((double) rd->lengthInSamples
                                                     / rd->sampleRate * bpm / 60.0
                                                     * Pattern::kTicksPerBeat));
    host_.pushUndo();
    const auto dest = addTrack(true, {});
    if (dest.empty()) return;
    const int made = host_.clips().addAudio(dest, ci.startTick, ticks, path);
    if (made >= 0)
        host_.clips().rename(dest, made,
                             (ci.name.empty() ? std::string("stretched") : ci.name)
                                 + " x" + std::to_string((int) factor));
    rebuild();
    repaint();
}

std::string TracksPane::addTrack(bool audio, const std::string& target) {
    clearTimeSelection();
    if (audio) {
        const auto node = host_.addOrganism("AudioTrack", host_.spotBelowPatch());
        if (!node.empty()) host_.connectToMaster(node);
        return node;
    }
    if (!target.empty()) {
        host_.patterns().ensureNote(target);
        return target;
    }
    const auto node = host_.addOrganism(kDefaultInstrument, host_.spotBelowPatch());
    if (node.empty()) return node;
    host_.connectToMaster(node);
    host_.patterns().ensureNote(node);
    return node;
}

void TracksPane::showAddTrackMenu(juce::Point<int> screenPos) {
    juce::PopupMenu m;
    m.addItem(1, "Audio Track");
    m.addItem(3, "MIDI Track");

    m.showMenuAsync(juce::PopupMenu::Options()
                        .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                    [this](int res) {
        if (res == 0) return;
        host_.pushUndo();
        if (res == 1) addTrack(true, {});
        else if (res == 3) {
            const auto node = host_.addOrganism("MidiTrack", host_.spotBelowPatch());
            if (!node.empty()) host_.patterns().ensureNote(node);
        }
        rebuild();
        repaint();
        if (onPatchChanged) onPatchChanged();
    });
}

static const juce::String& audioWildcard() {
    static const juce::String w = [] {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        return fm.getWildcardForAllFormats();
    }();
    return w;
}

bool TracksPane::isInterestedInFileDrag(const juce::StringArray& files) {
    static const juce::String exts = audioWildcard().replace("*.", "").replace(";*", ";");
    for (const auto& f : files)
        if (juce::File(f).hasFileExtension(exts)) return true;
    return false;
}

std::string TracksPane::dropTargetNode(int y) {
    if (const int row = rowAt(y); row >= 0) {
        const auto& node = rows_[(size_t) row];
        if (host_.nodeRecordsAudio(node)) return node;
    }
    const int n = (int) host_.arrangeableNodes().size();
    const auto node = host_.addOrganism("AudioTrack", host_.spotBelowPatch());
    if (node.empty()) return node;
    host_.connectToMaster(node);
    return node;
}

void TracksPane::filesDropped(const juce::StringArray& files, int x, int y) {
    dropHot_ = false;
    const int atTick = (int) std::llround(
        snapBeats(std::max(0.0, xToBeat((float) x)), false) * Pattern::kTicksPerBeat);
    host_.beginTransaction();
    host_.pushUndo();
    int placed = 0, tick = atTick;
    for (const auto& path : files) {
        const juce::File f(path);
        if (!f.existsAsFile() || !isInterestedInFileDrag({path})) continue;
        const auto node = dropTargetNode(y);
        if (node.empty()) continue;
        const int clip = placeAudioFile(node, tick, f);
        if (clip < 0) continue;
        ++placed;
        for (const auto& ci : host_.clips().list(node))
            if (ci.index == clip) tick = ci.startTick + ci.lengthTicks;
    }
    host_.endTransaction();
    if (placed > 0) { rebuild(); repaint(); return; }
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, "Import",
        files.size() == 1
            ? juce::File(files[0]).getFileName() + " could not be read as audio."
            : juce::String("None of those files could be read as audio."));
}

void TracksPane::importAudioInto(const std::string& node, int atTick) {
    chooser_ = std::make_unique<juce::FileChooser>(
        "Import Audio File",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        audioWildcard());
    chooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [this, node, atTick](const juce::FileChooser& fc) {
        const auto f = fc.getResult();
        if (!f.existsAsFile()) return;
        host_.pushUndo();
        placeAudioFile(node, atTick, f);
        rebuild();
        repaint();
    });
}

}
