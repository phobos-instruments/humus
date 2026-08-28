# OscMonitor

A diagnostic terminal that logs OSC traffic as human-readable lines (direction, address, arguments). It has no pins at all: drop a bare OscMonitor anywhere and it watches everything crossing the app's OSC boundary, in both directions:

**In** Every message arriving on the OSC input port (Settings - MIDI & OSC, default 9000) - including addresses that are NOT mapped to any parameter yet. Point any OSC app (a phone controller, a sensor bridge) at Humus and see exactly what it sends before you Learn-map anything. String-only messages show too, even though only numeric ones can drive parameters.

**Out** Every value the OSC-out sources transmit (a Hands with its OSC toggle on), with the exact address it was sent to - handy for checking what the receiving end should expect.

Use the Show filter (All / Received / Sent) to narrow a busy log - a tracked hand streams continuously, so Sent traffic can flood the view. Pause freezes the display without blocking anything; Clear empties it. Filtering happens at display time, so nothing is lost while you change it.

The OSC input must be enabled in Settings for "in" rows to appear; the header warns when it is off. Typical uses: checking that a phone controller actually reaches the machine (firewalls eat UDP silently), finding the address a fader sends before mapping it, and verifying the Hands stream while wiring up a visualizer.

## Related Organisms

Hands, MidiMonitor
