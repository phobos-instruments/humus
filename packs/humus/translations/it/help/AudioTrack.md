# AudioTrack

Il canale audio del sequencer di registrazione: una riga della Timeline che suona le sue clip a tempo con il transport e registra take di qualunque cosa le arrivi via cavo.

Sta in linea nel percorso del segnale, con ingresso e uscita stereo, quindi va a valle della sorgente che vuoi catturare. Quello che ne esce sono le sue clip, il suo ingresso o entrambi, a seconda di Monitor. Un take finisce come clip sulla riga della traccia, e un secondo passaggio sulle stesse battute si posa sopra il primo, così non si perde niente. Anche un file audio trascinato sulla Timeline finisce su un AudioTrack: se sotto il cursore non ce n'è uno, viene creata una nuova riga già collegata al master. Collegalo prima di un Mixer, o direttamente a un SoundOut.

## Parametri

**Record** Arma la traccia. Lo comanda il tasto Registra del transport, quindi di solito la armi dal transport e non a mano.

**Monitor** Cosa arriva alle uscite. In fa passare sempre l'ingresso e aggiunge le clip durante la riproduzione. Auto fa passare l'ingresso mentre la traccia è armata o il transport è fermo, e le clip mentre suona senza essere armata: è la scelta giusta per quasi ogni registrazione. Off suona solo le clip.

**Gain** Livello d'uscita della traccia.

**Mute** Silenzia l'uscita. Visto che la traccia è in linea, silenzia anche qualunque cosa ci passi attraverso.

## Ricetta

**Registrare un synth** Collega un Rhizome all'AudioTrack e l'AudioTrack al master. Lascia Monitor su Auto, arma dal transport, premi Riproduci e suona la parte. Ferma il transport e il take è una clip sulla riga, che puoi spostare e rifilare come qualsiasi altra.

## Organism correlati

Deck, Sampler, PianoRoll, Sequence
