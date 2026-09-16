// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
// GameController.framework sees paired PS/Xbox/MFi controllers automatically; polled as plain property reads on GamepadHost's 30 Hz timer.
#include "gui/host/GamepadHost.h"

#if defined(__APPLE__)

#import <GameController/GameController.h>

namespace hum {

void gamepadPlatformPoll(std::vector<GamepadSnapshot>& out) {
    out.clear();
    for (GCController* c in [GCController controllers]) {
        GCExtendedGamepad* g = c.extendedGamepad;
        if (g == nil) continue;
        GamepadSnapshot s;
        s.name = c.vendorName != nil ? [c.vendorName UTF8String] : "controller";
        s.axes[0] = (g.leftThumbstick.xAxis.value + 1.0f) * 0.5f;
        s.axes[1] = (g.leftThumbstick.yAxis.value + 1.0f) * 0.5f;    // GC: up is +1 already
        s.axes[2] = (g.rightThumbstick.xAxis.value + 1.0f) * 0.5f;
        s.axes[3] = (g.rightThumbstick.yAxis.value + 1.0f) * 0.5f;
        s.axes[4] = g.leftTrigger.value;
        s.axes[5] = g.rightTrigger.value;
        s.buttons[0] = g.buttonA.isPressed ? 1.0f : 0.0f;
        s.buttons[1] = g.buttonB.isPressed ? 1.0f : 0.0f;
        s.buttons[2] = g.buttonX.isPressed ? 1.0f : 0.0f;
        s.buttons[3] = g.buttonY.isPressed ? 1.0f : 0.0f;
        out.push_back(std::move(s));
    }
}

}  // namespace hum

#endif
