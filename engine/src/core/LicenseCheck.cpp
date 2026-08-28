#include "core/LicenseCheck.h"

#include <juce_core/juce_core.h>

#include "monocypher-ed25519.h"

namespace hum {
namespace licensecheck {

static const char* kAlphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
static constexpr int kDataLen = 14;
static constexpr int kCodeLen = kDataLen + 1;

static int charValue(char c) {
    for (int i = 0; i < 32; ++i)
        if (kAlphabet[i] == c) return i;
    return -1;
}

std::string normalizeCode(const std::string& raw) {
    std::string s;
    for (char c : raw) {
        if (c == ' ' || c == '-' || c == '\t') continue;
        s += (char) std::toupper((unsigned char) c);
    }
    if (s.rfind("HUM", 0) == 0 && (int) s.size() > kCodeLen) s = s.substr(3);
    for (auto& c : s) {
        if (c == 'O') c = '0';
        if (c == 'I' || c == 'L') c = '1';
    }
    return s;
}

static char checkChar(const std::string& data) {
    int total = 0;
    for (int i = 0; i < (int) data.size(); ++i)
        total += charValue(data[(size_t) i]) * (2 * i + 3);
    return kAlphabet[((total % 32) + 32) % 32];
}

bool checksumOk(const std::string& raw) {
    const std::string s = normalizeCode(raw);
    if ((int) s.size() != kCodeLen) return false;
    for (char c : s)
        if (charValue(c) < 0) return false;
    return s[(size_t) kDataLen] == checkChar(s.substr(0, kDataLen));
}

std::string formatPretty(const std::string& normalized) {
    std::string out = "HUM";
    for (size_t i = 0; i < normalized.size(); i += 5)
        out += "-" + normalized.substr(i, 5);
    return out;
}

static bool decodeB64(const std::string& b64, juce::MemoryBlock& out) {
    juce::MemoryOutputStream mo;
    if (!juce::Base64::convertFromBase64(mo, juce::String(b64))) return false;
    out = mo.getMemoryBlock();
    return out.getSize() > 0;
}

License verifyLicense(const std::string& payloadB64, const std::string& sigB64,
                      const std::string& instanceId,
                      const unsigned char (*trustedKeys)[32], int numKeys) {
    License lic;
    juce::MemoryBlock payload, sig;
    if (!decodeB64(payloadB64, payload) || !decodeB64(sigB64, sig)) return lic;
    if (sig.getSize() != 64) return lic;

    bool signatureHolds = false;
    for (int k = 0; k < numKeys && !signatureHolds; ++k)
        signatureHolds = crypto_ed25519_check(
                             static_cast<const juce::uint8*>(sig.getData()),
                             trustedKeys[k],
                             static_cast<const juce::uint8*>(payload.getData()),
                             payload.getSize()) == 0;
    if (!signatureHolds) return lic;

    const auto v = juce::JSON::parse(payload.toString());
    if (!v.isObject()) return lic;
    const auto str = [&v](const char* key) {
        return v.getProperty(key, "").toString().toStdString();
    };
    lic.licenseId = str("license_id");
    lic.plan = str("plan");
    lic.name = str("name");
    lic.email = str("email");
    lic.instanceId = str("instance_id");
    lic.issuedAt = str("issued_at");

    lic.valid = !lic.licenseId.empty() && lic.instanceId == instanceId;
    return lic;
}

}
}
