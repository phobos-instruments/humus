// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/LayoutEditor.h"

#include "gui/editor/AutomateMenu.h"
#include "gui/editor/MomentaryPress.h"

namespace hum {

void LayoutEditor::openMenu(const std::string& param, Point at) {
    showAutomateMenu(host_, organism(), param, {at.x, at.y}, [this] { automationChanged(); });
}

int LayoutEditor::coverTicks() const {
    auto* device = host_.audioDevices().getCurrentAudioDevice();
    return device == nullptr ? momentary::coverTicksFor(0.0, 0)
                             : momentary::coverTicksFor(device->getCurrentSampleRate(),
                                                        device->getCurrentBufferSizeSamples());
}

}
