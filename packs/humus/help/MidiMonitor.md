# MidiMonitor

A log of MIDI events as readable lines: source, channel, type and data.

It watches two feeds at once. With nothing corded it logs everything arriving from every enabled MIDI device, tagged in1 to in8 by port, and from the on-screen keyboard, tagged kbd, including controller traffic that is consumed for parameter mapping before it reaches a synth. Corded inline between a MidiIn and a Rhizome, its MIDI inlet is a scoped tap, tagged cord, that shows exactly what that synth receives, and the outlet passes events through unchanged. Source and Channel filter at display time, so nothing is lost while you change them. Use it to find which hardware knob sends which controller, or to confirm that a cords-only synth gets what its cord carries.

## Parameters

**Source** All shows both feeds. Cords only shows the tap; Live only shows the devices and the keyboard.

**Channel** Shows one MIDI channel, 1 to 16. 0 shows all of them.

## Recipe

**Find a knob** Drop a MidiMonitor with nothing corded, Source Live only, Channel 0. Turn the hardware knob and read the controller number off the newest line, then right-click the parameter you want it on and use MIDI Learn.

## Related Organisms

MidiIn, OscMonitor, MidiBus
