# Humus

[humus.phobos-instruments.com](https://humus.phobos-instruments.com) · by Phobos Instruments

**Humus** is an interactive, patchable music environment - a live modular instrument that
is also a DAW. You wire *organisms* (instruments, effects, utilities) into a signal graph
on the canvas, play and morph the whole patch in real time, and capture the performance.

The source is here, and building it yourself is your right under the licence below.

## Building

You need a C++17 compiler, [CMake](https://cmake.org) 3.22 or newer,
[Ninja](https://ninja-build.org) and Python 3. The first configure needs a network
connection; nothing after it does. It fetches [JUCE](https://juce.com) and
[Ableton Link](https://github.com/Ableton/link), both pinned to a commit, and rebuilds
the hand-tracking model from Google's published MediaPipe bundle with
`tools/hand_model_convert.py` - download, convert, check the digest, all three pinned in
`engine/CMakeLists.txt`. If the model cannot be reached the configure warns and carries
on: hand tracking then does nothing where the system has no tracker of its own, and
everything else builds. Bytes that are not the model stop the configure instead.

### macOS and Linux

```bash
make build      # the app and the command-line tool
make run        # build it, then launch it in this terminal
```

`make` on its own lists everything it can do. It only wraps CMake, so this works just
as well:

```bash
cmake -S engine -B engine/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build engine/build
```

and then launch what it built:

```bash
open engine/build/hum_gui_artefacts/Release/Humus.app   # macOS
engine/build/hum_gui_artefacts/Release/Humus            # Linux
```

Linux also needs the usual JUCE development packages - ALSA, X11, FreeType, GTK3,
WebKit2GTK and libcurl.

The minimum on macOS is 11.0 (Big Sur). Building for Intel from an Apple Silicon Mac is
`make build ARCH=x86_64`, which keeps its own build tree. Running `make run` on macOS
attributes the microphone and camera permission to your terminal; open `Humus.app`
directly when you want Humus to own its own.

### Windows

Install Visual Studio's **Desktop development with C++** workload, which brings CMake and
Ninja with it, and run these from a *Developer Command Prompt* (not a plain one - the
compiler has to be on PATH):

```
cmake -S engine -B engine\build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build engine\build
engine\build\hum_gui_artefacts\Release\Humus.exe
```

If you would rather use the Visual Studio generator, drop `-G Ninja` - but then
`CMAKE_BUILD_TYPE` is ignored and you have to name the configuration when you build,
or you get a Debug app in a Debug folder:

```
cmake -S engine -B engine\build
cmake --build engine\build --config Release
```

Every platform also gets a command-line tool beside the app, for reading a patch and
rendering one offline without a display:

```bash
engine/build/hum info   patch.hum
engine/build/hum render patch.hum out.wav --seconds 10
```

First launch scans your plugins - this can take a few minutes and is cached afterwards.
Grant microphone and camera permission if you want live input or the camera organism.

## What is in here

```
engine/     the C++/JUCE application: audio graph, transport, GUI
sdk/        the organism SDK - one Organism type, capabilities, DSP helpers
packs/      organism packs: core, humus, av, and a fully commented sample pack
assets/     what ships to be used rather than compiled: patches, scales, shaders
```

Patches are UTF-8 XML `.hum` documents. To write your own organism or pack, start with
the commented example in [`packs/sample/README.md`](packs/sample/README.md); the SDK
reference and the user guide are at
[humus.phobos-instruments.com/docs](https://humus.phobos-instruments.com/docs/).

## License

Humus, copyright (C) 2026 Phobos Instruments. The source is at
[github.com/phobos-instruments/humus](https://github.com/phobos-instruments/humus).

It is free software under the [GNU AGPLv3](LICENSE) - fork it, learn from it, grow your
own; anything built on it stays open under the same terms. That includes the official
builds: there is no commercial edition, no dual license and no proprietary component. The
**Humus** name and logo are trademarks and stay with the project
([TRADEMARKS.md](TRADEMARKS.md)). Official signed builds, activation codes and updates are
how the project sustains itself - the AGPL expressly allows charging for copies, and the
code is not what you pay for. Contributions are AGPL, with a DCO sign-off on
each commit.

Fetched by the build, pinned to a commit: [Ableton Link](https://github.com/Ableton/link)
(GPLv2+) with standalone [Asio](https://think-async.com) (Boost Software License 1.0);
their licence texts and the note on what is taken ride in
`engine/third_party/ableton_link/`.

Vendored: [Monocypher](https://monocypher.org) (CC0/BSD-2), and the two FM engines behind
the pH synthesizer -
[music-synthesizer-for-android](https://github.com/google/music-synthesizer-for-android)
(Apache-2.0) and [Nuked-OPN2](https://github.com/nukeykt/Nuked-OPN2) (LGPL-2.1) - details
in `engine/third_party/*/README*.md`. pH also ships chip instrument banks by Vitaliy
Novichkov from [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) (MIT), see
`packs/humus/organisms/Ph/banks/README-HUMUS.md`. The Hands organism ships two hand models
from [MediaPipe](https://github.com/google-ai-edge/mediapipe) (Apache-2.0, per their model
card), converted and run by our own interpreter with no MediaPipe code vendored, see
[`assets/Models/hands/README.md`](assets/Models/hands/README.md).

Windows builds also vendor the [Steinberg ASIO SDK](https://www.steinberg.net/asiosdk)
(2.3.4) under the **GPLv3** arm of its dual licence - AGPLv3 §13 is what permits that
combination. This is the audio driver API, and despite the name it is entirely unrelated to
the Asio networking library above. ASIO is a trademark of Steinberg Media Technologies
GmbH; the GPL covers the SDK code, not the name or logo.
