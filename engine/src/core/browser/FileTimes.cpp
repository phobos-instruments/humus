// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/FileTimes.h"

#include <cstdint>

#include "hum/FileBytes.h"

#if defined(_WIN32)
#include <sys/stat.h>
#include <sys/types.h>
#elif defined(__APPLE__)
#include <sys/stat.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <sys/stat.h>
#endif

namespace hum::browser {

std::int64_t createdSeconds(const std::string& path) {
    const auto p = utf8Path(path);
#if defined(_WIN32)
    struct _stat64 st {};
    return _wstat64(p.c_str(), &st) == 0 ? (std::int64_t) st.st_ctime : 0;
#elif defined(__APPLE__)
    struct stat st {};
    return stat(p.c_str(), &st) == 0 ? (std::int64_t) st.st_birthtimespec.tv_sec : 0;
#elif defined(__linux__) && defined(STATX_BTIME)
    struct statx st {};
    if (statx(AT_FDCWD, p.c_str(), 0, STATX_BTIME, &st) != 0 || (st.stx_mask & STATX_BTIME) == 0) return 0;
    return (std::int64_t) st.stx_btime.tv_sec;
#else
    (void) p;
    return 0;
#endif
}

}
