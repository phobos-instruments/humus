# Skeleton

Un tracker di tutto il corpo che trasforma postura e movimento in MIDI e valori di controllo.

Mettiti davanti alla videocamera e la tua testa, le mani, le braccia alzate, l'inclinazione, l'accovacciamento e l'apertura delle gambe diventano segnali smussati su un'unica uscita MIDI, senza audio. Collegalo a un organism synth o a un MidiOut, oppure fai clic destro su una manopola e scegli Skeleton sotto Controlla con per guidarla senza cavo. Dodici valori viaggiano sui CC da base +0 a +11: testa X e Y, mano sinistra X e Y, mano destra X e Y, braccio sinistro e destro alzati, inclinazione, accovacciamento, apertura delle gambe e presenza. Sinistra e destra sono le tue, comunque Mirror ribalti l'immagine. Si traccia un solo performer alla volta, inquadrato dalle ginocchia o dai piedi in su, e una lettura con le articolazioni nascoste tiene il suo ultimo valore. L'ingresso video accetta un cavo da CameraIn, VideoPlayer o qualsiasi organism video; finché è collegato, il tracker legge quell'immagine e la sua videocamera resta chiusa.

## Imparare le pose

Premi Impara su uno slot e tieni la posa, braccia incrociate o un pugno alzato, per circa un secondo e mezzo; la forma media diventa il modello e viene salvata nella patch. Premi Rinf e rifalla da un altro punto per aggiungerla al modello. Una posa è la geometria delle braccia, non una posizione nell'inquadratura, quindi scatta ovunque ti trovi. Il segno sulla barra di corrispondenza è la soglia di attivazione, e la corrispondenza migliore esclude le altre.

## Parametri

**Enabled** Apre la videocamera. Quando è disattivato non si cattura niente, e un cavo video collegato tiene comunque chiusa la videocamera.

**Camera** Quale dispositivo di cattura aprire, quando ce n'è più di uno.

**Mirror** Ribalta l'immagine, così se ti muovi a destra la lettura va a destra. Se l'organism sorgente ribalta già l'immagine, disattiva uno dei due.

**Confidence** Quanto deve essere sicuro il tracker prima di accettare una persona. Alzalo se un mobile o un poster continua a essere scambiato per una persona, abbassalo con poca luce.

**Smooth** Smussamento della risposta in millisecondi. Più alto è più calmo e più lento.

**Tolerance** Quanto liberamente una posa dal vivo può aderire a un modello e risultare comunque riconosciuta.

**SendCC** Invia i dodici valori del corpo come controller continui. Disattivato, lascia solo le note delle pose.

**SendNotes** Invia una nota per ogni posa riconosciuta. Disattivarlo rilascia qualsiasi nota tenuta in quel momento.

**SendOSC** Pubblica anche ogni valore sotto il nome OSC dell'organism.

**MidiCC** Il primo numero di CC; i dodici valori salgono da lì.

**MidiChannel** Il canale MIDI per i CC e le note.

**Gestures** I modelli imparati, salvati nella patch. Lo scrivono Impara e Rinf; non c'è niente da digitare.

**GNote_1** La nota che invia lo slot di posa 1, con la velocity data dalla forza della corrispondenza, e così via da 2 a 4.

**GThresh_1** La forza di corrispondenza oltre la quale scatta lo slot 1; il rilascio sta appena sotto, così una posa incerta non sfarfalla, e così via da 2 a 4.

## Ricetta

**Crescendo col braccio** Enabled attivo, SendNotes disattivato, Smooth 200. Fai passare una sorgente audio attraverso un Gain, fai clic destro sulla sua manopola Gain, scegli Skeleton sotto Controlla con e poi raise/r. Alza il braccio destro per far salire il suono e abbassalo per toglierlo.

## Organism correlati

Hands, CameraIn, MidiOut
