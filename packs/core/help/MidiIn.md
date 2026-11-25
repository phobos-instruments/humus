# MidiIn

Live MIDI from a hardware device or another application, delivered on a MIDI outlet.

Cord its outlet into an instrument such as Rhizome, into a MidiBus, or into a MidiMonitor to see what arrives. The patch stores a port number rather than a device name, so a patch moved to another machine picks up whatever device that machine assigns to the same port; devices are assigned to the eight ports in the settings, and changing the port applies immediately. The on-screen keyboard at the bottom of the property editor plays notes straight into this organism's outlet, so the instruments behind it can be tried without hardware. It also lights the keys you hold on a connected keyboard, so you can see what is arriving. Older patches use MidiIn1 to MidiIn8, which load as this organism with Port preset accordingly.

## Parameters

**Port** Which MIDI input ports this organism listens to. All inputs, the default for a new MidiIn, hears every connected keyboard and controller at once; a single port hears only the device assigned to it, and the dropdown shows each port with that device.

**Channel** Which of the sixteen MIDI channels a device gets through on. All, the default, lets every channel in; a single channel keeps out what a device sends on the others. The on-screen keys always play, whatever the channel.

## Recipe

**Keyboard to synth** Drop a MidiIn, pick the port your keyboard is assigned to, cord it into a Rhizome and the Rhizome into the Mixer. Click the on-screen keys first to hear the voice, then play the hardware.

**Keys and pads apart** Many controllers send their keys on channel 1 and their pads on channel 10. Set the MidiIn's Channel to 1 and the synth behind it hears only the keys, while the pads stay free for MIDI Learn; a pad learned onto a parameter keeps to its own channel, so the keys never fire it.

## Related Organisms

MidiOut, MidiMonitor
