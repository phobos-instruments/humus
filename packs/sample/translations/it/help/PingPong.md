# PingPong

Un delay ping-pong stereo: gli echi si alternano tra il canale sinistro e quello destro (ogni lato si rigenera dall'eco dell'altro lato).

## Parametri

**Time ms** Tempo del delay in millisecondi (1.. 2000), quando Sync è disattivato.

**Feedback** Rigenerazione dell'eco (0.. 0.95 - limitata sotto l'unità per stabilità).

**Mix** Bilanciamento secco/effettato.

**Sync** Aggancia il delay a un ottavo, al tempo del transport.

Questo è l'esempio "dal mondo reale" dell'SDK: riusa l'helper DelayLine dell'SDK e si sincronizza al tempo tramite il Transport. Codice sorgente: packs/sample/organisms/PingPong/.
