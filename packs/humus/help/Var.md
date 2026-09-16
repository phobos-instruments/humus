# Var

A control value with a name, so a cord or a serial line can carry the number and the word together.

Type a name, and the Value socket leaves the outlet like any other control source; the name lets the wire say what the number means. A SerialOut with a Var on its socket prints the name next to the value with %n1, and a SerialIn that reads a line such as "temp 23.5" hands the number to the Var called temp. A value that arrives by name shows in the readout and leaves the outlet; typing a new Value takes over until the next one arrives. Cord its outlet onto any knob, or onto a SerialOut socket.

## Parameters

**Value** The socket, and the number to hold when nothing arrives by name. Draw an LFO or a Slider onto it and the Var follows.

**Name** The word this Var answers to, and the word it prints on a SerialOut. Letters, digits and underscores travel best.

## Recipe

**Sensor by name** Name temp, and a SerialIn with an empty Parse reading a board that prints "temp 23.5" once a second. Cord the Var's outlet onto a Filter's Frequency, and the room's temperature sweeps the cutoff with no cord between the SerialIn and the Var.

## Related Organisms

Number, SerialIn, SerialOut, Slider
