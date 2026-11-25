// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatchDocument.h"
#include "io/PatchWriter.h"

#include <juce_core/juce_core.h>

#include "core/xml/Xml.h"
#include "io/PatchFormat.h"
#include "io/PatchMigrate.h"

namespace hum {

namespace {

bool carriesAScheme(const juce::String& text) {
    for (const char* scheme : {"asset:", "library:", "bank:", "file://"})
        if (text.startsWith(scheme)) return true;
    return false;
}

void anchorSoundFile(Parameter& p, const juce::File& docDir) {
    if (p.type != "soundfile" || p.text.empty()) return;
    const auto stored = juce::String::fromUTF8(p.text.c_str());
    if (juce::File::isAbsolutePath(stored) || carriesAScheme(stored)) return;
    const auto beside = docDir.getChildFile(fromDocumentPath(stored));
    if (beside.existsAsFile()) p.text = beside.getFullPathName().toStdString();
}

}

bool parsePatchTextBeside(const std::string& text, const std::string& folder, PatchDocumentModel& out,
                          std::string& error, std::unique_ptr<xml::Element>* rawOut) {
    if (!parsePatchText(text, out, error, rawOut)) return false;
    if (folder.empty()) {
        migrateLegacyControl(out);
        return true;
    }
    const juce::File dir(juce::String::fromUTF8(folder.c_str()));
    for (auto& c : out.organisms)
        for (auto& ch : c.pattern.channels) {
            const auto stored = juce::String::fromUTF8(ch.audioFile.c_str());
            if (!ch.audioFile.empty() && !juce::File::isAbsolutePath(stored))
                ch.audioFile = dir.getChildFile(fromDocumentPath(stored)).getFullPathName().toStdString();
        }
    for (auto& c : out.organisms) {
        for (auto& p : c.properties) anchorSoundFile(p, dir);
        for (auto& preset : c.presets)
            for (auto& p : preset.properties) anchorSoundFile(p, dir);
    }
    migrateLegacyControl(out);
    return true;
}

bool parsePatchFile(const std::string& path, PatchDocumentModel& out, std::string& error,
                    std::unique_ptr<xml::Element>* rawOut) {
    juce::File f(juce::String::fromUTF8(path.c_str()));
    if (!f.existsAsFile()) { error = "file not found: " + path; return false; }
    return parsePatchTextBeside(f.loadFileAsString().toStdString(),
                                f.getParentDirectory().getFullPathName().toStdString(), out, error, rawOut);
}

namespace {

void relativizeMediaPaths(xml::Element& el, const juce::File& docDir) {
    if (el.hasTag("pattern-channel") && el.hasAttribute("file")) {
        const auto stored = juce::String::fromUTF8(el.attribute("file").c_str());
        if (juce::File::isAbsolutePath(stored)) {
            const juce::File f(stored);
            if (f.isAChildOf(docDir))
                el.setAttribute("file", toDocumentPath(f.getRelativePathFrom(docDir)).toStdString());
        }
    }
    if (el.hasTag("soundfile")) {
        const auto stored = juce::String::fromUTF8(el.allSubText().c_str()).trim();
        if (juce::File::isAbsolutePath(stored)) {
            const juce::File f(stored);
            if (f.isAChildOf(docDir)) {
                el.deleteChildren();
                el.addText(toDocumentPath(f.getRelativePathFrom(docDir)).toStdString());
            }
        }
        return;
    }
    for (auto* c : el.children()) relativizeMediaPaths(*c, docDir);
}

}

std::string writePatchTextBeside(const PatchDocumentModel& doc, const std::string& folder,
                                 const xml::Element* original) {
    auto root = buildPatchTree(doc, original);
    if (!folder.empty()) relativizeMediaPaths(*root, juce::File(juce::String::fromUTF8(folder.c_str())));
    return xml::write(*root);
}

bool writePatchFile(const std::string& path, const PatchDocumentModel& doc, std::string& error,
                    const xml::Element* original) {
    const juce::File f(juce::String::fromUTF8(path.c_str()));
    const auto text = writePatchTextBeside(doc, f.getParentDirectory().getFullPathName().toStdString(), original);
    juce::TemporaryFile staging(f);
    bool written = false;
    {
        juce::FileOutputStream out(staging.getFile());
        written = out.openedOk() && out.write(text.data(), text.size());
    }
    if (!written || !staging.overwriteTargetFileWithTemporary()) {
        error = "could not write " + path;
        return false;
    }
    return true;
}

}
