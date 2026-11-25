// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "gui/host/PropertiesHost.h"
#include "gui/properties/PropertiesPane.h"

namespace hum {

class PaneRefresher {
public:
    void tick(PropertiesPane& pane, const PropertiesHost& host) {
        const unsigned generation = host.liveControlGeneration();
        if (host.isPlaying() || host.routesAlive() || generation != lastGeneration_ || dawControlsMapped(host))
            pane.refreshLiveValues();
        lastGeneration_ = generation;
        pane.prune();
        pane.refreshPresetState();
        pane.refreshTextEdits();
    }

private:
    static bool dawControlsMapped(const PropertiesHost& host) {
        for (int i = 0; i < host.dawControlCount(); ++i)
            if (!host.dawControlTargets(i).empty()) return true;
        return false;
    }

    unsigned lastGeneration_ = 0;
};

}
