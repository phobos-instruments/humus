// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/WhatsNew.h"

#include <BinaryData.h>
#include <HumBuildId.h>

#include "gui/app/AppSettings.h"

namespace hum {

namespace {

constexpr const char* kSeenKey = "whatsnew.version";

}

std::string bundledChangelog() {
    return std::string(BinaryData::CHANGELOG_md, (size_t) BinaryData::CHANGELOG_mdSize);
}

const char* runningVersion() { return HUM_VERSION; }

void showWhatsNew(std::unique_ptr<WhatsNewWindow>& held, const std::vector<ReleaseNotes>& notes,
                  juce::Point<int> centre) {
    if (held) { held->toFront(true); return; }
    auto* slot = &held;
    held = std::make_unique<WhatsNewWindow>(whatsNewMarkdown(notes), runningVersion(), [slot] {
        if (auto* w = slot->release()) juce::MessageManager::callAsync([w] { delete w; });
    });
    held->setAlwaysOnTop(true);
    held->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
    held->setCentrePosition(centre);
    held->setVisible(true);
    held->toFront(true);
}

void showWhatsNewForThisVersion(std::unique_ptr<WhatsNewWindow>& held, juce::Point<int> centre) {
    showWhatsNew(held, notesFor(bundledChangelog(), runningVersion()), centre);
}

void offerWhatsNewAtLaunch(std::unique_ptr<WhatsNewWindow>& held, bool firstBoot, juce::Point<int> centre) {
    auto& settings = AppSettings::instance();
    const auto offer = whatsNewAtLaunch(bundledChangelog(), settings.getString(kSeenKey).toStdString(),
                                        runningVersion(), firstBoot);
    settings.set(kSeenKey, juce::String(offer.remember));
    if (!offer.notes.empty()) showWhatsNew(held, offer.notes, centre);
}

}
