# SerialIn

Reads numbers from a serial port and puts them on control outlets a to h.

Cord a microcontroller board that prints readings over its serial connection and each number on a line lands on its own outlet: the first on a, the second on b, and so on. Separate numbers with spaces, commas or semicolons, and put a word before a number to send it to the Var of that name with no cord. A Parse template reads lines or byte packets that are not bare numbers, and while one is set a ninth outlet called match pulses whenever a line fits. The port is read away from the audio path, a replugged device reconnects on its own, and any parameter can pick Control with SerialIn from its right-click menu. Cord the outlets onto Number, Gain or LFO sockets.

## Parameters

**Device** Which serial port to listen to. Auto takes the first port found; the rest of the list is every port present.

**BaudRate** The port speed, which has to match what the firmware opens. 9600 and 115200 are the usual choices.

**Values** How many of the eight outlets the organism shows, a first.

**Frame** Data bits, parity and stop bits: 8N1 is what nearly everything uses; 8E1, 8O1 and 8N2 are there for the gear that wants them.

**Custom** Any other baud rate, typed in. 0 leaves the BaudRate list in charge.

**Reset** Pulses the board's reset line when the port opens, so its program starts from the top on every connect. Off leaves the board running across a reconnect.

**Reconnect** Closes the port and opens it again, for every organism sharing it.

**Parse** The template a line or packet has to match: %1 to %8 capture numbers as text, %b1 to %b8 single bytes and %w1 to %w8 two-byte words, both read as 0 to 1. Empty takes every number on the line in order; a line that does not fit is ignored and shown as skipped.

**Port** A port path typed in by hand, which overrides the Device choice while it is set.

## Recipe

**Light sensor to filter** Have the board print its sensor as a 0 to 1 number thirty times a second, one per line. Leave Parse empty, Values 1, BaudRate 115200. Right-click the Cutoff of a Filter, choose Control with SerialIn a, and the filter follows the sensor. Watch the readout under Parse to confirm what arrives.

## Related Organisms

SerialOut, Number, Follower, LFO
