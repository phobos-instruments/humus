# Third-party components

Humus is under the [GNU AGPLv3](LICENSE). These components by other people are
fetched by its build or vendored in this tree, each under its own licence.

Fetched by the build, pinned to a commit: [Ableton Link](https://github.com/Ableton/link)
(GPLv2+) with standalone [Asio](https://think-async.com) (Boost Software License 1.0);
their licence texts and the note on what is taken ride in
`engine/third_party/ableton_link/`.

Vendored: [Monocypher](https://monocypher.org) (CC0/BSD-2), and the four FM engines behind
the pH synthesizer -
[music-synthesizer-for-android](https://github.com/google/music-synthesizer-for-android)
(Apache-2.0), [Nuked-OPN2](https://github.com/nukeykt/Nuked-OPN2),
[Nuked-OPM](https://github.com/nukeykt/Nuked-OPM) and
[Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3) (all LGPL-2.1), plus
[reSID](https://github.com/daglem/reSID) (GPL-2.0-or-later) behind the Silt
organism - details in `engine/third_party/*/README*.md`. pH also ships chip instrument
banks by Vitaliy Novichkov from [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI)
(MIT), see `packs/humus/organisms/Ph/banks/README-HUMUS.md`. The Hands and Skeleton
organisms run the hand and pose models from
[MediaPipe](https://github.com/google-ai-edge/mediapipe) (Apache-2.0, per their model
cards), converted and run by our own interpreter with no MediaPipe code vendored, see
[`packs/av/assets/Models/hands/README.md`](packs/av/assets/Models/hands/README.md) and
[`packs/av/assets/Models/body/README.md`](packs/av/assets/Models/body/README.md).

The Grit organism's Famicom sound - the Ricoh 2A03/2A07 APU and the VRC6,
VRC7, FDS, MMC5, N163 and Sunsoft 5B cartridge expansions - is
[NSFPlay](https://github.com/bbbradsmith/nsfplay)'s emulation (maintained by
Brad Smith, descending from Brezza's NSFPlug, distributed freely per its
readme; the embedded emu2413 and emu2149 cores are by Mitsutaka Okazaki),
vendored under `engine/third_party/nsfplay/` with its terms carried verbatim.

Video plays through each platform's own framework where it has one -
AVFoundation on macOS, Media Foundation on Windows - with
[FFmpeg](https://ffmpeg.org) behind it for what those will not open, and as
the only decoder on Linux. FFmpeg also reads the sound inside a film, which
no system audio reader does from a Matroska or an AVI.

The macOS and Windows builds carry the LGPL-2.1-or-later libraries only,
decode-only, with no encoder and nothing GPL; the Linux build adds x264, and
that one is GPL-2.0-or-later, because a bounce there has no system encoder to
use. Either way the libraries stay separable: the build is reproducible from
packaging/linux/ffmpeg-lite.sh, which pins every download by digest, and a
build with no FFmpeg at all plays HAP videos only and reads no film sound.

The Paulstretch organism is an in-house implementation of
[Paul's Extreme Sound Stretch](https://github.com/paulnasca/paulstretch_python)
by Nasca Octavian Paul, whose algorithm is released as public domain. The
PinkTrombone organism is a C++ port of
[Pink Trombone](https://dood.al/pinktrombone/) by Neil Thapen (MIT,
Copyright 2017 Neil Thapen; full notice in
`packs/humus/organisms/PinkTrombone/LICENSE-pink-trombone.txt`).

The Deck reads control records with the timecode decoder from
[xwax](https://xwax.org) by Mark Hills (**GPL-3.0-only**, vendored unmodified in
`engine/third_party/xwax`, whose README says why) - AGPLv3 §13 permits that combination,
the same as for the SDK below.

Windows builds also vendor the [Steinberg ASIO SDK](https://www.steinberg.net/asiosdk)
(2.3.4) under the **GPLv3** arm of its dual licence - AGPLv3 §13 is what permits that
combination. This is the audio driver API, and despite the name it is entirely unrelated to
the Asio networking library above. ASIO is a trademark of Steinberg Media Technologies
GmbH; the GPL covers the SDK code, not the name or logo.
