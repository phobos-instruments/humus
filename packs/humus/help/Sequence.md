# Sequence

The classic step-grid sequencer: eight rows of sixteen steps, grouped in fours, with an enable LED, a velocity knob and a name plate on every row. One difference from the classic grid, and it's the whole point: each row is a MIDI OUTLET. Wire row 1 to a Kick, row 2 to a Sampler, row 3 to a plugin drum machine - the name plate shows what the row is driving.

## Programming

**Steps** click to set, click again to clear; drag to paint runs. The running column lights while the transport plays.

**LED** the row's mute - dark rows fall silent (steps kept).

**Knob** the row's velocity (how hard its notes leave).

**Name** shows the wired organism. Mouse-wheel over it to change the row's note (GM drum defaults: 36 kick, 39 clap, 42 closed hat,... shown at the plate's right edge). Synth organisms like Kick ignore the note; samplers and plugin drum machines map it.

## Dialing it in

**Swing** delays the offbeat sixteenths - 0 straight, 1 full triplet shuffle.

**Gate** note length, as a fraction of a step.

The pattern is one 4/4 bar of sixteenths, stored with the patch and edited live - changes land without interrupting the audio.

## Related Organisms

Riff (acid lines), Steps (single-row trigger lane), PianoRoll
