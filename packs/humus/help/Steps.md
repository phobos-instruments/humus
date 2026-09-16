# Steps

A one-row, sixteen-step trigger sequencer that sends one MIDI note with swing, gate and velocity.

One row is one voice: give it a Note and cord its MIDI outlet into a Kick, a Mineral or a Sampler, then stack several Steps organisms, one per drum, each with its own swing and gate. It is silent until the transport plays, and steps are read from the transport's beat position, so it always lands in time with the patch. A hit lasts Gate of a step unless the hold row ties it into the steps that follow. Muting a row's cord, or its channel on the Mixer, is the live performance mute.

## The grid

Click a step to place a hit and click it again to clear it. The ^ row above lifts that hit one octave. The hold row below ties a hit across the next step boundary, and a run of holds makes one long note.

## Parameters

**Note** The MIDI note every hit sends. 36 is C1, the usual kick.

**SwingFollow** Follow makes the row take the patch groove from the transport instead of its own Swing knob.

**Swing** Delays every second sixteenth, up to a triplet feel. Used only while Follow is off; give hats more swing than the kick and the groove opens up.

**Gate** Hit length as a fraction of the step. Ties extend past it.

**Velocity** Velocity of every hit. Automate it for builds.

## Recipe

**Hats over a Kick** Two Steps organisms. The first, Note 36, hits on steps 1, 5, 9 and 13 and cords into a Kick with FourFloor off. The second, Note 42, hits every second step with Follow off and Swing 0.3, and cords into a Sampler zone holding a hat. Lift a few hat steps with the ^ row for accents.

## Related Organisms

Kick, Riff, Acid
