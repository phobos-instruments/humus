// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "hum/AbiShape.h"
#include "hum/PackEntry.h"
#include "hum/Registry.h"

#define HUM_DEFINE_PACK_ENTRY(registerFn)                                        \
    namespace {                                                                  \
    hum::Registry& humPackLocalRegistry() {                                      \
        static hum::Registry reg;                                                \
        static bool once = [] { registerFn(reg); return true; }();               \
        (void) once;                                                             \
        return reg;                                                              \
    }                                                                            \
    }                                                                            \
    extern "C" {                                                                 \
    HUM_PACK_EXPORT int32_t hum_pack_abi() { return HUM_PACK_ABI; }              \
    HUM_PACK_EXPORT uint64_t hum_pack_shape() { return hum::abiShape(); }        \
    HUM_PACK_EXPORT void* hum_pack_create(const char* className) {               \
        if (className == nullptr) return nullptr;                                \
        return humPackLocalRegistry().createExact(className).release();          \
    }                                                                            \
    HUM_PACK_EXPORT                                                              \
    const char* hum_pack_layout_json(const char* genId, const char* className) { \
        if (genId == nullptr || className == nullptr) return nullptr;            \
        thread_local std::string json;                                           \
        json = humPackLocalRegistry().layoutJsonExact(genId, className);         \
        return json.empty() ? nullptr : json.c_str();                            \
    }                                                                            \
    }
