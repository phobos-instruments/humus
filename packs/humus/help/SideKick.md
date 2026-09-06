# SideKick

The fake sidechain, volume-shaper edition: a gain curve is scanned over a synced interval, locked to the transport grid - phase 0 lands exactly on the beat, so the classic duck dips where the kick lives. No key input, no thresholds, no detector: the shape is the pump, identical every cycle. This is how most modern club records do it.

## Inlets

**1-2** The signal to shape (bass, pads, the whole bus).

## The shape

While the transport runs a cursor rides the curve, so you can see where in the cycle you are and how deep the duck is at that moment. The same two readings, phase and gain, are control values: right-click any knob, Control with, and pick SideKick's gain to pump something else in step.

Ten factory tiles below the curve - ducks of varying laziness, a slow swell, a blink, three trance-gate patterns, a riser and a triangle. Click one to load it, or draw directly on the curve: drag to sketch any shape freehand; it snaps to an efficient set of breakpoints on release. The curve is stored in the patch, so custom shapes travel with your session.

The small buttons under the dB scale rework whatever is loaded. The arrows slide the shape earlier or later by a 32nd of the cycle, an eighth with Shift held, wrapping round the end: a duck nudged early pumps ahead of the kick, a gate pattern shifts by a step. REV plays the shape backwards in time, so a duck that dips on the beat becomes a swell that lands on it and a riser becomes a fall. INV flips the gain, turning dips into peaks: the complement of a gate pattern, handy on a second SideKick so two parts take turns.

## Dialing it in

**Sync** the cycle length: 1/4 bar (one beat) is the classic pump; 1 bar turns gate patterns into full-bar phrases; 1/16 chops.

**Smooth** rounds hard gate edges (milliseconds) - raise it if square shapes click, lower it for tighter chops.

**Mix** 1.0 replaces the signal; less layers the shaped copy over the dry for parallel pumping.

SideKick only runs while the transport plays; stopped, it passes the signal through untouched. For level-dependent ducking keyed by real audio (a live kick, an unquantized groove), use SideChain - the compressor. SideKick is the deterministic one.

## Related Organisms

SideChain (the compressor version), Kick, Microdot
