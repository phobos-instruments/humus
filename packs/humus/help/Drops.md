# Drops

Water drops as an instrument: random rain of small physical events while the transport plays.

Every drop is a damped sine at a bubble's resonance whose pitch rises as it dies away, which is what makes a drip sound wet. Drops fall at random while the transport runs and stop when it stops, and the same patch renders the same rain every time, so a bounce holds no surprises. Size sets the bubble, Spread how much each drop differs from it, and Pool adds a small resonant tail under the rain. Cord it into a Fern for drops that answer themselves, or into a SoundSpace for a cave.

## Parameters

**Density** Drops per second. A leak at 1, rain at 8, a downpour at 20.

**Size** The bubble's scale. Left is short bright ticks, right is fat drops with real pitch to them.

**Spread** How much each drop differs from Size. At zero every drop is a clone; wide open it rains all sizes at once.

**Chirp** The upward pitch bend inside each drop. At zero the drops are plain pings; past halfway they bloop.

**Pool** A small resonant wet tail under the rain. Dry stone at zero, a cistern at one.

**Width** How far across the stereo field the drops fall.

**Level** Output volume.

## Recipe

**Slow leak** Density 2, Size 0.8, Spread 0.3, Chirp 0.8, Pool 0.5, Width 0.7. Cord it through a Fern with Feedback around 0.5 and every drop returns a beat later. Push Density to 15 and Size down to 0.2 for rain on a window.

## Related Organisms

SoundSpace, Fern, Morse
