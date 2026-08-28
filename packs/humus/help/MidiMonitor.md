# MidiMonitor

A diagnostic terminal that logs MIDI events as human-readable lines (source, channel, type, data). It watches two feeds at once:

**Live** No cords needed. Drop a bare MidiMonitor anywhere and it logs everything arriving from every enabled MIDI device (tagged in1..in8 by arrival port) and the on-screen keyboard (kbd) - including CC traffic that is consumed for parameter mapping before it reaches any synth.

**Cord** The MIDI inlet is a scoped tap: insert the monitor inline between a MidiIn and a synth and the log shows exactly what that synth receives (tagged cord). The outlet passes events through unchanged, so patching it into a chain never alters the music. Leave the ports uncorded if you only need the live view.

Use the Source / Channel filters to narrow a busy log; filtering happens at display time, so nothing is lost while you change them. Typical uses: finding which hardware knob sends which CC, spotting duplicate notes, and confirming a synth set to "patch cords only" really gets what its cord carries.

## Related Organisms

MidiIn, OscMonitor, MidiBus
