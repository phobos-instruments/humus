#include "gui/PatcherCanvas.h"

#include "io/PatchFormat.h"

#include "core/Categories.h"
#include "core/ClassString.h"
#include "core/HostedPlugin.h"
#include "core/PluginHost.h"
#include "core/PluginNode.h"
#include "core/PodModel.h"
#include "gui/ClassPickerMenu.h"
#include "gui/HelpView.h"
#include "gui/NewPodPanel.h"
#include "gui/PickerLauncher.h"
#include "gui/QuickAddPalette.h"
#include "gui/Localisation.h"

namespace hum {

void PatcherCanvas::showAddMenu(juce::Point<int> at, juce::Point<int> screenPos) {
    enum { kNewPod = 900001, kPodFromPatch, kAddInlet, kAddOutlet,
           kAddMidiInlet, kAddMidiOutlet, kAddVideoInlet, kAddVideoOutlet,
           kArrange, kSearch };
    std::vector<std::string> ids;
    juce::PopupMenu menu;
    menu.addItem(kSearch, juce::String::fromUTF8("New Organism\xe2\x80\xa6"));
    menu.addSeparator();
    if (!modernMenusEnabled()) {
        juce::PopupMenu picker = classPickerMenu(0, ids);
        for (juce::PopupMenu::MenuItemIterator it(picker); it.next();)
            menu.addItem(it.getItem());
        menu.addSeparator();
    }
    menu.addItem(kNewPod, tr("patcher-canvas-menus.new-pod", "New Pod..."));
    menu.addItem(kPodFromPatch, tr("patcher-canvas-menus.pod-from-patch", "Pod from Patch..."));
    if (!scope_.empty()) {
        menu.addItem(kAddInlet, tr("patcher-canvas-menus.add-pod-inlet", "Add Pod Inlet"));
        menu.addItem(kAddOutlet, tr("patcher-canvas-menus.add-pod-outlet", "Add Pod Outlet"));
        menu.addItem(kAddMidiInlet, tr("patcher-canvas-menus.add-pod-midi-inlet", "Add Pod MIDI Inlet"));
        menu.addItem(kAddMidiOutlet, tr("patcher-canvas-menus.add-pod-midi-outlet", "Add Pod MIDI Outlet"));
        menu.addItem(kAddVideoInlet, tr("patcher-canvas-menus.add-pod-video-inlet", "Add Pod Video Inlet"));
        menu.addItem(kAddVideoOutlet, tr("patcher-canvas-menus.add-pod-video-outlet", "Add Pod Video Outlet"));
    }
    menu.addSeparator();
    menu.addItem(kArrange, tr("patcher-canvas-menus.auto-arrange", "Auto-arrange"));
    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                       [this, ids, at, screenPos](int result) {
        if (result == kSearch) { openQuickAdd(at, screenPos); return; }
        if (result == kNewPod) { showNewPodDialog(at); return; }
        if (result == kPodFromPatch) { podFromPatchDialog(at); return; }
        if (result == kArrange) { autoArrange(); return; }
        if (result == kAddInlet || result == kAddOutlet
            || result == kAddMidiInlet || result == kAddMidiOutlet
            || result == kAddVideoInlet || result == kAddVideoOutlet) {
            const char* cls = result == kAddInlet       ? pods::kInletClass
                            : result == kAddOutlet      ? pods::kOutletClass
                            : result == kAddMidiInlet   ? pods::kMidiInletClass
                            : result == kAddMidiOutlet  ? pods::kMidiOutletClass
                            : result == kAddVideoInlet  ? pods::kVideoInletClass
                                                        : pods::kVideoOutletClass;
            host_.addOrganism(cls, at, scope_);
            refresh();
            return;
        }
        if (result >= 1 && result <= (int) ids.size() && onAddRequest)
            onAddRequest(ids[(size_t) (result - 1)], at);
    });
}

void PatcherCanvas::openQuickAdd(juce::Point<int> at, juce::Point<int> screenPos) {
    showCreatePicker({screenPos.x, screenPos.y, 1, 1},
                     [this, at](const std::string& cls) {
                         if (onAddRequest) onAddRequest(cls, at);
                     });
}

void PatcherCanvas::podFromPatchDialog(juce::Point<int> at) {
    chooser_ = std::make_unique<juce::FileChooser>("Pod from Patch", juce::File(),
                                                   kPatchOpenFilter);
    chooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [this, at](const juce::FileChooser& fc) {
        if (fc.getResult() == juce::File()) return;
        std::string err;
        const auto pod = host_.importPatchAsPod(
            fc.getResult().getFullPathName().toStdString(), at, scope_, err);
        if (pod.empty()) {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                   "Pod from Patch", juce::String(err));
            return;
        }
        select(pod);
        notifySelection();
        refresh();
    });
}

void PatcherCanvas::showNewPodDialog(juce::Point<int> at) {
    const auto screen = localPointToGlobal(at);
    NewPodPanel::show({screen.x, screen.y, 1, 1},
                      [this, at](int ins, int outs, int midiIns, int midiOuts, bool stereo) {
        const auto pod = host_.createPod(ins, outs, at, scope_, stereo, midiIns, midiOuts);
        select(pod);
        notifySelection();
        refresh();
    });
}

void PatcherCanvas::showPodMenu(const std::string& pod, juce::Point<int> screenPos) {
    enum { Open = 1, Rename, Duplicate, Cut, Copy, Delete_, Disconnect, Ungroup };
    static constexpr int kInsBefore = 3000000, kInsAfter = 4000000;
    static constexpr int kSearchBase = 8000000;

    const auto groups = classPickerGroups();
    std::vector<std::string> classOrder;
    auto pickerSubmenu = [&](int base, int searchId, bool capture) {
        std::vector<std::string> scratch;
        juce::PopupMenu m;
        m.addItem(kSearchBase + searchId, juce::String::fromUTF8("Search\xe2\x80\xa6"));
        m.addSeparator();
        juce::PopupMenu picker = classPickerMenu(groups, base, capture ? classOrder : scratch);
        for (juce::PopupMenu::MenuItemIterator it(picker); it.next();)
            m.addItem(it.getItem());
        return m;
    };

    juce::PopupMenu menu;
    menu.addItem(Open, tr("patcher-canvas-menus.open-pod", "Open Pod"));
    menu.addSeparator();
    menu.addItem(Cut, tr("patcher-canvas-menus.cut", "Cut"));
    menu.addItem(Copy, tr("patcher-canvas-menus.copy", "Copy"));
    menu.addItem(Duplicate, tr("patcher-canvas-menus.duplicate", "Duplicate"));
    menu.addSeparator();
    menu.addItem(Delete_, tr("patcher-canvas-menus.delete", "Delete"));
    menu.addItem(Ungroup, tr("patcher-canvas-menus.ungroup", "Ungroup"));
    menu.addItem(Rename, tr("patcher-canvas-menus.rename-mac", "Rename... (Cmd+R)"));
    menu.addSeparator();
    if (modernMenusEnabled()) {
        menu.addItem(kSearchBase + 1, juce::String::fromUTF8("Insert Before\xe2\x80\xa6"));
        menu.addItem(kSearchBase + 2, juce::String::fromUTF8("Insert After\xe2\x80\xa6"));
    } else {
        menu.addSubMenu(tr("patcher-canvas-menus.insert-before", "Insert Before"), pickerSubmenu(kInsBefore, 1, true));
        menu.addSubMenu(tr("patcher-canvas-menus.insert-after", "Insert After"), pickerSubmenu(kInsAfter, 2, false));
    }
    menu.addSeparator();
    menu.addItem(Disconnect, tr("patcher-canvas-menus.disconnect", "Disconnect"));
    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                       [this, pod, classOrder, screenPos](int r) {
        if (r == 0) return;
        const int sz = (int) classOrder.size();
        auto classAt = [&](int base) -> const std::string& {
            return classOrder[(size_t) (r - base - 1)];
        };
        if (r == kSearchBase + 1 || r == kSearchBase + 2) {
            const bool before = r == kSearchBase + 1;
            showCreatePicker({screenPos.x, screenPos.y, 1, 1},
                             [this, pod, before](const std::string& cls) {
                focusNewNode(before ? host_.insertBeforePod(pod, cls)
                                    : host_.insertAfterPod(pod, cls));
            });
            return;
        }
        if (r > kInsBefore && r <= kInsBefore + sz) {
            focusNewNode(host_.insertBeforePod(pod, classAt(kInsBefore)));
            return;
        }
        if (r > kInsAfter && r <= kInsAfter + sz) {
            focusNewNode(host_.insertAfterPod(pod, classAt(kInsAfter)));
            return;
        }
        switch (r) {
            case Open:      enterPod(pod); break;
            case Rename:    renamePodDialog(pod); break;
            case Duplicate: select(pod); duplicateSelection(); break;
            case Cut:       select(pod); cutSelection(); break;
            case Copy:      select(pod); copySelection(); break;
            case Delete_:   select(pod); deleteSelection(); break;
            case Disconnect: host_.disconnectPod(pod); refresh(); break;
            case Ungroup:
                host_.ungroupPod(pod);
                selection_.clear();
                primary_.clear();
                notifySelection();
                refresh();
                break;
            default: break;
        }
    });
}

void PatcherCanvas::renamePodDialog(const std::string& pod) {
    auto* aw = new juce::AlertWindow("Rename Pod",
                                     "New name for \"" + pods::leafOf(pod) + "\":",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", pods::leafOf(pod));
    aw->addButton(tr("patcher-canvas-menus.ok", "OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton(tr("patcher-canvas-menus.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw, pod](int r) {
        if (r == 1) {
            const auto leaf = aw->getTextEditorContents("name").trim()
                                  .replaceCharacter('/', '-').toStdString();
            if (host_.renamePod(pod, leaf)) {
                const auto parent = pods::parentOf(pod);
                select(parent.empty() ? leaf : parent + "/" + leaf);
                notifySelection();
                refresh();
            }
        }
        delete aw;
    }), false);
}

void PatcherCanvas::focusNewNode(const std::string& name) {
    if (name.empty()) return;
    select(name);
    notifySelection();
    if (onActivate) onActivate(name);
    refresh();
}

void PatcherCanvas::showNodeMenu(const std::string& node, juce::Point<int> screenPos) {
    enum Id { Edit = 1, Rename, Cut, Copy, Delete_, Swap, Bypass, Disconnect, Help, ParamControl, PluginUI, RestartPlugin, RecordOut, VisualUI, MakePod, Print };
    static constexpr int kReplace = 1000000, kSubstitute = 2000000,
                  kInsBefore = 3000000, kInsAfter = 4000000;

    static constexpr int kSearchBase = 8000000;
    enum { SearchReplace = 1, SearchSubstitute, SearchInsBefore, SearchInsAfter };

    const auto groups = classPickerGroups();
    std::vector<std::string> classOrder;
    auto pickerSubmenu = [&](int base, int searchId, bool capture) {
        std::vector<std::string> scratch;
        juce::PopupMenu m;
        m.addItem(kSearchBase + searchId, juce::String::fromUTF8("Search\xe2\x80\xa6"));
        m.addSeparator();
        juce::PopupMenu picker = classPickerMenu(groups, base, capture ? classOrder : scratch);
        for (juce::PopupMenu::MenuItemIterator it(picker); it.next();)
            m.addItem(it.getItem());
        return m;
    };

    std::vector<std::string> two(selection_.begin(), selection_.end());
    const bool canSwap = two.size() == 2;

    juce::PopupMenu menu;
    menu.addItem(Edit, tr("patcher-canvas-menus.edit", "Edit"));
    {
        auto* hp = host_.hostedPluginFor(node);
        if (hp) menu.addItem(PluginUI, tr("patcher-canvas-menus.open-plugin-ui", "Open Plugin UI"), hp->instance()->hasEditor());
        if (dynamic_cast<VideoNode*>(host_.liveOrganism(node)) != nullptr
            && dynamic_cast<VisualSource*>(host_.liveOrganism(node)) == nullptr)
            menu.addItem(VisualUI, tr("patcher-canvas-menus.open-visuals", "Open Visuals"));
        if (auto* pn = host_.pluginNodeFor(node); pn != nullptr && !pn->responding())
            menu.addItem(RestartPlugin, tr("patcher-canvas-menus.restart-plugin", "Restart Plugin"));
    }
    menu.addItem(Help, tr("patcher-canvas-menus.help", "Help"));
    menu.addItem(ParamControl, tr("patcher-canvas-menus.parameter-control", "Parameter Control"), false);
    menu.addSeparator();
    menu.addItem(Cut, tr("patcher-canvas-menus.cut", "Cut"));
    menu.addItem(Copy, tr("patcher-canvas-menus.copy", "Copy"));
    menu.addSeparator();
    menu.addItem(Delete_, tr("patcher-canvas-menus.delete", "Delete"));
    menu.addItem(Rename, tr("patcher-canvas-menus.rename-mac", "Rename... (Cmd+R)"));
    menu.addSeparator();
    if (modernMenusEnabled()) {
        menu.addItem(kSearchBase + SearchReplace, juce::String::fromUTF8("Replace\xe2\x80\xa6"));
        menu.addItem(kSearchBase + SearchSubstitute, juce::String::fromUTF8("Substitute\xe2\x80\xa6"));
        menu.addItem(kSearchBase + SearchInsBefore, juce::String::fromUTF8("Insert Before\xe2\x80\xa6"));
        menu.addItem(kSearchBase + SearchInsAfter, juce::String::fromUTF8("Insert After\xe2\x80\xa6"));
    } else {
        menu.addSubMenu(tr("patcher-canvas-menus.replace", "Replace"), pickerSubmenu(kReplace, SearchReplace, true));
        menu.addSubMenu(tr("patcher-canvas-menus.substitute", "Substitute"), pickerSubmenu(kSubstitute, SearchSubstitute, false));
        menu.addSubMenu(tr("patcher-canvas-menus.insert-before", "Insert Before"), pickerSubmenu(kInsBefore, SearchInsBefore, false));
        menu.addSubMenu(tr("patcher-canvas-menus.insert-after", "Insert After"), pickerSubmenu(kInsAfter, SearchInsAfter, false));
    }
    menu.addSeparator();
    menu.addItem(Swap, canSwap ? tr("patcher-canvas-menus.swap", "Swap") : tr("patcher-canvas-menus.swap-select-two-nodes", "Swap (select two nodes)"), canSwap);
    menu.addItem(MakePod, tr("patcher-canvas-menus.make-pod", "Make Pod"));
    menu.addItem(Bypass, tr("patcher-canvas-menus.bypass", "Bypass"), true, host_.bypassed(node));
    menu.addItem(Disconnect, tr("patcher-canvas-menus.disconnect", "Disconnect"));
    const auto printTargets = host_.midiOutletsOf(node) > 0 ? host_.noteTargets(node)
                                                            : std::vector<std::string>{};
    static constexpr int kPrintTo = 9200000;
    if (host_.midiOutletsOf(node) > 0) {
        if (printTargets.size() > 1) {
            juce::PopupMenu to;
            for (int i = 0; i < (int) printTargets.size(); ++i)
                to.addItem(kPrintTo + i, juce::String(juce::CharPointer_UTF8(printTargets[(size_t) i].c_str())));
            menu.addSubMenu(tr("patcher-canvas-menus.midi-to-track", "MIDI to Track"), to);
        } else {
            menu.addItem(Print, tr("patcher-canvas-menus.midi-to-track", "MIDI to Track"));
        }
    }

    static constexpr int kRecv = 9000000;
    const auto* cmNode = host_.model().byName(node);
    if (cmNode && host_.midiInletsOf(node) > 0) {
        juce::PopupMenu recv;
        recv.addItem(kRecv + 0, tr("patcher-canvas-menus.omni", "Omni"), true,
                     cmNode->midiReceiveMode == OrganismModel::kMidiOmni);
        recv.addItem(kRecv + 1, tr("patcher-canvas-menus.patch-cords-only", "Patch cords only"), true,
                     cmNode->midiReceiveMode == OrganismModel::kMidiCordsOnly);
        juce::PopupMenu chans;
        for (int ch = 1; ch <= 16; ++ch)
            chans.addItem(kRecv + 1 + ch, tr("patcher-canvas-menus.channel", "Channel ") + juce::String(ch), true,
                          cmNode->midiReceiveMode == OrganismModel::kMidiChannel
                              && cmNode->midiReceiveChannel == ch);
        recv.addSubMenu(tr("patcher-canvas-menus.channel-2", "Channel"), chans);
        menu.addSeparator();
        menu.addSubMenu(tr("patcher-canvas-menus.midi-receive", "MIDI Receive"), recv);
    }

    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                       [this, node, classOrder, two, canSwap, screenPos, printTargets](int r) {
        if (r == 0) return;
        if (r >= kRecv && r <= kRecv + 17) {
            if (r == kRecv)          host_.midi().setReceiveMode(node, OrganismModel::kMidiOmni, 1);
            else if (r == kRecv + 1) host_.midi().setReceiveMode(node, OrganismModel::kMidiCordsOnly, 1);
            else                     host_.midi().setReceiveMode(node, OrganismModel::kMidiChannel, r - kRecv - 1);
            return;
        }
        if (r == RestartPlugin) {
            host_.restartPluginNode(node);
            refresh();
            return;
        }
        if (r == PluginUI) {
            if (onOpenPluginUI) onOpenPluginUI(node);
            return;
        }
        if (r == VisualUI) {
            if (onOpenVisuals) onOpenVisuals(node);
            return;
        }
        if (r == Print || (r >= kPrintTo && r < kPrintTo + (int) printTargets.size())) {
            std::string err;
            const std::string pref = r >= kPrintTo ? printTargets[(size_t) (r - kPrintTo)] : std::string();
            host_.printToTimeline(node, err, -1, pref);
            if (!err.empty())
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                                       "MIDI to Track",
                                                       juce::String(juce::CharPointer_UTF8(err.c_str())));
            return;
        }
        if (r == Help) {
            const auto* cm = host_.model().byName(node);
            HelpView::show(cm ? cm->displayClass : node, {screenPos.x, screenPos.y, 1, 1});
            return;
        }
        if (r == Swap) {
            if (canSwap) { host_.swapOrganisms(two[0], two[1]); refresh(); }
            return;
        }
        if (r == MakePod) {
            juce::Point<int> at = host_.position(node);
            for (const auto& n : two) {
                const auto p = host_.position(n);
                at = {juce::jmin(at.x, p.x), juce::jmin(at.y, p.y)};
            }
            const auto pod = host_.makePod(two, at, scope_);
            if (!pod.empty()) { selection_.clear(); selection_.insert(pod); primary_ = pod; }
            notifySelection();
            refresh();
            return;
        }
        if (r > kSearchBase && r <= kSearchBase + SearchInsAfter) {
            const int which = r - kSearchBase;
            showCreatePicker({screenPos.x, screenPos.y, 1, 1},
                             [this, node, which](const std::string& cls) {
                switch (which) {
                    case SearchReplace:    focusNewNode(host_.replaceOrganism(node, cls)); break;
                    case SearchSubstitute: focusNewNode(host_.substituteOrganism(node, cls)); break;
                    case SearchInsBefore:  focusNewNode(host_.insertBefore(node, cls)); break;
                    case SearchInsAfter:   focusNewNode(host_.insertAfter(node, cls)); break;
                    default: break;
                }
            });
            return;
        }
        auto classAt = [&](int base) -> const std::string& { return classOrder[(size_t) (r - base - 1)]; };
        const int sz = (int) classOrder.size();
        if (r > kReplace    && r <= kReplace    + sz) { focusNewNode(host_.replaceOrganism(node, classAt(kReplace)));       return; }
        if (r > kSubstitute && r <= kSubstitute + sz) { focusNewNode(host_.substituteOrganism(node, classAt(kSubstitute))); return; }
        if (r > kInsBefore  && r <= kInsBefore  + sz) { focusNewNode(host_.insertBefore(node, classAt(kInsBefore)));            return; }
        if (r > kInsAfter   && r <= kInsAfter   + sz) { focusNewNode(host_.insertAfter(node, classAt(kInsAfter)));              return; }

        select(node); notifySelection();
        switch (r) {
            case Edit:    if (onActivate) onActivate(node); break;
            case Rename:  renameNode(node); break;
            case Cut:     cutSelection(); break;
            case Copy:    copySelection(); break;
            case Delete_: deleteSelection(); break;
            case Bypass:  host_.setBypass(node, !host_.bypassed(node)); refresh(); break;
            case Disconnect: host_.disconnectOrganism(node); refresh(); break;
            default: break;
        }
    });
}

void PatcherCanvas::showCordMenu(const Edge& cord, juce::Point<int> at, juce::Point<int> screenPos) {
    enum { Delete_ = 1, SearchInsert };
    static constexpr int kInsert = 100;
    static constexpr int kChan = 200;

    std::vector<std::string> classOrder;
    juce::PopupMenu menu;
    if (!cord.midi && !cord.video) {
        if (modernMenusEnabled()) {
            menu.addItem(SearchInsert, juce::String::fromUTF8("Insert\xe2\x80\xa6"));
        } else {
            juce::PopupMenu ins;
            ins.addItem(SearchInsert, juce::String::fromUTF8("Search\xe2\x80\xa6"));
            ins.addSeparator();
            juce::PopupMenu picker = classPickerMenu(kInsert, classOrder);
            for (juce::PopupMenu::MenuItemIterator it(picker); it.next();)
                ins.addItem(it.getItem());
            menu.addSubMenu(tr("patcher-canvas-menus.insert", "Insert"), ins);
        }
        menu.addSeparator();
    } else if (cord.midi) {
        const int cur = host_.midiCordChannel(cord.src, cord.srcOutlet, cord.dst, cord.dstInlet);
        juce::PopupMenu chan;
        chan.addItem(kChan, tr("patcher-canvas-menus.omni-all-channels", "Omni (all channels)"), true, cur == 0);
        for (int c = 1; c <= 16; ++c)
            chan.addItem(kChan + c, tr("patcher-canvas-menus.channel", "Channel ") + juce::String(c), true, cur == c);
        menu.addSubMenu(tr("patcher-canvas-menus.midi-channel", "MIDI Channel"), chan);
        menu.addSeparator();
    }
    menu.addItem(Delete_, tr("patcher-canvas-menus.delete", "Delete"));

    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                       [this, cord, at, classOrder, screenPos](int r) {
        if (r == 0) return;
        if (!cord.midi && !cord.video && r == SearchInsert) {
            showCreatePicker({screenPos.x, screenPos.y, 1, 1},
                             [this, cord, at](const std::string& cls) {
                focusNewNode(host_.insertOnCord(cord.src, cord.srcOutlet,
                                                cord.dst, cord.dstInlet, cls, at, scope_));
            });
            return;
        }
        if (r == Delete_) {
            if (cord.midi)
                host_.removeMidiConnection(cord.src, cord.srcOutlet, cord.dst, cord.dstInlet);
            else if (cord.video)
                host_.removeVideoConnection(cord.src, cord.srcOutlet, cord.dst, cord.dstInlet);
            else
                host_.removeConnection(cord.src, cord.srcOutlet, cord.dst, cord.dstInlet);
            cordSelected_ = false;
            refresh();
            return;
        }
        if (cord.midi && r >= kChan && r <= kChan + 16) {
            host_.setMidiCordChannel(cord.src, cord.srcOutlet, cord.dst, cord.dstInlet, r - kChan);
            return;
        }
        if (r > kInsert && r <= kInsert + (int) classOrder.size()) {
            focusNewNode(host_.insertOnCord(cord.src, cord.srcOutlet, cord.dst, cord.dstInlet,
                                            classOrder[(size_t) (r - kInsert - 1)], at, scope_));
        }
    });
}

void PatcherCanvas::renameSelection() {
    if (primary_.empty()) return;
    if (isPodBox(primary_)) renamePodDialog(primary_);
    else renameNode(primary_);
}

void PatcherCanvas::renameNode(const std::string& node) {
    const auto leaf = pods::leafOf(node);
    const auto parent = pods::parentOf(node);
    auto* aw = new juce::AlertWindow("Rename Organism",
                                     "New name for \"" + leaf + "\":",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", leaf);
    aw->addButton(tr("patcher-canvas-menus.ok", "OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton(tr("patcher-canvas-menus.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw, node, parent](int r) {
        if (r == 1) {
            const auto nl = aw->getTextEditorContents("name").trim()
                                .replaceCharacter('/', '-').toStdString();
            const auto nn = nl.empty() ? nl : (parent.empty() ? nl : parent + "/" + nl);
            if (!nn.empty() && nn != node && host_.renameOrganism(node, nn)) {
                select(nn); notifySelection(); refresh();
            }
        }
        delete aw;
    }), false);
}

}
