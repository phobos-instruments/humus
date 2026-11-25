// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/clap/ClapSession.h"

#include <numeric>
#include <utility>

#include <juce_events/juce_events.h>

#include <HumBuildId.h>
#include <clap/ext/log.h>

namespace hum::claphost {

namespace {

HostIdentity identity() {
    HostIdentity id;
    id.version = HUM_VERSION;
    return id;
}

template <class T>
const T* extension(const clap_plugin_t* p, const char* id) {
    return static_cast<const T*>(p->get_extension(p, id));
}

}

uint32_t uidFor(const std::string& clapId) {
    uint32_t hash = 2166136261u;
    for (unsigned char c : clapId) {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}

const char* windowApi() {
#if JUCE_MAC
    return CLAP_WINDOW_API_COCOA;
#elif JUCE_WINDOWS
    return CLAP_WINDOW_API_WIN32;
#else
    return CLAP_WINDOW_API_X11;
#endif
}

int PortLayout::inChannels() const { return std::accumulate(ins.begin(), ins.end(), 0); }
int PortLayout::outChannels() const { return std::accumulate(outs.begin(), outs.end(), 0); }

std::unique_ptr<Session> Session::open(const std::string& path, uint32_t uid, std::string& error) {
    auto module = Module::open(path, error);
    if (module == nullptr) return nullptr;
    for (auto& d : module->descriptors()) {
        if (uidFor(d.id) != uid) continue;
        std::unique_ptr<Session> session(new Session(module, std::move(d)));
        if (!session->start(error)) return nullptr;
        return session;
    }
    error = "the file no longer holds this plug-in";
    return nullptr;
}

Session::Session(std::shared_ptr<Module> module, Descriptor descriptor)
    : module_(std::move(module)), descriptor_(std::move(descriptor)), host_(*this, identity()),
      alive_(std::make_shared<std::atomic<bool>>(true)) {
    timers_ = std::make_unique<Timers>(
        [this](clap_id id) {
            if (plugin_ != nullptr && ext_.timers != nullptr) ext_.timers->on_timer(plugin_, id);
        },
        [this](int fd, uint32_t flags) {
            if (plugin_ != nullptr && ext_.fds != nullptr) ext_.fds->on_fd(plugin_, fd, flags);
        });
}

bool Session::start(std::string& error) {
    const auto* made = module_->create(host_.clapHost(), descriptor_.id);
    if (made == nullptr) {
        error = "the plug-in could not be made";
        return false;
    }
    if (!made->init(made)) {
        made->destroy(made);
        error = "the plug-in did not start";
        return false;
    }
    plugin_ = made;
    ext_.audioPorts = extension<clap_plugin_audio_ports_t>(made, CLAP_EXT_AUDIO_PORTS);
    ext_.notePorts = extension<clap_plugin_note_ports_t>(made, CLAP_EXT_NOTE_PORTS);
    ext_.params = extension<clap_plugin_params_t>(made, CLAP_EXT_PARAMS);
    ext_.state = extension<clap_plugin_state_t>(made, CLAP_EXT_STATE);
    ext_.latency = extension<clap_plugin_latency_t>(made, CLAP_EXT_LATENCY);
    ext_.tail = extension<clap_plugin_tail_t>(made, CLAP_EXT_TAIL);
    ext_.gui = extension<clap_plugin_gui_t>(made, CLAP_EXT_GUI);
    ext_.render = extension<clap_plugin_render_t>(made, CLAP_EXT_RENDER);
    ext_.timers = extension<clap_plugin_timer_support_t>(made, CLAP_EXT_TIMER_SUPPORT);
    ext_.fds = extension<clap_plugin_posix_fd_support_t>(made, CLAP_EXT_POSIX_FD_SUPPORT);
    return true;
}

Session::~Session() {
    alive_->store(false);
    owner_ = nullptr;
    if (plugin_ != nullptr) plugin_->destroy(plugin_);
    plugin_ = nullptr;
    timers_.reset();
}

PortLayout Session::ports() const {
    PortLayout layout;
    if (ext_.audioPorts != nullptr)
        for (bool input : {true, false}) {
            const uint32_t n = ext_.audioPorts->count(plugin_, input);
            for (uint32_t i = 0; i < n; ++i) {
                clap_audio_port_info_t info{};
                if (!ext_.audioPorts->get(plugin_, i, input, &info) || info.channel_count == 0) continue;
                auto& list = input ? layout.ins : layout.outs;
                if ((info.flags & CLAP_AUDIO_PORT_IS_MAIN) != 0) list.insert(list.begin(), (int) info.channel_count);
                else list.push_back((int) info.channel_count);
            }
        }
    if (ext_.notePorts != nullptr) {
        clap_note_port_info_t info{};
        layout.noteIn = ext_.notePorts->count(plugin_, true) > 0 && ext_.notePorts->get(plugin_, 0, true, &info);
        if (layout.noteIn) layout.dialect = dialectFor(info.supported_dialects, info.preferred_dialect);
        layout.noteOut = ext_.notePorts->count(plugin_, false) > 0;
    }
    return layout;
}

bool Session::hasWindow() const {
    return ext_.gui != nullptr && ext_.gui->is_api_supported(plugin_, windowApi(), false);
}

void Session::onMessageThread(std::function<void()> fn) {
    if (isMainThread()) {
        fn();
        return;
    }
    juce::MessageManager::callAsync([alive = alive_, fn = std::move(fn)] {
        if (alive->load()) fn();
    });
}

bool Session::isMainThread() const { return juce::MessageManager::existsAndIsCurrentThread(); }

void Session::requestRestart() {
    juce::MessageManager::callAsync([this, alive = alive_] {
        if (alive->load() && owner_ != nullptr) owner_->restartRequested();
    });
}

void Session::requestCallback() {
    juce::MessageManager::callAsync([this, alive = alive_] {
        if (alive->load() && plugin_ != nullptr) plugin_->on_main_thread(plugin_);
    });
}

void Session::paramsRescan(uint32_t flags) {
    onMessageThread([this, flags] { if (owner_ != nullptr) owner_->paramsChanged(flags); });
}

void Session::paramsRequestFlush() {
    if (owner_ != nullptr) owner_->flushRequested();
}

void Session::latencyChanged() {
    onMessageThread([this] { if (owner_ != nullptr) owner_->latencyChanged(); });
}

void Session::portsRescan() {
    onMessageThread([this] { if (owner_ != nullptr) owner_->portsChanged(); });
}

bool Session::guiRequestResize(uint32_t w, uint32_t h) {
    return isMainThread() && owner_ != nullptr && owner_->resizeRequested(w, h);
}

bool Session::registerTimer(uint32_t periodMs, clap_id& id) { return isMainThread() && timers_->add(periodMs, id); }
bool Session::unregisterTimer(clap_id id) { return isMainThread() && timers_->remove(id); }
bool Session::registerFd(int fd, uint32_t flags) { return isMainThread() && timers_->watchFd(fd, flags); }
bool Session::modifyFd(int fd, uint32_t flags) { return isMainThread() && timers_->watchFd(fd, flags); }
bool Session::unregisterFd(int fd) { return isMainThread() && timers_->unwatchFd(fd); }

void Session::log(int severity, const char* message) {
    if (severity >= CLAP_LOG_WARNING && message != nullptr)
        juce::Logger::writeToLog("CLAP " + juce::String(descriptor_.name) + ": " + juce::String::fromUTF8(message));
}

}
