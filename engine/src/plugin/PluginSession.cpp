// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin/PluginSession.h"

#include <utility>

namespace hum {

namespace {

constexpr int kMagicV2 = 0x484D5832;
constexpr int kMagicV3 = 0x484D5833;
constexpr int kMagicV4 = 0x484D5834;
constexpr int kMagicV5 = 0x484D5835;

juce::String utf8(const std::string& s) { return juce::String::fromUTF8(s.c_str()); }

}

juce::MemoryBlock encodeSession(const PluginSession& session) {
    juce::MemoryBlock block;
    juce::MemoryOutputStream os(block, false);
    os.writeInt(kMagicV5);
    os.writeString(utf8(session.documentPath));
    for (const auto& targets : session.macros) {
        os.writeInt((int) targets.size());
        for (const auto& t : targets) {
            os.writeString(utf8(t.organism));
            os.writeString(utf8(t.param));
            os.writeDouble(t.from);
            os.writeDouble(t.to);
        }
    }
    os.writeBool(session.playWhenStopped);
    os.writeString(utf8(session.text));
    os.flush();
    return block;
}

std::optional<PluginSession> decodeSession(const void* data, int size) {
    if (data == nullptr || size <= 0) return std::nullopt;
    PluginSession s;
    juce::MemoryInputStream is(data, (size_t) size, false);
    const int magic = size >= 4 ? is.readInt() : 0;
    if (magic != kMagicV2 && magic != kMagicV3 && magic != kMagicV4 && magic != kMagicV5) {
        s.text.assign(static_cast<const char*>(data), (size_t) size);
        return s;
    }
    if (magic == kMagicV4 || magic == kMagicV5) s.documentPath = is.readString().toStdString();
    for (auto& targets : s.macros) {
        if (magic == kMagicV5) {
            const int count = is.readInt();
            for (int i = 0; i < count && !is.isExhausted(); ++i) {
                DawMacroTarget t;
                t.organism = is.readString().toStdString();
                t.param = is.readString().toStdString();
                t.from = is.readDouble();
                t.to = is.readDouble();
                targets.push_back(std::move(t));
            }
            continue;
        }
        DawMacroTarget t;
        t.organism = is.readString().toStdString();
        t.param = is.readString().toStdString();
        if (!t.organism.empty()) targets.push_back(std::move(t));
    }
    s.playWhenStopped = magic == kMagicV2 || is.readBool();
    s.text = is.readString().toStdString();
    return s;
}

}
