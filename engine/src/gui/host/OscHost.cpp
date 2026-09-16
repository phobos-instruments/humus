// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/OscHost.h"
#include "hum/Organism.h"

#include "gui/app/AppSettings.h"
#include "gui/editor/ControlDefaults.h"
#include "hum/caps/Osc.h"

namespace hum {

OscHost::OscHost(BrickHost& host, HostCore& core) : host_(host), doc_(core) {
    receiver_.addListener(this);
    serial_.onMessage = [this, alive = alive_](const osc::Message& m) {
        juce::MessageManager::callAsync([this, alive, m] {
            if (*alive) takeSerialMessage(m);
        });
    };
}

OscHost::~OscHost() {
    *alive_ = false;
    receiver_.removeListener(this);
}

void OscHost::applySerialSettings() {
    auto& s = AppSettings::instance();
    serial_.configure(s.getInt("osc.serial.enabled", 0) != 0,
                      s.getString("osc.serial.port").toStdString(),
                      s.getInt("osc.serial.baud", 115200));
}

void OscHost::takeSerialMessage(const osc::Message& m) {
    try {
        juce::OSCMessage msg{juce::OSCAddressPattern(juce::String(m.address))};
        for (const auto& a : m.args) {
            if (a.tag == 'f') msg.addFloat32(a.f);
            else if (a.tag == 'i') msg.addInt32(a.i);
            else if (a.tag == 's') msg.addString(juce::String(a.s));
        }
        oscMessageReceived(msg);
    } catch (const juce::OSCFormatError&) {
    }
}

int OscHost::port() const {
    const int p = AppSettings::instance().getInt("osc.port", 9000);
    return p > 0 && p < 65536 ? p : 9000;
}

bool OscHost::setEnabled(bool on) {
    receiver_.disconnect();
    enabled_ = false;
    if (!on) return false;
    enabled_ = receiver_.connect(port());
    return enabled_;
}

void OscHost::oscMessageReceived(const juce::OSCMessage& m) {
    {
        juce::String args;
        for (const auto& arg : m) {
            if (args.isNotEmpty()) args << " ";
            if (arg.isFloat32())     args << juce::String(arg.getFloat32(), 3);
            else if (arg.isInt32())  args << juce::String(arg.getInt32());
            else if (arg.isString()) args << arg.getString();
            else                     args << "?";
        }
        logToMonitors(false, m.getAddressPattern().toString(), args);
    }

    double v = 1.0;
    bool numeric = m.isEmpty();
    for (const auto& arg : m) {
        if (arg.isFloat32()) { v = arg.getFloat32(); numeric = true; break; }
        if (arg.isInt32()) { v = arg.getInt32(); numeric = true; break; }
    }
    if (!numeric) return;
    inject(m.getAddressPattern().toString().toStdString(), v);
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
        host_.setParam(u.organism, u.param, u.value);
}

void OscHost::setShape(const std::string& address, const std::string& organism,
                       const std::string& param, const ControlShape& shape) {
    map_.setShape(address, organism, param, shape);
    doc_.markDirty();
}

juce::String OscHost::mapAddress(const std::string& address, const std::string& organism,
                                 const std::string& param, double min, double max, bool steal) {
    if (address.empty()) return {};
    juce::String stolenText;
    if (steal) {
        const auto stolen = map_.steal(address, organism, param);
        for (const auto& [sc, sp] : stolen) {
            if (stolenText.isNotEmpty()) stolenText << ", ";
            stolenText << juce::String(sc) << "/" << juce::String(sp);
        }
    }
    map_.set(address, organism, param, min, max);
    if (const auto* sh = map_.shapeOf(address, organism, param)) {
        if (sh->isDefault() && isHostSwitchTarget(param)) {
            ControlShape d;
            d.isSwitch = true;
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
            ControlShape sh;
            sh.smoothing = s.smoothing;
            sh.curve = s.curve;
            sh.isSwitch = s.isSwitch;
            sh.inverted = s.inverted;
            sh.toggle = s.toggle;
            sh.threshold = s.threshold;
            sh.isSwitch = sh.isSwitch || isHostSwitchTarget(s.propertyName);
            if (!sh.isSwitch) sh.logScale = paramIsLog(host_, c.name, s.propertyName);
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
        s.smoothing = e.shape.smoothing;
        s.curve = e.shape.curve;
        s.isSwitch = e.shape.isSwitch;
        s.inverted = e.shape.inverted;
        s.toggle = e.shape.toggle;
        s.threshold = e.shape.threshold;
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
