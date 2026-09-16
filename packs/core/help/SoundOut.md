# SoundOut

The audio output: everything it receives goes to the interface.

Cord the Mixer or any final organism into it and choose the output pair; the Output dropdown lists the interface's own output names, and the meter shows what leaves, after Gain. The first SoundOut in a patch is the master, the one the toolbar meter shows, and a new patch starts with one. Drop more for monitor sends, headphone cues or surround stems, each pointed at its own pair; where two address the same channel their signals sum. A SoundOut adopts its channel count from what is corded into it, at least stereo, so a six-channel feed occupies six consecutive device outputs starting at the chosen channel. Older patches with numbered auxiliary outputs still load, each carrying its channel as a parameter.

## Parameters

**Channel** Which device output pair this SoundOut feeds, shown as Output; 1/2 is the main pair. Changing it applies live.

**Gain** A clean output trim, unity by default. The meter reads after it, so it shows exactly what leaves for the interface.

**DcGuard** A gentle 5 Hz high-pass on the way out, on by default, so a constant value from a Number or a Follower becomes a short click and then silence instead of a held speaker cone. Switch it off only on an output feeding a DC-coupled interface for control voltage.

## Recipe

**Headphone cue** Keep the first SoundOut on 1/2 for the room and add a second set to 3/4 with the headphones on outputs 3 and 4 of the interface. Cord a Send's send pair, or a second Mixer, into it and set its Gain on its own, so the cue level never touches the main mix.

## Related Organisms

SoundIn, Mixer, FileRecorder
