// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

#include "core/packs/Roles.h"
#include "gui/common/Localisation.h"

namespace hum {

namespace {
constexpr int kLitMs = 160;
constexpr int kAutoItem = 1, kAllItem = 2, kNoneItem = 3, kFirstPortItem = 10;
}

void SongView::showInputMenu(const std::string& node, juce::Point<int> screen) {
    const int current = host().midi().trackInput(node);
    juce::PopupMenu m;
    m.addItem(kAutoItem, tr("tracks-pane.input-auto", "Auto - follow the patch"), true,
              current == OrganismModel::kTrackInputAuto);
    m.addItem(kAllItem, tr("tracks-pane.input-all", "All inputs"), true, current == OrganismModel::kTrackInputAll);
    for (const auto& item : host().choiceItems("midi-in-ports", node))
        if (item.first >= 1)
            m.addItem(kFirstPortItem + item.first - 1, juce::String::fromUTF8(item.second.c_str()), true,
                      current == item.first - 1);
    m.addSeparator();
    m.addItem(kNoneItem, tr("tracks-pane.input-none", "None - only its cord"), true,
              current == OrganismModel::kTrackInputNone);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screen.x, screen.y, 1, 1}),
                    [this, node](int r) {
        if (r <= 0) return;
        const int input = r == kAutoItem ? OrganismModel::kTrackInputAuto
                        : r == kAllItem  ? OrganismModel::kTrackInputAll
                        : r == kNoneItem ? OrganismModel::kTrackInputNone
                                         : r - kFirstPortItem;
        host().midi().setTrackInput(node, input);
        repaintAll();
        ctx_.patchChanged();
    });
}

void SongView::pollInputLights() {
    const double now = juce::Time::getMillisecondCounterHiRes();
    for (int row = 0; row < (int) rows_.size(); ++row) {
        const auto& node = rows_[(size_t) row];
        const auto* cm = host().model().byName(node);
        if (cm == nullptr || !classHasRole(cm->classRaw, role::kMidiTrack)) continue;
        const unsigned count = host().midi().inputActivity(node);
        auto seen = inputSeen_.find(node);
        const bool fresh = seen != inputSeen_.end() && seen->second != count;
        inputSeen_[node] = count;
        const auto lit = inputLitAt_.find(node);
        if (fresh) {
            const bool wasDark = lit == inputLitAt_.end();
            inputLitAt_[node] = now;
            if (wasDark) repaintPane(inputBox(row));
        } else if (lit != inputLitAt_.end() && now - lit->second > kLitMs) {
            inputLitAt_.erase(lit);
            repaintPane(inputBox(row));
        }
    }
}

}
