# SoundOut

L'uscita audio: tutto quello che riceve va all'interfaccia.

Collegaci il Mixer o qualsiasi organism finale e scegli la coppia di uscite; il menu a tendina Output elenca i nomi delle uscite dell'interfaccia, e il misuratore mostra quello che esce, dopo Gain. Il primo SoundOut di una patch è il master, quello che mostra il misuratore della barra degli strumenti, e una patch nuova parte con uno. Aggiungine altri per mandate di monitor, ascolti in cuffia o stem surround, ognuno puntato sulla sua coppia; dove due puntano allo stesso canale, i loro segnali si sommano. Un SoundOut prende il numero di canali da quello che gli colleghi, almeno stereo, così un segnale a sei canali occupa sei uscite consecutive del dispositivo a partire dal canale scelto. Le patch più vecchie con uscite ausiliarie numerate si caricano ancora, ognuna con il suo canale come parametro.

## Parametri

**Channel** La coppia di uscite del dispositivo che questo SoundOut alimenta, mostrata come Output; 1/2 è la coppia principale. Se lo cambi, ha effetto subito.

**Gain** Una regolazione pulita del livello d'uscita, a guadagno unitario di default. Il misuratore legge dopo Gain, quindi mostra esattamente quello che va all'interfaccia.

**DcGuard** Un leggero passa-alto a 5 Hz in uscita, attivo di default, così un valore costante da un Number o da un Follower diventa un breve clic e poi silenzio, invece di tenere spinto il cono dell'altoparlante. Disattivalo solo su un'uscita che alimenta un'interfaccia accoppiata in DC per tensioni di controllo.

## Ricetta

**Ascolto in cuffia** Lascia il primo SoundOut su 1/2 per la sala e aggiungine un secondo impostato su 3/4, con le cuffie sulle uscite 3 e 4 dell'interfaccia. Collegaci la coppia di mandata di un Send, o un secondo Mixer, e regola il suo Gain a parte, così il livello in cuffia non tocca mai il mix principale.

## Organism correlati

SoundIn, Mixer, FileRecorder
