// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/params/ParamSchema.h"

namespace hum {

class HostedPlugins {
public:
    virtual ~HostedPlugins() = default;
    virtual const std::vector<ParamDesc>& schemaFor(const std::string& className) const = 0;
    virtual bool isInstrument(const std::string& className) const = 0;
    virtual std::string vendorOf(const std::string& className) const = 0;
};

const HostedPlugins* setHostedPlugins(const HostedPlugins* plugins);
const HostedPlugins& hostedPlugins();

}
