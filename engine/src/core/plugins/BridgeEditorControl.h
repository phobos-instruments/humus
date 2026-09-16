// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

namespace hum {

struct BridgeEditorControl {
    virtual ~BridgeEditorControl() = default;

    virtual void bridgeCreateEditor() = 0;
    virtual void bridgeDestroyEditor() = 0;
    virtual void bridgeSetFloating(bool floating) = 0;
    virtual bool bridgeIsFloating() const = 0;

    virtual void bridgeSetEditorCallbacks(
        std::function<void(unsigned long xid, int w, int h)> onCreated,
        std::function<void(int w, int h)> onSize,
        std::function<void()> onFloatClosed) = 0;
};

}
