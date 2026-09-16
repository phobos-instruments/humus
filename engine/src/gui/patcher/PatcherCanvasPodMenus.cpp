// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"
#include "io/PatchFormat.h"
#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginHost.h"
#include "core/plugins/PluginNode.h"
#include "core/graph/PodModel.h"
#include "gui/patcher/ClassPickerMenu.h"
#include "gui/help/HelpView.h"
#include "gui/patcher/NewPodPanel.h"
#include "gui/patcher/PickerLauncher.h"
#include "gui/patcher/QuickAddPalette.h"
#include "gui/common/Localisation.h"

namespace hum {

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
    menu.addItem(Rename, tr("patcher-canvas-menus.rename", "Rename... (F2)"));
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

}
