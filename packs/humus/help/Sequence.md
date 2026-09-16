# Sequence

An eight-row, sixteen-step drum grid where every row is its own MIDI outlet.

Each row sends its notes out of its own MIDI outlet, so row 1 can drive a Kick, row 2 a Sampler and row 3 a Silt, with the name plate on the row showing what it is corded to. The last outlet is the master and carries every row together, for one drum sampler that maps the whole kit by note. Eight banks each hold a full grid; the row notes, velocities and enables are the kit and stay the same across banks. The pattern is one bar of sixteenths and edits land while the transport plays.

## The grid

Click a step to set it, click again to clear it, drag to paint a run. The running column lights while the transport plays. The LED at the row's left is the row's enable, the knob is its velocity, and the mouse wheel over the name plate changes the row's note. Right-click a step for Random, Clear and Copy to another bank.

## Parameters

**SwingFollow** Follow makes the grid take the patch groove from the transport instead of its own Swing knob.

**Swing** Delays the offbeat sixteenths, from straight at 0 to a full triplet shuffle at 1. Used only while Follow is off.

**Gate** Note length as a fraction of a step.

**Enable_1** Whether row 1 plays; steps are kept while it is off. And so on for rows 2 to 8.

**Vel_1** Velocity of row 1's notes. And so on for rows 2 to 8.

**Note_1** The MIDI note row 1 sends, defaulting to the general drum map, 36 for a kick. Synth drums such as Kick ignore it; samplers map it. And so on for rows 2 to 8.

**Bank** Which of the eight grids plays, A to H.

## Recipe

**Four on the floor** Cord row 1 to a Kick, row 2 to a Cicada or a Sampler clap, row 3 to a hat sample. Steps 1, 5, 9 and 13 on row 1, 5 and 13 on row 2, every other step on row 3. Swing 0.15 with Follow off, Gate 0.5. Draw a fill in bank B and switch Bank from a Button for the turnaround.

## Related Organisms

Riff, Steps, PianoRoll
