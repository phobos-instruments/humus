// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"

namespace hum {

std::vector<PortSpot> PatcherCanvas::portSpots(const std::string& name) {
    std::vector<PortSpot> spots;
    auto add = [&](PortKind kind, bool outlet, int index, std::string label, juce::Point<int> at) {
        spots.push_back({kind, outlet, index, std::move(label), at});
    };
    int ins, outs;
    portCounts(name, ins, outs);
    for (int i = 0; i < ins; ++i) add(PortKind::Audio, false, i, portName(name, i, false), inletPos(name, i, ins));
    for (int o = 0; o < outs; ++o) add(PortKind::Audio, true, o, portName(name, o, true), outletPos(name, o, outs));
    int mIns, mOuts;
    midiPortCounts(name, mIns, mOuts);
    for (int i = 0; i < mIns; ++i) add(PortKind::Midi, false, i, {}, midiInletPos(name, i));
    for (int o = 0; o < mOuts; ++o) add(PortKind::Midi, true, o, {}, midiOutletPos(name, o));
    int vIns, vOuts;
    videoPortCounts(name, vIns, vOuts);
    for (int i = 0; i < vIns; ++i) add(PortKind::Video, false, i, {}, videoInletPos(name, i));
    for (int o = 0; o < vOuts; ++o) add(PortKind::Video, true, o, {}, videoOutletPos(name, o));
    int cIns, cOuts;
    controlPortCounts(name, cIns, cOuts);
    for (int i = 0; i < cIns; ++i)
        add(PortKind::Control, false, i, host_.controlInletParam(name, i), controlInletPos(name, i));
    for (int o = 0; o < cOuts; ++o)
        add(PortKind::Control, true, o, host_.controlOutletValue(name, o), controlOutletPos(name, o));
    return spots;
}

}
