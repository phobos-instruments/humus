# Changelog

User-facing changes to Humus, newest first.

## [0.6.0] - 2026-11-24

- Added Humus as a plug-in: the whole patcher inside your DAW, VST3 and Audio Unit on the Mac and VST3 on Windows and Linux, with patch editing, Undo and Redo, faceplates, floating plug-in windows, MIDI in and out, offline bounces, video windows, and Settings for theme and language. It starts on Sound In to Sound Out with a MIDI In, loads from your Patches folder, and old sessions open as they were
- Added eight DAW Macros to the Humus plug-in, each driving several controls of the patch over its own range
- Added a plug-in download for every platform: one package with both formats on the Mac, an installer of its own on Windows, and a ticked box in the Windows app installer too
- Added CLAP plug-ins, in the app and inside the Humus plug-in, with MPE so every note bends and swells on its own, and AUv3 plug-ins on the Mac
- Added songs that switch patterns: automate a sequencer's Bank and the timeline shows A-H blocks you select, cut, move, copy and resize like clips; where the lane is empty, the bank buttons decide
- Added eight banks to PianoRoll, Steps, Atom and DNA, and chance on every step of Sequence, Riff, Steps and DNA and every ring of Atom: the same on each play and bounce, and mappable from MIDI. A % key sets it, unlikely steps draw fainter, a click on a dot silences that hit on Atom, and presets now keep their steps
- Added nature to Math: gravity, pendulum, heart, breath, swarm, wind and wander as functions, with rate, phase and presets. Cord a Math or a Wave into the LFO's new FN wave and it plays that shape, and Wave gains an env outlet
- Added an Organisms pane on the left: every organism and plug-in in one searchable tree, plug-ins sorted by format, kind and maker, each with its own mark, ready to drag onto the patcher
- Added quicker hands: Auto-arrange Selection, many cords picked at once with Shift-click or Alt-drag, Alt-click on a mute or solo to set them all, Cmd-click on a solo to hear only it, File > Autosave, and a + badge when a box will take the file you drag
- Added mapping everywhere: Parameter Control learns a plug-in's knob as you move it, Evolve takes MIDI and OSC, MidiIn and MIDI Tracks get a Channel, and Sequence can keep a row, or any velocity, out of Random
- Added small, useful things: parallel compression with Mix, faders on Mixer and Console, Forward and Ping-pong loops on the Sampler, a length per Leafcutter slice, sound on VideoTrack, and a Pencil and Repair Selection in the clip editor
- Added video previews, pause, pinned folders and Movies to the file browser, Move to Trash to the Library, and your computer's address and an OSC output to Settings; pH's help now explains loading M-VAVE FM-1 banks as they are
- Changed Evolve, noise() and MIDI Tracks to be predictable: Evolve keeps to what Random leaves out and the ranges it keeps to, noise() plays the same way every time, and a MIDI Track open in the editor plays only from its own port and channel
- Changed file previews to sit at the top of the right pane
- Fixed MIDI and OSC: OSC reads bundles and every value type, shares its port and says when it is taken; MIDI Learn and Parameter Control keep to their channel; a toggle pad catches its first press; the monitors keep up with bursts
- Fixed sound glitches: warped and transposed clips stop pulsing and doubling, the Harmonizer and Trellis follow bells, high notes and gliding pitch, Sample Lab sounds come through clean and no longer replace a Sampler's sounds, SoundSpace glides under a MIDI fader, and held automation stays held in command-line renders
- Fixed small snags: Kick's Tune follows its note, Save as User Preset selects the new one, long cords take a click between their bends, previews scrub without restarting and their fader works, a plug-in that changes its sidechain gets the right inlets, a lone pane fills its space, the transport buttons on Deck, FilePlayer and five more faceplates show in their real colour, middle C is C4, and no English is left in translated Settings, the setup wizard or the audio panel
- Fixed Windows and Linux: on Windows plug-in windows stay in their box and follow your drags, Humus quits cleanly after hosting its own plug-in, and Settings opens at once when an audio driver cannot start; on Linux the AppImage finds everything it carries, so organism windows, demos and examples open on every system, and Humus no longer picks a hidden Aux box for its Sound In or Sound Out
- Fixed Humus getting heavy: the interface rests when the music stops, and memory stays flat while you resize faceplates

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
