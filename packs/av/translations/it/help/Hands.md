# Hands

Un tracker delle mani che trasforma i movimenti delle dita e della mano in MIDI e in valori di controllo.

Mostra una mano alla videocamera e ogni dito, la posizione del polso e il pinch diventano segnali smussati su un'unica uscita MIDI, senza audio. Collegalo a un organism synth o a un MidiOut, oppure fai clic destro su una manopola qualsiasi e scegli Hands sotto Controlla con per pilotare un parametro senza cavo. Le due mani vengono tracciate in due banchi fissi, sinistro e destro: il banco sinistro manda i CC da base +0 a +8 (apertura di Thumb, Index, Middle, Ring e Pinky, X, Y, Present, Pinch) e il banco destro da +9 a +17. Quattro slot dei gesti mandano ognuno una nota quando la mano dal vivo corrisponde a una posa imparata. L'ingresso video accetta un cavo da CameraIn, VideoPlayer o qualsiasi organism video; finché c'è un cavo collegato, il tracker legge quell'immagine e la sua videocamera resta chiusa.

## Insegnare i gesti

Premi Impara su uno slot e tieni la posa per circa un secondo e mezzo; la forma media diventa il modello e viene salvata nella patch. Premi Rinf e mostra di nuovo la stessa posa da un'altra angolazione per aggiungerla al modello. Il segno sulla barra di corrispondenza è la soglia di scatto di quello slot; trascinalo a destra così solo una posa decisa manda la nota. Quando corrispondono più modelli, il migliore zittisce gli altri.

## Parametri

**Enabled** Apre la videocamera. Quando è disattivato non si cattura niente, e un cavo video collegato tiene comunque la videocamera chiusa.

**Camera** Quale dispositivo di cattura aprire, quando ce n'è più di uno.

**Mirror** Specchia l'immagine, così se ti muovi a destra anche la lettura va a destra. Se l'organism sorgente specchia già, disattiva uno dei due.

**BuiltIn** Usa il tracker integrato invece di quello del sistema. È più lento, e gira già ovunque il sistema non abbia un tracker suo.

**TwoHands** Traccia una seconda mano nel banco destro. Disattivato segue una mano sola.

**Confidence** Quanto deve essere sicuro il rilevatore prima di accettare una mano. Alzalo se lampade o facce creano mani fantasma, abbassalo per i guanti o con poca luce.

**Smooth** Smussatura della risposta in millisecondi. Basso è nervoso, alto è calmo e più lento.

**Tolerance** Quanto una posa dal vivo può scostarsi da un modello e contare comunque come corrispondenza. Le corrispondenze sono continue, quindi possono crescere come velocity invece di scattare.

**SendCC** Manda i diciotto valori di movimento come controller continui. Disattivato lascia solo le note dei gesti.

**SendNotes** Manda una nota per ogni gesto riconosciuto. Se lo disattivi, rilascia qualsiasi nota tenuta in quel momento.

**SendOSC** Pubblica anche ogni valore sotto il nome OSC dell'organism.

**MidiCC** Il numero del primo CC; i diciotto valori proseguono in ordine crescente da lì.

**MidiChannel** Il canale MIDI per i CC e le note.

**Gestures** I modelli imparati, memorizzati nella patch. Li scrivono Impara e Rinf; non c'è niente da digitare.

**GNote_1** La nota che manda lo slot 1, con la velocity data dalla forza della corrispondenza, e così via da 2 a 4.

**GThresh_1** La forza di corrispondenza oltre la quale scatta lo slot 1; il rilascio sta appena sotto, così una posa incerta non fa sfarfallare la nota, e così via da 2 a 4.

## Ricetta

**Filtro col pinch** Enabled attivo, TwoHands disattivato, Smooth 150. Fai passare una sorgente audio per un Filter, fai clic destro sulla manopola della frequenza di taglio del Filter, scegli Hands sotto Controlla con e poi pinch. Stringi pollice e indice per chiudere il filtro e apri la mano per lasciar passare il suono.

## Organism correlati

CameraIn, Skeleton, SoundSpace, MidiOut
