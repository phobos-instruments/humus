# Tremolo

Un modulatore d'ampiezza stereo: un LFO fa oscillare il volume a Rate Hz, con profondità Depth.

## Parametri

**Rate** La velocità dell'LFO in Hz (0.1.. 20).

**Depth** Quanto scende il volume (0 = disattivato, 1 = silenzio totale nel punto più basso).

**Waveform** La forma dell'LFO: Sine, Triangle o Square (gating a scatti).

**Sync** Aggancia un ciclo dell'LFO a un ottavo del tempo del transport invece di girare liberamente a Rate.

Questo organism è il "hello world" dell'SDK: il suo sorgente completo e commentato si trova in packs/sample/organisms/Tremolo/. Vedi packs/sample/README.md.
