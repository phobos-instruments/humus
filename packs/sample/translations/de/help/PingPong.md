# PingPong

Ein Stereo-Ping-Pong-Delay: Die Echos wechseln zwischen linkem und rechtem Kanal (jede Seite speist sich aus dem Echo der anderen Seite).

## Parameter

**Time ms** Delay-Zeit in Millisekunden (1.. 2000), wenn Sync aus ist.

**Feedback** Rückkopplung der Echos (0.. 0.95 - knapp unter 1 begrenzt, damit es stabil bleibt).

**Mix** Balance zwischen Direkt- und Effektsignal.

**Sync** Rastet den Delay auf eine Achtelnote des Transport-Tempos ein.

Dies ist das Beispiel "aus der Praxis" im SDK: Es nutzt den SDK-Helfer DelayLine wieder und synchronisiert sich über den Transport auf das Tempo. Quelle: packs/sample/organisms/PingPong/.
