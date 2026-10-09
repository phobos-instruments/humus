# VideoPad

Otto pad per clip, con una dissolvenza tra un lancio e l'altro.

Ogni pad contiene una clip: trascinaci sopra un video o clicca la cartella per caricarne uno, clicca l'immagine per lanciarla, e il passaggio alla clip avviene con una dissolvenza lunga quanto Fade. L'ingresso MIDI lancia i pad per nota, il Do centrale per il pad 1 e i semitoni sopra per gli altri; lanciare un pad che sta già suonando lo fa ripartire da In. Randomizza tira a sorte nuovi In, Out e Loop per ogni pad caricato e ne lancia uno. Collega l'uscita principale a un layer di Lumen, a un VideoFX, a un VideoMix o a un VideoOut.


I pad portano il loro suono oltre all'immagine. Una clip lanciata suona il proprio audio dall'uscita stereo, a partire dal suo punto di In, in loop tra In e Out se quel pad va in loop, e passa al pad successivo con la stessa dissolvenza dell'immagine. Mute agisce su entrambi. L'immagine segue il suono invece di correre accanto a esso, così un taglio cade su entrambi nello stesso istante invece di andare fuori sincrono lungo una clip lunga - e un pad premuto mentre sta già suonando torna al suo punto di In, suono e immagine insieme.

## Modifica dei pad

La riga superiore sotto ogni immagine è l'intero nastro, con il tratto da In a Out illuminato; la riga inferiore è il taglio, con la sua testina. Trascina un segno per spostare un punto, trascina sull'immagine per fare scrub, e premi In o Out per fissare nel pad la posizione della testina. Trascina un pad tenendolo per il numero su un altro pad per copiarlo, o sulla timeline come clip.

## Parametri

**File1** La clip del pad 1, e così via da 2 a 8.

**Launch1** Lancia il pad 1. Momentaneo; fai clic destro su un pad e scegli Controllo lancio per mapparlo, e così via da 2 a 8.

**In1** Dove inizia la clip del pad 1, in secondi. Randomizza lo tira a sorte, e così via da 2 a 8.

**Out1** Dove finisce la clip del pad 1, in secondi; 0 arriva fino alla fine del file. Randomizza lo tira a sorte, e così via da 2 a 8.

**Loop1** Se il pad 1 gira in loop tra In e Out o suona una volta sola e resta fermo sull'ultimo fotogramma. Randomizza lo tira a sorte, e così via da 2 a 8.

**Stop** Sfuma l'immagine al nero nel tempo di Fade.

**Mute** Tiene nera l'uscita finché è attivo: un interruttore di blackout che lascia le clip in esecuzione sotto.

**Fade** Quanto dura una dissolvenza, in secondi. 0 è un taglio netto.

**Rate** La manopola Speed, in percentuale, per tutti i pad insieme. 100% è la velocità originale, 0% blocca un fotogramma.

**Pads** 4 pads o 8 pads.

**Outlets** One out porta solo il mix dei lanci. Per pad aggiunge un'uscita video per ogni pad dopo quella principale, così un singolo pad può alimentare un suo VideoFX o un suo tracker.

**Opacity** Quanto si vede l'uscita principale quando alimenta un layer di Lumen.

**Blend** Come si combina lì: Normal, Add, Multiply o Screen.

## Ricetta

**Tagli da drum pad** Carica quattro clip, imposta Pads su 4 pads e Fade su 0.1. Collega un PianoRoll o un MidiIn all'ingresso MIDI e suona il Do centrale e i tre semitoni sopra per tagliare tra le clip sul beat. Collega l'uscita a un VideoOut.

## Organism correlati

VideoPlayer, VideoFX, VideoMix, Lumen
