# SerialOut

Streams control signals out to a serial device - a microcontroller board, a sensor rig, anything listening on a USB serial port.

Wire up to two control signals into its inlets: a Number, an envelope follower, an LFO. They are sampled Rate times per second and writes them to the port, so audio is never blocked by the device. Unplugging and replugging the device reconnects automatically.

## Parameters

**Device** which serial device to talk to. Auto picks the first USB serial device found, which is the right answer whenever one board is plugged in; the rest of the list is every device present right now.

**Baud** the port's speed. It has to match what the firmware at the other end opens; 9600 and 115200 are the usual defaults.

**Rate** values sent per second. Higher is smoother and busier; a board that cannot keep up falls behind rather than dropping values.

**ASCII** on, each message is a text line - "v1 v2" and a newline - which is what firmware parsing readable numbers expects. Off sends one raw byte per inlet instead, the input clamped to 0..1 and scaled to 0..255.

## Notes

The mirror organism is SerialIn, which reads values back from the device. Serial I/O is not available on Windows yet.

## Related Organisms

SerialIn, Number, LFO
