// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/clap/ClapModule.h"

#include <algorithm>

#include <clap/plugin-features.h>
#include <clap/version.h>

namespace hum::claphost {

namespace {
std::string text(const char* s) { return s != nullptr ? std::string(s) : std::string(); }
}

bool Descriptor::has(const std::string& feature) const {
    return std::find(features.begin(), features.end(), feature) != features.end();
}

bool Descriptor::isInstrument() const { return has(CLAP_PLUGIN_FEATURE_INSTRUMENT); }

Modules& Modules::instance() {
    static Modules modules;
    return modules;
}

std::shared_ptr<Module> Modules::open(const std::string& path, std::string& error) {
    const std::lock_guard<std::mutex> hold(lock_);
    if (auto live = open_[path].lock()) return live;
    const clap_plugin_entry_t* entry = loadEntry(path, error);
    if (entry == nullptr) return nullptr;
    if (!clap_version_is_compatible(entry->clap_version)) {
        error = "made for a newer CLAP than this host reads";
        return nullptr;
    }
    if (entry->init == nullptr || entry->get_factory == nullptr || !entry->init(path.c_str())) {
        error = "the plug-in did not start";
        return nullptr;
    }
    const auto* factory = static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (factory == nullptr || factory->get_plugin_count == nullptr || factory->get_plugin_descriptor == nullptr
        || factory->create_plugin == nullptr) {
        if (entry->deinit != nullptr) entry->deinit();
        error = "the file holds no CLAP plug-ins";
        return nullptr;
    }
    std::shared_ptr<Module> made(new Module(path, entry, factory), [this](Module* m) {
        const std::lock_guard<std::mutex> closing(lock_);
        delete m;
    });
    open_[path] = made;
    return made;
}

std::shared_ptr<Module> Module::open(const std::string& path, std::string& error) {
    return Modules::instance().open(path, error);
}

Module::Module(std::string path, const clap_plugin_entry_t* entry, const clap_plugin_factory_t* factory)
    : path_(std::move(path)), entry_(entry), factory_(factory) {}

Module::~Module() {
    if (entry_->deinit != nullptr) entry_->deinit();
}

std::vector<Descriptor> Module::descriptors() const {
    std::vector<Descriptor> out;
    const uint32_t n = factory_->get_plugin_count(factory_);
    for (uint32_t i = 0; i < n; ++i) {
        const auto* d = factory_->get_plugin_descriptor(factory_, i);
        if (d == nullptr || d->id == nullptr || d->name == nullptr || !clap_version_is_compatible(d->clap_version))
            continue;
        Descriptor desc{text(d->id), text(d->name), text(d->vendor), text(d->version),
                        text(d->url), text(d->description), {}};
        for (auto* f = d->features; f != nullptr && *f != nullptr; ++f) desc.features.emplace_back(*f);
        out.push_back(std::move(desc));
    }
    return out;
}

const clap_plugin_t* Module::create(const clap_host_t* host, const std::string& id) const {
    return factory_->create_plugin(factory_, host, id.c_str());
}

}
