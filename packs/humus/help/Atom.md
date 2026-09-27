# Atom

Four rings of evenly spread hits that arpeggiate the notes you hold, or walk a scale when you hold none.

Each ring answers one question: when. It takes a number of steps and a number of hits and spreads the hits round the circle as evenly as they will go, which is the rule behind a great many of the world's dance rhythms: three hits in eight is the tresillo, five in eight the cinquillo, seven in twelve the standard West African bell. Which note sounds is answered by your hands. Cord a keyboard or a PianoRoll into the inlet and every hit plays the next note of the chord you hold; hold nothing and the rings walk the chosen key and scale by themselves. Notes you play are pulled onto the scale first, so nothing lands outside the key. It runs while the transport plays, and sends everything from one MIDI outlet.

The face shows the rings from the outside in, ring 1 outermost. Lit dots are hits, hollow dots are rests, and the bright dot is the step playing now.

The presets come in three families. Rhythms are the dance patterns above. Elements treat the rings as electron shells: the innermost ring is the first shell, and each ring is as long as its shell can hold (2, 8, 18 and 32 steps, from the inside out), its hits are the electrons in it, and every shell turns once a bar, so heavier atoms fill in denser, faster layers. Planets make the rings orbits whose lengths keep the proportions of the real ones: eight turns of the Earth ring against thirteen of Venus, Jupiter's twelve against Saturn's twenty-nine, Neptune three times round while Pluto goes twice, so the rings drift apart and come back together the way the planets do.

## Parameters

**Steps** How many steps round the ring, from 1 to 32.

**Hits** How many of those steps sound. The ring spaces them as evenly as it can. 0 silences the ring.

**Turn** Rotates the whole figure round the ring, one step at a time, so the same rhythm starts from a different place.

**Rate** How long a step lasts: a quarter, eighth, sixteenth or thirty-second note. Fit bar instead stretches the whole ring across one bar, whatever its step count, and that is what makes a true cross-rhythm: a ring of 3 and a ring of 4, both on Fit bar, play three against four. Rings on a note rate with different step counts simply run on and drift against each other, coming back together only after many bars.

**Fix** Makes the ring play its own Note on every hit instead of the arpeggio, for driving a drum or a bell.

**Note** The note a fixed ring plays.

**Order** The path the arpeggio takes through the notes: Up, Down, Up-down, Random, or As played, which keeps the order your fingers went down in. Each ring keeps its own place on the path.

**Octaves** Repeats the notes over this many octaves, upward.

**Key** The tonic of the scale the rings walk.

**Scale** The scale the rings walk when no note is held, and the one held notes are pulled onto. Chromatic leaves held notes alone.

**Latch** Keeps the chord after you let go. The next chord you play replaces it.

**Gate** How much of a step each note lasts.

**Vel** The velocity of every note.

**Swing** This organism's own swing, used when Follow is off. A ring on Fit bar is never swung.

**Follow** Takes the swing from the transport; switch it off to set it here with Swing.

## Recipe

**An arpeggiator** Cord a keyboard into Atom and Atom into a synth. Leave ring 1 at 5 hits in 16, set Order to Up-down and Octaves to 2, press play and hold a chord. Raise Hits for a busier line, and use Turn to move the accents.

**Three against four** Set ring 1 to 4 steps and 4 hits and ring 2 to 3 steps and 3 hits, both on Fit bar with Fix on and two different notes, and cord the outlet into a Sampler. Add ring 3 as an arpeggio over the top.

**A bell pattern** One ring of 12 steps and 7 hits on Fit bar gives the long bell of much West and Central African music. Put a second ring of 12 with 4 hits under it for the dancers' beat, and turn the bell until it sits right.

## Related Organisms

Steps, Sequence, Riff, DNA, PianoRoll
