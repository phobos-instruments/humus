# Fern

A tempo-syncable stereo echo whose repeats thin, darken and warm as they decay.

Body, Damp and Drive sit inside the feedback loop, so each pass through the loop applies them again: the first repeat is shaped once, the fifth five times. That is what makes a tail sound like it is moving away rather than being copied. Past three quarters of Feedback it holds a note for as long as you like, and the read point moves every sample, so turning Time by hand bends pitch instead of clicking.

## Parameters

**Mode** Stereo keeps each side to itself. Ping-pong crosses the feedback so every repeat lands on the opposite side from the one before.

**Time** Delay time in milliseconds. Ignored while Sync is on.

**Sync** Locks the time to the transport instead of milliseconds.

**SyncMultiplier** How many units of SyncUnit make one repeat while Sync is on.

**SyncUnit** The note value the synced time counts in, such as 1/8 or 1/16.

**Feedback** How much of each repeat becomes the next. Low is a slap, high is a tail that outlives the note.

**Spread** Leans the right side's time away from the left, opening the echo across the image. Zero keeps the two sides together.

**Body** Thins each repeat from below, so a tail steps out of the way of the bass instead of piling up under it.

**Damp** Darkens each repeat from above. This is the control that turns a clean digital echo into a room or a tape.

**Drive** Warms each repeat. At zero the loop is clean; higher, the tail thickens as it decays.

**Glide** What happens when Time changes. Tape slides the read point, so the repeats bend in pitch on the way. Fade crossfades the old repeats into a new set at the new time, with no pitch change; use it when Time follows the transport.

**Mix** How much echo is heard against the dry signal.

## Recipe

**Dub tail** Sync on, SyncUnit 1/8 with a multiplier of 3 for a dotted feel, Feedback around 0.7, Damp 0.6, Drive 0.3, Mode Ping-pong. Send a short stab in and let the tail carry the rhythm; ride Feedback towards the top for a build and back down to clear it.

## Related Organisms

Repeater, SoundSpace, Trellis
