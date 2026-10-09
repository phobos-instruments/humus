# MidiFilter

Lascia passare le note e i controller scelti, trattiene gli altri, o ne trasforma uno in un altro.

Collega una tastiera, un MidiIn o un sequencer all'ingresso, e l'uscita a uno strumento o a un MidiOut. Parte con una regola vuota segnata da un più: scegli Note o CC al suo interno per creare una regola, e sotto compare la successiva vuota, fino a otto. Scegli Off per togliere una regola. Quando le regole sono più di quante ne mostri il pannello, scorre. Clicca un riquadro di nota e suona un tasto sulla tua tastiera MIDI per scegliere quella nota. Ogni regola ascolta una nota o un controller, o un intervallo di note o di controller, e decide cosa succede: Block lo ferma, To note lo inoltra come un'altra nota, To CC lo inoltra come controller. Le regole si leggono dall'alto in basso e vince la prima che corrisponde, quindi una regola stretta sopra una larga crea un'eccezione. Un intervallo si sposta tutto insieme: da C1 a B1 mandato su C4 suona l'intera ottava tre ottave sopra. Una nota rilasciata arriva sempre alla nota a cui era andata quando l'hai premuta, anche se cambi una riga mentre la tieni.

## Parametri

**In1** Cosa ascolta la riga 1: Off, Note o CC. E così via per le righe da 2 a 8.

**Low1** Il numero di nota o di controller più basso a cui risponde la riga 1. E così via per le righe da 2 a 8.

**High1** Il numero di nota o di controller più alto a cui risponde la riga 1. Impostalo uguale a Low per uno solo. E così via per le righe da 2 a 8.

**Do1** Cosa fa la riga 1 con ciò che corrisponde. Block lo ferma. To note manda una nota, e un controller diventa una nota che suona quando il suo valore arriva a metà e si rilascia quando torna sotto. To CC manda un controller, e una nota diventa quel controller, con la sua velocity quando la premi e a zero quando la rilasci. E così via per le righe da 2 a 8.

**Target1** La nota o il controller a cui la riga 1 manda la corrispondenza più bassa; il resto dell'intervallo segue in ordine. E così via per le righe da 2 a 8.

**Others** Cosa succede a tutto ciò che nessuna riga intercetta, compresi pitch bend, pressione e program change. Pass lo lascia passare. Block tiene solo quello che mandano le righe.

## Ricetta

**Un pad che muove un filtro** Riga 1: In Note, Low e High C1, Do To CC, Target 74. Collega in ingresso il controller a pad e l'uscita a un synth con la frequenza di taglio mappata su CC 74: più forte colpisci C1, più si apre il filtro, e quando lasci si richiude.

**Solo la batteria** Riga 1: In Note, Low C1, High B2, Do To note, Target C1. Others Block. Tutto quello che sta sopra le due ottave più basse, e ogni manopola, resta fuori dalla drum machine.

## Organism correlati

MidiIn, MidiOut, MidiMonitor, Cluster
