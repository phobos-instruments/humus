# MidiTrack

Una corsia della timeline che contiene una parte MIDI e la suona con qualsiasi strumento della patch.

Non ha un suono suo e nessun box nel patcher: la parte vive sulla corsia, e l'etichetta della riga indica l'organism che la suona. Scegli una destinazione diversa e le stesse note passano a un altro strumento, intatte, così puoi provare una parte su diversi synth o tenerla al sicuro mentre ricostruisci la patch intorno a essa. Le tracce nuove partono da (nothing) e restano mute finché non scegli una destinazione. Il pulsante che arma la riga manda l'ingresso MIDI dal vivo attraverso la traccia fino alla destinazione, e ciò che suoni finisce in una clip mentre registri. Mute, solo, rinomina ed elimina funzionano dalla riga come per qualsiasi altra traccia.

## Parametri

**Target** Lo strumento che suona questa traccia, mostrato sull'etichetta della riga. La lista offre ogni organism della patch con un ingresso MIDI; quando ne scegli uno, l'uscita MIDI della traccia viene collegata a quell'organism, e un cavo tirato a mano da quell'uscita imposta Target allo stesso modo.

## Ricetta

**Una parte, due synth** Imposta Target su un Rhizome, arma la riga e registra una frase, oppure disegnala nella clip. Aggiungi un secondo strumento, sposta Target su di esso e la stessa clip suona il nuovo synth; torna indietro quando hai confrontato i due.

## Organism correlati

MidiBus, PianoRoll, Steps
