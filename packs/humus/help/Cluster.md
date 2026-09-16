# Cluster

Eight chord slots that fire as whole voicings out of a MIDI outlet.

Each slot holds a handful of notes typed as names, such as C3 E3 G3, with sharps, flats and bare note numbers accepted. A slot fires when you press its button, play its trigger note, or drive its Fire parameter from a control route. In PADS mode the eight slots sit on eight incoming notes starting at TriggerNote and sound at the velocity you play. In FOLLOW mode one slot becomes a shape that follows your playing: every incoming note becomes the root and the chord moves with it. Notes shared between two sounding slots are counted, so releasing one pad never cuts a note another still holds. Cord the outlet to a Rhizome, a Wave or any other instrument.

## Parameters

**Mode** PADS plays the eight slots from eight keys. FOLLOW transposes one slot from whatever you play.

**Hold** What a press means. GATE sounds the chord while the trigger is down. LATCH makes every trigger a toggle, so pads can layer. PEDAL plays one chord at a time: each press replaces the last and releases are ignored, so an empty slot becomes a stop button.

**TriggerNote** The incoming note mapped to slot 1. The next seven notes up map to slots 2 to 8.

**Slot** Which slot FOLLOW mode carries around.

**Velocity** The velocity used when a slot is fired from its button or a control route.

**Notes1** The notes of slot 1, typed as names or numbers. And so on for 2 to 8.

**Fire1** The trigger behind slot 1's button, reachable by control routes and automation. And so on for 2 to 8.

## Recipe

**One-finger changes** Mode PADS, Hold PEDAL, TriggerNote 36. Type a progression into slots 1 to 4 and leave slot 5 empty. Cord a PianoRoll into the inlet and this into a Rhizome, then draw single notes at 36 to 39 where each chord should change and a 40 where it should stop.

## Related Organisms

PianoRoll, Steps, Riff, Harmonizer
