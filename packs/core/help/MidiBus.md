# MidiBus

A MIDI merge junction with no controls: several MIDI cords in, one time-sorted stream out.

Fanning out never needs an organism, since one MIDI outlet can be corded to any number of instruments; the MidiBus is for fanning in. Cord controllers and sequencers into it and one cord per instrument out of it, and re-pointing everything then means moving one cord. Events from all inlets are merged in time order within each block. The Inputs dropdown in the property editor sets how many inlets it has, 2 to 8, resizing the bus in place with its name, cords and automation kept. A Tuning corded into a MidiBus scopes every instrument downstream of it, which makes it the place to hang one scale for a whole section.

## Recipe

**Keyboard plus sequencer** Cord a MidiIn and a Microdot into a MidiBus and the bus into a Rhizome. Played notes and sequenced notes reach the same voice, and a Tuning on a third inlet retunes both at once.

## Related Organisms

Bus, MidiIn, MidiOut, Tuning
