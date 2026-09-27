// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include "core/xml/Xml.h"
#include "io/PatchDocument.h"

namespace hum {

std::unique_ptr<xml::Element> buildPatchTree(const PatchDocumentModel& doc, const xml::Element* original = nullptr);

bool writePatchFile(const std::string& path, const PatchDocumentModel& doc, std::string& error,
                    const xml::Element* original = nullptr);

std::string writePatchString(const PatchDocumentModel& doc, const xml::Element* original = nullptr);

}
