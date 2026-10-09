# Graft

Un cucitore di campioni che taglia fino a otto file audio in frammenti e mette in fila un frammento per slot in un nuovo campione.

Carica i file audio negli slot, scegli come vengono tagliati e come gli slot scelgono i loro frammenti, e il risultato cucito è lo strumento: suona dall'ingresso MIDI, con C4 alla sua velocità originale, o suona da solo come un drone con Play. Ogni taglio si sposta sul passaggio per lo zero più vicino e i frammenti vicini si sovrappongono con un crossfade a potenza costante, così una giunzione tra registrazioni che non c'entrano niente l'una con l'altra passa senza clic. Lo stesso Seed ricostruisce sempre lo stesso risultato, così una patch salvata si riapre com'era, e Randomizza ne tira fuori uno nuovo senza toccare le altre impostazioni. Collega un PianoRoll o un DNA all'ingresso MIDI, oppure attiva Play e collega l'uscita a un Fern o a un Filter.

## La mappa

Ogni banda ha il colore del file da cui viene, come lo slot qui sopra. Clicca una banda per sentire quel frammento da solo. Trascina in su o in giù per riaccordarlo, trascina di lato per passare agli altri frammenti dello stesso file, e tieni premuto Shift mentre trascini di lato per passare invece da un file all'altro. Con il clic destro puoi invertire o silenziare lo slot, rimescolarlo da solo o tenerlo. Uno slot tenuto conserva il suo frammento mentre tutto intorno viene rimescolato.

## Parametri

**File1** Un file sorgente da tagliare, e così via per File2 fino a File8. Qualsiasi file audio.

**Slices** Quanti slot ha il risultato, da 4 a 64.

**Order** Come scelgono gli slot. Random prende qualsiasi frammento di qualsiasi file; In order e Round robin percorrono i file caricati uno slot alla volta; Blocks dà a ogni file una serie di slot contigui; Mirror costruisce la prima metà a caso e la ripete ribaltata all'indietro; One source riordina solo il file indicato da Only.

**Cut** Da dove vengono i frammenti. Free dà a ogni file tanti pezzi quanti sono gli slot, Grid taglia su un valore di nota al tempo del transport, Onsets taglia sui transienti.

**Div** La lunghezza dei frammenti in modalità Grid, come valore di nota.

**Length** In modalità Free, quanta parte di ogni frammento naturale viene presa, dal 10 al 200 percento. Corto balbetta, lungo si sovrappone.

**Sense** In modalità Onsets, quanti transienti valgono come tagli.

**Xfade** La sovrapposizione a ogni giunzione, in millisecondi.

**Seed** Mostrato come Creature. Lo stesso numero ricostruisce sempre lo stesso risultato.

**Bars** 0 lascia il risultato della lunghezza che viene. Qualsiasi altro numero lo ricampiona perché duri esattamente quel numero di battute al tempo attuale, il che sposta il pitch fino a un'ottava; la riga di stato dice di quanto.

**Normalize** Porta il risultato finito appena sotto il fondo scala.

**Source** Mostrato come Only: quale file usa One source.

**Play** Suona il risultato di continuo come un drone, senza bisogno di una nota. Le modifiche lo ricostruiscono sotto senza interruzioni.

**Attack** Attacco dell'inviluppo in millisecondi, per ogni nota.

**Decay** Decadimento dell'inviluppo in millisecondi.

**Sustain** Livello di sostegno dell'inviluppo.

**Release** Rilascio dell'inviluppo in millisecondi.

**Loop** Una nota tenuta riparte da capo invece di fermarsi alla fine.

**Reverse** Suona dalla fine.

**Speed** Velocità di riproduzione, da un quarto a quattro volte, senza spostare la tastiera.

**Start** Il punto del risultato in cui comincia una nota. Trascina la striscia sotto la mappa per posizionarlo.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore più comune; zero ignora la rotella.

**Voices** Quante note possono suonare insieme, fino a 16.

**SliceEdits** Le modifiche per slot fatte sulla mappa, cioè riaccordatura, inversione e mute, salvate con la patch.

**Pins** Gli slot tenuti, salvati con la patch.

## Ricetta

**Break cucito** Carica un loop di batteria, una voce e una registrazione ambientale in tre slot. Cut Onsets, Sense 0.5, Slices 16, Order Random, Bars 2, Play attivo. Premi Randomizza finché uno slot non viene bene, fai clic destro e tienilo, poi rimescola finché il resto non va al suo posto. Fai passare l'uscita per un Fern.

## Organism correlati

Leafcutter, Sampler, Paulstretch, PianoRoll
