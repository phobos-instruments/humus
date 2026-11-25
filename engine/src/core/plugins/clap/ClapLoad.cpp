// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/clap/ClapModule.h"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "hum/FileBytes.h"
#else
#include <dlfcn.h>
#endif

namespace hum::claphost {

#if defined(__APPLE__)

const clap_plugin_entry_t* loadEntry(const std::string& path, std::string& error) {
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path.data()), (CFIndex) path.size(), true);
    if (url == nullptr) {
        error = "the path could not be read";
        return nullptr;
    }
    CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFRelease(url);
    if (bundle == nullptr) {
        error = "not a plug-in bundle";
        return nullptr;
    }
    if (!CFBundleLoadExecutableAndReturnError(bundle, nullptr)) {
        CFRelease(bundle);
        error = "the plug-in's code would not load";
        return nullptr;
    }
    auto* entry = static_cast<const clap_plugin_entry_t*>(CFBundleGetDataPointerForName(bundle, CFSTR("clap_entry")));
    if (entry == nullptr) {
        CFRelease(bundle);
        error = "no CLAP entry point";
    }
    return entry;
}

#elif defined(_WIN32)

const clap_plugin_entry_t* loadEntry(const std::string& path, std::string& error) {
    HMODULE lib = LoadLibraryW(utf8Path(path).c_str());
    if (lib == nullptr) {
        error = "the plug-in's code would not load";
        return nullptr;
    }
    auto* entry = reinterpret_cast<const clap_plugin_entry_t*>(GetProcAddress(lib, "clap_entry"));
    if (entry == nullptr) {
        FreeLibrary(lib);
        error = "no CLAP entry point";
    }
    return entry;
}

#else

const clap_plugin_entry_t* loadEntry(const std::string& path, std::string& error) {
    void* lib = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (lib == nullptr) {
        const char* why = dlerror();
        error = why != nullptr ? why : "the plug-in's code would not load";
        return nullptr;
    }
    auto* entry = static_cast<const clap_plugin_entry_t*>(dlsym(lib, "clap_entry"));
    if (entry == nullptr) {
        dlclose(lib);
        error = "no CLAP entry point";
    }
    return entry;
}

#endif

}
