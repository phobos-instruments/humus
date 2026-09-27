// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"
#include "gui/app/FreeWindow.h"
#include "gui/app/HistoryView.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"

#include "gui/common/Localisation.h"

namespace hum {

void MainComponent::refreshAfterLoad() {
    canvas_->exitToScope("");
    canvas_->select("");
    propsPane_->reload();
    refreshTimelinePanes();
    canvas_->refresh();
    menuItemsChanged();
    reopenVisualOutputs();
}

void MainComponent::openHistory() {
    if (!historyWindow_) {
        HistoryView::Actions a;
        a.list = [this] { return history_.entries(); };
        a.restore = [this](const HistoryView::Entry& e) {
            std::string err;
            const bool sounding = host_.audioRunning();
            if (!history_.restore(e, currentFile_, nowMs(), err)) {
                notifyError(tr("main-history.restore-failed", "restore failed: ") + juce::String(err));
                return;
            }
            refreshAfterLoad();
            if (sounding) ensureAudio();
            if (auto* view = dynamic_cast<HistoryView*>(historyWindow_->getContentComponent())) view->refresh();
        };
        a.openCopy = [this](const HistoryView::Entry& e) {
            confirmDiscardThenRun([this, e] {
                std::string err;
                const bool sounding = host_.audioRunning();
                const auto original = currentFile_;
                if (!history_.openCopy(e, original, nowMs(), err)) {
                    notifyError(tr("main-history.open-failed", "could not open that version: ") + juce::String(err));
                    return;
                }
                currentFile_.clear();
                refreshAfterLoad();
                if (sounding) ensureAudio();
                if (historyWindow_ != nullptr)
                    if (auto* view = dynamic_cast<HistoryView*>(historyWindow_->getContentComponent())) view->refresh();
            });
        };
        a.keep = [this](const HistoryView::Entry& e, const juce::String& name) { history_.keep(e.at, name); };
        a.release = [this](const HistoryView::Entry& e) { history_.release(e.at); };
        a.clear = [this] { history_.clear(currentFile_, nowMs()); };
        historyWindow_ = std::make_unique<FreeWindow>(tr("main-history.title", "History"), new HistoryView(a));
        historyWindow_->onClose = [this] { historyWindow_.reset(); };
        wireGlobalKeys(*historyWindow_);
    }
    if (auto* view = dynamic_cast<HistoryView*>(historyWindow_->getContentComponent())) view->refresh();
    historyWindow_->setVisible(true);
    historyWindow_->toFront(true);
}

}
