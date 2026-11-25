// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class EngineHost;

class VideoWindows {
public:
    using OnClosed = std::function<void(const std::string&)>;
    using Make = std::function<std::unique_ptr<juce::DocumentWindow>(const std::string& name, int width, int height,
                                                                     OnClosed onClosed)>;

    explicit VideoWindows(EngineHost& host, Make make = {});
    ~VideoWindows();

    void open(const std::string& name, int width = 0, int height = 0);
    void reopenOutputs();
    bool isOpen(const std::string& name) const { return windows_.count(name) != 0; }
    int openCount() const { return (int) windows_.size(); }
    void closeAll() { windows_.clear(); }

private:
    bool showsPicture(const std::string& name) const;
    void closed(const std::string& name);

    EngineHost& host_;
    Make make_;
    std::map<std::string, std::unique_ptr<juce::DocumentWindow>> windows_;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);

    JUCE_DECLARE_NON_COPYABLE(VideoWindows)
};

}
