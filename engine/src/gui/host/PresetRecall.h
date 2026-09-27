// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include "gui/host/PropertiesHost.h"

namespace hum {

namespace presets {

inline void recallBracketed(PropertiesHost& host, const std::string& node,
                            const std::function<void()>& op) {
    auto& hist = host.paramHistory();
    hist.commit(node, host.captureNodeState(node));
    op();
    hist.commit(node, host.captureNodeState(node));
}

}
}
