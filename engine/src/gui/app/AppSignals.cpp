// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/AppSignals.h"

#include <csignal>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <io.h>
#else
#include <execinfo.h>
#include <unistd.h>
#endif

namespace hum::appsignals {

namespace {

char gAutosaveSlotPath[1024];
char gAutosaveMetaPath[1024];

void removeRaw(const char* p) {
#if defined(_WIN32)
    ::_unlink(p);
#else
    ::unlink(p);
#endif
}

void writeRaw(const char* s) {
#if defined(_WIN32)
    ::_write(2, s, (unsigned) strlen(s));
#else
    const auto ignored = ::write(2, s, strlen(s));
    (void) ignored;
#endif
}

void deliberateQuitHandler(int sig) {
    removeRaw(gAutosaveSlotPath);
    removeRaw(gAutosaveMetaPath);
    signal(sig, SIG_DFL);
    raise(sig);
}

void crashHandler(int sig) {
    char head[64] = "\n=== CRASH: signal ";
    char d[4] = { (char) ('0' + (sig / 10) % 10), (char) ('0' + sig % 10), ' ', 0 };
    writeRaw(head);
    writeRaw(sig >= 10 ? d : d + 1);
    writeRaw("===\n");
#if !defined(_WIN32)
    void* frames[64];
    backtrace_symbols_fd(frames, backtrace(frames, 64), 2);
#endif
    writeRaw("=== END CRASH ===\n");
    signal(sig, SIG_DFL);
    raise(sig);
}

}

void installCrashHandlers() {
    signal(SIGSEGV, crashHandler);
    signal(SIGABRT, crashHandler);
}

void installQuitHandlers(const juce::File& autosaveFile) {
    snprintf(gAutosaveSlotPath, sizeof(gAutosaveSlotPath), "%s",
             autosaveFile.getFullPathName().toRawUTF8());
    snprintf(gAutosaveMetaPath, sizeof(gAutosaveMetaPath), "%s",
             autosaveFile.withFileExtension("meta.xml").getFullPathName().toRawUTF8());
    signal(SIGINT, deliberateQuitHandler);
    signal(SIGTERM, deliberateQuitHandler);
#if !defined(_WIN32)
    signal(SIGHUP, deliberateQuitHandler);
#endif
}

void printBacktrace() {
#if !defined(_WIN32)
    void* frames[64];
    const int n = backtrace(frames, 64);
    char** syms = backtrace_symbols(frames, n);
    for (int i = 0; i < n; ++i)
        fprintf(stderr, "  %s\n", syms ? syms[i] : "?");
#endif
}

}
