#pragma once
#include <map>
#include <memory>
#include <string>

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

    std::shared_ptr<VideoLayer> peek(const std::string& node) const {
        const auto it = decks_.find(node);
        return it != decks_.end() ? it->second.layer.lock() : nullptr;
    }

    std::shared_ptr<VideoLayer> open(const std::string& node, const juce::String& path) {
        auto& e = decks_[node];
        auto layer = e.layer.lock();
        if (layer == nullptr) {
            layer = VideoLayer::create();
            if (layer == nullptr) return nullptr;
            e.layer = layer;
            e.path = {};
        }
        if (e.path != path) {
            e.path = path;
            layer->load(path);
        }
        return layer;
    }

private:
    struct Entry {
        std::weak_ptr<VideoLayer> layer;
        juce::String path;
    };

    std::map<std::string, Entry> decks_;
};

}
