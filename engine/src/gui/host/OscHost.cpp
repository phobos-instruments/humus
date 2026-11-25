// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/OscHost.h"
#include "hum/Organism.h"

#include <cstdint>

#include "gui/app/AppSettings.h"
#include "gui/common/Localisation.h"
#include "gui/host/ControlApply.h"
#include "gui/editor/ControlDefaults.h"
#include "hum/caps/Osc.h"

namespace hum {

OscHost::OscHost(BrickHost& host, HostCore& core, HostScheduler& scheduler)
    : host_(host), doc_(core), scheduler_(scheduler) {
    serial_.onMessage = [this, alive = alive_](const osc::Message& m) {
        scheduler_.post([this, alive, m] {
            if (*alive) takeMessage(m);
        });
    };
    udp_.onPacket = [this, alive = alive_](OscUdpListener::Packet packet, std::string from) {
        scheduler_.post([this, alive, packet = std::move(packet), from = std::move(from)] {
            if (*alive) takePacket(packet.data(), packet.size(), from);
        });
    };
}

OscHost::~OscHost() {
    *alive_ = false;
    udp_.close();
}

void OscHost::applySerialSettings() {
    auto& s = AppSettings::instance();
    serial_.configure(s.getInt("osc.serial.enabled", 0) != 0,
                      s.getString("osc.serial.port").toStdString(),
                      s.getInt("osc.serial.baud", 115200));
}

void OscHost::takeMessage(const osc::Message& m) {
    juce::String args;
    for (const auto& a : m.args) {
        if (args.isNotEmpty()) args << " ";
        if (a.tag == 'f')      args << juce::String(a.f, 3);
        else if (a.tag == 'i') args << juce::String(a.i);
        else                   args << juce::String(a.s);
    }
    logToMonitors(false, juce::String(m.address), args);

    double v = 1.0;
    bool numeric = m.args.empty();
    for (const auto& a : m.args) {
        if (a.tag == 'f') { v = a.f; numeric = true; break; }
        if (a.tag == 'i') { v = a.i; numeric = true; break; }
    }
    if (numeric) inject(m.address, v);
}

void OscHost::takePacket(const std::uint8_t* data, size_t size, const std::string& from) {
    bool any = false;
    osc::decodePacket(data, size, [this, &any](const osc::Message& m) {
        any = true;
        takeMessage(m);
    });
    if (!any)
        logToMonitors(false, tr("osc-monitor.not-osc", "(not OSC)"),
                      trn("osc-monitor.bytes-from", "{n} byte from {sender}", "{n} bytes from {sender}",
                          (long long) size, {{"sender", juce::String(from)}}));
}

int OscHost::port() const {
    const int p = AppSettings::instance().getInt("osc.port", 9000);
    return p > 0 && p < 65536 ? p : 9000;
}

bool OscHost::setEnabled(bool on) {
    udp_.close();
    enabled_ = false;
    if (!on || !host_.environment().claimsMachineResources) return false;
    enabled_ = udp_.open(port());
    return enabled_;
}

void OscHost::logToMonitors(bool out, const juce::String& address, const juce::String& args) {
    for (auto& cm : host_.model().organisms)
        if (auto* mon = dynamic_cast<OscLogSource*>(host_.liveOrganism(cm.name)))
            mon->pushOsc(out, address.toRawUTF8(), args.toRawUTF8());
}

void OscHost::inject(const std::string& address, double value01) {
    lastAddress_ = juce::String(address);
    LiveControlHold live(doc_);
    for (const auto& u : map_.deliver(address, value01))
        applyControl(host_, u);
}

void OscHost::setShape(const std::string& address, const std::string& organism,
                       const std::string& param, const ControlShape& shape) {
    map_.setShape(address, organism, param, shape);
    doc_.markDirty();
}

std::string OscHost::mapAddress(const std::string& address, const std::string& organism,
                                const std::string& param, double min, double max, bool steal) {
    if (address.empty()) return {};
    std::string stolenText;
    if (steal)
        for (const auto& [sc, sp] : map_.steal(address, organism, param))
            stolenText += (stolenText.empty() ? "" : ", ") + sc + "/" + sp;
    map_.set(address, organism, param, min, max);
    if (const auto* sh = map_.shapeOf(address, organism, param)) {
        if (sh->isDefault() && paramIsSwitch(host_, organism, param)) {
            ControlShape d;
            d.type = ControlType::Button;
            map_.setShape(address, organism, param, d);
        } else if (sh->isDefault() && paramIsLog(host_, organism, param)) {
            ControlShape d;
            d.logScale = true;
            map_.setShape(address, organism, param, d);
        }
    }
    doc_.markDirty();
    doc_.pokeLiveRefresh();
    return stolenText;
}

void OscHost::clearAddress(const std::string& address, const std::string& organism,
                           const std::string& param) {
    map_.clear(address, organism, param);
    doc_.markDirty();
    doc_.pokeLiveRefresh();
}

void OscHost::clearForOrganism(const std::string& organism) {
    map_.clearOrganism(organism);
}

void OscHost::renameOrganism(const std::string& oldName, const std::string& newName) {
    map_.renameOrganism(oldName, newName);
}

void OscHost::syncMapFromModel() {
    map_.clearAll();
    for (auto& c : doc_.document().organisms)
        for (auto& s : c.oscSources) {
            map_.set(s.address, c.name, s.propertyName, s.mapMin, s.mapMax);
            ControlShape sh = s.shape;
            if (isHostSwitchTarget(s.propertyName)) sh.type = ControlType::Button;
            sh.logScale = sh.isFader() && paramIsLog(host_, c.name, s.propertyName);
            if (!sh.isDefault()) map_.setShape(s.address, c.name, s.propertyName, sh);
        }
}

void OscHost::syncMapToModel() {
    for (auto& c : doc_.document().organisms) c.oscSources.clear();
    for (const auto& e : map_.entries()) {
        auto* cm = doc_.mutableByName(e.organism);
        if (!cm) continue;
        OscControllerSource s;
        s.propertyName = e.param;
        s.propertyIndex = -1;
        for (auto& pr : cm->properties)
            if (pr.name == e.param) { s.propertyIndex = pr.index; break; }
        s.address = e.address;
        s.mapMin = e.min;
        s.mapMax = e.max;
        s.shape = e.shape;
        s.shape.logScale = false;
        cm->oscSources.push_back(std::move(s));
    }
}

bool OscHost::sendValue(const juce::String& address, float value) {
    auto& s = AppSettings::instance();
    const auto host = s.getString("osc.sendHost").isEmpty() ? juce::String("127.0.0.1")
                                                            : s.getString("osc.sendHost");
    const int port = s.getInt("osc.sendPort", 9001);
    if (!senderOk_ || host != sendHost_ || port != sendPort_) {
        sender_.disconnect();
        senderOk_ = sender_.connect(host, port);
        sendHost_ = host;
        sendPort_ = port;
    }
    const bool sentUdp =
        senderOk_ && sender_.send(juce::OSCMessage(juce::OSCAddressPattern(address), value));
    const bool sentSerial = serial_.enabled() && serial_.send(address.toStdString(), value);
    if (sentUdp || sentSerial) logToMonitors(true, address, juce::String(value, 3));
    return sentUdp || sentSerial;
}

}
