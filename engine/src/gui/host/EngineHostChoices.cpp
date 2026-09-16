// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/packs/Categories.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "hum/CameraCapture.h"
#include "hum/SerialPort.h"
#include "core/graph/GraphIo.h"
#include "core/net/LinkChase.h"
#include "core/packs/Roles.h"
#include "gui/app/AppSettings.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"
#include "hum/caps/Params.h"
#include "hum/dsp/DspMath.h"

namespace hum {

std::vector<std::pair<int, std::string>> EngineHost::choiceItems(const std::string& source,
                                                                 const std::string& organism) {
    std::vector<std::pair<int, std::string>> items;
    if (source == "patch-bank") {
        if (auto* pb = dynamic_cast<PatchBank*>(graph_ ? graph_->find(organism) : nullptr))
            for (int i = 0, n = pb->patchCount(); i < n; ++i) {
                auto name = pb->patchNameAt(i);
                items.push_back({i + 1, name.empty() ? std::to_string(i + 1)
                                                     : std::to_string(i + 1) + " " + name});
            }
        if (items.empty()) items.push_back({1, "1"});
        return items;
    }
    if (source == "video-inputs") {
        const auto cams = CameraCapture::availableDevices();
        for (size_t i = 0; i < cams.size(); ++i)
            items.push_back({(int) i + 1, cams[i]});   // utf8-ok: data
        if (items.empty()) items.push_back({1, "Default camera"});
        return items;
    }
    if (source == "midi-targets") {
        items.push_back({1, "(nothing)"});
        int id = 2;
        for (const auto& cm : model_.organisms) {
            if (cm.name == organism) continue;
            if (classHasRole(cm.classRaw, role::kMidiTrack)) continue;
            if (isHiddenOrganism(cm.displayClass)) continue;
            if (midiInletsOf(cm.name) < 1) continue;
            items.push_back({id++, cm.name});
        }
        return items;
    }
    if (source == "serial-ports") {
        items.push_back({1, "Auto - first USB serial"});
        const auto devs = serial::listDevices();
        for (size_t i = 0; i < devs.size(); ++i) {
            const auto slash = devs[i].rfind('/');
            items.push_back({(int) i + 2,
                             slash == std::string::npos ? devs[i]
                                                        : devs[i].substr(slash + 1)});
        }
        return items;
    }
    const bool midiIn = source == "midi-in-ports", midiOut = source == "midi-out-ports";
    if (midiIn || midiOut) {
        auto& s = AppSettings::instance();
        for (int p = 1; p <= kMidiPorts; ++p) {
            const auto key = juce::String(midiIn ? "midi.in." : "midi.out.") + juce::String(p);
            auto name = s.getString(key + ".name");
            if (name.isEmpty()) name = s.getString(key).isEmpty() ? "(none)" : "(not connected)";
            if (name.length() > 16) name = name.substring(0, 15).trim() + "...";
            items.push_back({p, "Port " + std::to_string(p) + " - "   // utf8-ok: data
                            + name.toStdString()});
        }
        return items;
    }
    const bool inPairs = source == "audio-in-pairs";
    if (inPairs || source == "audio-out-pairs") {
        auto* dev = devices_.getCurrentAudioDevice();
        const auto names = dev ? (inPairs ? dev->getInputChannelNames()
                                          : dev->getOutputChannelNames())
                               : juce::StringArray();
        for (int i = 0; i < names.size(); i += 2) {
            std::string label = std::to_string(i + 1) + "/" + std::to_string(i + 2)
                              + " - " + names[i].toStdString();   // utf8-ok: data
            if (i + 1 < names.size()) label += " / " + names[i + 1].toStdString();
            items.push_back({i + 1, label});
        }
        if (items.empty())
            for (int i = 1; i <= 7; i += 2)
                items.push_back({i, "Channels " + std::to_string(i) + "/" + std::to_string(i + 1)});
        return items;
    }
    const bool audioIn = source == "audio-in-channels";
    if (audioIn || source == "audio-out-channels") {
        auto* dev = devices_.getCurrentAudioDevice();
        const auto names = dev ? (audioIn ? dev->getInputChannelNames()
                                          : dev->getOutputChannelNames())
                               : juce::StringArray();
        for (int i = 0; i < names.size(); ++i)
            items.push_back({i + 1, std::to_string(i + 1) + " - "   // utf8-ok: data
                                + names[i].toStdString()});
        if (items.empty())
            for (int i = 1; i <= 8; ++i) items.push_back({i, "Channel " + std::to_string(i)});
    }
    return items;
}

}
