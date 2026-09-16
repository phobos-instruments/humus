// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstring>
#include <type_traits>
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

namespace hum {

#if defined(_MSC_VER) && !defined(__clang__)

template <typename T>
inline T rtLoadWord(const T& v) {
    static_assert(std::is_trivially_copyable<T>::value
                      && (sizeof(T) == 1 || sizeof(T) == 2
                          || sizeof(T) == 4 || sizeof(T) == 8),
                  "rtLoadWord: single machine word only");
    T out;
    if constexpr (sizeof(T) == 1) {
        const auto b = __iso_volatile_load8((const volatile __int8*) &v);
        std::memcpy(&out, &b, 1);
    } else if constexpr (sizeof(T) == 2) {
        const auto b = __iso_volatile_load16((const volatile __int16*) &v);
        std::memcpy(&out, &b, 2);
    } else if constexpr (sizeof(T) == 4) {
        const auto b = __iso_volatile_load32((const volatile __int32*) &v);
        std::memcpy(&out, &b, 4);
    } else {
        const auto b = __iso_volatile_load64((const volatile __int64*) &v);
        std::memcpy(&out, &b, 8);
    }
    return out;
}

template <typename T>
inline void rtStoreWord(T& v, T x) {
    static_assert(std::is_trivially_copyable<T>::value
                      && (sizeof(T) == 1 || sizeof(T) == 2
                          || sizeof(T) == 4 || sizeof(T) == 8),
                  "rtStoreWord: single machine word only");
    if constexpr (sizeof(T) == 1) {
        __int8 b; std::memcpy(&b, &x, 1);
        __iso_volatile_store8((volatile __int8*) &v, b);
    } else if constexpr (sizeof(T) == 2) {
        __int16 b; std::memcpy(&b, &x, 2);
        __iso_volatile_store16((volatile __int16*) &v, b);
    } else if constexpr (sizeof(T) == 4) {
        __int32 b; std::memcpy(&b, &x, 4);
        __iso_volatile_store32((volatile __int32*) &v, b);
    } else {
        __int64 b; std::memcpy(&b, &x, 8);
        __iso_volatile_store64((volatile __int64*) &v, b);
    }
}

#else

template <typename T>
inline T rtLoadWord(const T& v) {
    static_assert(std::is_trivially_copyable<T>::value && sizeof(T) <= 8,
                  "rtLoadWord: single machine word only");
    T out;
    __atomic_load(const_cast<T*>(&v), &out, __ATOMIC_RELAXED);
    return out;
}

template <typename T>
inline void rtStoreWord(T& v, T x) {
    static_assert(std::is_trivially_copyable<T>::value && sizeof(T) <= 8,
                  "rtStoreWord: single machine word only");
    __atomic_store(&v, &x, __ATOMIC_RELAXED);
}

#endif

}
