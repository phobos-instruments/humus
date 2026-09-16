# SerialOut

Sends control values out of a serial port in whatever text or byte form the device expects.

Cord Sliders, a Follower, an LFO or a Number onto its sockets and write the line they should make in the Format field: %1 to %8 print the sockets as text, %b1 to %b8 send one as a raw byte and %w1 to %w8 as a two-byte word, \n ends a line and \xNN is a raw byte. The default "%1 %2\n" prints two numbers and a newline. Scale multiplies a value before it prints and Int rounds it, so a 0 to 1 control leaves as 0 to 255. Several lines in Format are several messages, and a Message socket picks which one goes out. The readout at the bottom shows the port and the last bytes sent. SerialIn reads values back the same way.

## Parameters

**Device** Which serial port to talk to. Auto takes the first port found; the rest of the list is every port present.

**BaudRate** The port speed, which has to match what the firmware opens. 9600 and 115200 are the usual choices.

**Rate** How many times a second the line goes out in Continuous mode, and the ceiling in the other two. Shown as Per second.

**Frame** Data bits, parity and stop bits: 8N1 is what nearly everything uses; 8E1, 8O1 and 8N2 are there for the gear that wants them.

**Custom** Any other baud rate, typed in. 0 leaves the BaudRate list in charge.

**Reset** Pulses the board's reset line when the port opens, so its program starts from the top on every connect. Off leaves the board running across a reconnect.

**Reconnect** Closes the port and opens it again, for every organism sharing it.

**Send** Continuous writes the line Rate times a second. On change writes it once whenever one of the shown sockets moves. On trigger writes it when the Trigger socket rises.

**Values** How many sockets the organism shows, and how many count for On change. Shown as Sockets.

**Value1** The first socket, and the number it sends when nothing is corded onto it. And so on for 2 to 8; %5 in Format prints Value5 whether or not its socket is shown.

**Trigger** The socket On trigger reads; a Button on it sends the message once per press.

**Message** Picks which line of a multi-line Format goes out, from 1. It appears once Format has more than one line.

**Scale1** Multiplies Value1 before it prints or packs into a byte. And so on for 2 to 8.

**Int1** Prints Value1 as a whole number instead of with decimals. And so on for 2 to 8.

**Format** The message template. Besides the placeholders above, %n1 prints the name of the source on a socket, %c a count byte, %x and %X a checksum, and %(v1*180) a computed value.

**Port** A port path typed in by hand, which overrides the Device choice while it is set.

## Recipe

**Servo from an LFO** Cord an LFO onto Value1, set Scale1 to 180 and Int1 on, Format "M %1\r", Send Continuous, Rate 30. The board reads the line and moves the servo with the LFO; slow the LFO to a bar and the arm sweeps in time with the patch.

## Related Organisms

SerialIn, Number, Slider, Button
