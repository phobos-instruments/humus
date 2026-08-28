# Fern

The echo. A fern frond is built from smaller copies of itself, each a little thinner than the last, and that is exactly what this does to a sound: it comes back a little later, darker, thinner and warmer, and then does it again. That progressive loss is the whole difference between an echo that sounds like a place and one that sounds like a buffer read twice. Past three quarters of Feedback it becomes an instrument you can play by hand.

## Moving the time

A delay has to decide what happens when you move its time, and there is no single right answer, so the Glide dropdown carries both.

**Tape** the read point glides to the new time. It travels while the recording head runs at normal speed, so the stored sound is stretched on the way and the repeats bend in pitch - a tape motor changing speed. Its speed is capped, so a big jump swoops rather than dives.

**Fade** the old repeats keep running while a second set opens at the new time, and the two crossfade. No pitch bend at all. Use this when the time follows the transport: changing 1/8 to 1/16 should not swoop every time.

## Parameters

**Mode** where the repeats go. Stereo keeps each side to itself. Ping-pong crosses the feedback, so every repeat lands on the opposite side of the image from the one before.

**Time** how long before the sound returns. Ignored while Sync is on.

**Sync** locks the time to the transport instead of to milliseconds.

**Tap** tap it in time and Time follows your taps, averaged over the last few so jitter cancels. Tapping chooses a free time, so it switches Sync off; a pause starts a fresh chain.

**Feedback** how much of each repeat becomes the next one. Low is a slap, high is a tail that outlives the note.

**Spread** leans the right side away from the left, opening the echo out across the image. At the centre the two run together.

**Body** thins each repeat from below, so a tail steps out of the way of the bass instead of piling up under it.

**Damp** darkens each repeat from above. This is the one that turns a digital-sounding echo into a room, a tape or a bucket brigade.

**Drive** warms each repeat. At zero the loop is mathematically clean; up, the tail thickens and settles as it decays.

**Mix** how much of the echo is heard against the dry sound.

## How it works

Body, Damp and Drive sit inside the feedback loop rather than on the output, which is why they are cumulative: the first repeat passes through them once, the fifth five times. That is what makes a tail sound like it is travelling somewhere.

The read point moves every single sample, which is why moving Time sounds like tape rather than like something breaking. Delays that update in coarser steps splice the waveform at regular intervals, and regular splices are heard not as clicks but as a buzz - which is why they sound distorted rather than glitchy when you turn the time knob.

## Related Organisms

Repeater, SoundSpace, Trellis
