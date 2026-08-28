# SerialIn

Reads control values from a serial device - a microcontroller board reading sensors, a fader box, anything printing numbers to a USB serial port. The mirror of SerialOut: the device's values come out as two control signals.

## The sensor path

Firmware that prints readable numbers, one line per reading - a value for outlet A, optionally a space and a second value for outlet B - is all it takes. A light sensor, a potentiometer, a distance sensor: print its reading 30 or so times a second and patch outlet A into whatever should follow it. The port is read away from the audio path, so audio never waits on the device, and unplugging and replugging reconnects automatically.

Beyond the two outlets, SerialIn is also a modulation source: right-click any parameter and pick Modulate with - SerialIn (a or b) to have the sensor drive it directly, no patch cord needed. The box meter flickers as data arrives, so a silent patch still shows the sensor is alive.

## Parameters

**Device** which serial device to listen to. Auto picks the first USB serial device found; the rest of the list is every device present.

**Baud** the port's speed. It has to match what the firmware opens; 9600 and 115200 are the usual defaults.

**Smooth** slew toward each new value, in milliseconds. A jumpy sensor at 0 steps hard; the 20 ms default rounds steps off without feeling laggy. Raise it for deliberately slow drifts.

**ASCII** on, incoming text lines are parsed as numbers - up to two per line, separated by anything. Off treats every raw byte as outlet A, scaled 0..255 to 0..1.

## Notes

Values are used exactly as sent: firmware that prints 0..1023 drives parameters expecting 0..1 a thousand times too hard, so divide before printing, or map the range in the Modulate-with route. Serial I/O is not available on Windows yet.

## Related Organisms

SerialOut, Number, Follower, LFO
