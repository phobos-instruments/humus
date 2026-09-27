// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "core/browser/BrowserPlaces.h"

namespace hum::browser {

std::string encodePlace(const Place& place);
std::optional<Place> decodePlace(const std::string& text);
bool placeStillThere(const Place& place, const std::vector<Kind>& wanted, const PlaceRoots& roots, const FileIndex& index);

}
