# SoundOut

The audio output organism: everything it receives goes to your audio interface.

## Channel

Which device output pair this SoundOut feeds - the dropdown lists your interface's own output names. Default is 1/2, the main pair. Humus opens addressed channels automatically when the device has them; changing the channel applies live. The bar beside it shows the outgoing level.

## Gain

A clean output trim - a plain level multiply, so it adds no latency and no noise, and it is bit-transparent at 12 o'clock (unity). The meter reads the level after it, so it shows exactly what leaves for the interface.

The first SoundOut in a patch is the master: the toolbar meter and master level, Record Master Mix, and Export to Sound File all read it. Drop more SoundOuts for monitor sends, headphone cues or surround stems, each pointed at its own pair. Where two outputs address the same channel their signals sum.

A SoundOut adopts its channel count from what you wire into it (at least stereo), so a 6-channel surround feed occupies six consecutive device outputs starting at the chosen channel.

Retro-compatibility: older patches use AuxOut1 to AuxOut8 for extra outputs (mono, mapped from channel 3 up). These still load - each carries its channel as a parameter behind the scenes.

## Related Organisms

SoundIn
