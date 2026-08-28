# StereoTool

A stereo imaging utility that works in mid/side, with zero latency. Mid is what the two channels share (the centre); side is what they do not (the difference). Splitting a stereo signal into those two parts is what lets you widen an image, mono the bass, or rotate a field without disturbing the rest of the mix.

Stereo in, stereo out. Everything here is surgical rather than colouring - for the colour, put a Console after it.

## Parameters

**Width** side level. 0% collapses to mono, 100% leaves the image alone, and it runs to 400%. Above 100% exaggerates whatever the two channels do not share, so it thins the centre as it widens.

**Balance** left/right level balance, the ordinary pan-style control.

**Rotation** turns the whole stereo field by an angle, in degrees. Unlike Balance it moves the image rather than just levelling it, so the centre travels with everything else.

**MonoBass** below this frequency the side is removed, so the low end collapses to mono. 0 is off. The classic "mono maker".

**PhaseL** inverts the phase of the left channel.

**PhaseR** inverts the phase of the right channel.

**Swap** swaps left and right.

**Mono** collapses the output to mono.

**Output** output gain, applied last.

## Signal flow

The order is fixed, and knowing it explains most surprises:

**1** Phase and Swap

**2** Encode to mid/side

**3** Width and MonoBass act on the side

**4** Decode back to left/right

**5** Rotation, then Balance, then Mono, then Output

Because Width and MonoBass act on the side, they are the only two controls that change the image itself; the rest move or level it.

## Usage

MonoBass around 100-150 Hz is the standard move: it keeps the low end centred and phase-solid on big systems while leaving the top as wide as you like. Set Width after MonoBass, not before, or you will be judging a width the mono-maker is about to take back.

Check Mono constantly, because Width above 100% is exactly the setting that sounds huge in stereo and vanishes in mono - you are boosting the part mono subtracts away. If a stereo source sounds hollow and disappears in mono its channels are already out of phase, so flip one and re-check. A few degrees of Rotation fixes a lopsided recording far more naturally than Balance does.

## Notes

Width 0%, Mono on, and MonoBass at maximum are three different routes to a mono signal and are not interchangeable: Width 0% removes the side, Mono sums the output, and MonoBass only collapses below its corner.

## Related Organisms

Console, Mixer, Gain, SoundSpace
