# Morse

A message as a pattern generator. Type into Text and it is keyed out in real ITU morse timing - dits, dahs, letter gaps, word gaps - as a clean sine tone locked to the transport. Silent while stopped; press Play and it keys from the top of the message.

## Parameters

**Text** The message. Letters, digits and common punctuation; anything morse cannot say is skipped.

**WPM** Keying speed in words per minute (the PARIS standard: one unit is 1.2/WPM seconds). 15 is a comfortable listen; 5 is a dirge.

**Sync** Ignore WPM and make one unit exactly one 16th note instead - the message becomes a rhythm on the grid, and tempo changes re-time it live.

**Note** The tone's pitch, as a note - it follows the patch Tuning like every native instrument.

**Loop** Repeat the message forever with the standard seven-unit word gap between passes. Off = one pass per Play.

**Level** Output volume.

The 3 ms key edges are deliberately soft - no clicks, close to a real keyed oscillator. Send it through Fern or a Filter sweep and the telegraph turns into a melody line; Sync + a short percussive Decay-style chain makes it a step pattern you can read.

The dotted MIDI outlet keys the same message as note-on/off pairs at the Note pitch: cord it into a Kick and the message becomes a rhythm, into Acid or a plugin and it plays as a line. Turn Level to 0 and Morse is a pure MIDI pattern source.

Try: "CQ CQ" at 20 WPM, or your own name with Sync on at 140 BPM.

## Related Organisms

Rhizome, Sequence, SideKick
