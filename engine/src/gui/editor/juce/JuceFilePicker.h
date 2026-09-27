// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/files/FilePick.h"

namespace hum {

class JuceFilePicker : public files::FilePicker {
public:
    using Route = std::function<bool(const files::FilePick&, files::Picked, std::function<void()> other)>;
    static Route& browserRoute() {
        static Route route;
        return route;
    }

    void pick(const files::FilePick& request, files::Picked done) override {
        if (!request.save && browserRoute()) {
            routed_ = true;
            auto finish = [this, alive = std::weak_ptr<bool>(alive_), done](const std::vector<std::string>& paths) {
                if (alive.expired()) return;
                routed_ = false;
                if (done) done(paths);
            };
            auto other = [this, alive = std::weak_ptr<bool>(alive_), request, finish] {
                if (!alive.expired()) pickNative(request, finish);
            };
            if (browserRoute()(request, finish, other)) return;
            routed_ = false;
        }
        pickNative(request, std::move(done));
    }

    void pickNative(const files::FilePick& request, files::Picked done) {
        chooser_ = std::make_unique<juce::FileChooser>(juce::String::fromUTF8(request.title.c_str()),
                                                       juce::File(juce::String::fromUTF8(request.startDir.c_str())),
                                                       juce::String::fromUTF8(request.patterns.c_str()));
        int flags = juce::FileBrowserComponent::canSelectFiles;
        flags |= request.save ? (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting)
                              : juce::FileBrowserComponent::openMode;
        if (request.directories) flags |= juce::FileBrowserComponent::canSelectDirectories;
        if (request.multiple) flags |= juce::FileBrowserComponent::canSelectMultipleItems;
        chooser_->launchAsync(flags, [done = std::move(done)](const juce::FileChooser& fc) {
            std::vector<std::string> paths;
            for (const auto& f : fc.getResults())
                if (f != juce::File()) paths.push_back(f.getFullPathName().toStdString());
            if (done) done(paths);
        });
    }

    bool open() const override { return chooser_ != nullptr || routed_; }

private:
    std::unique_ptr<juce::FileChooser> chooser_;
    bool routed_ = false;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

}
