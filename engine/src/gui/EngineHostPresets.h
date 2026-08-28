#pragma once
#include <string>

#include "gui/PresetStack.h"
#include "io/PatchDocument.h"

namespace hum {

class EngineHost;

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

    explicit PresetHost(EngineHost& host) : host_(host) {}

private:
    EngineHost& host_;
    static PresetModel presetClip_;
    static std::string presetClipClass_;
    static bool hasPresetClip_;
};

}
