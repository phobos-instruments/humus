# FileRecorder

Un organism passante che scrive su un file audio quello che gli arriva mentre la patch suona.

L'ingresso arriva all'uscita così com'è, quindi puoi inserirlo su un cavo in qualunque punto della patch e registrare lì uno stem, non solo sul master. Il menu a tendina Tracks decide quanti file stereo scrive, da 1 a 32, con una riga per ogni traccia; i canali vengono dati ai file in ordine, il campo Ch di ogni riga dice quanti ne prende, e un file che va oltre l'ultimo ingresso registra silenzio. I file vengono scritti fuori dal percorso audio, così un disco lento non può interrompere il suono, e il misuratore di ogni riga mostra cosa arriva, che un take stia girando o no. Il pulsante Registra del transport invece cattura sulla timeline gli organism AudioTrack armati; usa questo organism quando vuoi semplici file su disco.

## Parametri

**File_1** Dove va il file 1, e così via per le righe sotto. Lascialo vuoto e ogni volta che armi viene scritto un take con data e ora nella cartella delle registrazioni, e la riga si compila; un file a cui hai dato tu il nome viene tenuto e riutilizzato.

**RequestedChannelCounts_1** Quanti canali prende il file 1, e così via per le righe sotto. Due di default; zero disattiva la riga.

**FileMode** Overwrite sostituisce il file indicato. Append registra dopo la sua fine, così una sessione si può costruire in più passate.

**PunchMode** Manual e SoundIn Sync si fermano con il pulsante. Timed si ferma da solo dopo RecordDuration e chiude il file.

**RecordDuration** Quanto dura una passata Timed, in millisecondi.

**Record** Arma e disarma tutti i file insieme. Si disattiva da solo quando nessuna riga ha canali da scrivere, e un take va fermato qui invece che abbandonato, perché è fermandolo che si scrive l'intestazione che rende il file leggibile.

## Ricetta

**Stem di batteria** Metti il FileRecorder tra l'organism della batteria e il Mixer, lascia File_1 vuoto, Ch a 2, PunchMode Manual e FileMode Overwrite. Arma Record per la passata e disarmalo alla fine; la riga mostra dove è finito il take, e il mix è andato avanti invariato mentre veniva scritto.

## Organism correlati

AudioTrack, SoundOut, Mixer, Bus
