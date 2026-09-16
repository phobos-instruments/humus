// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <memory>
#include <string>

#include "gui/editor/OrganismEditor.h"

namespace hum {

class EditorHost;

std::unique_ptr<OrganismEditor> makeOrganismEditor(EditorHost& host, const std::string& name);

}
