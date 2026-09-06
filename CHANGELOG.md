# Changelog

User-facing changes to Humus, newest first.

## [0.3.0] - 2026-09-06

### Added
- Video tracks on the timeline: cut, trim, move, loop, fade, warp, thumbnails
- Video recording onto a Video Track, encoded as H.264 while it records
- Bounce: sound as WAV, FLAC, Ogg Vorbis or MP3, picture as H.264 .mp4,
  notes as .mid, for the song, the loop, a selection or a bar count
- Bounce dialog with size and time estimates and a native progress window
- Reels: merged video clips that open as their own timeline
- VideoFX, VideoPad and Button organisms
- Crossfader cut buttons
- Follower gate outlet, MIDI outlet, and a panel with a threshold meter
- SideKick playhead, dB scale, shift arrows, reverse and invert
- MIDI shift combos: mappings that answer only while a button is held
- Riff and Sequence pattern banks A to H
- Sequence master MIDI outlet
- Acid filter and output stages reworked, five new controls, seven presets
- Media Info on audio and video clips
- About Humus window
- Start here column on the start window
- Reading Humus help section, with a page on the organism families
- Help > Report a Bug
- Cut at Playhead on the timeline
- Timeline thumbnails kept on disk, with a Video page in Settings
- Interface strings can be translated
- A caution note on organisms that can hurt ears or speakers

### Changed
- Record Master Mix is called Record Live Performance
- New audio tracks arrive without a cord to the master
- Convert to MIDI Track runs in the background, onto a plain MIDI track
- Larger timeline lettering; the tempo readout shows its unit in full
- The timeline follows locates and jumps; the loop band is easier to grab
- Video Out and VideoPad panels scale with their window
- VideoPad Fade defaults to 0; pad drags copy, Alt-drag moves
- Insert Before and Insert After splice video and MIDI cords too
- Video decoding notes only print in a debug build

### Fixed
- Open Recent's first entry opened nothing
- The automation box after looping while recording stopped short
- VideoPad clips lost inside a pod after save and load
- Levels and envelope stages no longer roll with the dice; Riff rolls stay
  mid-lane
- Video effects apply in previews and bounces, not only in the output window
- A sound bounce into a movie no longer stalls
- On macOS: bounce start, filmstrip thumbnails, and dropping a movie
- Notes written straight onto an instrument were silent in an export
- Grit plays at the same loudness on every platform
- VideoPad In frame, playhead, cut kept across pad loads, drags in floating
  windows
- Remove track on a video track removes it

## [0.2.1] - 2026-09-02

### Added
- Video playback on Windows and Linux, with the system decoders or a bundled one
- HAP video playback on every platform
- VideoPlayer transport: play, pause, rewind, speed, floating video window
- Hands and Skeleton as modulation sources, and both take video in
- Trackers run on the Apple silicon neural accelerator
- One video renderer for every view, with a frame-rate readout
- Grit, Skeleton, Pink Trombone, Paulstretch, Harmonizer, Cluster, Gate
  and Slider organisms
- Paulstretch on timeline clips
- MIDI tracks with a destination chip, no instrument of their own

### Changed
- VideoPlayer loses the Wear knob

### Fixed
- Silt and Grit loudness calibrated to the other instruments
- A loaded tape rolls in its editor without a Video Out
- Smaller macOS download

## [0.2.0] - 2026-08-31

### Added
- Silt, the MOS SID instrument, with Twin and Chain modes
- pH grows to five engines: OPM, OPP and OPL3 join; .opm and .wopl banks
- Convert to MIDI Track on audio clips

## [0.1.5] - 2026-08-31

### Added
- .hum files carry the Humus icon on every platform

### Fixed
- Double-clicking a .hum file opens it on Linux and Windows
- Hand model conversion drift
- Dragging a clip while playing no longer crackles
- Playhead on an empty song, missing-file clips say so, track selection

## [0.1.4] - 2026-08-31

### Added
- Fade shapes and hand-bent fades, in the timeline and the clip editor
- Tooltips on clip handles; channel pots name what is plugged in
- The plugin takes and sends MIDI in a DAW

### Fixed
- Fade handles, marks and cursors
- Clip editor start trim, wheel step, zoom limits, window switcher icon

## [0.1.3] - 2026-08-30

### Added
- Dragging past the timeline edge scrolls the view

### Fixed
- Group drags, drag-out to the desktop, bypass bar redraw
- Bluetooth MIDI pairing permission on macOS
- Update card text, download page shows the current build

## [0.1.2] - 2026-08-29

### Fixed
- First window size, Linux minimum size
- Update card names the file; .deb installs are offered a .deb
- Tempo readout width

## [0.1.1] - 2026-08-28

### Added
- Update card downloads the build for your platform
- Follow: any parameter can mirror another
- DNA step mute; Mineral facets and five presets
- Sliced-loop files (.rx2 and .rex) in Leafcutter
- Cmd+N and Cmd+O

### Changed
- Five themes, one of them light
- Modulate with and Follow are separate menu entries

### Fixed
- Text colours on light themes; theme changes reach every control
- Route rings clear at once; metapad snapshots no longer ring every knob
- Plugin editors survive patch changes; plugins with their own window float
- Properties pane scrolling

## [0.1.0] - 2026-08-28

The first published build. What grew before it, by fortnight.

### 2026-08-16 to 2026-08-28
- pH, an FM instrument with two engines, 885 factory sounds, operator matrix
- Sampler reads .sf2 banks, records inputs, three layers of presets
- Helix, a four-strand live looper with direct outs
- Crossover, a 2 to 5 band splitter; Hollow, a convolution reverb
- DigiRust, a lo-fi channel; Halogen; Spark; Wave grows into a synth
- Math and Formula: an expression as an organism
- Clip editor: trim, slip, split, transients, reverse, normalise, warp, pitch
- Timeline rework: one surface, folder tracks as pods, take lanes, track mode,
  envelopes, consolidate, MIDI to Track, merge, edit tools
- Touch and Latch capture; armed tracks record notes; ghost clips while recording
- Swing on every sequencer and a patch-wide groove
- Presets as files, user banks in Documents, a preset rail on every box
- User Library in Documents/Humus; assets referenced by name
- JACK on Linux, ASIO on Windows, a master safety limiter, Linux cameras
- Leafcutter chops loops at transients; timecode vinyl control
- Humus as a plugin: a pod you can see and play
- Meet Humus tour, Cmd+N, Cmd+O, Cmd+W, first-boot reset
- Icons redrawn as one set; the toolbar and the left rail
- The download door, the source zip, a changelog, one version source

### 2026-08-01 to 2026-08-15
- Ravine, Morse, Drops, Cicada, Bloom, Spectrum, VuMeter, Send
- Acid grows a diode ladder, accent and performance mod
- Console with per-channel mute, solo and pan; Crossfader twins
- Organism families, faces and textures; bypass and random on every bar
- The global die; per-organism settings history; the dice on one knob
- Tempo on an automation lane; tap tempo; wall time on the clock
- Modulated knobs move and wear a ring; every button answers the mapping menu
- Microdot's roll is a step grid; Morse keys MIDI
- Factory presets per class; the field guide

### 2026-07-16 to 2026-07-31
- Tuning: a Tuning organism, scales in cents, a 4,500-scale browser, Rhizome
- Hosted plugins follow the tuning
- Ableton Link
- Video: shader scenes, ISF files, video cords, VideoPlayer, VideoMix, VideoOut
- Modulation routes: Follower, LFO and camera onto any parameter
- Automation without arming, performance boxes, re-record
- Timeline feel: fullscreen, 2D scroll, pinch zoom, edit tools
- The create picker replaces the palette tree; setup wizard; Meet Humus
- Hub: updates, licensing, telemetry with consent
- Help browser, Notes pad, Parameter Control tab, Document Switcher
- Wave, Prism, Phono; DNA rates and scales; sigils
- Metasurface becomes the Metapad
- .hum patches; a macOS DMG; Windows and Linux packaging

### 2026-07-01 to 2026-07-15
- Organism packs with a C ABI, a pack loader and an SDK; editor blueprints
- VST3, AU and LV2 hosting, out of process, with delay compensation and quarantine
- Tracks arranger with piano-roll tools, clip colours, automation lanes
- Pods: subpatches on the canvas, with Bypass and Insert Before/After
- AI assistant, preset genie, sample lab, with a local or hosted model
- Themes, UI scale, named user themes
- MIDI device setup, MIDI Learn, OSC input, MIDI clock, Bluetooth MIDI
- Recording: AudioTrack, one Record button, waveform overviews
- Performance capture: gestures, sessions, morph paths, retroactive keep
- Metasurface with natural-neighbour interpolation
- Spectral organisms, StereoTool, SideChain, SideKick, Repeater, TransientShaper
- Tribe: Kick, Acid, Riff, Steps, Sequence
- Substrate, Mineral, DNA; SoundSpace; Sampler; Hands with learnable gestures
- ValveFilter, Console, Decomposer, Trellis, OscMonitor, MidiMonitor
- Random roll on every organism; quick-add search; auto-arrange; flow lights
- Settings hub; Parameter Control window; mapping curves; game controllers
- Renamed to Humus

### 2026-06-17 to 2026-06-30
- First patcher: contraptions, cords, undo, settings, presets, patches
- Automation sequencer; file player; drums; mixer and crossfaders
- Deck with scrubbing and vinyl physics
- Categories, more contraptions, the first metasurface
