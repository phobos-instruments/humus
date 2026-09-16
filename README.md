# Humus

**Humus** is an interactive, patchable music environment - a live modular instrument
inspired by nature. You wire _organisms_ (instruments, effects, utilities) into a
signal graph on the canvas, play and morph the whole patch in real time, and capture
the performance.

If Humus is useful to you, starring the repository helps other people find it.

- **Download** builds for macOS, Windows and Linux:
  [humus.phobos-instruments.com/downloads](https://humus.phobos-instruments.com/downloads)
- **Build** it from this source:
  [humus.phobos-instruments.com/docs/build](https://humus.phobos-instruments.com/docs/build)
- **Learn** it, and write your own organisms:
  [humus.phobos-instruments.com/docs](https://humus.phobos-instruments.com/docs/)

## What is in here

```
engine/     the C++/JUCE application: audio graph, transport, GUI
sdk/        the organism SDK - one Organism type, capabilities, DSP helpers
packs/      organism packs: core, humus, av, and a fully commented sample pack
assets/     what ships to be used rather than compiled: patches, scales, shaders
examples/   worked examples (File > Examples shows this tree): a sketch per scenario
```

## License

Humus, copyright (C) 2026 Gabriele Arcangelo Scalici (Phobos Instruments), is free
software under the [GNU AGPLv3](LICENSE). The **Humus** name and logo are trademarks
and stay with the project ([TRADEMARKS.md](TRADEMARKS.md)). Third-party components and
their licences are listed in [THIRD-PARTY.md](THIRD-PARTY.md).
