# Leafcutter

Un affettaloop che taglia un file audio sui suoi transienti e suona le slice a tempo con il transport, al loro pitch originale.

Carica un loop e viene tagliato sui suoi transienti; poi le slice seguono il tempo del transport senza time-stretch, così un break resta pulito a qualsiasi tempo. I tempi più veloci troncano ogni slice, quelli più lenti lasciano aria tra un taglio e l'altro. I file di loop già affettati, .rx2 e il più vecchio .rex, si caricano con la loro mappa delle slice, il loro tempo e il loro numero di beat. L'ingresso MIDI suona le slice come un kit, C3 per la prima slice e ogni tasto più su per la successiva, così con un PianoRoll o una tastiera puoi suonare il loop a mano, con o senza il loop che suona da solo sotto. Collega l'uscita a un Filter o a un Fern.

## La mappa

La mappa mostra il loop con un marcatore a ogni taglio e illumina la slice che sta suonando. Trascina in su o in giù su una slice per riaccordare solo quella, e fai clic destro per invertirla, silenziarla o ripristinarla, o per cancellare tutte le modifiche alle slice. Quelle modifiche stanno in SliceEdits e restano con la patch.

## Parametri

**File** Il loop da affettare. Qualsiasi file audio, o un file di loop già affettato con la sua mappa delle slice.

**Play** Il loop che suona da solo. Da disattivato lascia solo il kit MIDI.

**Sense** Quanti dei transienti rilevati diventano tagli. Il rilevamento avviene una volta sola al caricamento, quindi la manopola riaffetta all'istante.

**Gate** Quanta parte della finestra di ogni slice suona, da spezzettato a pieno.

**Pitch** Traspone ogni slice in semitoni, da -24 a 24, ricampionando.

**Reverse** Suona ogni slice al contrario.

**Decay** Fa sfumare ogni slice a partire dal suo inizio. 0 lascia le slice intere; al massimo ogni slice è un colpo breve.

**Shuffle** Seed dell'ordine delle slice. 0 suona il loop com'è scritto; qualsiasi altro numero è un riordino ripetibile.

**Beats** Lunghezza del loop in beat. 0 la prende dal file, o la stima dalla lunghezza e dal tempo.

**Level** Livello d'uscita.

**SliceEdits** Le modifiche per slice fatte sulla mappa, cioè pitch, inversione e mute, salvate con la patch.

## Ricetta

**Break riaffettato** Carica un loop di batteria di due battute, Beats 8, Sense 0.6, Gate 0.7, Shuffle 17, Decay 0.2. Premi Riproduci e il break suona riordinato, a tempo; premi Randomizza per un altro ordine e collega l'uscita a un Filter con LfoSync attivo.

## Organism correlati

Sampler, Deck, PianoRoll, Repeater
