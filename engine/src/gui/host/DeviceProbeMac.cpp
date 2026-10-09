// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/DeviceProbe.h"

#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

namespace hum {

namespace {

constexpr AudioObjectPropertyElement kMainElement = 0;
constexpr auto kPollStep = std::chrono::milliseconds(5);

AudioObjectPropertyAddress everywhere(AudioObjectPropertySelector selector) {
    return {selector, kAudioObjectPropertyScopeWildcard, kMainElement};
}

std::string nameOf(AudioObjectID id) {
    const auto where = everywhere(kAudioDevicePropertyDeviceNameCFString);
    CFStringRef name = nullptr;
    UInt32 size = sizeof(name);
    if (AudioObjectGetPropertyData(id, &where, 0, nullptr, &size, &name) != noErr || name == nullptr) return {};
    std::string out;
    char text[512];
    if (CFStringGetCString(name, text, sizeof(text), kCFStringEncodingUTF8)) out = text;
    CFRelease(name);
    return out;
}

AudioObjectID deviceNamed(const std::string& device) {
    const auto where = everywhere(kAudioHardwarePropertyDevices);
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &where, 0, nullptr, &size) != noErr)
        return kAudioObjectUnknown;
    std::vector<AudioObjectID> ids(size / sizeof(AudioObjectID));
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &where, 0, nullptr, &size, ids.data()) != noErr)
        return kAudioObjectUnknown;
    for (const auto id : ids)
        if (nameOf(id) == device) return id;
    return kAudioObjectUnknown;
}

OSStatus heardAudio(AudioObjectID, const AudioTimeStamp*, const AudioBufferList*, const AudioTimeStamp*,
                    AudioBufferList* out, const AudioTimeStamp*, void* heard) {
    if (out != nullptr)
        for (UInt32 i = 0; i < out->mNumberBuffers; ++i)
            if (out->mBuffers[i].mData != nullptr)
                std::memset(out->mBuffers[i].mData, 0, out->mBuffers[i].mDataByteSize);
    static_cast<std::atomic<bool>*>(heard)->store(true);
    return noErr;
}

}

DeviceAnswer probeDeviceStart(const std::string& device) {
    const auto id = deviceNamed(device);
    if (id == kAudioObjectUnknown) return DeviceAnswer::Unknown;
    std::atomic<bool> heard{false};
    AudioDeviceIOProcID proc = nullptr;
    if (AudioDeviceCreateIOProcID(id, heardAudio, &heard, &proc) != noErr || proc == nullptr)
        return DeviceAnswer::Unknown;
    const bool started = AudioDeviceStart(id, proc) == noErr;
    const auto until = std::chrono::steady_clock::now() + kDeviceFirstAudioWait;
    while (started && !heard.load() && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(kPollStep);
    if (started) AudioDeviceStop(id, proc);
    AudioDeviceDestroyIOProcID(id, proc);
    return heard.load() ? DeviceAnswer::Answered : DeviceAnswer::Silent;
}

}
