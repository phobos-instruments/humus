// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/BridgeRing.h"

#include "core/plugins/ProcessId.h"

#include <cerrno>
#include <climits>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define HUM_BRIDGE_WIN_SEM 1
#elif defined(__linux__) && !defined(HUM_BRIDGE_FORCE_PSHARED_COND)
#include <semaphore.h>
#include <time.h>
#define HUM_BRIDGE_POSIX_SEM 1
#else
#include <pthread.h>
#include <time.h>
#define HUM_BRIDGE_PSHARED_COND 1
#endif

namespace hum {

namespace {
static_assert(std::atomic<uint32_t>::is_always_lock_free, "seq atomics must be lock-free");
static_assert(std::atomic<float>::is_always_lock_free, "param mirror must be lock-free");

#if !defined(_WIN32)
timespec deadlineAfter(int timeoutMs) {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += (long) (timeoutMs % 1000) * 1000000L;
    ts.tv_sec += timeoutMs / 1000 + ts.tv_nsec / 1000000000L;
    ts.tv_nsec %= 1000000000L;
    return ts;
}
#endif

#if HUM_BRIDGE_POSIX_SEM
static_assert(sizeof(sem_t) <= sizeof(BridgeSignalStorage), "sem_t exceeds reserved storage");
sem_t* asSem(BridgeSignalStorage& s) { return reinterpret_cast<sem_t*>(s.bytes); }

void initSignal(BridgeSignalStorage& s) { sem_init(asSem(s), 1, 0); }
void signal(BridgeSignalStorage& s) { sem_post(asSem(s)); }

template <class Ready> bool waitFor(BridgeSignalStorage& s, int timeoutMs, Ready ready) {
    const timespec deadline = deadlineAfter(timeoutMs);
    while (!ready())
        if (sem_timedwait(asSem(s), &deadline) != 0 && errno != EINTR) break;
    return ready();
}
#endif

#if HUM_BRIDGE_PSHARED_COND
struct SharedCond { pthread_mutex_t mutex; pthread_cond_t cond; };
static_assert(sizeof(SharedCond) <= sizeof(BridgeSignalStorage),
              "pthread mutex + cond exceed reserved storage");
SharedCond* asCond(BridgeSignalStorage& s) { return reinterpret_cast<SharedCond*>(s.bytes); }

void initSignal(BridgeSignalStorage& s) {
    auto* c = asCond(s);
    pthread_mutexattr_t ma;
    pthread_mutexattr_init(&ma);
    pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&c->mutex, &ma);
    pthread_mutexattr_destroy(&ma);
    pthread_condattr_t ca;
    pthread_condattr_init(&ca);
    pthread_condattr_setpshared(&ca, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&c->cond, &ca);
    pthread_condattr_destroy(&ca);
}

void signal(BridgeSignalStorage& s) {
    auto* c = asCond(s);
    pthread_mutex_lock(&c->mutex);
    pthread_cond_broadcast(&c->cond);
    pthread_mutex_unlock(&c->mutex);
}

template <class Ready> bool waitFor(BridgeSignalStorage& s, int timeoutMs, Ready ready) {
    auto* c = asCond(s);
    const timespec deadline = deadlineAfter(timeoutMs);
    pthread_mutex_lock(&c->mutex);
    int rc = 0;
    while (!ready() && rc == 0) rc = pthread_cond_timedwait(&c->cond, &c->mutex, &deadline);
    pthread_mutex_unlock(&c->mutex);
    return ready();
}
#endif

#if HUM_BRIDGE_WIN_SEM
std::string semaphoreName(const juce::File& file, const char* which) {
    return "Local\\" + file.getFileName().toStdString() + "-" + which;
}

void signal(void* handle) {
    if (handle != nullptr) ReleaseSemaphore((HANDLE) handle, 1, nullptr);
}

template <class Ready> bool waitFor(void* handle, int timeoutMs, Ready ready) {
    const auto deadline = juce::Time::getMillisecondCounterHiRes() + timeoutMs;
    while (!ready()) {
        const int left = (int) (deadline - juce::Time::getMillisecondCounterHiRes());
        if (left <= 0 || handle == nullptr) break;
        if (WaitForSingleObject((HANDLE) handle, (DWORD) left) == WAIT_FAILED) break;
    }
    return ready();
}
#endif
}

const char* BridgeRing::signalBackendName() {
#if HUM_BRIDGE_WIN_SEM
    return "named-semaphore";
#elif HUM_BRIDGE_POSIX_SEM
    return "posix-semaphore";
#else
    return "pshared-cond";
#endif
}

juce::File BridgeRing::preferredDir() {
    const juce::File shm("/dev/shm");
    if (shm.isDirectory()) return shm;
    return juce::File::getSpecialLocation(juce::File::tempDirectory);
}

void BridgeRing::sweepStale(const juce::File& dir) {
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "hum-bridge-*.shm")) {
        const auto name = f.getFileNameWithoutExtension();
        const int pid = name.fromFirstOccurrenceOf("hum-bridge-", false, false)
                            .upToFirstOccurrenceOf("-", false, false)
                            .getIntValue();
        if (pid > 0 && !processAlive(pid)) f.deleteFile();
    }
}

bool BridgeRing::openSignals(bool create, std::string& err) {
#if HUM_BRIDGE_WIN_SEM
    const auto req = semaphoreName(file_, "req"), ack = semaphoreName(file_, "ack");
    if (create) {
        reqHandle_ = CreateSemaphoreA(nullptr, 0, LONG_MAX, req.c_str());
        ackHandle_ = CreateSemaphoreA(nullptr, 0, LONG_MAX, ack.c_str());
    } else {
        const DWORD access = SYNCHRONIZE | SEMAPHORE_MODIFY_STATE;
        reqHandle_ = OpenSemaphoreA(access, FALSE, req.c_str());
        ackHandle_ = OpenSemaphoreA(access, FALSE, ack.c_str());
    }
    if (reqHandle_ == nullptr || ackHandle_ == nullptr) {
        err = "bridge shm: cannot open the wake-up semaphores";
        closeSignals();
        return false;
    }
#else
    juce::ignoreUnused(err);
    if (create) {
        initSignal(header_->reqSignal);
        initSignal(header_->ackSignal);
    }
#endif
    return true;
}

void BridgeRing::closeSignals() {
#if HUM_BRIDGE_WIN_SEM
    if (reqHandle_ != nullptr) CloseHandle((HANDLE) reqHandle_);
    if (ackHandle_ != nullptr) CloseHandle((HANDLE) ackHandle_);
#endif
    reqHandle_ = ackHandle_ = nullptr;
}

bool BridgeRing::create(const juce::File& path, int ins, int outs, std::string& err) {
    close();
    const auto size = bridgeShmSize(ins, outs);
    path.deleteFile();
    {
        juce::FileOutputStream os(path);
        if (os.failedToOpen()) { err = "bridge shm: cannot create " + path.getFullPathName().toStdString(); return false; }
        os.setPosition((juce::int64) size - 1);
        os.writeByte(0);
    }
    map_ = std::make_unique<juce::MemoryMappedFile>(path, juce::MemoryMappedFile::readWrite);
    if (map_->getData() == nullptr || map_->getSize() < (size_t) size) {
        err = "bridge shm: mmap failed";
        map_.reset();
        return false;
    }
    file_ = path;
    owner_ = true;
    header_ = new (map_->getData()) BridgeShmHeader();
    header_->magic = kBridgeMagic;
    header_->version = kBridgeVersion;
    header_->numIns = (uint32_t) ins;
    header_->numOuts = (uint32_t) outs;
    if (!openSignals(true, err)) { close(); return false; }
    return true;
}

bool BridgeRing::open(const juce::File& path, std::string& err) {
    close();
    map_ = std::make_unique<juce::MemoryMappedFile>(path, juce::MemoryMappedFile::readWrite);
    if (map_->getData() == nullptr || map_->getSize() < sizeof(BridgeShmHeader)) {
        err = "bridge shm: cannot map " + path.getFullPathName().toStdString();
        map_.reset();
        return false;
    }
    header_ = reinterpret_cast<BridgeShmHeader*>(map_->getData());
    if (header_->magic != kBridgeMagic || header_->version != kBridgeVersion) {
        err = "bridge shm: bad magic/version";
        header_ = nullptr;
        map_.reset();
        return false;
    }
    file_ = path;
    owner_ = false;
    if (!openSignals(false, err)) { close(); return false; }
    return true;
}

void BridgeRing::close() {
    closeSignals();
    header_ = nullptr;
    map_.reset();
}

void BridgeRing::postReq() {
    if (header_ == nullptr) return;
#if HUM_BRIDGE_WIN_SEM
    signal(reqHandle_);
#else
    signal(header_->reqSignal);
#endif
}

void BridgeRing::postAck() {
    if (header_ == nullptr) return;
#if HUM_BRIDGE_WIN_SEM
    signal(ackHandle_);
#else
    signal(header_->ackSignal);
#endif
}

bool BridgeRing::waitAckAtLeast(uint32_t seq, int timeoutMs) {
    if (header_ == nullptr) return false;
    auto* h = header_;
    auto ready = [h, seq] {
        return (int32_t) (h->ackSeq.load(std::memory_order_acquire) - seq) >= 0;
    };
#if HUM_BRIDGE_WIN_SEM
    return waitFor(ackHandle_, timeoutMs, ready);
#else
    return waitFor(h->ackSignal, timeoutMs, ready);
#endif
}

bool BridgeRing::waitReqAbove(uint32_t lastSeen, int timeoutMs) {
    if (header_ == nullptr) return false;
    auto* h = header_;
    auto ready = [h, lastSeen] {
        return (int32_t) (h->reqSeq.load(std::memory_order_acquire) - lastSeen) > 0;
    };
#if HUM_BRIDGE_WIN_SEM
    return waitFor(reqHandle_, timeoutMs, ready);
#else
    return waitFor(h->reqSignal, timeoutMs, ready);
#endif
}

}
