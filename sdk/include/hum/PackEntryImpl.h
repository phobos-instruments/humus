#pragma once
#include "hum/PackEntry.h"
#include "hum/Registry.h"

#define HUM_DEFINE_PACK_ENTRY(registerFn, manifestJson)                          \
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
    HUM_PACK_EXPORT const char* hum_pack_manifest_json() { return manifestJson; }\
    HUM_PACK_EXPORT void* hum_pack_create(const char* className) {               \
        if (className == nullptr) return nullptr;                                \
        return humPackLocalRegistry().createExact(className).release();          \
    }                                                                            \
    HUM_PACK_EXPORT void hum_pack_destroy(void* organism) {                      \
        delete static_cast<hum::Organism*>(organism);                            \
    }                                                                            \
    HUM_PACK_EXPORT                                                              \
    const char* hum_pack_layout_json(const char* genId, const char* className) { \
        if (genId == nullptr || className == nullptr) return nullptr;            \
        \
        static std::string json;                                                 \
        json = humPackLocalRegistry().layoutJsonExact(genId, className);         \
        return json.empty() ? nullptr : json.c_str();                            \
    }                                                                            \
    }
