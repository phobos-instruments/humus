# Monocypher (vendored)

Monocypher **4.0.2**, dual-licensed CC0 / BSD-2 — https://monocypher.org

Four files copied verbatim from the upstream tag (`src/` and
`src/optional/`): `monocypher.{c,h}` + `monocypher-ed25519.{c,h}`.

Humus uses exactly one thing from it: `crypto_ed25519_check()` from the
OPTIONAL ed25519 files — real RFC 8032 Ed25519 (SHA-512), interoperable
with the hub's Python `cryptography` signer. The core library's similarly
named `crypto_eddsa_check()` is EdDSA-with-BLAKE2b and is NOT compatible;
never switch to it.
