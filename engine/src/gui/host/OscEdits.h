// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/net/ControlShape.h"
#include "core/net/OscControl.h"

namespace hum {

class OscEdits {
public:
    virtual ~OscEdits() = default;

    virtual bool setEnabled(bool on) = 0;
    virtual bool enabled() const = 0;
    virtual int port() const = 0;
    virtual std::string mapAddress(const std::string& address, const std::string& organism, const std::string& param,
                                   double min, double max, bool steal) = 0;
    virtual void clearAddress(const std::string& address, const std::string& organism, const std::string& param) = 0;
    virtual void clearForOrganism(const std::string& organism) = 0;
    virtual void renameOrganism(const std::string& oldName, const std::string& newName) = 0;
    virtual const OscControlMap& map() const = 0;
    virtual void setShape(const std::string& address, const std::string& organism, const std::string& param,
                          const ControlShape& shape) = 0;
    virtual void inject(const std::string& address, double value01) = 0;
    virtual std::string lastAddress() const = 0;
    virtual void clearLastAddress() = 0;
    virtual void applySerialSettings() = 0;
    virtual bool serialEnabled() const = 0;
    virtual bool serialOpen() const = 0;
};

}
