# Bus

A plain summing junction: several signals in, their sum out. No gain controls - use a Mixer when you need per-input levels.

## Size

The Inputs and Mode dropdowns in the property editor set how many inputs the bus has (2 to 8) and whether they are stereo pairs or mono channels. Changing them resizes the bus in place: its name, cords (clamped to the new inlet count) and automation survive.

Behind the scenes each size is its own class (a 2-input stereo bus is an S2Bus), so older patches load with their buses intact.

## Related Organisms

Mixer, Gain, Matrix
