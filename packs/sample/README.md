# The Sample Pack - build your own organisms

This pack is the tutorial for the **Humus organism SDK**. It contains two
fully-commented organisms:

- **Tremolo** - the "hello world": ~50 lines of DSP showing the whole
  organism contract (parameters, the transport clock, block processing).
- **PingPong** - the "real world" example: reuses the SDK's `DelayLine` helper,
  tempo-syncs via rhythmic units, and shows feedback safety.

Build it, install the resulting `sample.humpack` through
**Edit ▸ Organism Manager ▸ Install pack…**, and both organisms appear in
the palette under their own **Sample** root. Uninstall removes them again -
patches that used them still load (the nodes become transparent pass-throughs).

## Anatomy of a pack

```
mypack/
├── pack.json                    # identity: id, name, version, origin
├── register.cpp                 # code: registerClass() per organism
├── entry.cpp                    # 3 lines of boilerplate -> the C ABI
├── CMakeLists.txt               # builds static lib + pack.so + .humpack
├── help/<Class>.txt             # the "?" button text per class
└── organisms/<Name>/
    ├── <Name>.h / <Name>.cpp    # the DSP (subclass hum::Organism)
    ├── organism.json         # metadata: class, category, params, editor
    └── editor.json              # declarative editor layout (optional)
```

**The rule of the system: code registers factories; manifests carry metadata.**
The host learns your parameter ranges, palette category and editor layout from
JSON - your C++ only makes sound.

## 1. The DSP class

Subclass `hum::Organism` (see `organisms/Tremolo/Tremolo.h`):

```cpp
class Tremolo : public hum::Organism {
    int numAudioInputs()  const override { return 2; }   // mono channel counts
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;   // allocate HERE
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const hum::Transport& t) override;
};
```

Ground rules for `process()` (it runs on the audio thread):

- **Never** allocate, lock, or touch files. Preallocate in `prepare()`.
- Read parameters with `params.get("Name", fallback)` - the host updates them
  between blocks; no synchronisation needed.
- The `Transport` is the musical clock: `samplesPerBeat()`, `beats()`,
  `playing()`, `rhythmicUnitToSamples("1/8")` for tempo-synced behaviour, and
  `liveInput(ch)` for the sound card's capture channels.
- Guard your inputs (`c < numIn && in[c]`) - inlets may be unconnected - and
  always fill every output channel.

DSP building blocks in `hum/dsp/` (dependency-free - both built-in packs are
made of them, and yours should reach here before writing DSP from scratch):
`DspMath` (dB↔linear, smoothing coefficients, T60 feedback, linked peak),
`Lfo` (phase-accumulator LFO with sine/tri/square), `EnvelopeFollower`
(attack/release follower), `DynamicsCore` (Compressor/Limiter/Gate gain
computers), `Biquad` (RBJ filters), `DelayLine` (fractional circular buffer),
`Interpolation` (Hermite/sinc), `BeatGrid`, `SoundFileBuffer` (file decoding),
`LiveWavWriter` (lock-free recording). The humus pack's **Primitives**
(LFO, Follower, Filter, VCA, Number) each wrap exactly one of these in ~40
lines - read them as minimal complete examples. Richer host features
(recorders, file transports, DJ decks) implement the interfaces in
`hum/Capabilities.h`.

## 2. The manifests

`organism.json` declares each class this folder provides:

```json
{ "classes": [ {
    "class": "Tremolo",
    "category": "Effects",
    "editor": "layout:editor.json",
    "params": [
      {"name": "Rate",  "min": 0.1, "max": 20.0, "def": 4.0},
      {"name": "Depth", "min": 0.0, "max": 1.0,  "def": 0.8},
      {"name": "Waveform", "min": 0, "max": 2, "def": 0, "enum": true},
      {"name": "Sync",  "min": 0, "max": 1, "def": 0, "bool": true} ] } ] }
```

Param flags: `bool`, `enum`, `range` (+ `defMax` - a two-handle morph range),
`isText` (+ `text` default - file paths, rhythmic units). One folder may
declare several classes (families) sharing the same sources.

`editor` picks the property editor: `layout:<file>` renders the declarative
`editor.json` (knobs, sliders, combos, toggles, file slots… - see
`hum/LayoutSpec.h` for the vocabulary); omit it for an auto-generated panel.

`gen:<id>` is the third option, for an editor whose shape depends on the class
name (a mixer whose columns follow its channel count) - a fixed `editor.json`
cannot express that. Register a layout provider (`Registry::registerLayoutProvider`)
that builds a `LayoutSpec` and returns `hum::toJson(spec)`; the pack's entry
macro exports it as `hum_pack_layout_json` automatically. See
`packs/core/layouts.cpp`. Your pack owns those editors - the host never needs to
know your class names, which is what lets a pack it doesn't link draw its own UI.

## 3. Registration + the C ABI

`register.cpp` is one function (also used when a pack is statically linked):

```cpp
void hum_register_pack_sample(hum::Registry& r) {
    r.registerClass("Tremolo",  [] { return std::make_unique<Tremolo>(); });
    r.registerClass("PingPong", [] { return std::make_unique<PingPong>(); });
}
```

`entry.cpp` turns that into a dynamic pack:

```cpp
#include "hum/PackEntryImpl.h"
#include "PackManifestJson.h"                              // generated
namespace hum { void hum_register_pack_sample(Registry&); }
HUM_DEFINE_PACK_ENTRY(hum::hum_register_pack_sample, kPackManifestJson)
```

That macro exports the four C symbols of the pack ABI (`hum/PackEntry.h`):
`hum_pack_abi`, `hum_pack_manifest_json`, `hum_pack_create`,
`hum_pack_destroy`. Packs must be built with a toolchain C++-ABI-compatible
with the host (Linux: GCC/Clang + libstdc++).

## 4. Building the .humpack

Copy this pack's `CMakeLists.txt` - it's three calls:

```cmake
add_library(hum_pack_mypack STATIC register.cpp ${SOURCES})
target_link_libraries(hum_pack_mypack PUBLIC hum_sdk)
hum_add_pack_dyn(mypack)      # -> bin/<platform>/pack.so
hum_pack_bundle(mypack)       # -> <build>/humpacks/mypack.humpack
```

From the repo root, `make pack` builds every bundle. A `.humpack` is a plain
zip: the manifests + help + `bin/<platform>/pack.so` (no sources).

## 5. Test it

Put a suite in `tests/` beside `organisms/` and the build picks it up. The
shape to copy: install the pack, check the schema resolves, render a block and
assert the tremolo actually modulates, then uninstall. Minimal loop:
`prepare()`, build
channel-pointer arrays, a local `Transport`, call `process()`, assert on the
buffers.

## 6. Publishing a pack (macOS)

A pack is a **dlopened binary**, so shipping one to other people has two
contracts to meet. Both are enforced - a pack that fails either is withheld by
the update manifest rather than offered and left to fail on load.

**The ABI contract.** `HUM_PACK_ABI` (`hum/PackEntry.h`) must match the host
*exactly*, and the pack must be built with a toolchain that is C++-ABI-compatible
with it (same compiler family + standard library) - capability discovery uses
`dynamic_cast` across the boundary, so keep default symbol visibility on the SDK
types. The ABI major bumps whenever a type crossing the boundary changes shape,
which invalidates every pack built against the old one. Publish per-ABI builds:
`versions.json` is keyed by `abi`, so an older host is simply offered the last
pack it can load instead of a broken one.

**Signing.** The build signs `pack.dylib` with `HUM_CODESIGN_IDENTITY`
(`"-"` = ad-hoc by default). Two things worth knowing:

* Apple Silicon refuses to execute unsigned code at all, so an arm64 pack MUST
  be signed - at minimum ad-hoc. x86_64 has no such rule, which is exactly why
  the Intel `pack.dylib` shipped unsigned for a while: the linker auto-signs
  arm64 and leaves x86_64 alone. Do not rely on that accident; the build now
  signs both.
* Your pack does **not** need to share Humus's Team ID, and does **not** need
  notarizing. Humus is signed with
  `com.apple.security.cs.disable-library-validation` (the entitlement every
  plugin host declares), so it will load a binary signed by anyone; and because
  `PackLoader` unzips the `.humpack` itself, the extracted binary never inherits
  `com.apple.quarantine` and Gatekeeper never assesses it. Signing with your own
  Developer ID is good practice, not a requirement.

**Getting listed.** Publish the `.humpack` files and a `versions.json` beside
them as release assets; the manager reads the manifest from the releases page,
and `latest/download/<asset>` is a stable URL onto the newest release, so an
update means uploading files and running no service.

```json
{
  "packs": [
    {
      "id": "yourpack",
      "version": "1.0.0",
      "abi": 1,
      "notes": "What changed, in one line - shown beside the update.",
      "platforms": {
        "darwin-arm64": "https://.../yourpack-1.0.0-darwin-arm64.humpack",
        "linux-x86_64": "https://.../yourpack-1.0.0-linux-x86_64.humpack"
      }
    }
  ]
}
```

`version` is dotted numeric and compared numerically, so `1.2.10` is newer than
`1.2.9`. `abi` and the `platforms` keys are both enforced - an entry failing
either is withheld rather than offered and left to fail on load
(`engine/src/core/PackUpdates.h`). Build one architecture at a time,
`make build ARCH=x86_64`, since a pack's binary lives under `bin/<tag>/` and the
host resolves that tag per slice.
