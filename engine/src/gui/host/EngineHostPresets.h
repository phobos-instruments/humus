// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "gui/properties/PresetStack.h"
#include "io/PatchDocument.h"

namespace hum {


class PresetHost {
public:
    using Ref = presets::Ref;

    int  store(const std::string& name, const Ref& ref);
    void recall(const std::string& name, const Ref& ref);
    void clear(const std::string& name, const Ref& ref);
    void rename(const std::string& name, const Ref& ref, const std::string& newName);
    Ref  recallAdjacent(const std::string& name, int dir);
    void copy(const std::string& name, const Ref& ref);
    void cut(const std::string& name, const Ref& ref);
    bool paste(const std::string& name);
    int  adopt(const std::string& name, PresetModel pm);
    bool canPaste(const std::string& name) const;

    Ref current(const std::string& name) const;
    void setCurrent(const std::string& name, const Ref& ref);

    PresetHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), nodes_(core) {}

private:
    BrickHost& host_;
    HostDocument& doc_;
    HostNodes& nodes_;
    static PresetModel presetClip_;
    static std::string presetClipClass_;
    static bool hasPresetClip_;
};

}
