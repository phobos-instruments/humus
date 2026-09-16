// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {
namespace licensecheck {

std::string normalizeCode(const std::string& raw);
bool checksumOk(const std::string& raw);
std::string formatPretty(const std::string& normalized);

struct License {
    bool valid = false;
    std::string licenseId, plan, name, email, instanceId, issuedAt;
};

License verifyLicense(const std::string& payloadB64, const std::string& sigB64,
                      const std::string& instanceId,
                      const unsigned char (*trustedKeys)[32], int numKeys);

}
}
