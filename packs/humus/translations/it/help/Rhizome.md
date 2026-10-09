# Rhizome

Un synth MIDI a sei voci che impila voci extra sopra ogni nota, distanziate in gradi della scala dell'accordatura della patch.

Ogni nota suonata fa crescere una serie di stoloni: altri oscillatori che stanno Interval gradi della scala sopra la fondamentale, con le altezze lette dall'accordatura della patch invece che dalla serie armonica. Cambia l'accordatura sul transport e cambia anche il timbro, non solo il pitch. Gli stoloni si alternano a sinistra e a destra per dare ampiezza, e un passa-basso sulla somma tiene a bada tante voci a dente di sega sovrapposte. Suonalo da un PianoRoll, da un DNA o dalla tastiera quando l'editor ha il focus, e mandalo in un Fern o in un SoundSpace.

## Parametri

**Runners** Quante voci crescono da ogni nota, dalla sola fondamentale fino a otto.

**Interval** La distanza tra uno stolone e il successivo, in gradi della scala. Sul pannello è mostrato come Degrees.

**Creep** Una lenta deriva di pitch, indipendente per ogni stolone, in cent. Anche poco Creep impedisce alle voci sovrapposte di agganciarsi in fase.

**Tilt** La pendenza del livello lungo la catena. Basso tiene il suono vicino alla fondamentale; alto fa emergere gli stoloni lontani.

**Wave** La forma d'onda degli stoloni: SIN, SAW o SQR.

**Bloom** Tempo d'attacco in millisecondi, quanto ci mette una nota ad aprirsi.

**Decay** Tempo di rilascio in millisecondi, quanto ci mette una nota a spegnersi dopo il note-off.

**Cutoff** Un passa-basso sull'uscita sommata, in Hz. Abbassalo quando tanti stoloni a dente di sega diventano aspri.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore più comune; zero ignora la rotella.

## Ricetta

**Cluster** Interval 1, Runners 6, Creep 0, Wave SIN. Suona note singole da un PianoRoll: ognuna diventa un cluster di gradi adiacenti della scala, che batte al ritmo del passo della scala stessa. Riaccorda la patch sul transport e il battimento cambia con lei.

## Organism correlati

Wave, Mineral, Substrate
