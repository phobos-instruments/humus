// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"

#include <algorithm>

#include "core/packs/Categories.h"
#include "core/graph/PodModel.h"
#include "gui/app/AppSettings.h"

namespace hum {

bool PatcherCanvas::keyPressed(const juce::KeyPress& k) {
    using juce::KeyPress;
    const auto cmd = juce::ModifierKeys::commandModifier;
    const auto cmdShift = cmd | juce::ModifierKeys::shiftModifier;
    if (k == KeyPress('z', cmdShift, 0)) { redo(); return true; }
    if (k == KeyPress('z', cmd, 0)) { undo(); return true; }
    if (k == KeyPress('a', cmd, 0)) { selectAll(); return true; }
    if (k == KeyPress('c', cmd, 0)) { copySelection(); return true; }
    if (k == KeyPress('x', cmd, 0)) { cutSelection(); return true; }
    if (k == KeyPress('v', cmd, 0)) { pasteClipboard(); return true; }
    if (k == KeyPress('d', cmd, 0)) { duplicateSelection(); return true; }
    if (k == KeyPress(juce::KeyPress::F2Key)) { renameSelection(); return true; }
    if (k == KeyPress('0', cmd, 0)) { resetView(); return true; }
    if (k == KeyPress('=', cmd, 0) || k == KeyPress('+', cmdShift, 0)) {
        setZoom(zoom() * 1.2f, getLocalBounds().getCentre());
        return true;
    }
    if (k == KeyPress('-', cmd, 0)) {
        setZoom(zoom() / 1.2f, getLocalBounds().getCentre());
        return true;
    }
    if (k.getKeyCode() == KeyPress::deleteKey ||
        k.getKeyCode() == KeyPress::backspaceKey) {
        deleteSelection();
        return true;
    }
    return false;
}

void PatcherCanvas::copySelection() {
    clipboard_.clear();
    clipboardPos_.clear();
    clipboardCords_.clear();
    clipboardMidiCords_.clear();
    clipboardVideoCords_.clear();
    podClipboard_.clear();
    pasteCount_ = 0;
    for (auto& s : selection_) {
        if (auto* m = host_.model().byName(s)) {
            clipboard_.push_back(*m);
            clipboardPos_[s] = host_.position(s);
        } else if (isPodBox(s)) {
            podClipboard_.push_back(host_.capturePod(s));
        }
    }
    auto inClip = [this](const std::string& n) {
        for (auto& m : clipboard_) if (m.name == n) return true;
        return false;
    };
    for (auto& c : host_.model().connections)
        if (inClip(c.src) && inClip(c.dst)) clipboardCords_.push_back(c);
    for (auto& c : host_.model().midiConnections)
        if (inClip(c.src) && inClip(c.dst)) clipboardMidiCords_.push_back(c);
    for (auto& c : host_.model().videoConnections)
        if (inClip(c.src) && inClip(c.dst)) clipboardVideoCords_.push_back(c);
}

void PatcherCanvas::cutSelection() {
    if (cordSelected_) { deleteSelection(); return; }
    if (selection_.empty()) return;
    copySelection();
    deleteSelection();
}

void PatcherCanvas::pasteClipboard() {
    if (clipboard_.empty() && podClipboard_.empty()) return;
    ++pasteCount_;
    const juce::Point<int> off{24 * pasteCount_, 24 * pasteCount_};
    instantiateClones(clipboard_, clipboardPos_, clipboardCords_, clipboardMidiCords_,
                      clipboardVideoCords_, off);
    for (const auto& k : podClipboard_) {
        const auto pod = host_.pastePod(k, k.boxPos + off, scope_);
        if (!pod.empty()) { selection_.insert(pod); primary_ = pod; }
    }
    if (selection_.size() != 1) primary_.clear();
    notifySelection();
    refresh();
}

void PatcherCanvas::duplicateSelection() {
    std::vector<OrganismModel> items;
    std::map<std::string, juce::Point<int>> positions;
    std::vector<PodClip> podItems;
    for (auto& s : selection_) {
        if (auto* m = host_.model().byName(s)) {
            items.push_back(*m);
            positions[s] = host_.position(s);
        } else if (isPodBox(s)) {
            podItems.push_back(host_.capturePod(s));
        }
    }
    auto inSet = [&items](const std::string& n) {
        for (auto& m : items) if (m.name == n) return true;
        return false;
    };
    std::vector<ConnectionModel> cords, midiCords, videoCords;
    for (auto& c : host_.model().connections)
        if (inSet(c.src) && inSet(c.dst)) cords.push_back(c);
    for (auto& c : host_.model().midiConnections)
        if (inSet(c.src) && inSet(c.dst)) midiCords.push_back(c);
    for (auto& c : host_.model().videoConnections)
        if (inSet(c.src) && inSet(c.dst)) videoCords.push_back(c);

    instantiateClones(items, positions, cords, midiCords, videoCords, {24, 24});
    for (const auto& k : podItems) {
        const auto pod = host_.pastePod(k, k.boxPos + juce::Point<int>{24, 24}, scope_);
        if (!pod.empty()) { selection_.insert(pod); primary_ = pod; }
    }
    if (selection_.size() != 1) primary_.clear();
    notifySelection();
    refresh();
}

void PatcherCanvas::instantiateClones(const std::vector<OrganismModel>& items,
                                      const std::map<std::string, juce::Point<int>>& positions,
                                      const std::vector<ConnectionModel>& cords,
                                      const std::vector<ConnectionModel>& midiCords,
                                      const std::vector<ConnectionModel>& videoCords,
                                      juce::Point<int> off) {
    if (items.empty()) return;
    host_.beginTransaction();
    selection_.clear();
    std::map<std::string, std::string> cloneOf;
    for (const auto& m : items) {
        auto it = positions.find(m.name);
        auto pos = (it != positions.end() ? it->second : host_.position(m.name)) + off;
        auto newName = host_.addOrganism(m.classRaw, pos, scope_);
        if (newName.empty()) continue;
        cloneOf[m.name] = newName;
        for (const auto& pr : m.properties) {
            if (!pr.text.empty())     host_.setParamText(newName, pr.name, pr.text);
            else if (pr.isRange)      host_.setParamRange(newName, pr.name, pr.rangeMin, pr.rangeMax);
            else                      host_.setParam(newName, pr.name, pr.value);
        }
        selection_.insert(newName);
        primary_ = newName;
    }
    for (const auto& c : cords) {
        auto s = cloneOf.find(c.src), d = cloneOf.find(c.dst);
        if (s != cloneOf.end() && d != cloneOf.end())
            host_.connect(s->second, c.srcOutlet, d->second, c.dstInlet);
    }
    for (const auto& c : midiCords) {
        auto s = cloneOf.find(c.src), d = cloneOf.find(c.dst);
        if (s != cloneOf.end() && d != cloneOf.end())
            host_.connectMidi(s->second, c.srcOutlet, d->second, c.dstInlet);
    }
    for (const auto& c : videoCords) {
        auto s = cloneOf.find(c.src), d = cloneOf.find(c.dst);
        if (s != cloneOf.end() && d != cloneOf.end())
            host_.connectVideo(s->second, c.srcOutlet, d->second, c.dstInlet);
    }
    host_.endTransaction();
    if (selection_.size() != 1) primary_.clear();
    notifySelection();
    refresh();
}

void PatcherCanvas::deleteSelection() {
    if (cordSelected_) {
        if (selectedCord_.control)
            host_.disconnectControl(selectedCord_.src, selectedCord_.srcOutlet,
                                    selectedCord_.dst, selectedCord_.dstInlet);
        else if (selectedCord_.midi)
            host_.removeMidiConnection(selectedCord_.src, selectedCord_.srcOutlet,
                                       selectedCord_.dst, selectedCord_.dstInlet);
        else if (selectedCord_.video)
            host_.removeVideoConnection(selectedCord_.src, selectedCord_.srcOutlet,
                                        selectedCord_.dst, selectedCord_.dstInlet);
        else
            host_.removeConnection(selectedCord_.src, selectedCord_.srcOutlet,
                                   selectedCord_.dst, selectedCord_.dstInlet);
        cordSelected_ = false;
        refresh();
        return;
    }
    auto sel = selection_;
    selection_.clear();
    primary_.clear();
    host_.beginTransaction();
    for (auto& s : sel) {
        if (host_.model().byName(s)) host_.removeOrganism(s);
        else if (pods::isPod(host_.model(), s)) host_.deletePod(s);
    }
    host_.endTransaction();
    notifySelection();
    refresh();
}

void PatcherCanvas::selectAll() {
    cordSelected_ = false;
    selection_.clear();
    for (auto& d : displayNodes()) selection_.insert(d.name);
    primary_ = selection_.size() == 1 ? *selection_.begin() : std::string();
    notifySelection();
    refresh();
}

void PatcherCanvas::undo() {
    if (!host_.canUndo()) return;
    cordSelected_ = false;
    selection_.clear();
    primary_.clear();
    host_.undo();
    if (!scope_.empty() && !pods::isPod(host_.model(), scope_)) scope_.clear();
    notifySelection();
    if (onUndoRedo) onUndoRedo();
    refresh();
}

void PatcherCanvas::redo() {
    if (!host_.canRedo()) return;
    cordSelected_ = false;
    selection_.clear();
    primary_.clear();
    host_.redo();
    if (!scope_.empty() && !pods::isPod(host_.model(), scope_)) scope_.clear();
    notifySelection();
    if (onUndoRedo) onUndoRedo();
    refresh();
}

}
