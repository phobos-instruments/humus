# MidiOut

Manda il MIDI che arriva al suo ingresso a un dispositivo hardware o a un'altra applicazione.

Collegagli un PianoRoll, un Microdot, un Arpeggiator o un MidiIn e le note e i controller escono dalla porta che scegli. La patch memorizza il numero di porta invece del nome del dispositivo, quindi su un'altra macchina si apre con il dispositivo che lì è assegnato a quella porta. I parametri dei messaggi rendono l'organism anche una sorgente MIDI a sé: cambiare Value manda il Controller scelto sul Channel scelto, e commutare Gate manda un note-on e un note-off, così una corsia di automazione o una manopola mappata possono pilotare hardware esterno senza nessun cavo MIDI. Abbinalo a un MidiMonitor per vedere cosa esce.

## Parametri

**Port** Quale delle otto porte d'uscita MIDI porta i messaggi. Assegna un dispositivo a ogni porta nelle impostazioni MIDI; il menu a tendina mostra ogni porta con il suo dispositivo.

**Channel** Il canale MIDI, da 1 a 16, su cui escono i messaggi di Controller e Gate. Le note che arrivano all'ingresso mantengono il proprio canale.

**Controller** Il numero di controller che Value invia. Se lo cambi, il Value attuale parte subito sul nuovo numero.

**Value** Il valore del controller, da 0 a 127. Ogni cambio manda un messaggio di controller, quindi automatizzalo o mappalo su una manopola per muovere un parametro dell'hardware dalla patch.

**Note** Il numero di nota che Gate suona, 60 di default.

**Velocity** La velocity del note-on che Gate invia.

**Gate** Attivo, manda un note-on di Note con velocity Velocity; disattivato, manda il note-off corrispondente. Mappalo su un pulsante o su un pad.

## Ricetta

**Filtro hardware in movimento** Imposta Port sulla porta a cui è assegnato il tuo synth, Channel sul canale su cui ascolta e Controller sul numero della frequenza di taglio del suo filtro. Automatizza Value sulla timeline, o modulalo con un LFO, e il filtro del synth segue la patch; collega un PianoRoll all'ingresso per suonare le note sulla stessa porta.

## Organism correlati

MidiIn, MidiMonitor
