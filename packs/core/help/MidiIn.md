# MidiIn

Feeds live MIDI from a hardware device (or another application) into the patch through its MIDI outlet.

## Port

Which of the eight MIDI input ports this organism listens to. Assign a device to each port on the MIDI page of the Settings dialog - the dropdown shows each port with its currently assigned device. Because the patch stores the port number (not the device name), a patch moved to another machine picks up whatever device that machine assigns to the same port.

Changing the port takes effect immediately; no need to stop audio.

The on-screen keyboard at the bottom of the property editor injects notes directly into this organism's outlet, so you can play its downstream instruments without hardware.

Retro-compatibility: older patches use numbered classes (MidiIn1 to MidiIn8). These still load - each is this same organism with its Port preset to the class number.

## Related Organisms

MidiOut, MidiMonitor
