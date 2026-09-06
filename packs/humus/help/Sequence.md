# Sequence

The classic step-grid sequencer: eight rows of sixteen steps, grouped in fours, with an enable LED, a velocity knob and a name plate on every row. One difference from the classic grid, and it's the whole point: each row is a MIDI OUTLET. Wire row 1 to a Kick, row 2 to a Sampler, row 3 to a drum machine - the name plate shows what the row is driving. The last outlet is the master: every row's notes together, for one drum sampler that maps the whole kit by note.

## Programming

**Steps** click to set, click again to clear; drag to paint runs. The running column lights while the transport plays.

**LED** the row's mute - dark rows fall silent (steps kept).

**Knob** the row's velocity (how hard its notes leave).

**Name** shows the wired organism. Mouse-wheel over it to change the row's note (GM drum defaults: 36 kick, 39 clap, 42 closed hat,... shown at the plate's right edge). Synth organisms like Kick ignore the note; samplers and plugin drum machines map it.

## Banks

Eight banks, A to H, each a full grid of eight rows. The bank buttons switch which one plays and which one you see; every step you set lands in the bank on screen, so a bank is saved as you draw it. The rows' notes, velocities and enables are the kit, not the pattern - they stay the same across banks. Bank is a parameter like any other: right-click the buttons to map a MIDI control, drive it from a Button, a Slider or another sequencer, or automate it on the timeline. Right-click a step for Random, Clear and Copy to another bank.

## Dialing it in

**Swing** delays the offbeat sixteenths - 0 straight, 1 full triplet shuffle.

**Gate** note length, as a fraction of a step.

**Bank** which of the eight grids plays, A to H.

The pattern is one 4/4 bar of sixteenths, stored with the patch and edited live - changes land without interrupting the audio.

## Related Organisms

Riff (acid lines), Steps (single-row trigger lane), PianoRoll
