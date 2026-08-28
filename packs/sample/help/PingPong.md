# PingPong

A stereo ping-pong delay: echoes alternate between the left and right channel (each side regenerates from the other side's echo).

## Parameters

**Time ms** Delay time in milliseconds (1.. 2000), when Sync is off.

**Feedback** Echo regeneration (0.. 0.95 - capped below unity for stability).

**Mix** Dry/wet balance.

**Sync** Lock the delay to an eighth note of the transport tempo.

This is the SDK's "real world" example: it reuses the SDK DelayLine helper and tempo-syncs via the Transport. Source: packs/sample/organisms/PingPong/.
