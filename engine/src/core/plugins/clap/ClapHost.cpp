// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/clap/ClapHost.h"

#include <cstring>
#include <utility>

#include <clap/ext/audio-ports.h>
#include <clap/ext/gui.h>
#include <clap/ext/latency.h>
#include <clap/ext/log.h>
#include <clap/ext/note-ports.h>
#include <clap/ext/params.h>
#include <clap/ext/posix-fd-support.h>
#include <clap/ext/state.h>
#include <clap/ext/tail.h>
#include <clap/ext/thread-check.h>
#include <clap/ext/timer-support.h>
#include <clap/version.h>

namespace hum::claphost {

namespace {

thread_local bool tAudioThread = false;

HostCallbacks& cb(const clap_host_t* h) { return static_cast<Host*>(h->host_data)->callbacks(); }

const clap_host_thread_check_t kThreadCheck = {
    [](const clap_host_t* h) { return cb(h).isMainThread(); },
    [](const clap_host_t*) { return Host::isAudioThread(); }};

const clap_host_log_t kLog = {
    [](const clap_host_t* h, clap_log_severity severity, const char* msg) { cb(h).log(severity, msg); }};

const clap_host_params_t kParams = {
    [](const clap_host_t* h, clap_param_rescan_flags flags) { cb(h).paramsRescan(flags); },
    [](const clap_host_t* h, clap_id id, clap_param_clear_flags flags) { cb(h).paramsClear(id, flags); },
    [](const clap_host_t* h) { cb(h).paramsRequestFlush(); }};

const clap_host_latency_t kLatency = {[](const clap_host_t* h) { cb(h).latencyChanged(); }};
const clap_host_tail_t kTail = {[](const clap_host_t* h) { cb(h).tailChanged(); }};
const clap_host_state_t kState = {[](const clap_host_t* h) { cb(h).stateMarkDirty(); }};

const clap_host_audio_ports_t kAudioPorts = {
    [](const clap_host_t*, uint32_t) { return true; },
    [](const clap_host_t* h, uint32_t) { cb(h).portsRescan(); }};

const clap_host_note_ports_t kNotePorts = {
    [](const clap_host_t*) -> uint32_t { return CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI; },
    [](const clap_host_t* h, uint32_t) { cb(h).portsRescan(); }};

const clap_host_gui_t kGui = {
    [](const clap_host_t* h) { cb(h).guiResizeHintsChanged(); },
    [](const clap_host_t* h, uint32_t w, uint32_t hh) { return cb(h).guiRequestResize(w, hh); },
    [](const clap_host_t* h) { return cb(h).guiRequestShow(); },
    [](const clap_host_t* h) { return cb(h).guiRequestHide(); },
    [](const clap_host_t* h, bool destroyed) { cb(h).guiClosed(destroyed); }};

const clap_host_timer_support_t kTimers = {
    [](const clap_host_t* h, uint32_t period, clap_id* id) { return id != nullptr && cb(h).registerTimer(period, *id); },
    [](const clap_host_t* h, clap_id id) { return cb(h).unregisterTimer(id); }};

#if !defined(_WIN32) && !defined(__APPLE__)
const clap_host_posix_fd_support_t kFds = {
    [](const clap_host_t* h, int fd, clap_posix_fd_flags_t flags) { return cb(h).registerFd(fd, flags); },
    [](const clap_host_t* h, int fd, clap_posix_fd_flags_t flags) { return cb(h).modifyFd(fd, flags); },
    [](const clap_host_t* h, int fd) { return cb(h).unregisterFd(fd); }};
#endif

const void* hostExtension(const clap_host_t*, const char* id) {
    if (id == nullptr) return nullptr;
    if (std::strcmp(id, CLAP_EXT_THREAD_CHECK) == 0) return &kThreadCheck;
    if (std::strcmp(id, CLAP_EXT_LOG) == 0) return &kLog;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &kParams;
    if (std::strcmp(id, CLAP_EXT_LATENCY) == 0) return &kLatency;
    if (std::strcmp(id, CLAP_EXT_TAIL) == 0) return &kTail;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0) return &kState;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &kAudioPorts;
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &kNotePorts;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0) return &kGui;
    if (std::strcmp(id, CLAP_EXT_TIMER_SUPPORT) == 0) return &kTimers;
#if !defined(_WIN32) && !defined(__APPLE__)
    if (std::strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT) == 0) return &kFds;
#endif
    return nullptr;
}

}

Host::Host(HostCallbacks& callbacks, HostIdentity identity)
    : callbacks_(callbacks), identity_(std::move(identity)) {
    host_.clap_version = CLAP_VERSION;
    host_.host_data = this;
    host_.name = identity_.name.c_str();
    host_.vendor = identity_.vendor.c_str();
    host_.url = identity_.url.c_str();
    host_.version = identity_.version.c_str();
    host_.get_extension = hostExtension;
    host_.request_restart = [](const clap_host_t* h) { cb(h).requestRestart(); };
    host_.request_process = [](const clap_host_t* h) { cb(h).requestProcess(); };
    host_.request_callback = [](const clap_host_t* h) { cb(h).requestCallback(); };
}

bool Host::isAudioThread() { return tAudioThread; }

Host::AudioThread::AudioThread() : was_(tAudioThread) { tAudioThread = true; }
Host::AudioThread::~AudioThread() { tAudioThread = was_; }

}
