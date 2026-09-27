// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/HostedPlugins.h"

namespace hum {

namespace {

class NoHostedPlugins : public HostedPlugins {
public:
    const std::vector<ParamDesc>& schemaFor(const std::string&) const override {
        static const std::vector<ParamDesc> none;
        return none;
    }
    bool isInstrument(const std::string&) const override { return false; }
    std::string vendorOf(const std::string&) const override { return {}; }
};

const HostedPlugins*& installed() {
    static const HostedPlugins* plugins = nullptr;
    return plugins;
}

}

const HostedPlugins* setHostedPlugins(const HostedPlugins* plugins) {
    const auto* previous = installed();
    installed() = plugins;
    return previous;
}

const HostedPlugins& hostedPlugins() {
    static const NoHostedPlugins none;
    const auto* plugins = installed();
    return plugins != nullptr ? *plugins : none;
}

}
