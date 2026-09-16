# MidiOut

Sends MIDI arriving on its inlet out to a hardware device or another application.

Cord a PianoRoll, a Microdot, an Arpeggiator or a MidiIn into it and the notes and controllers leave through the port you choose. The patch stores the port number rather than the device name, so it opens on another machine with whichever device is assigned to that port there. The message parameters also make the organism a MIDI source of its own: changing Value sends the chosen Controller on the chosen Channel, and switching Gate sends a note on and off, so an automation lane or a mapped knob can drive external hardware with no MIDI cord at all. Pair it with a MidiMonitor to watch what leaves.

## Parameters

**Port** Which of the eight MIDI output ports carries the messages. Assign a device to each port in the MIDI settings; the dropdown shows each port with its device.

**Channel** The MIDI channel, 1 to 16, that the Controller and Gate messages go out on. Notes arriving on the inlet keep their own channel.

**Controller** The controller number that Value sends. Changing it sends the current Value on the new number straight away.

**Value** The controller value, 0 to 127. Every change sends one controller message, so automate it or map it to a knob to ride a hardware parameter from the patch.

**Note** The note number that Gate plays, 60 by default.

**Velocity** The velocity of the note-on that Gate sends.

**Gate** On sends a note-on for Note at Velocity; off sends the matching note-off. Map it to a button or a pad.

## Recipe

**Hardware filter ride** Set Port to the port your synth is assigned to, Channel to the channel it listens on and Controller to the number of its filter cutoff. Automate Value on the timeline, or modulate it with an LFO, and the synth's filter follows the patch; cord a PianoRoll into the inlet to play the notes over the same port.

## Related Organisms

MidiIn, MidiMonitor
