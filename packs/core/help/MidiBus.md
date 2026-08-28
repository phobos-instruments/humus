# MidiBus

A plain MIDI merge junction: several dotted cords in, one time-sorted stream out. No controls - the MIDI twin of the audio Bus.

Use it as a hub. Fanning OUT never needs an organism (one MIDI outlet can be corded to any number of instruments); the MidiBus is for fanning in and for keeping a many-to-many patch readable: controllers and sequencers into the bus, one cord per instrument out of it. Re-pointing everything then means moving one cord.

Tunings flow through it. A Tuning corded into a MidiBus scopes every instrument downstream of the bus, so it is the natural place to hang one scale for a whole section.

## Size

The Inputs dropdown in the property editor sets how many inlets the bus has (2 to 8). Changing it resizes the bus in place: its name, cords (clamped to the new inlet count) and automation survive.

## Related Organisms

Bus, MidiIn, MidiOut, MidiMonitor, Tuning
