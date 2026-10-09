# Deck

Un deck da DJ per file audio e video: fader del pitch, griglia dei beat, cue, loop e controllo con vinile timecode.

**Vinile timecode** Testato e supportato con il Traktor timecode vinyl MK2 e con Phase Essential. Attiva Timecode, collega il giradischi agli ingressi del deck attraverso un organism Phono, e il disco guida il piatto: scratch, backspin, e se alzi la puntina si ferma. Il deck riconosce la stampa e impara il suo tono quando la puntina si posa. Se a velocità normale il piatto corre ancora troppo veloce o troppo lento, suona il disco a 33 (o 45) e premi Calibrate: quel tono diventa la velocità normale del deck, salvata con la patch, e il deck smette di rimetterla in discussione. Premi Calibrate con la puntina alzata per tornare a fargliela imparare da solo. Il disco può girare prima dell'inizio del brano o oltre la sua fine: lì il deck suona silenzio e torna a tempo quando ci torna il disco. Scegli il disco nella lista sotto Timecode (e 45 rpm se suona a 45): allora la velocità normale arriva dal disco stesso, e Absolute fa seguire al brano la posizione della puntina, così un needle drop cade nel punto giusto del brano e con lo scratch la posizione non può slittare. La prima volta che scegli un disco la sua tabella impiega qualche secondo a prepararsi, e nel frattempo il deck segue la velocità del disco. Se non viene agganciato niente, guarda lo scope accanto all'interruttore: un anello pulito è timecode, una linea diagonale significa che entrambi i canali portano la stessa cosa, una nuvola significa che non è affatto timecode.

**Beatmatching** Quando carichi un brano vengono rilevati BPM, griglia e tonalità. Metti due deck su Sync ed entrambi seguono il tempo della patch, a tempo con ogni sequencer e delay della patch.

**Video** Carica un video e il deck ne suona l'audio e invia l'immagine dall'uscita video; l'immagine segue il piatto, scratch compresi. PictureSync allinea l'immagine al suono in millisecondi se il tuo schermo è in ritardo o in anticipo.

**Peak** Premi Picco e il deck mette in loop la parte più forte del brano per otto beat, a partire dal beat precedente quando Quantize è attivo, così puoi regolare i livelli sul colpo più grosso. Premi di nuovo per la seconda parte più forte, e ancora per la terza; tutte e tre sono segnate sulla panoramica. Il numero accanto al pulsante è il campione più forte del brano, e diventa rosso al fondo scala. L'immagine di un video segue, quindi va in loop anche la sua scena più forte.

**La testina ovunque** Fai clic destro su qualsiasi manopola della patch e trovi la posizione, la velocità e i secondi del deck sotto Controlla con.

## Parametri

**File** Il file audio o il video che il deck suona.

**AudioTrack** Quale traccia audio di un video suonare, quando ne ha più di una.

**Record** Arma il deck: ciò che arriva ai suoi ingressi mentre la patch registra finisce nella cartella Recordings del progetto e torna su questo piatto.

**FileSound** Il suono riprodotto quando non è quello dentro File, impostato da un take registrato.

**Active** Avvia o ferma, con un breve fade così non si sente un clic.

**Loop** Mette in loop tra LoopIn e LoopOut.

**Sync** Segue il tempo della patch e allinea i beat alla griglia.

**Keylock** Cambia il tempo senza cambiare il pitch. Escluso mentre Timecode è attivo.

**PictureSync** Mostrato come Audio delay: sposta l'immagine rispetto al suono, fino a un secondo in entrambe le direzioni, per ogni deck. I valori positivi trattengono il suono dietro l'immagine; se il suono arriva dopo l'immagine, vai in negativo.

**Timestamp** Imprime il tempo della testina del deck sull'immagine che invia.

**Quantize** Aggancia cue e loop al beat della griglia più vicino.

**Timecode** Permette a un disco di controllo di guidare il piatto.

**TimecodeHz** Mantenuto perché le patch più vecchie si aprano invariate; il deck non lo legge più. La calibrazione sta in TimecodeRest.

**TimecodeFlip** Attivalo se il brano suona al contrario quando il disco va avanti.

**TimecodeRest** Il tono di controllo a velocità normale, impostato da Calibrate. 0 significa che il deck lo impara da solo quando la puntina si posa.

**TimecodeCalibrate** Prende come velocità normale il tono che sta arrivando adesso. Se non arriva nessun tono, torna a impararlo da solo.

**TimecodeRecord** Quale disco di controllo è sul piatto. Learn from the record legge la velocità di qualsiasi disco; indicarlo per nome dà la velocità normale esatta e permette ad Absolute di trovare la puntina.

**Timecode45** Il disco suona a 45 invece che a 33.

**TimecodeAbsolute** Il brano segue la posizione della puntina sul disco, meno il solco iniziale del disco. Tra una lettura e l'altra, durante lo scratch e quando va all'indietro, segue la velocità del disco.

**BPM** Il tempo del brano.

**PitchPercent** Il fader del pitch, in percentuale.

**PitchRange** Quanto si estende il fader, dall'8 al 50 percento.

**Volume** Livello d'uscita, non presente sul pannello frontale: regola il livello sul Mixer, oppure mappa questo parametro.

**GridOffset** Dove cade il primo downbeat della griglia, in campioni.

**LoopIn** Inizio del loop, in campioni.

**LoopOut** Fine del loop, in campioni.

**LoopBeats** Lunghezza del loop a beat, da un quarto di beat a 32 beat.

**Peak** Mette in loop la parte più forte del brano; ogni pressione passa alla successiva delle tre più forti.

**Cue** Il punto di cue principale, in campioni. Cue ci salta senza avviare né fermare il deck.

**Key** La tonalità rilevata quando è stato caricato il brano.

**HotCue_1** Il primo hot cue, in campioni; -1 significa non impostato. Premere uno vuoto memorizza la posizione della testina, premere uno memorizzato suona da lì, Shift-clic lo cancella. E così via da 2 a 8.

## Ricetta

**Mix a due deck** Carica un brano su ogni Deck e controlla che la griglia cada sui beat. Metti entrambi su Sync, collegali a un Mixer e fai entrare il secondo da un hot cue sul suo primo downbeat.

## Organism correlati

AudioTrack, Sampler, Mixer, Console, Phono
