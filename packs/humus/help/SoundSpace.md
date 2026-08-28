# SoundSpace

A granulator you fly through. Load up to 4 corpus sound files; SoundSpace slices them into grains, analyses each grain's sonic character (energy, brightness, noisiness, movement...), and scatters them on a 2D map - similar sounds land near each other. The cursor plays a granular cloud of the grains around it: drag across the map to explore and morph between textures.

## Controls

**Map** Drag to move the cursor (each corpus file has its own colour). Right-click to Automate or MIDI-learn the X / Y position - "MIDI Learn X, then Y" maps both coordinates in one pass (move one controller for X, then a second for Y), so a joystick or XY pad can fly the space for you.

**Density** Grains per second.

**Size** Grain length (ms).

**Spray** Picking radius around the cursor - small = focused on one texture, large = a cloud sampling the whole neighbourhood.

**Pitch** Transpose in semitones.

**Gain** Output level.

**Mute** Silences the output while the cloud keeps flying - unmuting rejoins mid-texture.

## The map

Grid lines mark quarters of each axis (labelled 0-100, matching the coordinate readout in the corner); the reticle lines through the target show your position against them from anywhere on the map.

## Notes

Mix very different material - voice, machines, field recordings - and the map spreads further apart, which makes travelling it more dramatic. Automate X and Y with slow ramps for evolving beds, or map them to a joystick or XY pad for performance. Analysis runs when you pick the files, and big ones are sliced to a few hundred grains each.

## Related Organisms

CameraIn, Sampler
