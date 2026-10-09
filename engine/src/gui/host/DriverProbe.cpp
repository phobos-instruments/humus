// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/DriverProbe.h"

#include <cstddef>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>

namespace hum::driverprobe {

namespace {

constexpr const char* kWorks = "ok";
constexpr double kPreferredRate = 48000.0;

const char* nameOf(Verdict v) {
    return v == Verdict::Works ? "works" : v == Verdict::Unsafe ? "unsafe" : "refused";
}

bool remembered(Verdict v) { return v == Verdict::Works || v == Verdict::Unsafe; }

Verdict inChild(const juce::File& exe, const juce::String& type, const juce::String& driver,
                int timeoutMs) {
    const auto reply = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("hum-driver-" + juce::Uuid().toString() + ".txt");
    juce::ChildProcess child;
    const bool started = child.start(
        juce::StringArray{exe.getFullPathName(), kFlag, reply.getFullPathName(), type, driver}, 0);
    const bool finished = started && child.waitForProcessToFinish(timeoutMs);
    if (started && !finished) child.kill();
    const auto text = reply.existsAsFile() ? reply.loadFileAsString().trim() : juce::String();
    reply.deleteFile();
    return classify(started, finished, text);
}

double rateFor(juce::AudioIODevice& device) {
    const auto rates = device.getAvailableSampleRates();
    if (rates.contains(kPreferredRate) || rates.isEmpty()) return kPreferredRate;
    return rates[0];
}

}

Verdict classify(bool started, bool finished, const juce::String& reply) {
    if (!started) return Verdict::Unknown;
    if (!finished || reply.isEmpty()) return Verdict::Unsafe;
    return reply == kWorks ? Verdict::Works : Verdict::Refused;
}

std::optional<Verdict> VerdictStore::lookup(const juce::String& type, const juce::String& driver,
                                            const juce::String& key) const {
    const auto xml = juce::XmlDocument::parse(file_);
    if (xml == nullptr) return std::nullopt;
    for (auto* e : xml->getChildWithTagNameIterator("driver"))
        if (e->getStringAttribute("type") == type && e->getStringAttribute("name") == driver
            && e->getStringAttribute("key") == key)
            return e->getStringAttribute("verdict") == "unsafe" ? Verdict::Unsafe : Verdict::Works;
    return std::nullopt;
}

void VerdictStore::store(const juce::String& type, const juce::String& driver,
                         const juce::String& key, Verdict verdict) {
    auto xml = juce::XmlDocument::parse(file_);
    if (xml == nullptr || !xml->hasTagName("audio-driver-probe"))
        xml = std::make_unique<juce::XmlElement>("audio-driver-probe");
    for (auto* e = xml->getFirstChildElement(); e != nullptr;) {
        auto* next = e->getNextElement();
        if (e->getStringAttribute("type") == type && e->getStringAttribute("name") == driver)
            xml->removeChildElement(e, true);
        e = next;
    }
    if (remembered(verdict)) {
        auto* e = xml->createNewChildElement("driver");
        e->setAttribute("type", type);
        e->setAttribute("name", driver);
        e->setAttribute("key", key);
        e->setAttribute("verdict", nameOf(verdict));
    }
    file_.getParentDirectory().createDirectory();
    xml->writeTo(file_);
}

juce::String driverKey(const juce::String& type, const juce::String& driver) {
#if JUCE_WINDOWS
    if (type == "ASIO") {
        const auto clsid = juce::WindowsRegistry::getValue(
            "HKEY_LOCAL_MACHINE\\SOFTWARE\\ASIO\\" + driver + "\\CLSID");
        const auto dll = juce::WindowsRegistry::getValue(
            "HKEY_CLASSES_ROOT\\CLSID\\" + clsid + "\\InprocServer32\\");
        const juce::File file(dll.unquoted());
        return clsid + ":" + juce::String(file.getLastModificationTime().toMilliseconds());
    }
#else
    juce::ignoreUnused(type, driver);
#endif
    return {};
}

juce::String openAndClose(const juce::String& type, const juce::String& driver) {
    juce::AudioDeviceManager manager;
    for (auto* kind : manager.getAvailableDeviceTypes()) {
        if (kind->getTypeName() != type) continue;
        kind->scanForDevices();
        std::unique_ptr<juce::AudioIODevice> device(kind->createDevice(driver, driver));
        if (device == nullptr) return "the driver could not be created";
        juce::BigInteger ins, outs;
        ins.setRange(0, device->getInputChannelNames().size(), true);
        outs.setRange(0, device->getOutputChannelNames().size(), true);
        const auto error = device->open(ins, outs, rateFor(*device), device->getDefaultBufferSize());
        device->close();
        return error.isEmpty() ? juce::String(kWorks) : error;
    }
    return "no " + type + " drivers here";
}

GuardedDeviceType::Probe childProbe(juce::File executable, juce::File storeFile, int timeoutMs) {
    return [exe = std::move(executable), file = std::move(storeFile), timeoutMs](
               const juce::String& type, const juce::StringArray& drivers) {
        VerdictStore store(file);
        std::vector<Verdict> verdicts(static_cast<std::size_t>(drivers.size()), Verdict::Unknown);
        std::vector<juce::String> keys;
        std::vector<bool> fresh(static_cast<std::size_t>(drivers.size()), false);
        std::vector<std::thread> probes;
        for (int i = 0; i < drivers.size(); ++i) {
            keys.push_back(driverKey(type, drivers[i]));
            if (const auto known = store.lookup(type, drivers[i], keys.back())) {
                verdicts[(std::size_t) i] = *known;
                continue;
            }
            fresh[(std::size_t) i] = true;
            probes.emplace_back([&verdicts, &exe, &type, &drivers, timeoutMs, i] {
                verdicts[(std::size_t) i] = inChild(exe, type, drivers[i], timeoutMs);
            });
        }
        for (auto& probe : probes) probe.join();
        GuardedDeviceType::Verdicts out;
        for (int i = 0; i < drivers.size(); ++i) {
            const auto v = verdicts[(std::size_t) i];
            if (fresh[(std::size_t) i]) store.store(type, drivers[i], keys[(std::size_t) i], v);
            if (v == Verdict::Unsafe) {
                out.unsafe.add(drivers[i]);
                juce::Logger::writeToLog("audio: the " + type + " driver \"" + drivers[i]
                                         + "\" crashed or hung when tried apart, so it is hidden");
            } else if (v == Verdict::Refused) {
                out.refused.add(drivers[i]);
            }
        }
        return out;
    };
}

}
