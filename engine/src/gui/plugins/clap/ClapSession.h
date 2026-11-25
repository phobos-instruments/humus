// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <clap/ext/audio-ports.h>
#include <clap/ext/gui.h>
#include <clap/ext/latency.h>
#include <clap/ext/note-ports.h>
#include <clap/ext/params.h>
#include <clap/ext/posix-fd-support.h>
#include <clap/ext/render.h>
#include <clap/ext/state.h>
#include <clap/ext/tail.h>
#include <clap/ext/timer-support.h>

#include "core/plugins/clap/ClapHost.h"
#include "core/plugins/clap/ClapModule.h"
#include "core/plugins/clap/ClapNotes.h"
#include "gui/plugins/clap/ClapTimers.h"

namespace hum::claphost {

class SessionOwner {
public:
    virtual ~SessionOwner() = default;
    virtual void restartRequested() = 0;
    virtual void paramsChanged(uint32_t flags) = 0;
    virtual void latencyChanged() = 0;
    virtual void flushRequested() = 0;
    virtual void portsChanged() = 0;
    virtual bool resizeRequested(uint32_t, uint32_t) { return false; }
};

struct Extensions {
    const clap_plugin_audio_ports_t* audioPorts = nullptr;
    const clap_plugin_note_ports_t* notePorts = nullptr;
    const clap_plugin_params_t* params = nullptr;
    const clap_plugin_state_t* state = nullptr;
    const clap_plugin_latency_t* latency = nullptr;
    const clap_plugin_tail_t* tail = nullptr;
    const clap_plugin_gui_t* gui = nullptr;
    const clap_plugin_render_t* render = nullptr;
    const clap_plugin_timer_support_t* timers = nullptr;
    const clap_plugin_posix_fd_support_t* fds = nullptr;
};

struct PortLayout {
    std::vector<int> ins, outs;
    bool noteIn = false, noteOut = false;
    NoteDialect dialect = NoteDialect::RawMidi;
    int inChannels() const;
    int outChannels() const;
};

uint32_t uidFor(const std::string& clapId);
const char* windowApi();

class Session : private HostCallbacks {
public:
    static std::unique_ptr<Session> open(const std::string& path, uint32_t uid, std::string& error);
    ~Session() override;

    const clap_plugin_t* plugin() const { return plugin_; }
    const Extensions& ext() const { return ext_; }
    const Descriptor& descriptor() const { return descriptor_; }
    const std::string& path() const { return module_->path(); }
    PortLayout ports() const;
    void setOwner(SessionOwner* owner) { owner_ = owner; }
    bool hasWindow() const;
    int timerCount() const { return timers_->count(); }
    void onMessageThread(std::function<void()> fn);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

private:
    Session(std::shared_ptr<Module> module, Descriptor descriptor);
    bool start(std::string& error);

    bool isMainThread() const override;
    void requestRestart() override;
    void requestCallback() override;
    void paramsRescan(uint32_t flags) override;
    void paramsRequestFlush() override;
    void latencyChanged() override;
    void portsRescan() override;
    bool guiRequestResize(uint32_t w, uint32_t h) override;
    bool registerTimer(uint32_t periodMs, clap_id& id) override;
    bool unregisterTimer(clap_id id) override;
    bool registerFd(int fd, uint32_t flags) override;
    bool modifyFd(int fd, uint32_t flags) override;
    bool unregisterFd(int fd) override;
    void log(int severity, const char* message) override;

    std::shared_ptr<Module> module_;
    Descriptor descriptor_;
    Host host_;
    const clap_plugin_t* plugin_ = nullptr;
    Extensions ext_;
    SessionOwner* owner_ = nullptr;
    std::shared_ptr<std::atomic<bool>> alive_;
    std::unique_ptr<Timers> timers_;
};

}
