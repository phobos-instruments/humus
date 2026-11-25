// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>

#include <clap/host.h>
#include <clap/id.h>

namespace hum::claphost {

class HostCallbacks {
public:
    virtual ~HostCallbacks() = default;

    virtual bool isMainThread() const = 0;
    virtual void requestRestart() = 0;
    virtual void requestCallback() = 0;
    virtual void requestProcess() {}
    virtual void paramsRescan(uint32_t flags) = 0;
    virtual void paramsClear(clap_id, uint32_t) {}
    virtual void paramsRequestFlush() = 0;
    virtual void latencyChanged() = 0;
    virtual void tailChanged() {}
    virtual void stateMarkDirty() {}
    virtual void portsRescan() {}
    virtual void guiResizeHintsChanged() {}
    virtual bool guiRequestResize(uint32_t, uint32_t) { return false; }
    virtual bool guiRequestShow() { return false; }
    virtual bool guiRequestHide() { return false; }
    virtual void guiClosed(bool) {}
    virtual bool registerTimer(uint32_t, clap_id&) { return false; }
    virtual bool unregisterTimer(clap_id) { return false; }
    virtual bool registerFd(int, uint32_t) { return false; }
    virtual bool modifyFd(int, uint32_t) { return false; }
    virtual bool unregisterFd(int) { return false; }
    virtual void log(int, const char*) {}
};

struct HostIdentity {
    std::string name = "Humus";
    std::string vendor = "Phobos Instruments";
    std::string url = "https://humus.art";
    std::string version;
};

class Host {
public:
    Host(HostCallbacks& callbacks, HostIdentity identity);
    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

    const clap_host_t* clapHost() const { return &host_; }
    HostCallbacks& callbacks() const { return callbacks_; }

    static bool isAudioThread();

    class AudioThread {
    public:
        AudioThread();
        ~AudioThread();
        AudioThread(const AudioThread&) = delete;
        AudioThread& operator=(const AudioThread&) = delete;

    private:
        bool was_;
    };

private:
    HostCallbacks& callbacks_;
    HostIdentity identity_;
    clap_host_t host_{};
};

}
