# Bus

A summing junction with no controls: several signals in, their sum out.

Use it where cords need a meeting point and nothing else: several sources into one effect, a group that a single Gain then rides, or a fan-in that keeps a large patch readable. The Inputs and Mode dropdowns in the property editor set how many inputs it has, 2 to 8, and whether they are Stereo pairs or Mono channels; changing them resizes the bus in place with its name, cords and automation kept. It adds nothing and scales nothing, so levels are set at the sources or on a Gain after it. For per-input levels, mutes and solos, use a Mixer instead.

## Recipe

**Reverb group** Cord the send pairs of three Send organisms into one Bus, the Bus into a Verbatim, and the Verbatim into a Mixer strip. One cord to the reverb carries every part, and one Gain in front of it sets how wet the whole group is.

## Related Organisms

Mixer, Gain, Send
