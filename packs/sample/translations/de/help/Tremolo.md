# Tremolo

Ein Stereo-Amplitudenmodulator: Ein LFO bewegt die Lautstärke mit Rate Hz und Depth Tiefe.

## Parameter

**Rate** LFO-Geschwindigkeit in Hz (0.1.. 20).

**Depth** Wie tief die Lautstärke absinkt (0 = aus, 1 = völlige Stille im Tal).

**Waveform** LFO-Form: Sine, Triangle oder Square (abgehacktes Gating).

**Sync** Bindet einen LFO-Zyklus an eine Achtelnote des Transport-Tempos statt an die freie Rate.

Dieses Organism ist das "hello world" des SDK - sein vollständig kommentierter Quellcode liegt in packs/sample/organisms/Tremolo/. Siehe packs/sample/README.md.
