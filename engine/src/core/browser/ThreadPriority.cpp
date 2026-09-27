// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/ThreadPriority.h"

#if defined(__APPLE__)
#include <pthread.h>
#include <pthread/qos.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace hum::browser {

bool lowerCurrentThreadPriority() {
#if defined(__APPLE__)
    return pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0) == 0;
#elif defined(_WIN32)
    return SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST) != 0;
#elif defined(__linux__)
    sched_param idle{};
    return pthread_setschedparam(pthread_self(), SCHED_IDLE, &idle) == 0;
#else
    return false;
#endif
}

}
