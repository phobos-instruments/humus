# Morse

Un generatore di pattern che batte il messaggio che scrivi, come tono sinusoidale e come note MIDI, con i veri tempi del morse.

Scrivi in Text e il messaggio viene battuto con punti, linee, pause tra le lettere e pause tra le parole, agganciato al transport: muto da fermo, batte dall'inizio del messaggio quando parte la riproduzione. Il tono segue il Tuning della patch e i fronti del tasto sono addolciti su 3 ms, così non fa clic. L'uscita MIDI batte lo stesso messaggio come coppie di note-on e note-off al pitch di Note, quindi collegala a un Kick e il messaggio diventa un ritmo, oppure a un Acid e suona come una linea; con Level a 0 è una pura sorgente di pattern MIDI. Manda il tono attraverso un Fern per una linea melodica.

## Parametri

**Text** Il messaggio. Lettere, cifre e la punteggiatura comune; tutto ciò che il morse non sa dire viene saltato. Il nastro sotto disegna il messaggio come punti e linee e illumina quello che sta battendo.

**WPM** Velocità di battuta in parole al minuto, da 5 a 40. Un'unità dura 1.2 diviso WPM secondi.

**Sync** Ignora WPM e rende un'unità esattamente un sedicesimo, così il messaggio diventa un ritmo sulla griglia e i cambi di tempo lo riallineano. Il messaggio parte sul beat 1 e la sua posizione segue il transport, quindi se salti o metti in loop la timeline cadi ogni volta sulla stessa lettera.

**Note** Il pitch del tono come nota, secondo il Tuning della patch.

**Loop** Ripete il messaggio, con la pausa standard di sette unità tra le parole fra un passaggio e l'altro. Disattivato, suona un passaggio a ogni avvio della riproduzione.

**Level** Livello d'uscita del tono.

## Ricetta

**Ritmo telegrafico** Text CQ CQ, Sync attivo, Loop attivo, Note D5. Collega l'uscita MIDI a un Kick e l'uscita audio attraverso un Fern con Sync attivo a 1/8; premi Riproduci a 140 BPM e il messaggio fa suonare il Kick.

## Organism correlati

Rhizome, Sequence, SideKick
