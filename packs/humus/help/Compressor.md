# Compressor

A feed-forward compressor with a soft knee, a hold stage and a tempo-synced release.

It measures the input, decides on a gain and applies it, so loud moments are turned down and the distance between loudest and quietest gets shorter. Detection is linked across channels: one gain is worked out and applied to every side, so a loud moment on the left cannot pull the image to the right. Set Threshold first, then CompressionRatio, then AttackTime and ReleaseTime, and MakeupGain last. On a part with a pulse, switch Sync on so the recovery lands on a division of the beat and stays there when the tempo changes. There is no key input; to compress one sound with another, use SideChain.

## Parameters

**InputGain** Trims the incoming sound before anything else. Raising it drives more of the sound over the threshold without moving the threshold.

**MakeupGain** Gain added after the reduction, to bring the levelled sound back up.

**AutoMakeupGain** Works the makeup gain out from Threshold and CompressionRatio, so changing either does not also change the level. Handy while dialling in.

**AttackTime** How quickly it responds once the threshold is crossed, in milliseconds. Fast catches the hit; slow lets the hit through and levels what follows.

**HoldTime** How long full reduction is kept before release begins, in milliseconds. Stops the compressor letting go and grabbing again on material that dips briefly.

**ReleaseTime** How quickly it stops once the sound falls back below the threshold, in milliseconds. Ignored while Sync is on.

**CompressionRatio** How firmly the sound is held. 2 means a sound 2 dB over the threshold ends up 1 dB over. Low ratios level, high ratios flatten.

**Threshold** The level where compression starts. Above it the sound is held down, below it nothing happens.

**KneeWidth** How gradually the ratio arrives, in dB either side of the threshold. A wide knee eases in instead of switching on.

**Sync** Times the release to the transport instead of ReleaseTime. Attack stays where you put it.

**SyncUnit** The note value the synced release counts in, such as 1/8.

**SyncMultiplier** How many of that unit make one release.

## Recipe

**Drum bus glue** Threshold 0.4, CompressionRatio 3, KneeWidth 8, AttackTime 20, HoldTime 0, Sync on with SyncUnit 1/8. Cord a Mixer's drum sum in, switch AutoMakeupGain on while setting the threshold, then off once the level sits where you want it.

## Related Organisms

SideChain, Limiter, NoiseGate, TransientShaper
