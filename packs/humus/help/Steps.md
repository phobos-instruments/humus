# Steps

A one-row, sixteen-step trigger sequencer that sends one MIDI note with swing, gate and velocity.

One row is one voice: give it a Note and cord its MIDI outlet into a Kick, a Mineral or a Sampler, then stack several Steps organisms, one per drum, each with its own swing and gate. It is silent until the transport plays, and steps are read from the transport's beat position, so it always lands in time with the patch. A hit lasts Gate of a step unless the hold row ties it into the steps that follow. Muting a row's cord, or its channel on the Mixer, is the live performance mute.

## The grid

Click a step to place a hit and click it again to clear it. The ^ row above lifts that hit one octave. The hold row below ties a hit across the next step boundary, and a run of holds makes one long note.

**Step chance** Click the % key at the grid's top right to see each step's chance as a bar, then drag up or down on a step to set how often it plays, from never to every time in steps of 5%. Cmd-drag (Ctrl-drag on Windows and Linux) does the same without switching. The % key is the View switch, so a mapped button or pad flips it too. Each step's chance is also a parameter, Chance_1 to Chance_16, so faders can be mapped to them. Right-click a step in chance view to map it. A hit below 100% draws fainter and shows its chance above it. The dice belong to the box: a patch plays the same hits on every play and every bounce, and two boxes roll their own. Each bank keeps its own chances, Copy to bank takes them along and Clear sets them back to sure.

## Parameters

**Note** The MIDI note every hit sends. 36 is C1, the usual kick.

**SwingFollow** Follow makes the row take the patch groove from the transport instead of its own Swing knob.

**Swing** Delays every second sixteenth, up to a triplet feel. Used only while Follow is off; give hats more swing than the kick and the groove opens up.

**Gate** Hit length as a fraction of the step. Ties extend past it.

**Velocity** Velocity of every hit. Automate it for builds.

**Bank** Which of the eight patterns plays, A to H. Each bank keeps its own steps and its own ^ row, and switching plays the new one at once.

**View** Steps or Chance: what the grid shows and edits. The % key switches it.

**Chance_1 to Chance_16** How often each step plays, from 0 to 100%. Each bank keeps its own.

## Recipe

**Hats over a Kick** Two Steps organisms. The first, Note 36, hits on steps 1, 5, 9 and 13 and cords into a Kick with FourFloor off. The second, Note 42, hits every second step with Follow off and Swing 0.3, and cords into a Sampler zone holding a hat. Lift a few hat steps with the ^ row for accents.

## Related Organisms

Kick, Riff, Acid
