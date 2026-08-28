#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_cryptography/juce_cryptography.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class AppUpdater : private juce::Thread {
public:
    AppUpdater() : juce::Thread("hum-update-download") {}
    ~AppUpdater() override { stopThread(2000); }

    std::function<void(double)> onProgress;
    std::function<void(bool ok, juce::String message, juce::File file)> onDone;

    void start(const juce::String& url, const juce::String& sha256,
               const juce::String& version) {
        stopThread(2000);
        url_ = url;
        sha_ = sha256.toLowerCase();
        version_ = version;
        startThread();
    }

    void cancel() { signalThreadShouldExit(); }
    using juce::Thread::isThreadRunning;

    static juce::File downloadsDir() {
        const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
        if (const auto d = home.getChildFile("Downloads"); d.isDirectory()) return d;
        return juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    }

    static juce::String revealVerb() {
#if JUCE_MAC
        return "Show in Finder";
#elif JUCE_WINDOWS
        return "Show in Explorer";
#else
        return "Show the file";
#endif
    }

private:
    void finish(bool ok, juce::String message, juce::File file) {
        juce::MessageManager::callAsync(
            [cb = onDone, ok, message, file] { if (cb) cb(ok, message, file); });
    }

    void run() override {
        const juce::URL url(url_);
        auto leaf = url.getFileName();
        if (leaf.isEmpty()) leaf = "Humus-" + version_;

        auto stream = url.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(15000)
                .withNumRedirectsToFollow(5));
        if (stream == nullptr) {
            finish(false, "Could not reach the download", {});
            return;
        }

        const auto target = downloadsDir().getChildFile(leaf);
        const auto partial = target.getSiblingFile(leaf + ".part");
        partial.deleteFile();
        std::unique_ptr<juce::FileOutputStream> out(partial.createOutputStream());
        if (out == nullptr || out->failedToOpen()) {
            finish(false, "Cannot write to " + downloadsDir().getFileName(), {});
            return;
        }

        const auto total = stream->getTotalLength();
        char buf[64 * 1024];
        juce::int64 got = 0;
        while (!threadShouldExit() && !stream->isExhausted()) {
            const int n = stream->read(buf, (int) sizeof(buf));
            if (n <= 0) break;
            out->write(buf, (size_t) n);
            got += n;
            if (onProgress)
                juce::MessageManager::callAsync(
                    [cb = onProgress, got, total] {
                        if (cb) cb(total > 0 ? (double) got / (double) total : -1.0);
                    });
        }
        out.reset();
        if (threadShouldExit()) {
            partial.deleteFile();
            return;
        }
        if (got == 0) {
            partial.deleteFile();
            finish(false, "The download was empty", {});
            return;
        }
        if (total > 0 && got != total) {
            partial.deleteFile();
            finish(false, "The download stopped early", {});
            return;
        }

        if (sha_.isNotEmpty()) {
            juce::FileInputStream in(partial);
            const auto have = juce::SHA256(in).toHexString().toLowerCase();
            if (have != sha_) {
                partial.deleteFile();
                finish(false, "The download did not match its checksum", {});
                return;
            }
        }

        target.deleteFile();
        if (!partial.moveFileTo(target)) {
            finish(false, "Could not put the file in " + downloadsDir().getFileName(),
                   partial);
            return;
        }
        finish(true, sha_.isEmpty() ? "Downloaded (unchecked)" : "Downloaded", target);
    }

    juce::String url_, sha_, version_;
};

}
