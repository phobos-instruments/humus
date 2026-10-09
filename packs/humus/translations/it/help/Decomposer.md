# Decomposer

Audio in ingresso, note MIDI in uscita: un rilevatore di intonazione che trasforma in note quello che suoni mentre lo suoni, una linea alla volta o un accordo intero.

Imposta Voices a 1 e segue una sola voce: è l'impostazione migliore per una linea di basso, una melodia cantata, un lead o una chitarra suonata una nota alla volta. La ricerca del pitch è limitata all'intervallo tra LowNote e HighNote, che è la sua difesa più forte contro gli errori d'ottava; i glitch di un singolo frame vengono smussati, e una nota ripetuta riparte dal suo attacco invece di fondersi in un'unica nota tenuta. Restituisce quello che sente senza correzioni di tonalità o di scala; per intonare il risultato, fai passare le note per un Trellis. Il display nell'editor mostra la nota che sta inviando e quanto la sorgente è crescente o calante. Collegaci un SoundIn e collega la sua uscita MIDI a uno strumento qualsiasi, o a un MidiMonitor per vederla. Aspettati qualche decina di millisecondi di latenza di rilevamento.

## Parametri

**Voices** Quante note alla volta. A 1 segue una linea singola, con la risposta più rapida e il rilevamento più stabile. Sopra 1 ascolta un accordo e invia ogni nota che trova, fino a sei.

Da 2 in su funziona in modo diverso: invece di seguire un solo pitch scompone il suono, dà un punteggio a ogni nota che potrebbe sentire in base alle parziali che quella nota produrrebbe, prende la più forte, toglie quello che quella nota spiega e cerca di nuovo in quello che resta. È questo che gli permette di distinguere un accordo da una nota singola dal timbro ricco. Costa tempo: per essere sicuro di un accordo gli serve circa un terzo di secondo di suono, contro qualche decina di millisecondi per una linea singola, quindi Voices 1 resta l'impostazione giusta per tutto quello che si suona una nota alla volta.

Chiedi il numero di note che prevedi di suonare, non di più. Chiederne sei su un accordo di tre note non trova tre note in più, ma dà tre occasioni in più di sbagliare. È più sicuro sui suoni con una serie armonica chiara e un livello stabile: un pianoforte, una chitarra pulita, un organo, un pad di synth. È meno sicuro sull'ottava più bassa, dove le note sono troppo vicine per separarle in fretta, e su tutto ciò che è percussivo o soffiato, dove non c'è una serie armonica da scomporre. Sensitivity stabilisce quanto piano può essere una nota, rispetto alla più forte dell'accordo, prima di essere esclusa.

**Sensitivity** Abbassa le soglie di livello e di chiarezza. Alzala per sorgenti piano o soffiate, abbassala se il rumore fa partire note vaganti.

**Response** Per quanti frame un pitch deve restare stabile prima di essere confermato. Fast è più scattante, Accurate è più stabile sulle sorgenti rumorose, Balanced sta nel mezzo.

**LowNote** La nota più bassa che rileverà. Alzala fino all'estensione reale della tua sorgente, così il rimbombo più in basso non può essere scelto.

**HighNote** La nota più alta che rileverà. Abbassala fino all'estensione reale della tua sorgente, così gli errori d'ottava più in alto non possono essere scelti.

**Channel** Il canale MIDI su cui escono le note.

## Ricetta

**Un accordo verso un pad** Collega al Decomposer un SoundIn da una chitarra o da una tastiera, Voices 4, LowNote 40, HighNote 84, e la sua uscita a un Rhizome o a un Sampler. Suona un accordo e tutta la sua forma esce dall'altra parte. Il display mostra le note che sta inviando, dalla più bassa, e la barra del timbro accanto mostra quanto è armonico quello che sente: una barra piena è un accordo pulito, una vuota è rumore a cui non darà un nome.

**Dalla voce al synth** Collega al Decomposer un SoundIn da un microfono e la sua uscita a un Rhizome. Response Balanced, Sensitivity 0.5, LowNote 48, HighNote 84 per un'estensione vocale tipica. Canticchia una linea e il synth la raddoppia; aggiungi un Trellis tra i due per restare in tonalità.

## Organism correlati

Trellis, OscMonitor, MidiMonitor
