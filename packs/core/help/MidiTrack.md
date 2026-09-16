# MidiTrack

A timeline lane that holds a MIDI part and plays it into any instrument in the patch.

It has no sound of its own and no box in the patcher: the part lives on the lane, and the row's chip names the organism it plays into. Pick a different target and the same notes move to a different instrument, untouched, so one part can be auditioned through several synths or kept safe while the patch is rebuilt around it. New tracks start at (nothing) and stay silent until a target is chosen. The arm button on the row routes live MIDI input through the track into the target, and what you play lands in a clip while recording. Mute, solo, rename and delete work from the row like any other track.

## Parameters

**Target** The instrument this track plays into, shown on the row's chip. The list offers every organism in the patch with a MIDI inlet; choosing one cords the track's MIDI outlet to it, and a cord drawn by hand from that outlet sets Target the same way.

## Recipe

**One part, two synths** Set Target to a Rhizome, arm the row and record a phrase, or draw one in the clip. Add a second instrument, switch Target to it, and the same clip plays the new synth; switch back once you have compared the two.

## Related Organisms

MidiBus, PianoRoll, Steps
