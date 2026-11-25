// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <clap/entry.h>
#include <clap/factory/plugin-factory.h>
#include <clap/host.h>
#include <clap/plugin.h>

namespace hum::claphost {

struct Descriptor {
    std::string id, name, vendor, version, url, description;
    std::vector<std::string> features;
    bool has(const std::string& feature) const;
    bool isInstrument() const;
};

const clap_plugin_entry_t* loadEntry(const std::string& path, std::string& error);

class Module {
public:
    static std::shared_ptr<Module> open(const std::string& path, std::string& error);
    ~Module();

    const std::string& path() const { return path_; }
    std::vector<Descriptor> descriptors() const;
    const clap_plugin_t* create(const clap_host_t* host, const std::string& id) const;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

private:
    friend class Modules;
    Module(std::string path, const clap_plugin_entry_t* entry, const clap_plugin_factory_t* factory);

    std::string path_;
    const clap_plugin_entry_t* entry_;
    const clap_plugin_factory_t* factory_;
};

class Modules {
public:
    static Modules& instance();
    std::shared_ptr<Module> open(const std::string& path, std::string& error);

private:
    std::mutex lock_;
    std::map<std::string, std::weak_ptr<Module>> open_;
};

}
