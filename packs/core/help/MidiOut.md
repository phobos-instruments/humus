# MidiOut

Sends MIDI arriving on its inlet out to a hardware device (or another application).

## Port

Which of the eight MIDI output ports this organism sends through. Assign a device to each port on the MIDI page of the Settings dialog - the dropdown shows each port with its currently assigned device. Patches store the port number, not the device name, so they stay portable across machines.

## Message parameters

Channel, Controller, Value, Note, Velocity and Gate generate MIDI directly from parameter changes: automating or MIDI-mapping Value sends the selected Controller on the selected Channel; toggling Gate sends note-on/note-off for Note at Velocity. This lets automation lanes and the Metapad drive external hardware without any MIDI cords.

Retro-compatibility: older patches use numbered classes (MidiOut1 to MidiOut8). These still load - each is this same organism with its Port preset to the class number.

## Related Organisms

MidiIn, MidiMonitor
