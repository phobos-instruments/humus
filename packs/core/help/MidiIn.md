# MidiIn

Live MIDI from a hardware device or another application, delivered on a MIDI outlet.

Cord its outlet into an instrument such as Rhizome, into a MidiBus, or into a MidiMonitor to see what arrives. The patch stores a port number rather than a device name, so a patch moved to another machine picks up whatever device that machine assigns to the same port; devices are assigned to the eight ports in the settings, and changing the port applies immediately. The on-screen keyboard at the bottom of the property editor plays notes straight into this organism's outlet, so the instruments behind it can be tried without hardware. Older patches use MidiIn1 to MidiIn8, which load as this organism with Port preset accordingly.

## Parameters

**Port** Which of the eight MIDI input ports this organism listens to. The dropdown shows each port with the device assigned to it.

## Recipe

**Keyboard to synth** Drop a MidiIn, pick the port your keyboard is assigned to, cord it into a Rhizome and the Rhizome into the Mixer. Click the on-screen keys first to hear the voice, then play the hardware.

## Related Organisms

MidiOut, MidiMonitor
