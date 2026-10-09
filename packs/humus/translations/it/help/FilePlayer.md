# FilePlayer

Riproduce file audio nella patch: un file stereo, oppure fino a 32 insieme, uno per uscita.

Usalo per basi, field recording e cue one-shot che non devono seguire il transport. Va per conto suo: Active lo avvia e lo ferma, e con la barra di posizione sul box ti sposti in qualsiasi punto, mentre suona o mentre è in pausa. Ogni pulsante della sua striscia di trasporto è un parametro, quindi fai clic destro su uno qualsiasi per MIDI learn, OSC learn, Controlla con o Segui. Lo Stop del transport lo lascia suonare; premi Stop una seconda volta, quando non sta scorrendo niente, e si ferma con tutto il resto. Loop ripete il file con una pausa di silenzio opzionale tra un giro e l'altro, e AutoRewind decide se ogni avvio riprende da dove si era fermato o dall'inizio. Per loop che restano agganciati al beat, usa invece LoopPlayer. Collega l'uscita a un Mixer o direttamente a SoundOut.

Outputs, in cima al box, lo trasforma in un lettore multi-file con 4, 6, 8, 16, 24 o 32 uscite e una riga di file per ogni uscita. Un file nella riga 1 suona dall'uscita 1 in poi, un'uscita per canale: un file mono occupa un'uscita, un file stereo nella riga 1 occupa le uscite 1 e 2, e un file a sei canali ne occupa sei, mentre le righe che copre restano mute. Tutti i file partono, si fermano, vanno in loop e si spostano insieme, il più lungo decide la durata, e quelli più corti tacciono alla loro fine fino al giro successivo. Cambiare Outputs mantiene i file e tutti i cavi che ci stanno ancora; a 2 torna il lettore stereo.

## Parametri

**File** Il file audio da riprodurre. Sceglilo con il pulsante del file o trascina un file sullo slot.

**File_1-File_32** Il file di ciascuna riga di un lettore multi-file, visibili quando Outputs è più di 2.

**Loop** Riproduce il file di nuovo dall'inizio ogni volta che arriva alla fine. Disattivato lo riproduce una volta e poi tace.

**LoopDelay** Secondi di silenzio inseriti tra la fine di un giro e l'inizio del successivo. Usato solo mentre Loop è attivo.

**AutoRewind** Torna all'inizio ogni volta che Active si attiva. Disattivato riprende da dove la riproduzione si era fermata l'ultima volta.

**Active** Avvia e ferma la riproduzione. Automatizzalo per far entrare un file in un punto preciso della patch.

## Ricetta

**Base pronta al via** Carica il file, AutoRewind attivo, Loop disattivato. Collega l'uscita a un Gain, poi a SoundOut, e mappa Active su un Button così una pressione fa partire la base dall'inizio ogni volta; una seconda pressione la ferma e quella dopo la fa ripartire pulita.

## Organism correlati

SoundIn, LoopPlayer, Drums
