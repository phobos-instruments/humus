# MidiPlayer

Un lettore di partiture che carica un file MIDI, suona tutti e sedici i suoi canali dalle proprie uscite audio e passa la partitura oltre da un'uscita MIDI, così anche altri strumenti possono suonarla.

Legge partiture .mid e .midi, file karaoke .kar e .mus, il formato di partitura compatto dei primi sparatutto in prima persona, e non ha bisogno di altri collegamenti: scegli un file e premi Riproduci. Di default le voci sono in modulazione di frequenza, un piccolo banco di impostazioni degli operatori nella tradizione degli home computer e delle schede audio, ed è per questo che suona così; trascina un banco campionato nello slot Bank e suonerà invece delle registrazioni. Host tempo suona la partitura a tempo con il transport; disattivato, le lascia la sua mappa dei tempi. Collega le uscite audio a un Fern o a una Console. L'uscita MIDI porta le note, i program change, i pitch bend e i controller contenuti nel file, così un synth collegato lì suona la stessa partitura con la sua voce; un canale in mute tace anche lì, e quando fermi la riproduzione ogni nota viene rilasciata. Per l'audio registrato c'è FilePlayer, per una partitura che arriva dall'esterno MidiIn.

## Parametri

**File** La partitura da suonare. Clicca lo slot per sceglierne una o trascinaci dentro un file.

**Bank** Gli strumenti che suonano i canali. Il banco di chip .wopn incluso è in modulazione di frequenza; un banco campionato .sf2 sostituisce i chip con registrazioni e ignora Chips.

**Play** Avvia e ferma la partitura, come il pulsante Riproduci sulla barra di trasporto; ogni pulsante di quella barra è un parametro, quindi fai clic destro su uno qualsiasi per MIDI learn, OSC learn, Controlla con o Segui. Fermarla silenzia ogni nota che sta suonando; premere Riproduci quando la partitura è finita la fa ripartire dall'inizio. Va con il suo clock, quindi lo Stop del transport la lascia stare; premi Stop una seconda volta, quando non sta girando niente, e si ferma insieme a tutto il resto.

**Loop** Torna all'inizio quando la partitura finisce, invece di tacere.

**FollowHost** Host tempo: se è attivo, il tempo del transport guida la partitura; se è disattivato, la partitura tiene il tempo con cui è stata scritta, compresi i cambi di tempo voluti dal compositore.

**Chips** Quanti chip sonori suonano insieme, da uno a quattro. Uno vuol dire sei voci, e un arrangiamento fitto perde note; quattro ne danno ventiquattro.

**Level** Livello d'uscita.

**Program1** Lo strumento che suona il canale 1. 0 suona quello che chiede il file; qualsiasi altro valore lo sostituisce per quel canale. Lo stesso vale da Program2 a Program16. Il canale 10 è quello della batteria e resta batteria.

**Mute1** Silenzia il canale 1, sia sulle uscite audio sia sull'uscita MIDI, e così via da Mute2 a Mute16. Un canale su cui il file caricato non suona mai diventa sbiadito e smette di rispondere, così quello che resta illuminato è ciò che il file contiene davvero.

## Ricetta

**Partitura d'epoca** Carica un .mid, Host tempo disattivato, Chips 4, Loop attivo, Level 0.7, e premi Riproduci. Metti in mute il canale che vuoi suonare tu e fai passare le uscite per un Fern con uno slap corto.

**Prestagli una voce** Carica una partitura, metti in mute il canale che vuoi sostituire e collega l'uscita MIDI a un synth. Il file suona come è scritto, con una parte cantata da qualcosa di tuo.

## Organism correlati

FilePlayer, MidiIn, pH
