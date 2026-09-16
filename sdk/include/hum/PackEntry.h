// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>

#define HUM_PACK_ABI 1

#if defined(_WIN32)
  #define HUM_PACK_EXPORT __declspec(dllexport)
#else
  #define HUM_PACK_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
HUM_PACK_EXPORT int32_t hum_pack_abi();
HUM_PACK_EXPORT uint64_t hum_pack_shape();
HUM_PACK_EXPORT void* hum_pack_create(const char* className);

HUM_PACK_EXPORT const char* hum_pack_layout_json(const char* genId, const char* className);
}
