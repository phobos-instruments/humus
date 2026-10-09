# MidiMonitor

Un registro degli eventi MIDI in righe leggibili: sorgente, canale, tipo e dati.

Tiene d'occhio due flussi insieme. Senza cavi registra tutto ciò che arriva da ogni dispositivo MIDI abilitato, marcato da in1 a in8 a seconda della porta, e dalla tastiera a schermo, marcato kbd, compreso il traffico dei controller che la mappatura dei parametri consuma prima che arrivi a un synth. Collegato in linea tra un MidiIn e un Rhizome, il suo ingresso MIDI diventa un punto d'ascolto limitato a quel cavo, marcato cord, che mostra esattamente cosa riceve quel synth, e l'uscita fa passare gli eventi invariati. Source e Channel filtrano solo la visualizzazione, quindi non si perde niente mentre li cambi. Usalo per scoprire quale manopola hardware manda quale controller, o per verificare che un synth che ascolta solo i cavi riceva quello che porta il suo cavo.

## Parametri

**Source** All mostra entrambi i flussi. Cords only mostra solo il punto d'ascolto; Live only mostra i dispositivi e la tastiera.

**Channel** Mostra un solo canale MIDI, da 1 a 16. 0 li mostra tutti.

## Ricetta

**Trova una manopola** Metti un MidiMonitor senza cavi, Source Live only, Channel 0. Gira la manopola hardware e leggi il numero del controller sulla riga più recente, poi fai clic destro sul parametro a cui vuoi assegnarla e usa MIDI learn.

## Organism correlati

MidiIn, OscMonitor, MidiBus
