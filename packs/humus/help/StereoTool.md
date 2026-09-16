# StereoTool

A mid/side stereo imaging utility with width, rotation, mono bass and phase controls.

Mid is what both channels share and side is what they do not; splitting a signal into the two lets Width and MonoBass change the image itself while the other controls move or level it. The order is fixed: PhaseL, PhaseR and Swap first, then the mid/side stage where Width and MonoBass act on the side, then Rotation, Balance, Mono and Output. The scope on the editor draws the field and its correlation, so a source that will vanish in mono shows before you hear it. Cord it before a Console or a SoundOut, or on a Send return.

## Parameters

**Width** Side level. 0% collapses to mono, 100% leaves the image alone, and it runs to 400%. Above 100% thins the centre as it widens, so check Mono while setting it.

**Balance** Left and right level balance, the ordinary pan-style control.

**Rotation** Turns the whole field by up to 45 degrees either way. Unlike Balance it moves the image rather than levelling it, so the centre travels with everything else.

**MonoBass** Below this frequency the side is removed and the low end collapses to mono. 0 is off; 100 to 150 Hz is the usual setting.

**PhaseL** Inverts the left channel. Flip one side when a stereo source sounds hollow and disappears in mono.

**PhaseR** Inverts the right channel.

**Swap** Exchanges left and right.

**Mono** Sums the output to mono. Use it to check what Width above 100% is doing.

**Output** Output gain, applied last.

## Recipe

**Mono-safe widen** MonoBass 120, Width 140%, Balance and Rotation at 0. Toggle Mono on and off while listening; if the centre thins too much in mono, bring Width back toward 120%. Set Width after MonoBass, not before, or you judge a width the mono maker is about to take back.

## Related Organisms

Console, Mixer, Gain, SoundSpace
