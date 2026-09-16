# Morse

A pattern generator that keys a typed message as a sine tone and as MIDI notes in real morse timing.

Type into Text and it is keyed out with dits, dahs, letter gaps and word gaps, locked to the transport: silent while stopped, keying from the top of the message when play starts. The tone follows the patch Tuning and the key edges are 3 ms soft, so it does not click. The MIDI outlet keys the same message as note-on and note-off pairs at the Note pitch, so cord it into a Kick and the message becomes a rhythm, or into an Acid and it plays as a line; with Level at 0 it is a pure MIDI pattern source. Send the tone through a Fern for a melody line.

## Parameters

**Text** The message. Letters, digits and common punctuation; anything morse cannot say is skipped.

**WPM** Keying speed in words per minute, 5 to 40. One unit lasts 1.2 divided by WPM seconds.

**Sync** Ignores WPM and makes one unit exactly one sixteenth, so the message becomes a rhythm on the grid and tempo changes re-time it.

**Note** The tone's pitch as a note, following the patch Tuning.

**Loop** Repeats the message with the standard seven-unit word gap between passes. Off plays one pass per Play.

**Level** Output level of the tone.

## Recipe

**Telegraph rhythm** Text CQ CQ, Sync on, Loop on, Note D5. Cord the MIDI outlet into a Kick and the audio outlet through a Fern with Sync on at 1/8; press play at 140 BPM and the message keys the kick.

## Related Organisms

Rhizome, Sequence, SideKick
