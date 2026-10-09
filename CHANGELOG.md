# Changelog

User-facing changes to Humus, newest first.

## [Unreleased]

## [0.5.1] - 2026-10-09

- Added multilanguage support: Italian, French and German
- Added a "What's New" popup after updates
- Added moving cords: drag a cord's end to another inlet, or Cmd-drag an outlet
- Added Shift-drag to wire every channel of two organisms at once, and Shift+Alt to fan one outlet across many inlets
- Added a choice of outputs for file previews in Audio settings, for a headphone cue
- Added Peak and timecode calibration to Deck
- Added copying text from Help, the tour and What's New
- Added multi-file playback: FilePlayer's Outputs menu makes it a 4- to 32-output player, one file row per output, multichannel files included
- Fixed a crash when choosing ASIO on Windows with an old driver installed
- Fixed interfaces with more than 32 channels: every output and input now plays
- Fixed AuxIn and AuxOut in imported patches: each is a stereo pair again (AuxOut1 on 3/4, AuxOut2 on 5/6), up to 127 pairs
- Fixed a crash when opening patches with some sample-based plug-ins
- Fixed audio dips while editing a patch, and crackles at small buffers
- Fixed a freeze at startup with an unresponsive audio interface on macOS
- Fixed timecode, loop and film playback on Deck
- Fixed smaller issues in the Metapad, Sampler, Leafcutter, Graft and organism search
- Changed the website address to humus.art

## [0.5.0] - 2026-09-27

- Added a file browser for the Library: places, tags, ratings, collections, search and audition
- Added a Projects view of every project and patch on your computer
- Added video on Deck: play, scratch and record sound and picture together
- Added a face for Helix, with tempo-following strands and Audio to Track
- Added MidiFilter and Firefly organisms
- Added File > History: snapshots of the patch, kept across restarts
- Added mapping kinds for MIDI controllers: knob, fader, button, pad and encoder
- Added Panic to stop hanging notes
- Added note and controller colours, and quantising, to the PianoRoll
- Added stage keyboard libraries to the Sampler, and picture skins for organisms
- Changed Rings to Atom, and redesigned the Console
- Changed the status line to notice cards that leave on their own
- Changed arming a MIDI track to put the keyboard on it
- Fixed clicks on bypass, solo and the master knob
- Fixed many Deck recording, video and Sampler loop issues
- Fixed a crash when quitting while a file loads

## [0.4.0] - 2026-09-14

- Added pitch bend on every instrument, with a bend lane in the PianoRoll
- Added the MidiPlayer organism: plays .mid scores, with .sf2 banks
- Added Board, and much richer SerialIn and SerialOut, for microcontrollers, including OSC over serial and Windows ports
- Added RNG, Siren, Var and Notes organisms
- Added control cords that carry numbers and settle within one audio block
- Added time signatures, with a meter you can automate
- Added maths in value fields: type /2 to halve a value
- Added painted skins for organisms
- Added detaching a box into its own window
- Added Wave tables from text files and dropped files, and a Brainwaves preset group
- Changed every organism's help page to one short, shared shape
- Changed saving to write beside the patch and then rename, so a crash never damages it
- Removed VCA (Gain does the same job, and old patches still load) and the Linux .deb package
- Fixed sandboxed plug-ins dying within a minute on macOS
- Fixed clicks and zipper noise on mute and gain
- Fixed slow routing in large patches
- Fixed many FileRecorder, PianoRoll and detached-window issues

## [0.3.0] - 2026-09-06

- Added video tracks on the timeline, and video recording to H.264
- Added Bounce to WAV, FLAC, Ogg Vorbis, MP3, .mp4 and .mid
- Added reels: merged video clips with a timeline of their own
- Added VideoFX, VideoPad and Button organisms
- Added pattern banks A to H on Riff and Sequence
- Added MIDI shift mappings that answer only while a button is held
- Added About Humus, Help > Report a Bug and Media Info
- Added a caution note on organisms that can hurt ears or speakers
- Changed Record Master Mix to Record Live Performance
- Fixed Open Recent's first entry, video effects in bounces, and silent note exports

## [0.2.1] - 2026-09-02

- Added video playback on Windows and Linux, and HAP video everywhere
- Added Hands and Skeleton as modulation sources
- Added Grit, Skeleton, Pink Trombone, Paulstretch, Harmonizer, Cluster, Gate and Slider organisms
- Added MIDI tracks with a destination
- Fixed Silt and Grit loudness, and made the macOS download smaller

## [0.2.0] - 2026-08-31

- Added Silt, a MOS SID instrument
- Added three more pH engines: OPM, OPP and OPL3
- Added Convert to MIDI Track on audio clips

## [0.1.5] - 2026-08-31

- Added the Humus icon to .hum files
- Fixed double-clicking a .hum file on Linux and Windows
- Fixed crackles while dragging a clip during playback

## [0.1.4] - 2026-08-31

- Added fade shapes in the timeline and the clip editor
- Added MIDI in and out for the plug-in inside a DAW
- Fixed fade handles and clip editor trimming

## [0.1.3] - 2026-08-30

- Added scrolling when dragging past the timeline edge
- Fixed Bluetooth MIDI pairing on macOS, and the update card text

## [0.1.2] - 2026-08-29

- Fixed the first window size and the tempo readout
- Fixed the update card offering the right Linux package

## [0.1.1] - 2026-08-28

- Added updates that download the build for your platform
- Added Follow: any parameter can mirror another
- Added five themes, one of them light
- Added .rx2 and .rex loops in Leafcutter
- Fixed text colours on light themes, and plug-in editors across patch changes

## [0.1.0] - 2026-08-28

- The first published build: a living patcher of organisms, with a timeline, recording, MIDI, OSC and video
- Instruments and effects from FM synthesis and samplers to loopers, decks and spectral tools
- Pods, presets, the Metapad, performance capture and an AI assistant
- Plug-in hosting, Humus as a plug-in, Ableton Link and microtonal tuning
