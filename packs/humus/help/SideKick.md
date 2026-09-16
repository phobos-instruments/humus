# SideKick

A transport-synced gain shaper that pumps the signal on a drawn curve, with no key input.

A gain curve is scanned once per synced interval, locked to the transport grid so that phase 0 lands on the beat; the classic duck dips exactly where the kick is and is identical every cycle. There is no key, threshold or detector, only the shape. It runs only while the transport plays and passes the signal through untouched when stopped. Its gain and phase are also control values, so any knob can pick Control with SideKick to pump in step. Put it on a bass, a pad or a whole Mixer bus. For ducking keyed by real audio, use SideChain.

## The shape

Click one of the tiles below the curve to load a duck, swell, gate pattern, riser or triangle, or drag on the curve to draw freehand. The arrows under the dB scale slide the shape earlier or later by a 32nd of the cycle, REV plays it backwards and INV flips the gain so dips become peaks.

## Parameters

**Sync** The cycle length: 1 bar, 1/2 bar, 1/4 bar, 1/8 bar or 1/16 bar. 1/4 bar is one beat, the classic pump.

**Mix** At 1 the shaped signal replaces the dry one; lower blends them for parallel pumping.

**Smooth** Rounds hard edges of the curve, in milliseconds. Raise it if a gate shape clicks, lower it for tighter chops.

**Shape** The drawn gain curve, stored with the patch.

## Recipe

**Club duck** Sync 1/4 bar, the first duck tile, Smooth 3, Mix 1, on the bass. Right-click a pad's Gain and pick Control with SideKick gain to make the pad pump on the same curve. Nudge the shape one arrow step early to pump ahead of the kick.

## Related Organisms

SideChain, Kick, Microdot
