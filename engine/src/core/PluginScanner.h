#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace hum {

class PluginHost;

class PluginScanner {
public:
    explicit PluginScanner(PluginHost& host) : host_(host) {}
    ~PluginScanner() { cancelAsync(); }

    void start(const juce::StringArray& extraPaths, const juce::File& deadMansPedal,
               bool includeDefaultPaths = true);

    void setSubprocessExe(const juce::File& exe) { exe_ = exe; }

    bool scanNext(juce::String& current);

    void skipKnownFiles(const juce::StringArray& alsoSkip);
    size_t remaining() const { return items_.size() - index_; }

    static juce::String skipEntryFor(const juce::String& file);

    void beginAsync(int workers);
    bool pumpAsync(juce::String& current);
    void cancelAsync();

    float progress() const;
    const juce::StringArray& failed() const { return failed_; }

    static juce::String probeFileXml(const juce::String& format, const juce::String& file);
    static juce::String probeFilesXml(const juce::String& format, const juce::StringArray& files);
    static juce::String enumerateLines(const juce::String& format);

private:
    struct Item { juce::String format, file; };
    struct Result {
        juce::StringArray files;
        juce::String xml;
        bool ok = false;
    };

    void mergeXml(const juce::String& xml);
    void noteFailed(const juce::String& file);
    void dropKnownTypesFor(const juce::String& file);
    bool probeViaSubprocess(const juce::String& format, const juce::StringArray& files,
                            juce::String& xml, int timeoutMs);
    bool childToFile(const juce::String& flag, const juce::StringArray& rest,
                     juce::String& textOut, int timeoutMs);
    void workerLoop(int slot);
    void mergeResult(const Result& r);

    PluginHost& host_;
    std::vector<Item> items_;
    size_t index_ = 0;
    juce::File exe_;
    juce::StringArray failed_;

    std::vector<std::thread> workers_;
    std::mutex queueMutex_;
    std::mutex resultsMutex_;
    std::mutex childrenMutex_;
    std::vector<Result> results_;
    std::vector<juce::String> inflight_;
    std::vector<juce::ChildProcess*> children_;
    std::atomic<bool> cancel_{false};
    std::atomic<int> activeWorkers_{0};
    size_t mergedFiles_ = 0;
};

}
