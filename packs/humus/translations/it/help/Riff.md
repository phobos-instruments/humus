# Riff

Un sequencer di linee di basso da acid box a sedici step che invia note MIDI con gate, accento e slide per ogni step.

Ogni step porta una nota, un gate, un accento e uno slide, il linguaggio di pattern dell'acid box. Uno step accentato esce a velocity 118, sopra la soglia di accento di Acid, e uno step con slide tiene la sua nota appena oltre il note-on successivo, che Acid legge come un glide legato. Otto banchi tengono ognuno il proprio pattern; i pulsanti dei banchi scelgono quale suona e quale mostra la griglia. Pilota anche qualsiasi altro strumento MIDI, quindi collega la sua uscita MIDI anche a un Sampler o a un Rhizome.

## La griglia

Trascina nella corsia delle note per impostare il pitch di uno step, e così gli attivi anche il gate; fai clic destro su uno step per trasformarlo in una pausa. Le righe A e S attivano e disattivano accento e slide. Uno slide verso la stessa nota la lega invece di ribatterla. Con il clic destro sulle righe A o S trovi Casuale, Cancella, Sposta, Carica riff e Copia nel banco. Carica riff apre un browser sulla cartella Riffs della tua libreria, con quelli caricati in precedenza sotto Recenti e Open a file per tutto il resto. Il riff che scegli, o un file trascinato sulla griglia, legge file MIDI e dump di pattern da acid box nel banco mostrato e in quelli successivi.

**Probabilità dello step** Clicca il tasto % in alto a destra della griglia per vedere la probabilità di ogni step come una barra, poi trascina su o giù su uno step per decidere quanto spesso suona, da mai a sempre, a scatti del 5%. Cmd-trascina (Ctrl-trascina su Windows e Linux) fa lo stesso senza cambiare modalità. Il tasto % è l'interruttore View, quindi anche un pulsante o un pad mappato lo commuta. La probabilità di ogni step è anche un parametro, da Chance_1 a Chance_16, così puoi mapparci dei fader. Nella vista Chance, fai clic destro su uno step per mapparlo. Uno step sotto il 100% appare più sbiadito e mostra la sua probabilità sopra di sé, e Nudge sposta le probabilità insieme agli step. I dadi sono del box: una patch suona gli stessi step a ogni riproduzione e a ogni bounce, e due box tirano ciascuno i propri. Ogni banco tiene le sue probabilità, Copia nel banco le porta con sé e Cancella le riporta al sicuro.

## Parametri

**Transpose** Traspone tutto il pattern, in semitoni.

**SwingFollow** Follow fa prendere al pattern il groove della patch dal transport invece che dalla sua manopola Swing.

**Swing** Ritarda ogni secondo sedicesimo, fino a un andamento terzinato. Conta solo mentre Follow è disattivato.

**Gate** Lunghezza di una nota senza slide, come frazione dello step.

**Velocity** Velocity delle note normali, senza accento.

**Bank** Quale degli otto pattern suona, da A a H.

**View** Steps o Chance: cosa mostra e modifica la griglia. Il tasto % lo cambia.

**Chance_1 a Chance_16** Quanto spesso suona ogni step, da 0 a 100%. Ogni banco tiene le sue.

**Nudge** Suona il riff quel numero di step più tardi, o prima sotto lo zero, ricominciando dall'altro capo. Il pattern resta come l'hai scritto, quindi sposta il punto in cui la linea cade rispetto alla cassa senza ridisegnarla.

## Ricetta

**Linea acid** Collega l'uscita MIDI a un Acid. Disegna la tonica sugli step 1, 4, 7, 11 e 14, un'ottava sopra sull'8 e sul 16, accenta gli step 1 e 11 e fai lo slide dal 7 all'8. Gate 0.5, Swing 0.2 con Follow disattivato. Abbassa Transpose di 5 per il breakdown.

## Organism correlati

Acid, Steps, Microdot
