#include "core/BridgeRing.h"

#include <thread>

#if defined(__linux__)
#include <semaphore.h>
#include <time.h>
#define HUM_BRIDGE_POSIX_SEM 1
#endif

namespace hum {

namespace {
#if HUM_BRIDGE_POSIX_SEM
static_assert(sizeof(sem_t) <= sizeof(BridgeSemStorage), "sem_t exceeds reserved storage");
sem_t* asSem(BridgeSemStorage& s) { return reinterpret_cast<sem_t*>(s.bytes); }

bool timedWait(sem_t* s, int timeoutMs) {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += (long) (timeoutMs % 1000) * 1000000L;
    ts.tv_sec += timeoutMs / 1000 + ts.tv_nsec / 1000000000L;
    ts.tv_nsec %= 1000000000L;
    return sem_timedwait(s, &ts) == 0;
}
#endif

static_assert(std::atomic<uint32_t>::is_always_lock_free, "seq atomics must be lock-free");
static_assert(std::atomic<float>::is_always_lock_free, "param mirror must be lock-free");
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
        if (pid <= 0) continue;
#if defined(__linux__)
        if (!juce::File("/proc/" + juce::String(pid)).isDirectory()) f.deleteFile();
#else
        juce::ignoreUnused(f);
#endif
    }
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
#if HUM_BRIDGE_POSIX_SEM
    sem_init(asSem(header_->reqSem), 1, 0);
    sem_init(asSem(header_->ackSem), 1, 0);
#endif
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
    return true;
}

void BridgeRing::close() {
    header_ = nullptr;
    map_.reset();
}

void BridgeRing::postReq() {
#if HUM_BRIDGE_POSIX_SEM
    if (header_) sem_post(asSem(header_->reqSem));
#endif
}

void BridgeRing::postAck() {
#if HUM_BRIDGE_POSIX_SEM
    if (header_) sem_post(asSem(header_->ackSem));
#endif
}

bool BridgeRing::waitAckAtLeast(uint32_t seq, int timeoutMs) {
    if (!header_) return false;
    if ((int32_t) (header_->ackSeq.load(std::memory_order_acquire) - seq) >= 0) return true;
#if HUM_BRIDGE_POSIX_SEM
    const auto deadline = juce::Time::getMillisecondCounterHiRes() + timeoutMs;
    while ((int32_t) (header_->ackSeq.load(std::memory_order_acquire) - seq) < 0) {
        const int left = (int) (deadline - juce::Time::getMillisecondCounterHiRes());
        if (left <= 0 || !timedWait(asSem(header_->ackSem), left)) break;
    }
#else
    for (int spins = 0; spins < timeoutMs * 100; ++spins) {
        if ((int32_t) (header_->ackSeq.load(std::memory_order_acquire) - seq) >= 0) break;
        std::this_thread::yield();
    }
#endif
    return (int32_t) (header_->ackSeq.load(std::memory_order_acquire) - seq) >= 0;
}

bool BridgeRing::waitReqAbove(uint32_t lastSeen, int timeoutMs) {
    if (!header_) return false;
    if ((int32_t) (header_->reqSeq.load(std::memory_order_acquire) - lastSeen) > 0) return true;
#if HUM_BRIDGE_POSIX_SEM
    const auto deadline = juce::Time::getMillisecondCounterHiRes() + timeoutMs;
    while ((int32_t) (header_->reqSeq.load(std::memory_order_acquire) - lastSeen) <= 0) {
        const int left = (int) (deadline - juce::Time::getMillisecondCounterHiRes());
        if (left <= 0 || !timedWait(asSem(header_->reqSem), left)) break;
    }
#else
    for (int spins = 0; spins < timeoutMs * 100; ++spins) {
        if ((int32_t) (header_->reqSeq.load(std::memory_order_acquire) - lastSeen) > 0) break;
        std::this_thread::yield();
    }
#endif
    return (int32_t) (header_->reqSeq.load(std::memory_order_acquire) - lastSeen) > 0;
}

}
