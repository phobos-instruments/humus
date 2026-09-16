// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/tracks/SongView.h"
#include "core/timeline/ClipDrag.h"
#include "core/packs/Roles.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "gui/tracks/AutoPointPopup.h"
#include "gui/app/QwertyPiano.h"
#include "gui/common/Localisation.h"

namespace hum {

void SongView::mouseDownHeader(const juce::MouseEvent& e, int row, juce::Point<int> p) {
    const auto& node = rows_[(size_t) row];
        if (e.mods.isPopupMenu()) {
            const auto* cm = host().model().byName(node);
            bool hasBoxes = false;
            for (const auto& b : host().automation().boxes())
                if (b.organism == node) { hasBoxes = true; break; }
            const bool has = (cm != nullptr && !cm->automation.empty()) || hasBoxes;
            juce::PopupMenu m;
            m.addItem(1, tr("tracks-pane-input.clear-automation-points", "Clear Automation Points"), has);
            m.addItem(2, tr("tracks-pane-input.delete-automation-lanes", "Delete Automation Lanes"), has);
            double from = 0.0, to = 0.0;
            const int bpb = juce::jmax(1, host().automation().timeSigNumerator());
            const bool canBounce = consolidateRange(row, from, to)
                                   && !host().bounceSourceOf(node).empty();
            m.addSeparator();
            m.addItem(3, canBounce
                             ? "Consolidate to Audio (bars " + juce::String((int) (from / bpb) + 1)
                                   + juce::String::fromUTF8("\xe2\x80\x93")
                                   + juce::String((int) std::ceil(to / bpb)) + ")"
                             : juce::String("Consolidate to Audio"),
                      canBounce);
            std::vector<std::string> targets;
            if (host().midiOutletsOf(node) == 1) {
                for (const auto& other : host().model().organisms) {
                    if (other.name == node) continue;
                    if (isHiddenOrganism(other.displayClass)) continue;
                    if (host().midiInletsOf(other.name) < 1) continue;
                    targets.push_back(other.name);
                }
                juce::PopupMenu send;
                for (int i = 0; i < (int) targets.size(); ++i) {
                    bool corded = false;
                    for (const auto& c : host().model().midiConnections)
                        if (c.src == node && c.dst == targets[(size_t) i]) corded = true;
                    send.addItem(100 + i, juce::String(targets[(size_t) i]), true, corded);
                }
                m.addSeparator();
                m.addSubMenu(tr("tracks-pane-input.send-midi-to", "Send MIDI to"), send, !targets.empty());
            }
            const bool ownsTheRow = ownsRow(node);
            std::vector<std::string> group;
            if (selTracks_.count(node) != 0 && selTracks_.size() > 1) group = selectedTracks();
            m.addSeparator();
            m.addItem(5, tr("tracks-pane-input.rename-track", "Rename Track..."));
            m.addItem(4, group.size() > 1
                             ? juce::String("Delete ") + juce::String((int) group.size())
                                   + " Tracks"
                             : (ownsTheRow ? tr("tracks-pane-input.delete-track", "Delete Track")
                                           : tr("tracks-pane-input.remove-track", "Remove Track")));
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, node, row, targets, group](int r) {
                if (r >= 100 && r - 100 < (int) targets.size()) {
                    host().pushUndo();
                    std::vector<ConnectionModel> old;
                    for (const auto& c : host().model().midiConnections)
                        if (c.src == node && c.srcOutlet == 0) old.push_back(c);
                    for (const auto& c : old)
                        host().removeMidiConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
                    host().connectMidi(node, 0, targets[(size_t) (r - 100)], 0);
                    ctx_.patchChanged();
                    return;
                }
                if (r == 3) { consolidateRow(row); return; }
                if (r == 5) {
                    const auto slash = node.rfind('/');
                    const auto prefix =
                        slash == std::string::npos ? std::string() : node.substr(0, slash + 1);
                    const auto leaf =
                        slash == std::string::npos ? node : node.substr(slash + 1);
                    auto* aw = new juce::AlertWindow(
                        tr("tracks-pane-input.rename-track-title", "Rename Track"),
                        tr("tracks-pane-input.new-name-for", "New name for") + " \""
                            + juce::String(leaf) + "\":",
                        juce::MessageBoxIconType::NoIcon);
                    aw->addTextEditor("name", juce::String(leaf));
                    aw->addButton(tr("tracks-pane-input.rename", "Rename"), 1, juce::KeyPress(juce::KeyPress::returnKey));
                    aw->addButton(tr("tracks-pane-input.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
                    aw->enterModalState(true, juce::ModalCallbackFunction::create(
                        [this, aw, node, prefix](int ok) {
                            const auto text = aw->getTextEditorContents("name")
                                                  .replaceCharacter('/', '-')
                                                  .trim();
                            aw->exitModalState(ok);
                            aw->setVisible(false);
                            delete aw;
                            if (ok != 1 || text.isEmpty()) return;
                            if (host().renameOrganism(node, prefix + text.toStdString())) {
                                rebuild();
                                repaintAll();
                                ctx_.patchChanged();
                            }
                        }));
                    return;
                }
                if (r == 4) {
                    deleteRows(group.size() > 1 ? group : std::vector<std::string>{node});
                    return;
                }
                if (r > 0) host().automation().clearOrganism(node, r == 2);
                if (r > 0) rebuild();
            });
            return;
        }
        if (heldBox(row).contains(p) && host().automation().anyHeld(node)) {
            if (const auto* cm = host().model().byName(node))
                for (const auto& l : cm->automation)
                    host().automation().release(node, l.propertyName);
            repaintAll();
            return;
        }
        if (const auto* cmDest = host().model().byName(node);
            cmDest != nullptr && classHasRole(cmDest->classRaw, role::kMidiTrack)
            && destBox(row, node).contains(p) && !e.mods.isPopupMenu()) {
            double current = 1.0;
            for (const auto& prm : cmDest->properties)
                if (prm.name == "Target") { current = prm.value; break; }
            juce::PopupMenu m;
            for (const auto& item : host().choiceItems("midi-targets", node))
                m.addItem(item.first, juce::String::fromUTF8(item.second.c_str()), true,
                          item.first == (int) current);
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, node](int r) {
                if (r <= 0) return;
                host().editParam(node, "Target", (double) r);
                repaintAll();
                ctx_.patchChanged();
            });
            return;
        }
        if (p.x > 18 && p.x < kStripW - 84 && !e.mods.isPopupMenu()) {
            selectTrack(row, e.mods.isShiftDown(), e.mods.isCommandDown());
            return;
        }
        if (foldBox(row).contains(p) && hasLanes(row) && !wantsBoxRow(row)) {
            if (!expanded_.insert(node).second) expanded_.erase(node);
            rebuildSlots();
            repaintAll();
            return;
        }
        if (e.mods.isPopupMenu()) {
            const char* target = muteBox(row).contains(p) ? kTrackMuteParam
                               : soloBox(row).contains(p) ? kSoloParam
                               : recBox(row).contains(p)  ? kArmParam : nullptr;
            if (target != nullptr) {
                showAutomateMenu(host(), node, target, paneToScreen(p),
                                 [this] { repaintAll(); }, false);
                return;
            }
        }
        if (soloBox(row).contains(p) && arrangeable_.count(node) != 0) {
            host().setSoloed(node, !host().soloed(node));
            repaintRow(row);
            return;
        }
        if (muteBox(row).contains(p)) {
            setNodeMuted(node, !nodeMuted(node));
            repaintRow(row);
        } else if (recBox(row).contains(p)) {
            if (host().nodeRecordsMedia(node)) {
                bool armed = false;
                if (const auto* cm = host().model().byName(node))
                    for (const auto& pr : cm->properties)
                        if (pr.name == "Record") { armed = pr.value >= 0.5; break; }
                host().setParam(node, "Record", armed ? 0.0 : 1.0);
                repaintRow(row);
                return;
            }
            const bool arm = !host().midi().isRecordTarget(node);
            host().midi().setRecordTarget(node, arm, 0, MidiHost::kClipOnDemand, true);
            ctx_.liveTargetsChanged();
            repaintRow(row);
        }
        return;
}

bool SongView::ownsRow(const std::string& node) const {
    const auto* cm = host().model().byName(node);
    return host().nodeRecordsAudio(node) || host().nodeArrangesVideo(node)
           || (cm != nullptr && classHasRole(cm->classRaw, role::kMidiTrack));
}

void SongView::selectTrack(int row, bool range, bool toggle) {
    const auto& node = rows_[(size_t) row];
    if (range && selClipRow_ >= 0 && selClipRow_ < (int) rows_.size()) {
        const int a = std::min(selClipRow_, row);
        const int b = std::max(selClipRow_, row);
        for (int i = a; i <= b; ++i) selTracks_.insert(rows_[(size_t) i]);
    } else if (toggle) {
        if (!selTracks_.insert(node).second) selTracks_.erase(node);
    } else {
        selTracks_.clear();
    }
    selectClip(row, -1);
    repaintAll();
}

std::vector<std::string> SongView::selectedTracks() const {
    std::vector<std::string> out;
    for (const auto& n : rows_)
        if (selTracks_.count(n) != 0) out.push_back(n);
    if (out.empty() && selClip_ < 0 && selClipRow_ >= 0 && selClipRow_ < (int) rows_.size())
        out.push_back(rows_[(size_t) selClipRow_]);
    return out;
}

void SongView::deleteRows(const std::vector<std::string>& nodes) {
    int first = (int) rows_.size();
    for (const auto& n : nodes)
        for (int r = 0; r < (int) rows_.size(); ++r)
            if (rows_[(size_t) r] == n) first = std::min(first, r);
    clearClipSel();
    host().beginTransaction();
    for (const auto& n : nodes) deleteRow(n);
    host().endTransaction();
    selTracks_.clear();
    rebuild();
    if (!rows_.empty()) selectClip(std::min(first, (int) rows_.size() - 1), -1);
    repaintAll();
    ctx_.patchChanged();
}

bool SongView::deleteSelectedTracks() {
    const auto tracks = selectedTracks();
    if (tracks.empty()) return false;
    deleteRows(tracks);
    return true;
}

void SongView::deleteRow(const std::string& node) {
    if (ownsRow(node)) {
        host().removeOrganism(node);
    } else if (arrangeable_.count(node) != 0) {
        host().pushUndo();
        host().clips().removeTrack(node);
    } else {
        host().automation().clearOrganism(node, true);
    }
}

}
