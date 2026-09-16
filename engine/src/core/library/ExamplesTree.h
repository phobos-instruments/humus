// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"

namespace hum::examples {

struct Node {
    std::string label;
    juce::File file;
    std::vector<Node> children;

    bool isPatch() const { return file.existsAsFile(); }
    int patchCount() const {
        if (isPatch()) return 1;
        int n = 0;
        for (const auto& c : children) n += c.patchCount();
        return n;
    }
};

inline std::string titleOf(const juce::String& stem) {
    auto name = stem.replaceCharacters("-_", "  ").trim();
    return name.isEmpty() ? std::string() : (name.substring(0, 1).toUpperCase() + name.substring(1)).toStdString();
}

inline juce::String blurbOf(const juce::File& patch) {
    const auto note = patch.getParentDirectory().getChildFile(patch.getFileNameWithoutExtension() + ".txt");
    if (!note.existsAsFile()) return {};
    return note.loadFileAsString().upToFirstOccurrenceOf("\n", false, false).trim();
}

inline Node scan(const juce::File& dir) {
    Node node;
    node.label = dir.getFileName().toStdString();
    if (!dir.isDirectory()) return node;
    juce::Array<juce::File> dirs, files;
    dir.findChildFiles(dirs, juce::File::findDirectories, false);
    dir.findChildFiles(files, juce::File::findFiles, false, "*.hum;*.amh");
    struct { int compareElements(const juce::File& a, const juce::File& b) {
        return a.getFileName().compareIgnoreCase(b.getFileName()); } } byName;
    dirs.sort(byName);
    files.sort(byName);
    for (const auto& d : dirs) {
        if (d.getFileName().startsWith(".")) continue;
        auto child = scan(d);
        if (child.patchCount() > 0) node.children.push_back(std::move(child));
    }
    for (const auto& f : files) {
        if (f.getFileName().startsWith("my-")) continue;
        Node leaf;
        leaf.label = titleOf(f.getFileNameWithoutExtension());
        leaf.file = f;
        node.children.push_back(std::move(leaf));
    }
    return node;
}

inline std::vector<juce::File> roots() {
    std::vector<juce::File> out;
    const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto dir = exe.getParentDirectory();
    for (const auto& c : {dir.getChildFile("examples"),
                          dir.getParentDirectory().getChildFile("Resources").getChildFile("examples")})
        if (c.isDirectory()) out.push_back(c);
#ifdef HUM_PACKS_DIR
    if (const auto repo = juce::File(HUM_PACKS_DIR).getParentDirectory().getChildFile("examples");
        repo.isDirectory())
        out.push_back(repo);
#endif
    return out;
}

inline Node shipped() {
    for (const auto& r : roots()) {
        auto tree = scan(r);
        if (tree.patchCount() > 0) return tree;
    }
    return {};
}

inline juce::File repoRoot() {
#ifdef HUM_PACKS_DIR
    return juce::File(HUM_PACKS_DIR).getParentDirectory().getChildFile("examples");
#else
    return {};
#endif
}

}
