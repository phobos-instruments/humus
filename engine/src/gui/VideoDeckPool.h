#pragma once
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/AppPaths.h"
#include "gui/VideoLayer.h"

namespace hum {

class VideoDeckPool {
public:
    static juce::File resolveTape(const std::string& documentPath, const juce::String& path) {
        if (path.isEmpty()) return {};
        if (const auto asset = resolveAssetRef(path); asset != juce::File()) return asset;
        if (juce::File::isAbsolutePath(path)) return juce::File(path);
        const auto doc = juce::String(documentPath);
        if (doc.isNotEmpty() && juce::File::isAbsolutePath(doc))
            return juce::File(doc).getParentDirectory().getChildFile(path);
        return {};
    }

    static VideoDeckPool& instance() {
        static VideoDeckPool p;
        return p;
    }

    std::vector<std::string> keysForTest() const {
        std::vector<std::string> out;
        for (const auto& [k, e] : decks_) if (!e.layer.expired()) out.push_back(k);
        return out;
    }

    std::shared_ptr<VideoLayer> peek(const std::string& node) const {
        const auto it = decks_.find(node);
        return it != decks_.end() ? it->second.layer.lock() : nullptr;
    }

    std::shared_ptr<VideoLayer> open(const std::string& node, const juce::String& path,
                                     bool offline = false) {
        if (decks_.size() > kPruneAbove) prune();
        auto& e = decks_[node];
        auto layer = e.layer.lock();
        if (layer == nullptr || e.offline != offline) {
            layer = VideoLayer::create(offline);
            if (layer == nullptr) return nullptr;
            e.layer = layer;
            e.path = {};
            e.offline = offline;
        }
        if (e.path != path) {
            e.path = path;
            layer->load(path);
        }
        return layer;
    }

private:
    static constexpr size_t kPruneAbove = 64;

    void prune() {
        for (auto it = decks_.begin(); it != decks_.end();)
            it = it->second.layer.expired() ? decks_.erase(it) : std::next(it);
    }

    struct Entry {
        std::weak_ptr<VideoLayer> layer;
        juce::String path;
        bool offline = false;
    };

    std::map<std::string, Entry> decks_;
};

}
