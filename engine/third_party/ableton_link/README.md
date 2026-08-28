# Ableton Link (vendored here, fetched by the distribution)

Ableton Link **4.0** (tag `Link-4.0`, commit
`e9a2e414d63f55f1aad158370b007a6fbdc1eeb9`) — https://github.com/Ableton/link

**License: GPLv2-or-later** (dual-licensed with a proprietary option from
Ableton). GPLv2+ is compatible with this repository's AGPLv3 — the combined
work is effectively AGPLv3. If Humus ever moves off the AGPL (e.g. a JUCE
commercial license), Link needs Ableton's proprietary license too
(link-devs@ableton.com). Full texts: `LICENSE-link.md`, `GNU-GPL-v2.0.md`.

Two trees, one target. **This** repository carries the headers beside this
file, so a build here needs no network and the exact code is reviewable in a
diff. The **source distribution** does not: `.gitattributes` drops `include/`
and `asio/`, and `engine/CMakeLists.txt` fetches them with FetchContent when
`include/ableton/Link.hpp` is missing, pinned to the commit above rather than
to the tag, so a moved tag cannot change what compiles.

The licence texts and this file ship either way. The obligation travels with
the binary we distribute, not with the copy we happen to keep on disk.

Standalone Asio (Boost Software License 1.0, `LICENSE-asio.txt`) arrives with
it as the `modules/asio-standalone` submodule, whose pin at that tag is commit
`231cb29bab30f82712fcd54faaea42424cc6e710`; `GIT_SUBMODULES` names it so the
fetch takes that one and no other.

Both include dirs plus the platform defines (`LINK_PLATFORM_MACOSX` etc.,
mirroring upstream `AbletonLinkConfig.cmake`) ride the `ableton_link`
INTERFACE target in `engine/CMakeLists.txt`. They go on as SYSTEM directories,
so upstream's warnings stay upstream's. The two layouts differ - vendored
flattens Asio to `asio/include`, upstream keeps it under
`modules/asio-standalone` - so the include dirs are chosen with the tree.

Building the distribution offline, or against a checkout of your own:
`-DFETCHCONTENT_SOURCE_DIR_ABLETONLINK=<path>`. `make dist-check` builds the
unpacked zip, which is what keeps the fetched half honest.

Gotchas:
- Constructing `ableton::Link` opens **no sockets**; the network starts only
  at `enable(true)`. Keep it that way — headless tools (`hum_snapshot`,
  `hum_tests`) may construct but must never enable.
- `Link.hpp` is a heavy include (drags in Asio). Only `gui/LinkSync.cpp`
  may include it; everything else talks to the `LinkSync` pimpl wrapper.
- On the audio thread use `captureAudioSessionState()` only (realtime-safe);
  `captureAppSessionState()`/`commitAppSessionState()` are for the message
  thread.
