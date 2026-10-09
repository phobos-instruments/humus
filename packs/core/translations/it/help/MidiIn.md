# MidiIn

MIDI dal vivo da un dispositivo hardware o da un'altra applicazione, consegnato su un'uscita MIDI.

Collega la sua uscita a uno strumento come Rhizome, a un MidiBus, o a un MidiMonitor per vedere cosa arriva. La patch memorizza un numero di porta invece del nome di un dispositivo, così una patch spostata su un'altra macchina usa il dispositivo che quella macchina assegna alla stessa porta; i dispositivi si assegnano alle otto porte nelle impostazioni, e un cambio di porta ha effetto subito. La tastiera a schermo in fondo all'editor delle proprietà suona le note direttamente nell'uscita di questo organism, così puoi provare gli strumenti collegati a valle senza hardware. Illumina anche i tasti che tieni premuti su una tastiera collegata, così vedi cosa sta arrivando. Le patch più vecchie usano da MidiIn1 a MidiIn8, che si caricano come questo organism con Port già impostato di conseguenza.

## Parametri

**Port** Le porte d'ingresso MIDI che questo organism ascolta. All inputs, il valore predefinito per un nuovo MidiIn, sente tutte le tastiere e i controller collegati insieme; una porta singola sente solo il dispositivo assegnato a quella porta, e il menu a tendina mostra ogni porta con il suo dispositivo.

## Ricetta

**Dalla tastiera al synth** Metti un MidiIn nella patch, scegli la porta a cui è assegnata la tua tastiera, collegalo a un Rhizome e il Rhizome al Mixer. Clicca prima i tasti a schermo per sentire la voce, poi suona l'hardware.

## Organism correlati

MidiOut, MidiMonitor
