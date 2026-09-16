# SideChain

A compressor whose gain reduction is driven by a separate key input.

The Main signal on inlets 1-2 is turned down by the level of the Key on inlets 3-4: cord the kick to the key and the bass or pad to the main, and the main ducks in time with the kick. The key is a detector tap only and never passes through, so cord the kick to your Mixer on its own as well. The Mode combo switches between stereo and mono in place; in mono, inlet 1 is Main and inlet 2 is Key. With nothing on the key the main passes through untouched. For a duck that is identical on every hit regardless of the kick, use SideKick instead.

## Parameters

**Detector** RMS averages the key, so the reduction grabs late and lets go slowly, the classic pumping character. Peak follows every transient exactly.

**Threshold** The key level above which reduction starts, in dB. Set it so the kick clearly crosses it.

**Ratio** How hard the main is turned down once the key is over the threshold. 4 to 8 gives a clear pump.

**Knee** How gradually the reduction sets in around the threshold, in dB.

**Attack** How fast the reduction sets in, in milliseconds. Keep it at a few milliseconds so the kick's transient gets through.

**Hold** How long the reduction stays at full depth after the key drops, in milliseconds.

**Release** How fast the main recovers, in milliseconds. 150 to 250 ms lets it swell back before the next kick.

**Makeup** Gain added after the compression, in dB.

**Mix** Below 1 blends the ducked signal with the dry one, for parallel ducking.

**Listen** Sends the key to the outlets instead of the main, so you can set Threshold by ear. Switch it back off afterwards.

## Recipe

**Pumping bass** Cord the Kick to inlets 3-4 and the bassline to inlets 1-2. Detector RMS, Threshold -20, Ratio 6, Attack 2, Release 180, Mix 1. Cord the Kick to the Mixer on its own cord too, then move Release until the bass rises back just before the next kick.

## Related Organisms

SideKick, Kick, Microdot
