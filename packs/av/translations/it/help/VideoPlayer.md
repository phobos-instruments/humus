# VideoPlayer

Un deck video che manda in loop un nastro nella catena video.

Carica un nastro nello slot File, o trascinaci sopra una clip dalla timeline, e collega l'uscita video a un layer di Lumen, a un VideoFX, a un VideoMix o a un VideoOut; lì il nastro scorre, in loop. Rate è il motore, Opacity e Blend decidono come si fonde il layer, e sono tutti parametri come gli altri, così un Follower può guidare la velocità con la musica. L'ingresso MIDI è un trigger di rilancio: qualsiasi note-on riavvolge il nastro e lo fa ripartire, così un deck per ogni corsia di lancio, pilotato da un Sequence o da un PianoRoll, taglia il video a tempo. Il deck gira mentre il suo editor è aperto o mentre qualcosa più a valle guarda la sua uscita, e altrimenti resta fermo.

## Parametri

**File** Il video da riprodurre.

**Follow** Prende la posizione dell'immagine da Position invece di andare alla sua velocità. Collega l'uscita di posizione di un Deck a Position e il video segue il disco: scratcha, dagli una spinta, riavvolgi, e l'immagine va dietro alla mano. Da disattivato, il file scorre per conto suo alla velocità di Rate.

**Position** Il punto del video da mostrare, dall'inizio alla fine, quando Follow è attivo.

**Rate** La manopola Speed, in percentuale. 100% è la velocità originale, 0% tiene un fermo immagine, 200% la raddoppia.

**Opacity** Quanto si vede questo layer sopra quelli sottostanti in un Lumen.

**Blend** Come si combina con quei layer: Normal, Add, Multiply o Screen.

## Ricetta

**Loop tagliato dalle note** Carica un nastro breve, collega l'uscita a un VideoOut e un Sequence all'ingresso MIDI con una nota per battuta, così il nastro riparte sul battere. Metti un LFO lento e leggero su Rate intorno al 100% e il loop respira contro la griglia.

## Organism correlati

Lumen, VideoMix, VideoOut
