// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class PropertiesHost;

namespace cards {

using Present = std::function<void(std::unique_ptr<juce::Component> card)>;

inline std::map<const PropertiesHost*, Present>& desks() {
    static std::map<const PropertiesHost*, Present> byHost;
    return byHost;
}

inline void attach(const PropertiesHost& host, Present present) {
    desks()[&host] = std::move(present);
}

inline void detach(const PropertiesHost& host) { desks().erase(&host); }

inline void present(const PropertiesHost& host, std::unique_ptr<juce::Component> card) {
    const auto it = desks().find(&host);
    if (it != desks().end() && it->second) it->second(std::move(card));
}

}
}
