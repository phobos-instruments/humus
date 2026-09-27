// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "hum/PixelField.h"

namespace hum::files {

inline std::string systemVideoPatterns() { return "*.mov;*.mp4;*.m4v;*.avi"; }

class MediaLibrary {
public:
    virtual ~MediaLibrary() = default;

    virtual std::string resolve(const std::string& ref, const std::string& displayClass) const = 0;
    virtual bool exists(const std::string& path) const = 0;
    virtual bool isFile(const std::string& path) const = 0;
    virtual bool isDirectory(const std::string& path) const = 0;

    virtual std::string audioPatterns() const = 0;
    virtual std::string videoPatterns() const { return systemVideoPatterns(); }
    virtual std::string kindPatterns(const std::string& kind) const = 0;
    virtual std::string kindStartDir(const std::string& kind) const = 0;
    virtual std::string recordingDir() const = 0;
    virtual std::string musicDir() const = 0;
    virtual void rememberRecording(const std::string& path) = 0;
    virtual std::string lastFolder(const std::string& kind) const = 0;
    virtual void rememberFolder(const std::string& kind, const std::string& path) = 0;

    virtual bool loadPicture(const std::string& path, PixelField& out, int maxWidth, int maxHeight) const = 0;
};

}
