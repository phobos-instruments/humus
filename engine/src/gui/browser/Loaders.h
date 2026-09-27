// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

namespace hum {
class PatcherHost;
}

namespace hum::browser {

struct Loader {
    std::string className;
    std::string param;
    std::string patterns;
    bool hidden = false;
    std::string category;
    std::string slotKind;
};

class LoaderTable {
public:
    static const LoaderTable& shared();

    void add(Loader loader) { loaders_.push_back(std::move(loader)); }
    std::vector<Loader> loadersFor(const std::string& path) const;
    std::vector<std::string> classesFor(const std::string& path) const;
    const Loader* slotIn(const std::string& className, const std::string& path) const;
    const std::vector<Loader>& all() const { return loaders_; }

private:
    std::vector<Loader> loaders_;
};

LoaderTable buildLoaderTable();
std::string loadInto(hum::PatcherHost& host, const std::string& node, const std::string& path);
std::vector<std::string> loadIntoNew(hum::PatcherHost& host, const std::string& className, const std::vector<std::string>& paths,
                                     int x, int y, const std::string& scope);
std::string loadsIntoText(const std::vector<std::string>& classes, int most = 3);

}
