// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/ReleaseNotes.h"
#include "gui/app/WhatsNewWindow.h"

namespace hum {

std::string bundledChangelog();

const char* runningVersion();

void showWhatsNew(std::unique_ptr<WhatsNewWindow>& held, const std::vector<ReleaseNotes>& notes,
                  juce::Point<int> centre);

void showWhatsNewForThisVersion(std::unique_ptr<WhatsNewWindow>& held, juce::Point<int> centre);

void offerWhatsNewAtLaunch(std::unique_ptr<WhatsNewWindow>& held, bool firstBoot, juce::Point<int> centre);

}
