# Changelog

User-facing changes to Humus, newest first.

## [0.4.0] - 2026-09-14

### Added
- Pitch bend. Every instrument follows the wheel now, each with a Bend Range of
  its own (two semitones unless you say otherwise), pH on all four of its
  engines, and MidiPlayer follows the bends written into a file
- A pitch bend lane in the piano roll, beside velocity and the controllers;
  it plays back from the timeline too, and a bounced .mid carries it as wheel
  data
- Quantise on a MIDI clip's right-click menu
- Resize handles on the selected clip in track mode, on the clip bar above the
  notes, where the clip's name lives now
- Delete removes the selected track, and the track that takes its place is the
  one selected next
- A notice at the bottom right when a MIDI or audio device appears or goes,
  with a button to the settings page for it, or Ignore
- The HUMUS wordmark under the sigil, traced from a friend's hand-drawn type
- A Number holds any number now, not just the small ones. Value runs to a
  billion either way, and Format decides the shape it leaves in: Free sends what
  it holds, Whole rounds it, and the four widths round and then hold it inside
  them, a byte signed or not, two bytes signed or not. The box shows what
  actually leaves rather than what you typed, and it drags the way a number box
  should, a fixed step per pixel instead of a slice of the range, so it stays
  precise whether it is holding 3 or 30000. Integer is gone, replaced by Format,
  and the face carries one value rather than a box and a readout saying the same
  thing. Hovering the Format list says what each one means and where it runs
- A Slider's Min, Max, Log and Slew do what the panel has always said they do.
  They were inert: the mapped value was computed every block and read by nobody,
  and the outlet sent the raw handle position. Now the outlet sends what the face
  reads, and declares the range it is in, so both ends still land - a knob follows
  the position across its own travel, and a Number wired to the same outlet reads
  the number rather than the position. That is what makes a Slider worth putting
  in the middle of a cord when something needs scaling on its way somewhere. Slew
  glides the value in milliseconds, and a Slider left at 0 and 1 behaves exactly
  as it did
- A control cord onto a Number or a Var carries the number itself, the way a
  number box in the patching tradition always has. Wire an RNG drawing bytes to
  a Number and the Number reads 173, not 0.678; wire one Number to another and
  the second reads what the first reads. A cord you have given a range in the
  Control rows still maps between its two ends, so anything deliberately scaled
  keeps working; leave the range empty and it carries. Sockets opt into this in
  their own manifest, so any organism can hold a number rather than a position
- A control outlet can declare the range its value is in, and a cord reads it
  against that range. Before this, an outlet carrying anything above one pinned
  every cord it fed at full scale, so a Number set to a byte width sent the same
  thing at 10 as it did at 200. Outlets that speak the plain nought to one, which
  is all of them until now, are unchanged
- An RNG organism: a source of numbers nobody chose. Every draw comes from the
  machine's own entropy rather than a formula, and leaves as a plain 0 to 1 on a
  control cord, onto any socket in the patch or into a Number to be held and
  passed on. The cord decides what the draw becomes: it carries the range, a
  switch shape turns the draw into a coin, and smoothing makes a new number glide
  instead of jump. A readout shows the draw and a lamp strikes on each one, so
  the rate is something you watch rather than infer. Rate draws on a clock and
  Trigger draws on demand. Sync hands the draw clock to the transport, so a
  number lands every quarter beat or every four bars and follows the tempo when
  it moves; off, Rate is plain hertz and keeps its own time. Format decides what kind of number a
  draw is: 0 to 1, a Coin, or a whole number drawn across one of four widths, and
  a drawn width still reads correctly on a cord. A Number names the same six
  shapes from the other side, so where it says Any and Whole, meaning hold this
  and round it, RNG says 0 to 1 and Coin, meaning draw one of these
- Drag a `.mid` onto the timeline and it becomes a clip. Drop it on a MIDI
  track and it lands there; drop it anywhere else and it brings its own track
  with it. Every track of the file comes in, at the bar the file put it, and
  the clip takes the file's name
- MidiPlayer takes a SoundFont. Point its bank slot at a `.sf2` and it plays
  recorded instruments instead of the chips, layered zones, loops, panning and
  envelopes off the bank, with no per-chip voice limit; point it back at a
  `.wopn` and the chips return. The engine follows the file, so there is no
  mode to set. No SoundFont ships - they are tens to hundreds of megabytes of
  someone else's recordings - so the chip bank remains what plays out of the box
- A MidiPlayer organism. Drop in a score someone else wrote - `.mid`, `.midi`,
  `.kar`, or `.mus`, the compact format the early first-person shooters shipped
  their music in - and it sounds all sixteen channels itself, out of its own
  audio outlets, with nothing else to wire. The voices are frequency modulation
  off a General MIDI bank that ships with it, which is a few kilobytes of
  instrument definitions rather than a library of recordings, so it works the
  moment it is dropped in and sounds like the sound card it is descended from.
  Follow the transport or keep the tempo the piece was written at; one to four
  chips, from the six voices the hardware had to twenty-four; and each channel
  can be muted or pointed at a different instrument than the file asks for
- Any value you can type now takes an operation as well as a number. `/2` halves the
  tempo, `*2` doubles it, `x1.5` scales a gain, in the transport's tempo box, on a
  knob's number and in the box a double-click opens. A plain number is still absolute,
  and a leading minus still means a negative value rather than a subtraction, so
  nothing you could type before means something different now
- Each kind of file remembers its own folder. Patches open in Patches, bounces and
  takes in Recordings, presets in Presets, and each reopens where you last put that
  kind of file. Bouncing to the desktop used to move the patch save dialog there too,
  because both were reading the same remembered folder
- Replacing a box with a different organism renames it, if you never named it
  yourself. A MidiIn you swapped for a DNA stayed called "MidiIn"; the first box of a
  kind is named plainly, and only a name with a number after it was recognised as
  ours to change. A name you chose - MIDI_GENERATOR - still survives a replace, and
  swapping one variant of an organism for another, a three-band Crossover for a
  five-band, keeps the name it had
- A recorder's file slot no longer calls a take "missing" before it has
  happened. The slot marked any name with no file behind it as lost media,
  which is right for something you are about to play and backwards for
  something you are about to write; it now says where the take will go
- A file slot has an arrow beside its folder and x buttons that opens the
  folder the file is in, which is how you get at a take you did not name
- A FileRecorder's meters read. The lanes were asking the organism for levels
  and it had none to give, so they sat dark; it measures its inlets now, take
  or no take, and doubles as a monitor while you set levels
- A FileRecorder shows what it is recording. Each track's row carries a level
  meter for the channels that track claims, a strip of peak history that
  scrolls while the take rolls and clears when the next one starts, and the
  elapsed time. Widening a track moves the next lane's channels along with it
- Bouncing to MP3 offers a bitrate: 245, 190, 165 or 130 kbit/s, beside the
  format and only when MP3 is chosen, with the estimated file size following
  the choice. It was pinned at the largest MP3 there is, which is a strange
  place to leave the one format people pick because the file has to be small
- Skins: an organism can wear a painted faceplate of its own, drawn behind its
  controls. A blueprint asks for one by name and everything else looks as it
  did. FilePlayer is the first to try it, as a cassette deck
- Wave seeds a table from more than sound: a text file of numbers (a scope
  export, a spreadsheet column), a biosignal recording in the exchange
  format or its 24-bit sibling, and a circuit simulator's raw dump. A
  recording is cut into frames along its length rather than pitch-hunted,
  and the display says what the table came from and from which channel
- Wave's display takes a dropped file: drag one onto the wave and it
  seeds the table, the frame lighting up while the file is over it
- Wave's Brainwaves preset group: twelve cycles drawn from formulas, four
  evoked shapes, four rhythms and four named shapes
- A click or a drag on the ruler lands the playhead on the snap grid like a
  clip or a point would; alt places it freely, and snap off leaves it free
- Before recording: right-click the record button for Start right away,
  Count-in of one, two or four bars of click, or Pre-roll of one, two or
  four bars of the song played back before the playhead with recording
  punching in on the spot; pre-roll with nothing before the playhead
  counts in instead. Count-in follows the meter, and applies to
  recording rather than every Play (UNTESTED BY HAND)
- File > Examples: the worked examples are a tree at the root of the
  source, one folder per scenario under Organisms with the sketch beside
  the patches that drive it, shipped with the app and mirrored as menus,
  so a scenario is a click away and a sketch is where the board's editor
  wants it
- Notes, a sticky note for the canvas: a box with no pins that holds a
  few lines of text, renamed for a headline, saved with the patch; the
  comment of the patching tradition (UNTESTED BY HAND)
- The organism picker takes the arrow keys, Return and Escape whether or
  not the search box has focus, remembers the row you opened at each
  level so going back lands on it, and scrolls a selected row's
  description sideways after a moment when it is too long to read
- The organism picker lists rows, one box per line: the grown mark each
  Humus box wears on the canvas sits on its family soil at the left, the
  name and a one-line description follow, the category tag sits at the
  right, and a plugin keeps its monogram tile. The description is the
  class's new "blurb" in its manifest when it has one and the first
  sentence of its help otherwise; the core pack's boxes carry blurbs
  (UNTESTED BY HAND)
- A tempo change made by hand while the transport rolls is captured like
  any knob: dragging or tapping the tempo during Capture writes the
  Clock's Tempo lane, and Keep Last Bars keeps it too; at rest the tempo
  box still just sets the tempo (UNTESTED BY HAND)
- Pods carry control: a Control inlet port is a socket on the pod's box
  whose value appears on the port's outlet inside, a Control outlet port
  is the mirror, the pod canvas menu and the port editor offer both, and a
  control cord drawn onto a pod box is drawn at every scope it crosses.
  Every pod port, mono, stereo, MIDI, video and control, now lives in one
  Pod category instead of the mono pair under Input/Output (UNTESTED BY
  HAND)
- Halogen has a video inlet: a video cord from any video box, a camera
  included, makes the moving picture the field under the scan head, frame
  by frame, and the File comes back when the cord goes (UNTESTED BY HAND)
- Cmd+R / Ctrl+R captures a performance, the same as the round record
  button; renaming a box moved to F2
- Missing media is said out loud: a patch whose sounds, banks, images or
  timeline clips are not where they were opens with a Missing media window
  listing them box by box, the file boxes show "missing:" in amber, Locate
  finds one file and with it every other that moved along, same folder or
  subfolder, and File > Locate Missing Media brings the window back; the
  patch used to open silent with no word (UNTESTED BY HAND)
- Automation lanes draw the bar and beat grid across their body, in the
  arranger and in the automation editor a double-click opens, so a point
  can be placed against the bars rather than the ruler alone
- Automation points follow the Snap chip: a click on empty lane space adds a
  point on the grid (the click that only started a selection before), a
  drag lands on it, a ruled line snaps both ends, a chosen grid wins over
  the zoom's, alt while dragging places freely, and the pencil stays
  freehand
- Hovering or dragging a point in an automation lane shows a readout beside
  it: the value in the lane's unit (a range lane shows both ends) and the
  bar:beat it sits on; double-clicking a point opens a popup to type both,
  the position as bar:beat and the value in the lane's unit (UNTESTED BY
  HAND)
- The time signature is a real meter now: the chip beside the tempo offers
  the common signatures and a Custom entry for any other, 13/16 or 4/1
  included, the beat unit counts (a bar of 7/8 is seven eighths long), and
  the ruler, grid, snapping, bar-beat clock, metronome, Mark beat 1, loop
  defaults, bar-synced organisms, the Ableton Link quantum and MIDI export
  all follow it. "Automate the meter" on the same chip adds two held lanes on
  the Clock, Meter beats and Meter unit, so a song can change meter at any
  bar, with the ruler labelling each change (UNTESTED BY HAND)
- Board, an organism that talks to a microcontroller board running the
  standard Firmata sketch with nothing to program: up to six outlets each
  read an analog channel as 0..1 and double as Control-with sources, up to
  six sockets switch or dim pins of your choosing, Outlets and Inlets set
  how many the box shows; presets for the Arduino Uno and Nano, Leonardo
  and Micro, Mega 2560, an ESP32 dev board and a Raspberry Pi Pico; two
  example patches walk a sensor into a filter and the microphone into an
  LED (UNTESTED BY HAND against a real board)
- SerialOut speaks whatever the sketch at the other end expects: as many
  value sockets as its Values field says, two to eight, a trigger socket in
  On trigger mode, a Format field where %1 to %8 print the values with \n,
  \t and \xNN escapes, a Scale and an Int per value, and Send as
  Continuous, On change or On trigger (UNTESTED BY HAND against a real
  board)
- SerialIn reads up to eight numbers per line onto outlets a to h, takes a
  Parse template that is the mirror of SerialOut's Format ("T=%1;L=%2"
  reads what the sketch prints and ignores lines that do not fit), and shows
  what arrived in a readout in its box
- Serial talks in bytes as well as text: %b1 and %w1 in a Format send a
  value as one raw byte or a two-byte word, the same in a Parse reads them,
  and a Parse with bytes in it frames packets by their header and resyncs
  past noise; the old text-or-raw switch is gone
- A Format computes: %(v1*180) prints an expression in the Math box's
  language, %b(expr) and %w(expr) send it as bytes, and %?(v1>0.5){...}
  prints its braces only when the test is true, so a message can change
  shape with a value
- The worked examples live in their own Examples folder beside the demo
  patches, shipped with the source and opened with File > Open; the start
  window lists only the demo patches. Everything that wants a board sits
  under examples/arduino, one subfolder per sketch with the sketch beside
  the patches that drive it
- A NeoPixel folder: NeoPixel.ino drives a 24-LED ring from one command
  per line (paint a pixel, light the first n, paint all, paint all by hue,
  brightness), paint.hum picks a pixel and its colour with four sliders,
  vu-meter.hum lights the ring from the microphone's level with the colour
  computed in the Format, spectrum.hum turns bass, mids and highs into red,
  green and blue through a Crossover and three Followers (UNTESTED BY HAND
  against a real ring)
- A serial self-test pair, SerialTest.ino and serial-test.hum: the sketch
  prints a ramp, an echo of what it last heard and A0, the patch shows all
  three on Numbers, feeds the echo from an LFO and lights the board's
  built-in LED from a Button, so the link is proven before any wiring
  (UNTESTED BY HAND against a real board)
- A Reconnect button on Board, SerialIn and SerialOut closes the port and
  opens it again for every box sharing it, resetting the board on the way
  when Reset is on
- Every serial box shows at its bottom which port it is on and the last
  line it sent or received with every byte spelled out, SerialIn marks a
  line that did not fit its Parse as skipped, Board says whether it is
  looking, waiting, booting or talking, and SerialOut and SerialIn carry
  presets for the usual message shapes
- Var, a value with a name: %n1 in a SerialOut's Format prints the name
  of the Var on socket 1 next to its value, and a SerialIn reading a line
  like "temp 23.5" hands the number to the Var called temp with no cord,
  wherever it sits
- A SerialOut holds several messages: one per line in its Format field, and
  a Message socket picks which line goes out, so a Number or a Slider
  chooses between commands and a Button sends the chosen one
- Format and Parse cover the protocols that need more than a number in a
  fixed place: %x %s %X %S add or verify an XOR or sum checksum, raw or as
  hex, over the range marked by %[ and %]; %{slow,fast,turbo}1 sends one
  word out of a list picked by a value and reads back as its position;
  %c and %b* or %w* send and read a count byte followed by that many bytes
  or words; %* prints or takes every number on a line
- With a Parse template set, SerialIn grows a match outlet that fires
  whenever a line or packet fits, so a sketch that prints "pressed" can
  press a Button
- OSC over serial: a board sending SLIP-framed OSC over USB joins the OSC
  control bus from Settings, MIDI & OSC, so OSC Learn, curves and the
  monitor apply to it, and OSC values sent out go over the same port
  (UNTESTED BY HAND against a real board)
- SerialIn and SerialOut take a Frame setting, 8N1, 8E1, 8O1 or 8N2, for
  the gear that is not 8N1, a baud list that runs up to 1000000 plus a
  Custom rate for anything else, and a Reset switch that decides whether
  opening the port reboots the board (UNTESTED BY HAND on a real device)
- A Boards & sensors guide page walking the Board stories, the sketch of
  your own and the pixel ring (UNTESTED BY HAND against a real board)
- Any number of SerialIn and SerialOut boxes share one serial port
- Serial devices work on Windows: SerialIn and SerialOut open COM ports
  there (UNTESTED BY HAND on Windows)
- Control-with routes run inside the audio engine, once per block with a
  ramp, instead of thirty times a second on the window: a Follower ducking a
  Gain through a route no longer steps, routes keep working when the window
  is busy or in the background, and they work in the plug-in, in a bounce
  and in an offline render, where they did nothing before
- Cords carry sound, numbers land on knobs: Number, Slider, Button, LFO,
  SerialIn and SerialOut have no audio pins any more, and Follower keeps only
  its audio inlet. Their values leave through control outlets, and a knob
  that wants a control signal shows it as a socket - Gain's Gain, Math's X
  Y Z W, an LFO's Rate, a Number's Value, SerialOut's values. A control cord
  from an outlet to a socket is a Control-with route drawn on the canvas, in
  its own colour, with the knob's range; it cannot reach an audio inlet, so a
  Number into SoundOut is impossible by construction
- A chain of control boxes settles within one block: a route's source runs
  before the box it drives, so Button into Number into Slider lands in the
  same block as the press
- Magnetic cords work for control too: drop a Follower or a Number just
  above a box with sockets and its outlets snap onto the free sockets, the
  way audio, MIDI and video pins already did
- Number is one value: the inlet sets it, the box shows it, the outlet sends
  it; the old sum of inlet and Value is gone
- Sig, the one bridge back: a socket in, a ramped audio signal out, for a
  number that has to be a signal - into Math's a or b, or a control-voltage
  output
- VCA is gone; a Gain with its socket does the same job. Old patches load:
  a VCA becomes a Gain, a cord into its control inlet becomes a route onto
  Gain, a cord from a primitive into an audio inlet is dropped, and the
  status line and the log say what changed
- SoundOut has a DC guard, on by default: a held offset from a control
  signal becomes a click and then silence instead of a speaker cone pushed
  to one side; switch it off on an output that feeds control voltage
- Number shows its live value in its box and is a Control-with source: wire
  a sensor, a Follower or an LFO into a Number to watch it there, then route
  the Number onto any knob without a cord each
- A box detaches into its own window from a button in its title bar, stays
  above the main window, and comes back when its window closes; the patch
  remembers it
- Math formulas can be several statements with named memory: lx := lx + dt*(ly - lx)
  keeps lx between samples, so strange attractors, coupled oscillators and
  filters fit on the line; dt is the sample period; a Lorenz voice that
  plays from the keyboard and a Lorenz Drift modulator
- Siren, a sound-system siren: two pitches, an up and a down time, four sweep
  shapes, three waves, a tone cap, a hold button and a manual wail

### Changed
- Every organism's help page was rewritten to the same short shape: what it
  is, what it does, its parameters with the reason each one exists, a recipe,
  and its relatives. The in-app help shows the parameters as a table, the way
  the website does
- Halogen's window is X, Y, Width and Height, its band Lowest and Highest,
  Rate sweeps up to a hundred times a second, and Blur softens the picture
  itself, up to ten pixels each way, instead of smoothing partials over
  time; saved patches keep the old names and lose those settings
- The UI scale slider is gone from Settings and the setup wizard: it
  scaled the drawing but not the windows, plugin editors or video views
  around it, so content got cut off; a stored scale is ignored
- Opacity is named too: 303 hand-written withAlpha calls in 40 distinct
  values snap to eleven hum::alpha steps, so a dimmed label is the same
  dimmed everywhere; 157 of them move by 0.05 or less
- The family accents and the MIDI and video cord colours are computed
  once when the palette changes instead of per paint
- Every colour the app paints that does not follow the theme is now named
  in one file, engine/src/gui/Colours.h, and a lint fails a hex colour
  written anywhere else; four near-identical shades merged into the one
  they were copies of, so the transport meter's peak and mid, the gain
  reduction bar and the looper's record light shift by a hair
- The modern picker's rows are a touch smaller again, between the first
  cut and the enlarged one
- Help lists one row per page: the ten pod ports collapse to PodIn and
  PodOut, the two pages they always shared
- Each help page names the family whose colour the node wears, under the
  title and in that family's colour
- Gain has no control socket any more; the one it carried was a leftover
  of the VCA it replaced, and a control still reaches its knob through
  Control with
- The pack boundary is frozen: every block type a pack reads carries a
  reserved extension slot, so future host features reach packs without
  breaking ones already built; an add-on installed before this release is
  refused with a message until its rebuilt bundle is offered
- Pack updates are read from the downloads server's packs folder, where each
  bundle carries its own ABI and platform, instead of a release page that was
  never published
- Wave's Drive runs at four times the sample rate and the valve filter's
  tube stage at twice, so hard settings no longer fold their harmonics back
  as the fizz that moved the wrong way up the keyboard; the valve filter
  reports the sixteen samples this adds and the graph compensates for them
- Console: Sag sits at 0.1 by default instead of 0.35, so a mix through the
  desk loses about three decibels at -6 dBFS rather than seven and a half; the
  Sag, Drive, Crosstalk and Output knobs ramp over 30 ms instead of stepping
  once per block, which is what crackled when they were dragged
- A pack built against a different SDK layout is refused with a message that
  says so, instead of loading and misreading memory; the pack binary no longer
  carries a second copy of its manifest
- Looper, mixer console, drum machine, step sequencer, crossover and the FM
  synth read their per-strip and per-operator parameters by cached slot
  instead of building the name every block
- Rack mode stacks every box at full width, a hair apart; boxes move with the
  arrows in their title bar instead of by dragging, and free mode has an
  Arrange button that lays the boxes out in rows

### Removed
- The Linux .deb package. The AppImage is the one Linux build the updater can
  replace in place and it runs on every distribution, so it is now the only
  one. It also no longer needs libfuse2 on your machine: the runtime on the
  front of the file carries its own.
- The Families page leaves the in-app help; categories, families and what
  the colours mean now live in the guide, on the website, as Concepts

### Fixed
- Paste after Go to start. Pasting a clip onto the one it was copied from
  lands right after it instead of doing nothing
- Windows lists it as "Humus" in Apps and features, not "Humus version 0.4.0".
  The version is already the column beside it
- The splash and the start window carry a taskbar button and the Humus icon,
  so the first seconds of a session are something you can click back to. They
  had neither, and a plug-in scan can hold the splash on screen for a while
- Humus opens once. Launching it again while it is running brings the window
  you already have to the front, and hands it a patch if the launch named one,
  instead of starting a second copy that fights the first for the audio device
  and shares its settings and its hang record. The code to receive that second
  launch was already written and had never been reachable
- Boot moves between its three windows instead of snapping between them. The
  splash fades into the start window and that fades into the main window, so
  the change of size is something you never catch happening. Nothing was
  easing either end before; macOS animates a window's appearance itself and
  was covering for it, which is why only Windows looked wrong
- Laying out the rack no longer cancels animations that belong to something
  else. The properties pane asked the shared animator to stop everything in
  the application before each layout pass, which was fine while it was the
  only thing animating and stopped being fine the moment anything else was
- The splash no longer stays behind the main window for the rest of the
  session on the two paths that skip the start window: a first boot, and
  launching by double-clicking a patch
- The meter automates itself when you change it while recording, the way the
  tempo already did. Change 4/4 to 7/8 mid-take and the lanes appear on their
  own, holding what you had from the first bar and taking the new meter from
  the bar line you were in. No option to tick first. Rolling without capture
  armed still just changes the meter, and so does changing it stopped
- MidiPlayer plays the moment a score is dropped in. It shipped with an empty
  bank slot and no default, so it had no instruments at all and was silent
  whatever you gave it - the tests all named a bank by hand and never saw it.
  It now points at the General MIDI bank that ships beside it, and a player
  whose slot was emptied gets it back rather than staying mute
- A library reference in a text slot reaches the organism as a file it can
  open. `bank:` and `asset:` references were resolved for parameters named
  File and nowhere else, so any other slot - MidiPlayer's bank among them -
  was handed a reference no organism can read. Patches still store the
  portable reference; only what the sound engine sees changed
- PianoRoll's bars box says how long the clip is. A new roll is four bars but
  the box came up blank, because nothing asked it what it was showing until
  you changed something; and a loop closed at a length the menu does not offer
  - three bars, five - blanked it again, since there was no entry to select.
  The box now carries the length it actually has, offered lengths or not
- A detached PianoRoll dragged taller gives the extra height to the roll
  rather than to empty space, and its Follow, Swing and unit row stays on the
  bottom edge where you left it instead of stranding itself above a gap
- The Sequence and PianoRoll faces are authored at the rack width the rest of
  the organisms use. Both were a little wider than the rack, which is not a
  free choice for a face that scales horizontally: everything on them was
  quietly drawn a few percent small, Sequence's knobs at 58 pixels where the
  face asked for 60
- A patch saved while a recorder was armed reopens disarmed. Record was
  stored like a setting and restored like one, so the button came back lit
  over a recorder that was not running - and the next thing you pressed
  stopped a take that had never started
- A toggle in an organism editor writes its parameter when you click it, not
  when the mouse merely passes over it. The framework's onStateChange fires on
  hover in, hover out, press and release alike, and every one of those was
  writing the parameter and pushing an undo step; on an armed FileRecorder it
  also restarted the take, which is why the folder filled with files
- Arming a recorder that is already recording does nothing, instead of
  stopping the take and starting another
- A FileRecorder records when you press Record. With no file named it did
  nothing at all: it went looking for a name, decided the organism named its
  own, and returned, leaving the button lit over a take that was not
  happening. Each track now writes a timestamped file into the recordings
  folder and the row fills in with where it went, arming starts the audio
  engine if it was idle, a take you did not name never lands on the one
  before it, and if there is genuinely nothing to record to the button turns
  itself back off
- A FileRecorder's browse button names a file to write instead of asking for
  one that already exists, so a take can go somewhere new. It was opening a
  load dialog, which on most systems will not accept a name that is not
  there yet
- A FileRecorder is one stereo track by default, and its Tracks combo counts
  files rather than channels: a track is a stereo file, so four tracks are
  four files and eight channels. The number used to be the channel count, so
  the stereo default drew two file rows and the second could never do
  anything. Saved patches keep their files and their channel counts and gain
  a second inlet per track
- An empty PianoRoll follows the transport. Until something was drawn it had
  no clip, and the playhead is drawn from the clip, so a roll you had not
  started sat perfectly still while the song played. A faint line now sweeps
  its bars and wraps at the last one, and becomes the real playhead as soon
  as there is a clip
- A detached organism cannot be shrunk away any more. Its window will not
  go below the size the organism opens at, and collapsing or reloading a
  detached box now resizes its window to match. Torn-off panes, the visual
  window and the Help, Notes, Library, Metapad, Parameter Control and
  Document Switcher windows had no floor at all and now have one. On Linux
  no window had one in practice: the minimum never reached the window
  manager, and now it does
- The card browser shows the pointing hand over a shelf, a box and a
  breadcrumb, so what is clickable looks clickable
- The overview map beside the rack is a handle, not a slider. Pressing the
  lit bar grabs it where you pressed and it follows the mouse from there,
  instead of jumping so the press became the new centre; pressing the map
  above or below the bar still jumps to that spot and then drags. Holding
  the mouse down closes the hand on every platform, not only macOS. It
  only scrolls now: clicking a block on it no longer selects that
  organism, which the rack and the patch canvas already do
- The rack has no horizontal scroll bar. Boxes span the pane exactly, but
  the surface behind them was still given the eight-pixel trailing margin
  the free layout needs, so it was always eight pixels too wide
- A detached organism keeps the window size you gave it. Changing a preset
  used to leave the organism drawn small in a large window, and the floor
  above then shrank the window back to it; a reload now restretches the
  organism to the window and only grows it if the new layout needs more
- Wave drives into the filter instead of out of it. The shaper ran last,
  so nothing tamed the harmonics it made and a hard drive stayed harsh
  whatever Cutoff said; it now sits before the lowpass, and Drive itself
  is smoothed. Patches that lean on Drive will sound warmer and darker
- Wave's Position is smoothed the way Warp already was. Swept by an LFO or
  an automation lane it stepped once per block, and the jumps at those
  edges were thirty times the waveform's own slope: the roughness anyone
  automating the table could hear
- A patch that routes or automates a parameter it never wrote as a
  property, as a hand-written patch easily does, now drives that
  parameter: the loader seeds every parameter the box's schema declares
  and the file omits, so a route lands on a live socket instead of
  nothing. The NeoPixel VU meter example was silent for exactly this
  reason
- Halogen's panel shows the picture the head is playing, a video frame
  included, gated, blurred, tilted and levelled as the head hears it; its
  scan window follows X, Y, Width and Height while the knob moves rather
  than on the next click, and a drag on the panel moves the X and Y knobs
- Editing a Control-with route's range, or removing a route or a MIDI
  mapping from the Parameter Control window, no longer crashes: the row was
  rebuilt from inside its own text field's callback
- A VCA with nothing on its control inlet passes the signal at unity instead
  of going silent
- The groove no longer drops to straight when an organism is added, deleted,
  undone or re-cabled while the transport button still shows it; a saved
  patch's groove also plays from the moment it opens, in the plug-in and in
  a bounce, not only after the menu is touched again
- The macOS disk image mounts with the Humus icon instead of a plain drive
  (UNTESTED BY HAND on macOS)
- Sandboxed plug-ins on macOS no longer die within a minute of loading: the
  child was being declared dead the moment it started (UNTESTED BY HAND on
  macOS and Windows)
- Sandboxed plug-ins on macOS and Windows wait for their block the way Linux
  does instead of giving up after a few microseconds, so they stop missing most
  blocks (UNTESTED BY HAND on macOS and Windows)
- The properties pane repaints far faster on high-density screens
- Mute and gain no longer click or zipper on Gain, the mixers, Send and VCA:
  every level change rides a 5 ms ramp
- Editing a Wave table, a Math formula, a gesture set, a Morse text, a
  SideKick shape, a Cluster chord, Leafcutter slice edits, a Grit sample or
  a Serial port no longer stalls the audio while the text is parsed
- Loading a file into a player, a deck, a sampler or a sound space no
  longer stalls the audio while the old buffer is freed
- Large patches route audio and MIDI between boxes several times faster
- The plug-in build swaps a newly loaded patch in without freeing the old
  one on the audio thread
- A hosted plug-in that changes its latency while running is re-aligned
  automatically
- Dropping a stereo cord, replacing a box or pasting a selection rebuilds the
  audio graph once instead of once per cord, so busy patches stop stuttering
  on every edit
- The assistant's levels, spectrum and automix tools no longer freeze the
  window for up to a second and a half while they listen
- Saving writes the patch beside itself and renames, so a crash or a full
  disk mid-save keeps the old file
- A patch saved by a newer Humus says so in the status line when it opens,
  and so does the number of boxes this build cannot provide
- A pack update is taken only over https and only when its checksum matches
  the manifest; a bundle that tries to unpack outside its folder is refused
- The app keeps a log under its data folder; a rebuild that fails says why
  in the status line instead of silently keeping the old graph
- Closing Settings with a colour picker open no longer risks a crash
- A patch naming an impossibly numbered mixer or bus opens instead of
  crashing the app

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
- Silt ships twelve presets
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
